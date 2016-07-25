#include <sstream>
#include <ostream>

#include <png.h>
#include <zlib.h>
#include <GfxState.h>

#include "image.h"
#include "pdf_error_tracker.h"
#include "util_encode.h"
#include "edsel_options.h"

namespace pdftoedn
{
    namespace util
    {
        namespace encode
        {
            // -------------------------------------------------------------------------------
            // image encoding using libpng
            //
            // write / flush methods to write to an ostream
            static void user_io_write(png_structp png_ptr, png_bytep data, png_size_t length)
            {
                std::ostream* str_p = static_cast<std::ostream*>( png_get_io_ptr(png_ptr) );

                if (str_p && data && (length > 0)) {
                    str_p->write((const char*) data, length);
                }
            }

            static void user_io_flush(png_structp png_ptr)
            {
                std::ostream* str_p = static_cast<std::ostream*>( png_get_io_ptr(png_ptr) );

                if (str_p) {
                    str_p->flush();
                }
            }


            //
            // poppler to libpng translators
            static int poppler_cspace_mode_to_png_type(uint8_t num_pix_comps, GfxColorSpaceMode cspace_mode)
            {
                int png_color_type;

                if (num_pix_comps == 1) {
                    png_color_type = PNG_COLOR_TYPE_PALETTE;
                }
                else
                {
                    switch (cspace_mode)
                    {
                      case csIndexed:
                      case csSeparation:
                          png_color_type = PNG_COLOR_TYPE_PALETTE;
                          //                          std::cerr << "    setting PNG type: PNG_COLOR_TYPE_PALETTE" << std::endl;
                          break;

                      case csDeviceGray:
                      case csCalGray:
                          png_color_type = PNG_COLOR_TYPE_GRAY;
                          //                          std::cerr << "    setting PNG type: PNG_COLOR_TYPE_GRAY" << std::endl;
                          break;

                      case csDeviceCMYK:
                      case csDeviceRGB:
                      case csCalRGB:
                      case csLab:
                      case csICCBased:
                      case csPattern:
                      case csDeviceN:
                      default:
                          png_color_type = PNG_COLOR_TYPE_RGB;
                          //                          std::cerr << "    setting PNG type: PNG_COLOR_TYPE_RGB" << std::endl;
                          break;
                    }
                }

                return png_color_type;
            }


            //
            // copy pixmap data
            static bool copy_image_data(png_structp png_ptr,
                                        ImageStream* img_str, uint32_t width, uint32_t height,
                                        uint8_t num_pix_comps, GfxColorSpaceMode cspace_mode, GfxColorSpace* cspace)
            {
                // set error handling since png_* calls are made in here
                if (setjmp(png_jmpbuf(png_ptr))) {
                    return false;
                }

                // read the image bytes
                img_str->reset();

                switch (cspace_mode)
                {
                  case csIndexed:
                  case csSeparation:
                      for (size_t y = 0; y < height; y++) {
                          Guchar *pix = img_str->getLine();
                          png_write_rows(png_ptr, &pix, 1);
                      }
                      break;

                  case csDeviceCMYK:
                      {
                          GfxDeviceCMYKColorSpace* cmyk_cs = dynamic_cast<GfxDeviceCMYKColorSpace*>(cspace);

                          if (!cmyk_cs) {
                              et.log_error( ErrorTracker::ERROR_UT_IMAGE_ENCODE, MODULE,
                                            "Poppler CMYK image not carrying CMYK color space?" );
                              return false;
                          }

                          // we'll convert cmyk to RGB so only need 3 color chans
                          png_bytep row = (png_bytep) png_malloc(png_ptr, 3 * sizeof(png_byte) * width);

                          if (!row) {
                              et.log_error( ErrorTracker::ERROR_UT_IMAGE_ENCODE, MODULE,
                                            "png_malloc failed when handling csDeviceCMYK" );
                              return false;
                          }

                          for (size_t y = 0; y < height; ++y)
                          {
                              cmyk_cs->getRGBLine(img_str->getLine(), row, width);
                              png_write_rows(png_ptr, &row, 1);
                          }

                          png_free(png_ptr, row);
                      }
                      break;

                  case csDeviceGray:
                  case csCalGray:
                  case csDeviceRGB:
                  case csCalRGB:
                  case csLab:
                  case csICCBased:
                  case csPattern:
                  case csDeviceN:
                      {
                          Guchar pix[num_pix_comps];

                          png_bytep row = (png_bytep) png_malloc(png_ptr, num_pix_comps * sizeof(png_byte) * width);

                          if (!row) {
                              et.log_error( ErrorTracker::ERROR_UT_IMAGE_ENCODE, MODULE,
                                            "png_malloc failed" );
                              return false;
                          }

                          for (size_t y = 0; y < height; ++y)
                          {
                              for (size_t x = 0; x < width * num_pix_comps;)
                              {
                                  if (img_str->getPixel(pix)) {
                                      for (int z = 0; z < num_pix_comps; z++) {
                                          row[x++] = pix[z];
                                      }
                                  }
                              }
                              png_write_rows(png_ptr, &row, 1);
                          }

                          png_free(png_ptr, row);
                      }
                      break;

                  default:
                      return false;
                      break;
                }
                return true;
            }


