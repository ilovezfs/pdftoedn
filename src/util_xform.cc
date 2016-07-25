#include <string>
#ifndef __linux
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#endif
#include <sstream>
#include <leptonica/allheaders.h>

#include "base_types.h"
#include "pdf_error_tracker.h"
#include "util_xform.h"

//#define XFORM_DEBUG

#ifdef XFORM_DEBUG
#include <iostream>
#define DBG_TRACE(t) t
#else
#define DBG_TRACE(t)
#endif

namespace pdftoedn
{
    namespace util
    {
        namespace xform
        {
            // =======================================================================
            // new trace control API as of Leptonica 1.71. Mute them all
            //
            bool init_transform_lib()
            {
                setMsgSeverity(L_SEVERITY_NONE);
                return true;
            }

#ifndef __linux
            //
            // two utility functions to read / write images using
            // files in tmp. Leptonica relies on open_memstream which
            // is not implemented in all OSs
            static const std::string TMPFILE_TEMPLATE = "dl.XXXXXX";
            static PIX* edsel_pixReadPng(const std::string& blob);
            static bool edsel_pixWritePng(std::ostringstream& blob, PIX* p);
#endif

            // =======================================================================
            //
            // transform image data and recompute bounding box
            uint8_t transform_image(const PdfTM& image_ctm, const std::string& blob,
                                    int& width, int& height, bool inverted_mask,
                                    std::ostringstream& xformed_blob)
            {
                uint8_t ops = XFORM_NONE;

                if (!image_ctm.is_transformed()) {
                    return ops;
                }

                PIX* p;
                PdfTM ctm(image_ctm);

#ifdef __linux
                p = pixReadMemPng((l_uint8*) blob.c_str(), blob.length());
#else // no memopen support
                p = edsel_pixReadPng(blob);
#endif

                if (!p) {
                    et.log_critical( ErrorTracker::ERROR_UT_IMAGE_XFORM, MODULE,
                                     "transform_image - error reading image data");
                    return ops;
                }

                bool status = true;
                PIX* p2 = NULL;

                DBG_TRACE(std::cerr << std::endl<< "\tXFORM === w: " << width << ", h: " << height << std::endl);

                // try to decompose the transformation. First, check if there's a rotation
                if (ctm.is_rotated()) {

                    double angle_d = std::round( ctm.rotation_deg() );

                    DBG_TRACE(std::cerr << "\trotation - ");

                    if (angle_d == 180) {
                        p2 = pixRotate180(NULL, p);

                        ctm = ctm * PdfTM(PdfTM::deg_to_rad(180), 0, 0);

                        ops |= XFORM_ROT_ORTH;
                        DBG_TRACE(std::cerr << "180 deg" << std::endl);
                    }
                    else if (angle_d == 90) {
                        p2 = pixRotate90(p, 1);

                        ctm = ctm * PdfTM(PdfTM::deg_to_rad(90), 0, 0);

                        ops |= XFORM_ROT_ORTH;
                        DBG_TRACE(std::cerr << "90 deg" << std::endl);
                    }
                    else if (angle_d == 270) {
                        p2 = pixRotate90(p, -1);

                        ctm = ctm * PdfTM(PdfTM::deg_to_rad(270), 0, 0);
                        ops |= XFORM_ROT_ORTH;
                        DBG_TRACE(std::cerr << "270 deg" << std::endl);
                    }
                    else {
                        double angle = ctm.rotation_deg();
                        p2 = pixRotate(p,  PdfTM::deg_to_rad(angle), L_ROTATE_SAMPLING,
                                       inverted_mask ? L_BRING_IN_BLACK : L_BRING_IN_WHITE,
                                       (l_int32) width, (l_int32) height);

                        ops |= XFORM_ROT_ARB;
                        DBG_TRACE(std::cerr << angle << " deg" << std::endl);
                    }

                    pixDestroy(&p);
                    p = p2;
                    p2 = NULL;
                }

                // next if there's a H or V flip
                if (ctm.is_flipped()) {
                    p2 = pixFlipLR(NULL, p);
                    pixDestroy(&p);
                    p = p2;
                    p2 = NULL;

                    ctm.scale(-1,1);
                    ops |= XFORM_FLIP_H;
                    DBG_TRACE(std::cerr << "\tis flipped " << std::endl);
                }
                if (ctm.is_upside_down()) {
                    p2 = pixFlipTB(NULL, p);
                    pixDestroy(&p);
                    p = p2;
                    p2 = NULL;

                    ctm.scale(1,-1);
                    ops |= XFORM_FLIP_V;
                    DBG_TRACE(std::cerr << "\tis upside down" << std::endl);
                }

                DBG_TRACE(std::cerr << std::endl<< "FINAL CTM:" << std::endl << ctm << std::endl);


                // TESLA-7266: found some images with bboxes that have
                // different aspect ratio than source pixmaps - these
                // require some scaling but they'll be handled by the
                // browser respecting the viewport. Still, mark as if
                // the op was done
                double img_ar, tm_ar;
                img_ar = (double) width / height;
                tm_ar = ctm.b() / ctm.c();

                double ratio_r = img_ar / tm_ar;
                double ar_delta = ratio_r - 1;

                if (std::abs(ar_delta) > 0.1) {
                    DBG_TRACE(std::cerr << " - AR delta: " << ar_delta << std::endl);
                    ops |= ((ratio_r > 0) ? XFORM_SCALE_H : XFORM_SCALE_V);
                }

                width = pixGetWidth(p);
                height = pixGetHeight(p);

#ifdef __linux
                {
                    int depth = pixGetDepth(p);

                    DBG_TRACE(std::cerr << "\tnew w: " << width << ", new h: " << height << ", depth: " << depth << std::endl);

                    // TODO: check size
                    size_t size = width * height * depth; //blob.length();
                    l_uint8* data = new l_uint8[size];

                    pixWriteMemPng(&data, &size, p, 0);
                    xformed_blob.write((const char*) data, size);

                    delete [] data;
                }
#else

                DBG_TRACE(std::cerr << "\tnew w: " << width << ", new h: " << height << std::endl);

                status = edsel_pixWritePng(xformed_blob, p);
#endif

                // cleanup
                if (p) {
                    pixDestroy(&p);
                }
                if (p2) {
                    pixDestroy(&p2);
                }

                if (!status) {
                    // error
                    ops = XFORM_NONE;
                }
                return ops;
            }

#ifndef __linux
            //
            // two utility functions to read / write images using
            // files in tmp. Leptonica relies on open_memstream which
            // is not implemented in all OSs
            static PIX* edsel_pixReadPng(const std::string& blob)
            {
                char tmpname[32];
                strncpy(tmpname, TMPFILE_TEMPLATE.c_str(), TMPFILE_TEMPLATE.length() + 1);

                errno = 0;
                int fp = mkstemp(tmpname);
                if (fp == -1) {
                    std::stringstream err;
                    err << __FUNCTION__ << " - mkstemp failed to create temp file " << tmpname
                        << " (errno: " << strerror(errno) << ")";
                    et.log_critical( ErrorTracker::ERROR_UT_IMAGE_XFORM, MODULE, err.str() );
                    return NULL;
                }

                std::ofstream tmp;
                tmp.open(tmpname, std::ios::binary);

                if (!tmp.is_open()) {
                    return NULL;
                }

                tmp.write(blob.c_str(), blob.length());
                tmp.close();

                PIX* p = pixRead(tmpname);
                close(fp);
                unlink(tmpname);
                return p;
            }

