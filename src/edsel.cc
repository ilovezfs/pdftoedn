#include <string>
#include <iostream>
#include <clocale>

#ifdef EDSEL_RUBY_GEM
// always include rice headers before ruby.h
#include <rice/Symbol.hpp>
#include <rice/Data_Type.hpp>
#include <rice/Constructor.hpp>
#include <rice/String.hpp>
#include <rice/Hash.hpp>

#include "util_ruby.h"
#endif

#include <poppler/GlobalParams.h>
#include <poppler/Error.h>

#include "base_types.h"
#include "pdf_error_tracker.h"
#include "pdf_reader.h"

#include "util.h"
#include "util_edn.h"
#include "util_xform.h"
#include "util_versions.h"
#include "edsel_options.h"


namespace pdftoedn {

    // task-level error handler for poppler errors
    extern pdftoedn::ErrorTracker et;

    // run-time options passed as args
    extern pdftoedn::Options options;

#ifdef EDSEL_RUBY_GEM
    //
    // C-extension Engine representation
    class Engine
    {
    private:
        // create the instance that will store the PDF data and generate
        // the ruby types
        pdftoedn::PDFReader *doc_reader;

    public:
        Engine() : doc_reader(NULL)
        {
            // poppler GlobalParams
            globalParams = new GlobalParams();
            globalParams->setProfileCommands(false);
            globalParams->setPrintCommands(false);
        }
        ~Engine() {
            delete doc_reader;
            delete globalParams;
        }

        //
        // opens a file for processing; returns doc properties
        Rice::Object c_open_file(std::string filename, Rice::Object opts)
        {
            // options passed from the ruby side
            static const Rice::Symbol SYMBOL_OPT_INVISIBLE_TEXT         = "include_invisible_text";
            static const Rice::Symbol SYMBOL_OPT_USE_PAGE_BOX           = "use_page_box";
            static const Rice::Symbol SYMBOL_OPT_CROP_PAGE              = "crop";
            static const Rice::Symbol SYMBOL_OPT_LINK_OUTPUT_ONLY       = "link_output_only";
            static const Rice::Symbol SYMBOL_OPT_OMIT_OUTLINE           = "omit_outline";
            static const Rice::Symbol SYMBOL_OPT_QUIETER_ERROR_OUTPUT   = "quieter_error_output";
            static const Rice::Symbol SYMBOL_OPT_PNG_BEST_COMPRESSION   = "png_best_compression";
            static const Rice::Symbol SYMBOL_OPT_OUTPUT_FILE            = "outputfile";
            static const Rice::Symbol SYMBOL_OPT_FONT_MAP_FILE          = "font_map_file";
            static const Rice::Symbol SYMBOL_OPT_DEBUG_INFO             = "include_debug_info";
            static const Rice::Symbol SYMBOL_OPT_PAGE_NUM               = "page_number";
            static const Rice::Symbol SYMBOL_OPT_PRE_PROCESS_FONTS      = "pre_process_fonts";
            static const Rice::Symbol SYMBOL_OPT_OVERWRITE_OUTPUT       = "overwrite";

            // two options for page processing: use media box (default) or use
            // page crop box. No need to define the default since we'll just
            // check if the crop option is set
            //static const Rice::Symbol SYMBOL_OPT_PAGE_MEDIA_BOX          = "page_media_box";
            static const Rice::Symbol SYMBOL_OPT_PAGE_CROP_BOX          = "page_crop_box";


            // we'll create a new instance of the reader so kill any old ones
            if (doc_reader) {
                delete doc_reader;
            }

            // parse the options
            pdftoedn::Options::Flags flags = { false };
            std::string output_file, font_map_file;
            intmax_t page_number = -1;

            // should not throw an exception since the default arg
            // assignment in base.rb is {} but let's be safe
            try
            {
                Rice::Hash options = from_ruby<Rice::Hash>(opts);

                for (Rice::Hash::const_iterator ii = options.begin(); ii != options.end(); ++ii)
                {
                    if (     (*ii).key == SYMBOL_OPT_INVISIBLE_TEXT && (*ii).value == Qtrue) {
                        flags.include_invisible_text = true;
                    }
                    else if ((*ii).key == SYMBOL_OPT_USE_PAGE_BOX && (*ii).value == SYMBOL_OPT_PAGE_CROP_BOX) {
                        flags.use_page_crop_box = true;
                    }
                    else if ((*ii).key == SYMBOL_OPT_LINK_OUTPUT_ONLY && (*ii).value == Qtrue) {
                        flags.link_output_only = true;
                    }
                    else if ((*ii).key == SYMBOL_OPT_OMIT_OUTLINE && (*ii).value == Qtrue) {
                        flags.omit_outline = true;
                    }
                    else if ((*ii).key == SYMBOL_OPT_CROP_PAGE && (*ii).value == Qtrue) {
                        flags.crop_page = true;
                    }
                    else if ((*ii).key == SYMBOL_OPT_OUTPUT_FILE && (*ii).value != Qnil) {
                        output_file = from_ruby<std::string>((*ii).value);
                    }
                    else if ((*ii).key == SYMBOL_OPT_FONT_MAP_FILE && (*ii).value != Qnil) {
                        font_map_file = from_ruby<std::string>((*ii).value);
                    }
                    else if ((*ii).key == SYMBOL_OPT_DEBUG_INFO && (*ii).value == Qtrue) {
                        flags.include_debug_info = true;
                    }
                    else if ((*ii).key == SYMBOL_OPT_PAGE_NUM && (*ii).value != Qnil) {
                        page_number = from_ruby<uintmax_t>((*ii).value);
                    }
                    else if ((*ii).key == SYMBOL_OPT_PRE_PROCESS_FONTS && (*ii).value == Qtrue) {
                        flags.force_font_preprocess = true;
                    }
                    else if ((*ii).key == SYMBOL_OPT_OVERWRITE_OUTPUT && (*ii).value == Qtrue) {
                        flags.force_output_write = true;
                    }
                    else if ((*ii).key == SYMBOL_OPT_QUIETER_ERROR_OUTPUT && (*ii).value == Qtrue) {
                        // mute poppler syntax warnings & errors
                        pdftoedn::et.ignore_error(pdftoedn::ErrorTracker::ERROR_SYNTAX_WARNING);
                        pdftoedn::et.ignore_error(pdftoedn::ErrorTracker::ERROR_SYNTAX_ERROR);

                        // mute unsupported fonts (type3 / unknown) and unimplemented CBs
                        pdftoedn::et.ignore_error(pdftoedn::ErrorTracker::ERROR_FE_FONT_READ_UNSUPPORTED);
                        pdftoedn::et.ignore_error(pdftoedn::ErrorTracker::ERROR_OD_UNIMPLEMENTED_CB);
                    }
                    else if ((*ii).key == SYMBOL_OPT_PNG_BEST_COMPRESSION && (*ii).value == Qtrue) {
                        flags.libpng_use_best_compression = true;
                    }
                }
            } catch (const std::invalid_argument& e) {
                std::cerr << __FUNCTION__ << "invalid 'options' type received" << std::endl;
            }

            try
            {
                options = pdftoedn::Options(filename, font_map_file, output_file, flags, page_number);
            }
            catch (std::exception& e)
            {
                return Qnil;
            }

            // init support libs if needed
            pdftoedn::util::xform::init_transform_lib();

            // register the error handler for this document
            et = pdftoedn::ErrorTracker();
            setErrorCallback(&pdftoedn::ErrorTracker::error_handler, &pdftoedn::et);

            // open the doc - this reads basic properties from the doc
            // (num pages, PDF version) and the outline
            doc_reader = new pdftoedn::PDFReader;

            // return a hash with the data
            return doc_reader->meta();
        }