            //
            // export the data as a PNG onto an ostream - based on:
            //
            //  http://www.linbox.com/ucome.rvt?file=/any/doc_distrib/libgr-2.0.13/png/example.c
            bool encode_image(std::ostream& output, ImageStream* img_str,
                              uint32_t width, uint32_t height, uint32_t num_pix_comps, uint8_t bpp,
                              GfxImageColorMap *color_map)
            {
                // Create and initialize the png_struct with the desired error handler
                // functions.  If you want to use the default stderr and longjump method,
                // you can supply NULL for the last three parameters.  We also check that
                // the library version is compatible with the one used at compile time,
                // in case we are using dynamically linked libraries.
                png_structp png_ptr;
                png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                  NULL, NULL, NULL); // (void *)user_error_ptr, user_error_fn, user_warning_fn);

                if (!png_ptr) {
                    return false;
                }

                // Allocate/initialize the image information data.
                png_infop info_ptr = png_create_info_struct(png_ptr);
                if (!info_ptr) {
                    png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
                    return false;
                }

                // Set error handling.  REQUIRED if you aren't supplying your own
                // error hadnling functions in the png_create_write_struct() call.
                if (setjmp(png_jmpbuf(png_ptr))) {
                    // If we get here, we had a problem writing to the stream
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    return false;
                }

                // use our write & flush functions
                png_set_write_fn(png_ptr,
                                 (png_voidp *)&output,
                                 user_io_write,
                                 user_io_flush);

                // Set the image information here.  Width and height
                // are up to 2^31, bit_depth is one of 1, 2, 4, 8, or
                // 16, but valid values also depend on the color_type
                // selected. color_type is one of PNG_COLOR_TYPE_GRAY,
                // PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE,
                // PNG_COLOR_TYPE_RGB, or PNG_COLOR_TYPE_RGB_ALPHA.
                // interlace is either PNG_INTERLACE_NONE or
                // PNG_INTERLACE_ADAM7, and the compression_type and
                // filter_type MUST currently be
                // PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE.
                //
                GfxColorSpaceMode cspace_mode = color_map->getColorSpace()->getMode();
                int png_type = poppler_cspace_mode_to_png_type(num_pix_comps, cspace_mode);

#if 0
                std::cerr << "color space mode is: '" << color_map->getColorSpace()->getColorSpaceModeName(cspace_mode)
                          << "', num pixel comps: " << num_pix_comps
                          << ", bpp: " << (int) bpp
                          << std::endl;
#endif