            //
            // a bit annoying because we have to write the PNG data to
            // a file and THEN open it and read it back
            static bool edsel_pixWritePng(std::ostringstream& blob, PIX* p)
            {
                bool status = true;
                int fp = 0;
                char tmpname[32];

                do
                {
                    strncpy(tmpname, TMPFILE_TEMPLATE.c_str(), TMPFILE_TEMPLATE.length() + 1);

                    errno = 0;
                    fp = mkstemp(tmpname);
                    if (fp == -1) {
                        std::stringstream err;
                        err << __FUNCTION__ << " - mkstemp failed to create temp file " << tmpname
                            << " (" << strerror(errno) << ")";
                        et.log_critical( ErrorTracker::ERROR_UT_IMAGE_XFORM, MODULE, err.str() );

                        DBG_TRACE(std::cerr << err.str() << std::endl);

                        return false;
                    }

                    // write the PNG data to the tmp file
                    if (pixWrite(tmpname, p, IFF_PNG) != 0) {
                        status = false;

                        std::stringstream err;
                        err << __FUNCTION__ << " - pixWrite() failed to write image data ("
                            << tmpname << ")";
                        et.log_critical( ErrorTracker::ERROR_UT_IMAGE_XFORM, MODULE, err.str() );

                        DBG_TRACE(std::cerr << err.str() << std::endl);

                        break;
                    }

                    // open it for reading
                    std::ifstream tmp(tmpname, std::ifstream::in | std::ifstream::binary);
                    if (!tmp.is_open()) {
                        status = false;

                        std::stringstream err;
                        err << __FUNCTION__ << " - failed to open tmp file " << tmpname;
                        et.log_critical( ErrorTracker::ERROR_UT_IMAGE_XFORM, MODULE, err.str() );

                        DBG_TRACE(std::cerr << err.str() << std::endl);

                        break;
                    }

                    // determine the size
                    tmp.seekg (0, std::ios::end);
                    size_t size = tmp.tellg();

                    // std::cerr << "file size is: " << size << std::endl;
                    char *memblock = new char[size];
                    tmp.seekg(0, std::ios::beg);
                    tmp.read(memblock, size);
                    tmp.close();

                    // write the data into the ostringstream
                    blob.write(memblock, size);
                    // util::debug::save_blob_to_disk(blob, "-wrote-img");

                    // cleanup
                    delete[] memblock;

                } while (0);

                if (fp) {
                    close(fp);
                    unlink(tmpname);
                }
                return status;
            }
#endif
        } // namespace xform
    } // namespace util
} // namespace
