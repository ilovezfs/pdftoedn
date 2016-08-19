#include <string>
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
            static const char* MODULE = "ImageXform";

            // =======================================================================
            // new trace control API as of Leptonica 1.71. Mute them all
            //
            bool init_transform_lib()
            {
                setMsgSeverity(L_SEVERITY_NONE);
                return true;
            }

            // =======================================================================
            //
            // transform image data and recompute bounding box
            uint8_t transform_image(const PdfTM& image_ctm, const std::string& blob,
                                    int& width, int& height, bool inverted_mask,
                                    std::ostringstream& xformed_blob)
            {
                uint8_t ops = XFORM_NONE;
                PdfTM ctm(image_ctm);
                PIX* p = pixReadMemPng(reinterpret_cast<const l_uint8*>(blob.c_str()),
                                       blob.length());
                if (!p) {
                    et.log_critical( ErrorTracker::ERROR_UT_IMAGE_XFORM, MODULE,
                                     "pixReadMemPng() error reading image data");
                    return XFORM_ERR;
                }

                PIX* p2 = NULL;

                DBG_TRACE(std::cerr << std::endl<< "\tXFORM === w: " << width << ", h: " << height << std::endl);

                // try to decompose the transformation. First, check if there's a rotation
                if (ctm.is_rotated()) {

                    double angle_d = std::round( ctm.rotation_deg() );

                    DBG_TRACE(std::cerr << "\trotation - ");

                    // leptonica has optimized orthogonal rotation operations
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
                        // arbitrary angle - note that this will
                        // affect the width & height of the resulting
                        // pixmap and this might likely affect the
                        // output in the user viewport if the angle is
                        // significant
                        double angle = ctm.rotation_deg();
                        p2 = pixRotate(p,  PdfTM::deg_to_rad(angle), L_ROTATE_SAMPLING,
                                       inverted_mask ? L_BRING_IN_BLACK : L_BRING_IN_WHITE,
                                       (l_int32) width, (l_int32) height);

                        ops |= XFORM_ROT_ARB;
                        DBG_TRACE(std::cerr << angle << " deg" << std::endl);
                    }

                    // check if the rotation failed
                    if (!p2) {
                        std::stringstream s;
                        s << "pixRotate[XX]() op failed for " << angle_d << " degrees";
                        et.log_critical( ErrorTracker::ERROR_UT_IMAGE_XFORM, MODULE, s.str() );
                        pixDestroy(&p);
                        return XFORM_ERR;
                    }

                    pixDestroy(&p);
                    p = p2;
                    p2 = NULL;
                }

                // next if there's a H or V flip
                if (ctm.is_flipped()) {
                    p2 = pixFlipLR(NULL, p);
                    pixDestroy(&p);

                    if (!p2) {
                        et.log_critical( ErrorTracker::ERROR_UT_IMAGE_XFORM, MODULE,
                                         "pixFlipLR() failed");
                        return XFORM_ERR;
                    }

                    p = p2;
                    p2 = NULL;

                    ctm.scale(-1,1);
                    ops |= XFORM_FLIP_H;
                    DBG_TRACE(std::cerr << "\tis flipped " << std::endl);
                }

                if (ctm.is_upside_down()) {
                    p2 = pixFlipTB(NULL, p);
                    pixDestroy(&p);

                    if (!p2) {
                        et.log_critical( ErrorTracker::ERROR_UT_IMAGE_XFORM, MODULE,
                                         "pixFlipTB() failed");
                        return XFORM_ERR;
                    }

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

                // get the dimensions of the resulting pixmap
                width = pixGetWidth(p);
                height = pixGetHeight(p);
                int depth = pixGetDepth(p);

                DBG_TRACE(std::cerr << "\tnew w: " << width << ", new h: " << height
                          << ", depth: " << depth << std::endl;);

                // write it to a PNG - TODO: optimize to directly write to disk instead
                size_t size = width * height * depth; //blob.length();     // TODO: check size
                l_uint8* data = new l_uint8[size];

                if (pixWriteMemPng(&data, &size, p, 0) == 0) {
                    xformed_blob.write(reinterpret_cast<const char*>(data), size);
                }
                else {
                    et.log_critical( ErrorTracker::ERROR_UT_IMAGE_XFORM, MODULE,
                                     "pixWriteMemPng() failed to write PNG");
                    ops = XFORM_ERR;
                }

                // cleanup
                delete [] data;

                if (p) {
                    pixDestroy(&p);
                }
                if (p2) {
                    pixDestroy(&p2);
                }

                return ops;
            }
        } // namespace xform
    } // namespace util
} // namespace