                png_set_IHDR(png_ptr, info_ptr, width, height, bpp,
                             png_type,
                             PNG_INTERLACE_NONE,
                             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

                if (pdftoedn::options.libpng_use_best_compression()) {
                    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
                }

                // set the palette for indexed-color images
                png_colorp palette = NULL;

                if (png_type == PNG_COLOR_TYPE_PALETTE)
                {
                    int n = 1 << bpp;
                    palette = (png_colorp) png_malloc(png_ptr, n * sizeof (png_color));

                    if (!palette) {
                        std::stringstream err;
                        err << __FUNCTION__ << " - couldn't alloc storage for image palette";
                        et.log_error( ErrorTracker::ERROR_UT_IMAGE_ENCODE, MODULE, err.str() );

                        png_destroy_write_struct(&png_ptr, &info_ptr);
                        return false;
                    }

                    Guchar pix;
                    GfxRGB rgb;

                    for (int i = 0; i < n; i++) {
                        pix = (Guchar) i;

                        color_map->getRGB(&pix, &rgb);
                        palette[i].red = colToByte(rgb.r);
                        palette[i].green = colToByte(rgb.g);
                        palette[i].blue = colToByte(rgb.b);
                    }

                    png_set_PLTE(png_ptr, info_ptr, palette, n);
                }

#if 0 //def PNG_sBIT_SUPPORTED
                // optional significant bit chunk
                png_color_8 sig_bit;
                // if we are dealing with a grayscale image then
                sig_bit.gray = true_bit_depth;
                // otherwise, if we are dealing with a color image then
                sig_bit.red = true_red_bit_depth;
                sig_bit.green = true_green_bit_depth;
                sig_bit.blue = true_blue_bit_depth;
                // if the image has an alpha channel then
                sig_bit.alpha = true_alpha_bit_depth;
                png_set_sBIT(png_ptr, info_ptr, sig_bit);
#endif

#if 0
                // Optional gamma chunk is strongly suggested if you have any guess
                // as to the correct gamma of the image.
                png_set_gAMA(png_ptr, info_ptr, gamma);
#endif

#if 0
                // Optionally write comments into the image
                text_ptr[0].key = "Title";
                text_ptr[0].text = "Mona Lisa";
                text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
                text_ptr[1].key = "Author";
                text_ptr[1].text = "Leonardo DaVinci";
                text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
                text_ptr[2].key = "Description";
                text_ptr[2].text = "<long text>";
                text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
                png_set_text(png_ptr, info_ptr, text_ptr, 2);
#endif
                // other optional chunks like cHRM, bKGD, tRNS, tIME, oFFs, pHYs,

                // Write the file header information.
                png_write_info(png_ptr, info_ptr);

                // Once we write out the header, the compression type on the text
                // chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
                // PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out again
                // at the end.

                // set up the transformations you want.  Note that these are
                // all optional.  Only call them if you want them.

#if 0 //def PNG_WRITE_INVERT_SUPPORTED
                // invert monocrome pixels
                png_set_invert_mono(png_ptr);
#endif

#if 0 //def PNG_sBIT_SUPPORTED
                // Shift the pixels up to a legal bit depth and fill in
                // as appropriate to correctly scale the image
                png_set_shift(png_ptr, &sig_bit);
#endif

                // pack pixels into bytes
                png_set_packing(png_ptr);

                // swap location of alpha bytes from ARGB to RGBA
                //            png_set_swap_alpha(png_ptr);

                // Get rid of filler (OR ALPHA) bytes, pack XRGB/RGBX/ARGB/RGBA into
                // RGB (4 channels -> 3 channels). The second parameter is not used.
                //            png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);

                // flip BGR pixels to RGB
                //            png_set_bgr(png_ptr);

                // swap bytes of 16-bit files to most significant byte first
                //            png_set_swap(png_ptr);

                // swap bits of 1, 2, 4 bit packed pixel formats
                png_set_packswap(png_ptr);

                // ready to copy the data - iterate through the lines
                if (!copy_image_data(png_ptr, img_str, width, height, num_pix_comps, cspace_mode, color_map->getColorSpace())) {
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    return false;
                }

                // You can write optional chunks like tEXt, zTXt, and tIME at the end
                // as well.

                // finish writing the rest of the file
                png_write_end(png_ptr, info_ptr);

                // free the palette if allocated
                if (palette) {
                    png_free(png_ptr, palette);
                }

                // clean up after the write, and free any memory allocated
                png_destroy_write_struct(&png_ptr, &info_ptr);

                return true;
            }


