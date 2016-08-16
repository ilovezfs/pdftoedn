#include <iostream>
#include <string>
#include <ostream>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

#include "edsel_options.h"
#include "util.h"
#include "util_config.h"
#include "util_fs.h"
#include "font_maps.h"
#include "pdf_error_tracker.h"

namespace pdftoedn {

    static const std::string EDN_FILE_EXT      = ".edn";
    static const std::string IMAGE_FILE_EXT    = ".png";
    static const std::string FONT_MAP_FILE_EXT = ".json";

    static const std::string DEFAULT_CONFIG_DIR = util::expand_environment_variables("${HOME}") + "/.pdftoedn/";

    //
    // constructor
    Options::Options(const std::string& filename, const std::string& fontmap, const std::string& out_filename,
                     const Flags& f, intmax_t pg_num) :
        file_name(filename), output_file(out_filename), flags(f), page_num(pg_num)
    {
        namespace fs = boost::filesystem;
        fs::path file_path(filename);

        // check input file
        if (util::fs::check_valid_input_file(file_path)) // throws if error
        {
            // check that it looks like a PDF by looking at the header
            // (first 8 bytes. e.g.: "%PDF-1.3")
            fs::ifstream f(filename);
            char buf[9] = { 0 };
            f.read(buf, 8);
            f.close();
            buf[8] = 0;

            boost::cmatch what;
            if (!boost::regex_match(buf, what, boost::regex("%PDF\\-[1-9]\\.[0-9]"), boost::match_default)) {
                std::stringstream ss;
                ss << file_name << " does not look like a valid PDF";
                throw invalid_file(ss.str());
            }
        }

        // check the destination
        fs::path output_filepath(out_filename);
        fs::path parent_path(output_filepath.parent_path());

        // output file path is required and will have to be created so
        // check that its parent path exists
        if (!parent_path.empty()) {
            if (!fs::exists(parent_path) || !fs::is_directory(parent_path)) {
                std::stringstream ss;
                ss << "destination folder '" << parent_path << "' does not exist";
                throw invalid_file(ss.str());
            }
            output_path = parent_path.string();
        }

        // check if the destination file exists
        if (fs::exists(output_filepath))
        {
            // remove the file if asked to do so
            if (flags.force_output_write) {
                // but check if it's a file that can be deleted
                if (!fs::is_regular_file(output_filepath)) {
                    std::stringstream ss;
                    ss << output_filepath << " destination exists but it is not a regular file and can't be overwritten";
                    throw invalid_file(ss.str());

                }
                fs::remove(output_filepath);
            } else {
                // something exists with the name
                std::stringstream ss;
                ss << output_filepath << " destination file exists";
                throw invalid_file(ss.str());
            }
        }

        // -- font maps --
        std::string f_map;

        // clear any loaded maps and load the default
        doc_font_maps.clear();

        // check input font map to make sure it's valid
        if (!fontmap.empty()) {
            // check the name in case it's passed with an absolute path
            try
            {
                if (util::fs::check_valid_input_file(fontmap)) {
                    f_map = fontmap;
                }

            } catch (std::exception& e) {
                // otherwise, load from the config directory
                fs::path mapfile(DEFAULT_CONFIG_DIR);
                mapfile.append(fontmap).replace_extension(FONT_MAP_FILE_EXT);

                // set the absolute path to the font map, then check it
                if (util::fs::check_valid_input_file(mapfile)) { // throws if error
                    f_map = mapfile.string();
                }
            }
        }

        // load the default config file - throws if it fails
        util::config::read_map_config(doc_font_maps, DEFAULT_FONT_MAP);

        // load the font map if set
        if (!f_map.empty()) {
            // load_config throws if error
            if (load_config(f_map)) {
                font_map = f_map;
            }
        }

        // configure some useful paths, etc.
        doc_base_name = output_filepath.stem().string();

        // determine the resource directory based on the output path
        // but don't create it yet as some documents might not have
        // images, etc. that will need saving.
        fs::path res_dir = (parent_path / doc_base_name);

        // but, if it exists, make sure it's a directory
        if (fs::exists(res_dir) && !fs::is_directory(res_dir)) {
            std::stringstream ss;
            ss << res_dir.string() << " resource path exists but is not a folder";
            throw invalid_file(ss.str());
        }
        resource_dir = res_dir.string();

        //        std::cerr << *this << std::endl;
    }