        //
        // process the page indicated by the arg
        Rice::Object c_process_page(int page_num)
        {
            if (doc_reader) {
                return doc_reader->process_page(page_num);
            }
            return Qnil;
        }

        Rice::Object c_end()
        {
            return Qnil;
        }

    }; // Engine

#endif
} // namespace


#ifdef EDSEL_RUBY_GEM
//
// ruby calls this to load the extension
extern "C"
void Init_edsel(void)
{
    // pass things back as utf-8
    if (!setlocale( LC_ALL, "" )) {
        std::cerr << "Error setting locale" << std::endl;
        return;
    }

    Rice::Module rb_mEdsel = Rice::define_module("Edsel");

    // read the VERSION constant from lib/edsel/version.rb so we
    // can include it in the output data
    pdftoedn::util::version::set_ext_version( from_ruby<std::string>(rb_mEdsel.const_get("VERSION")) );

    // bind the ruby Engine class to the C++ one
    Rice::Data_Type<pdftoedn::Engine> rb_cEngine =
        Rice::define_class_under<pdftoedn::Engine>(rb_mEdsel, "Engine")
        .define_constructor(Rice::Constructor<pdftoedn::Engine>())
        .define_method("c_version_info", &pdftoedn::util::version::info)
        .define_method("c_open_file", &pdftoedn::Engine::c_open_file,
                       (Rice::Arg("filename"),
                        Rice::Arg("options") = Qnil))
        .define_method("c_process_page", &pdftoedn::Engine::c_process_page,
                       (Rice::Arg("page_num")))
        .define_method("c_end", &pdftoedn::Engine::c_end);

    // add functions within the scope of the Edsel module and its
    // sub-modules
    define_module_under(rb_mEdsel, "Utils")
        .define_module_function("c_recompute_page_bounds",
                                &pdftoedn::util::ruby::recompute_page_bounds,
                                (Rice::Arg("page")));

    // import whatever else we've defined in the ruby side
    rb_require("edsel/engine");
    rb_require("edsel/version");
}

#endif