            //
            // export the data as a PNG RGBA onto an ostream - based on:
            //
            //  http://www.linbox.com/ucome.rvt?file=/any/doc_distrib/libgr-2.0.13/png/example.c
            bool encode_rgba_image(std::ostream& output,
                                   ImageStream* img_str, uint32_t width, uint32_t height,
                                   uint32_t num_pix_comps, uint8_t bpp, GfxImageColorMap *color_map,
                                   ImageStream* mask_str, uint32_t mask_width, uint32_t mask_height,
                                   uint32_t mask_num_pix_comps, uint8_t mask_bpp, GfxImageColorMap *mask_color_map,
                                   bool mask_invert)
            {
                // Create and initialize the png_struct with the desired error handler
                // functions.  If you want to use the default stderr and longjump method,
                // you can supply NULL for the last three parameters.  We also check that
                // the library version is compatible with the one used at compile time,
                // in case we are using dynamically linked libraries.
                png_structp png_ptr;
                png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                  NULL, NULL, NULL); // (void *)user_error_ptr, user_error_fn, user_warning_fn);

                if (!png_ptr) {
                    return false;
                }

                // Allocate/initialize the image information data.
                png_infop info_ptr = png_create_info_struct(png_ptr);
                if (!info_ptr) {
                    png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
                    return false;
                }

                // Set error handling.  REQUIRED if you aren't supplying your own
                // error hadnling functions in the png_create_write_struct() call.
                if (setjmp(png_jmpbuf(png_ptr))) {
                    // If we get here, we had a problem writing to the stream
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    return false;
                }

                // use our write & flush functions
                png_set_write_fn(png_ptr,
                                 (png_voidp *)&output,
                                 user_io_write,
                                 user_io_flush);

                // Set the image information here
                GfxColorSpaceMode cspace_mode = color_map->getColorSpace()->getMode();
                int png_type;

                switch (cspace_mode) {
                  case csDeviceRGB:
                  case csICCBased:
                  case csIndexed:
                      png_type = PNG_COLOR_TYPE_RGB_ALPHA;
                      break;
                  case csDeviceGray:
                      png_type = PNG_COLOR_TYPE_GRAY_ALPHA;
                      break;
                  default:
                      std::stringstream err;
                      err << __FUNCTION__ << " - image unexpected cspace mode (" << color_map->getColorSpace()->getColorSpaceModeName(cspace_mode) << ")";
                      et.log_error( ErrorTracker::ERROR_UT_IMAGE_ENCODE, MODULE, err.str() );

                      png_destroy_write_struct(&png_ptr, &info_ptr);
                      return false;
                }