    //
    // load the default config followed by the specified font map
    bool Options::load_config(const std::string& new_font_map_file)
    {
        char* font_map_data;
        // try load the file - first read the contents
        if (!util::fs::read_text_file(new_font_map_file, &font_map_data)) {
            std::stringstream ss;
            ss << "Error reading specified font map file: " << new_font_map_file << std::endl;
            throw invalid_file(ss.str());
        }

        // parse the JSON - throws if error
        util::config::read_map_config(doc_font_maps, font_map_data);
        delete [] font_map_data;

        return true;
    }


    //
    // create absolute and relative image paths
    bool Options::get_image_path(intmax_t img_id, std::string& image_path, bool create_res_dir) const
    {
        boost::filesystem::path file_path(resource_dir);

        // ensure the resource dir exists
        if (create_res_dir && !util::fs::create_fs_dir(file_path)) {
            return false;
        }

        // create the absolute path name for the image file
        std::stringstream image_filename;
        image_filename << doc_base_name << "-" << img_id << IMAGE_FILE_EXT;

        file_path.append(image_filename.str());
        image_path = file_path.string();
        return true;
    }

    //
    // extract the relative path of the image combining the resource
    // directory name and image name
    std::string Options::get_image_rel_path(const std::string& image_abs_file_name) const {
        boost::filesystem::path abs_path(image_abs_file_name);
        return (abs_path.parent_path().filename() / abs_path.filename()).string();
    }

    //
    // info output
    std::ostream& operator<<(std::ostream& o, const Options& opt)
    {
        o << "+- Options -+" << std::endl
          << "   Input file:        " << opt.file_name << std::endl
          << "   Doc base name:     " << opt.doc_base_name << std::endl
          << "   Output file:       " << opt.output_file << std::endl;
        if (!opt.output_path.empty()) {
            o << "   Output root path:  " << opt.output_path << boost::filesystem::path::preferred_separator << std::endl;
        }
        o << "   Resource path:     " << opt.resource_dir << boost::filesystem::path::preferred_separator << std::endl;

        {
            // image path test
            std::string test;
            opt.get_image_path(1, test, false);
            o << "   Image output test: " << test << std::endl
              << "   Relative img test: " << opt.get_image_rel_path(test) << std::endl;
        }

        if (!opt.font_map.empty()) {
            o << "   Font map file:     " << opt.font_map << std::endl;
        }

        if (opt.page_num != -1) {
            o << "   req'd page number: " <<opt.page_num;
        }

        std::list<std::string> opts;
        if (opt.flags.omit_outline)
            opts.push_back("omit_outline");
        if (opt.flags.use_page_crop_box)
            opts.push_back("use_page_crop_box");
        if (opt.flags.crop_page)
            opts.push_back("crop_page");
        if (opt.flags.include_invisible_text)
            opts.push_back("invisible_text");
        if (opt.flags.link_output_only)
            opts.push_back("links_only");
        if (opt.flags.include_debug_info)
            opts.push_back("debug_info");
        if (opt.flags.libpng_use_best_compression)
            opts.push_back("png_best_comp");
        if (opt.flags.force_font_preprocess)
            opts.push_back("font_preprocess");
        if (opt.flags.force_output_write)
            opts.push_back("force_output_write");

        if (!opts.empty()) {
            o << "   Flags:             ";
            for (const std::string& s : opts) { o << s << " "; }
        }

        return o;
    }

} // namespace