                png_set_IHDR(png_ptr, info_ptr, width, height, bpp,
                             png_type,
                             PNG_INTERLACE_NONE,
                             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

                if (pdftoedn::options.libpng_use_best_compression()) {
                    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
                }

                // set the palette for indexed-color images
                png_colorp palette = NULL;

                // Write the file header information.
                png_write_info(png_ptr, info_ptr);

                // pack pixels into bytes
                png_set_packing(png_ptr);

                // swap bits of 1, 2, 4 bit packed pixel formats
                png_set_packswap(png_ptr);

                // ready to copy image data - combine the image data
                // (RGB if 3 bpp, Grey if 1) with mask data
                // (1-channel) to form an RGBA png
                Guchar pix[num_pix_comps];
                Guchar* mask_bits;
                uint32_t o_num_pix_comps = 4; // add 1 for alpha channel

                png_bytep row = (png_bytep) png_malloc(png_ptr, o_num_pix_comps * sizeof(png_byte) * width);

                if (!row) {
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    return false;
                }

                // allocate a buffer for the mask line
                uint8_t* mask_buf = new uint8_t[mask_width * mask_num_pix_comps];

                if (!mask_buf) {
                    png_free(png_ptr, row);
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    return false;
                }

                img_str->reset();
                mask_str->reset();

                // process image line by line
                for (size_t y = 0; y < height; ++y)
                {
                    mask_bits = mask_str->getLine();

                    // poppler returns some masked images w/ a color map
                    if (mask_color_map) {
                        mask_color_map->getGrayLine(mask_bits, mask_buf, mask_width);
                    }
                    else {
                        // no cmap - just read the value and invert if needed
                        for (size_t mx = 0; mx < mask_width; mx++) {
                            bool bit = (mask_invert ? !mask_bits[mx] : mask_bits[mx]);
                            mask_buf[mx] = (bit ? 0 : 0xff);
                        }
                    }

                    // now process image data
                    for (size_t x = 0, mx = 0; x < width * o_num_pix_comps;)
                    {
                        if (img_str->getPixel(pix)) {
                            if (num_pix_comps > 1) {
                                GfxRGB rgb;
                                color_map->getRGB(pix, &rgb);
                                row[x++] = colToByte(rgb.r);
                                row[x++] = colToByte(rgb.g);
                                row[x++] = colToByte(rgb.b);
                            } else {
                                GfxGray g;
                                color_map->getGray(pix, &g);
                                uint8_t c = colToByte(g);

                                row[x++] = c;
                                row[x++] = c;
                                row[x++] = c;
                            }
                        }

                        // and set alpha to the mask value
                        row[x++] = mask_buf[mx++];
                    }
                    png_write_rows(png_ptr, &row, 1);
                }
                mask_str->close();

                delete [] mask_buf;
                png_free(png_ptr, row);

                // finish writing the rest of the file
                png_write_end(png_ptr, info_ptr);

                // free the palette if allocated
                if (palette) {
                    png_free(png_ptr, palette);
                }

                // clean up after the write, and free any memory allocated
                png_destroy_write_struct(&png_ptr, &info_ptr);

                return true;
            }


            //
            // export a mask onto a stream as a PNG w/ transparency.
            // Yeah, lots of replicated steps from encode_image.. :(
            bool encode_mask(std::ostream& output, ImageStream* img_str, uint32_t width, uint32_t height,
                             const StreamProps& properties)
            {
                png_structp png_ptr;
                png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                  NULL, NULL, NULL); // (void *)user_error_ptr, user_error_fn, user_warning_fn);

                if (!png_ptr) {
                    return false;
                }

                png_infop info_ptr = png_create_info_struct(png_ptr);
                if (!info_ptr) {
                    png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
                    return false;
                }

                if (setjmp(png_jmpbuf(png_ptr))) {
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    return false;
                }

                png_set_write_fn(png_ptr,
                                 (png_voidp *)&output,
                                 user_io_write,
                                 user_io_flush);

                // 2-palette entries.. one is set to transparent (0x00)
                png_byte transp[2] = { 0xff, 0x00 };
                png_set_tRNS(png_ptr, info_ptr, transp, 2, NULL);

                // save the header chunk
                png_set_IHDR(png_ptr, info_ptr, width, height, 1,
                             PNG_COLOR_TYPE_PALETTE,
                             PNG_INTERLACE_NONE,
                             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

                if (pdftoedn::options.libpng_use_best_compression()) {
                    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
                }

                // set the palette
                png_colorp palette = (png_colorp) png_malloc(png_ptr, 2 * sizeof(png_colorp));

                if (!palette)
                {
                    std::stringstream err;
                    err << __FUNCTION__ << " - couldn't alloc storage for image palette";
                    et.log_error( ErrorTracker::ERROR_UT_IMAGE_ENCODE, MODULE, err.str() );

                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    return false;
                }

                palette[0].red   = properties.mask_fill_color().red();
                palette[0].green = properties.mask_fill_color().green();
                palette[0].blue  = properties.mask_fill_color().blue();

                // by default set the 2nd color to white.. leptonica
                // does not preserve transparency so this should make
                // it match white background at least
                palette[1].red   = palette[1].green = palette[1].blue = 0xff;
                png_set_PLTE(png_ptr, info_ptr, palette, 2);

                // Write the file header information.
                png_write_info(png_ptr, info_ptr);

                // pack pixels into bytes
                png_set_packing(png_ptr);
                // swap bits of 1, 2, 4 bit packed pixel formats - we
                // don't need this but I've found some images in PDFs
                // that cause a segfault when this is disabled. Ugh.
                png_set_packswap(png_ptr);

                // reset the poppler image stream
                img_str->reset();

                uint8_t* row = new uint8_t[ width ];
                for (size_t y = 0; y < height; y++) {
                    Guchar* pix = img_str->getLine();
                    for (size_t x = 0; x < width; x++) {
                        row[x] = (properties.is_inverted() ? !pix[x] : pix[x]);
                    }
                    png_write_rows(png_ptr, &row, 1);
                }
                delete [] row;

                // finish writing the rest of the file
                png_write_end(png_ptr, info_ptr);

                // cleanup
                png_free(png_ptr, palette);

                // clean up after the write, and free any memory allocated
                png_destroy_write_struct(&png_ptr, &info_ptr);

                return true;
            }


#if 0
            //
            // export the data to PNG, forcing grayscale output instead of palletized
            //
            bool encode_grey_image(std::ostream& output, ImageStream* img_str,
                                   uint32_t width, uint32_t height, uint32_t num_pix_comps, uint8_t bpp,
                                   GfxImageColorMap *color_map)
            {
                png_structp png_ptr;
                png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

                if (!png_ptr) {
                    return false;
                }

                // Allocate/initialize the image information data.
                png_infop info_ptr = png_create_info_struct(png_ptr);
                if (!info_ptr) {
                    png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
                    return false;
                }

                if (setjmp(png_jmpbuf(png_ptr))) {
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    return false;
                }

                png_set_write_fn(png_ptr,
                                 (png_voidp *)&output,
                                 user_io_write,
                                 user_io_flush);

                GfxColorSpaceMode cspace_mode = color_map->getColorSpace()->getMode();
                int png_type = poppler_cspace_mode_to_png_type(num_pix_comps, cspace_mode);

#if 0
                std::cerr << "color space mode is: '" << color_map->getColorSpace()->getColorSpaceModeName(cspace_mode)
                          << "', num pixel comps: " << num_pix_comps
                          << ", bpp: " << (int) bpp
                          << ", w: " << width << ", h: " << height
                          << std::endl;
#endif

                png_set_IHDR(png_ptr, info_ptr, width, height, bpp, // change bpp to 1 / 8-bit if needed
                             PNG_COLOR_TYPE_GRAY, // forcing GRAY
                             PNG_INTERLACE_NONE,
                             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

#ifdef MOTO_USE_BEST_COMPRESSION
                png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
#endif

                // Write the file header information.
                png_write_info(png_ptr, info_ptr);

                // pack pixels into bytes
                png_set_packing(png_ptr);

                // swap bits of 1, 2, 4 bit packed pixel formats
                png_set_packswap(png_ptr);

                // ready to copy the data - iterate through the lines
                png_bytep row = (png_bytep) png_malloc(png_ptr, num_pix_comps * sizeof(png_byte) * width);

                if (!row) {
                    std::stringstream err;
                    err << __FUNCTION__ << " - png_malloc failed";
                    et.log_error( ErrorTracker::ERROR_UT_IMAGE_ENCODE, MODULE, err.str() );
                    return false;
                }

                // write the data, row by row
                img_str->reset();
                GfxGray gray;
                for (size_t y = 0; y < height; y++) {
                    Guchar* pix = img_str->getLine();

                    for (size_t x = 0; x < width; x++) {
                        color_map->getGray(&pix[x], &gray);
                        row[x] = (png_byte) colToByte(gray);
                    }
                    png_write_rows(png_ptr, &row, 1);
                }

                png_free(png_ptr, row);

                // finish writing the rest of the file
                png_write_end(png_ptr, info_ptr);

                // clean up after the write, and free any memory allocated
                png_destroy_write_struct(&png_ptr, &info_ptr);
                return true;
            }
#endif

        } // namespace encode

    } // namespace util

} // namespace
