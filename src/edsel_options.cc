#include <iostream>
#include <string>
#include <ostream>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

#include "edsel_options.h"
#include "util.h"
#include "util_fs.h"

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
        boost::filesystem::path file_path(filename);

        // check input file
        if (util::fs::check_valid_input_file(file_path)) // throws if error
        {
            // check that it looks like a PDF by looking at the header (first 8 bytes. e.g.: "%PDF-1.3")
            boost::filesystem::ifstream f(filename);
            char buf[9] = { 0 };
            f.read(buf, 8);
            f.close();
            buf[8] = 0;

            boost::cmatch what;
            if (!boost::regex_match(buf, what, boost::regex("%PDF\\-[1-9]\\.[0-9]"), boost::match_default)) {
                std::cerr << file_name << " does not look like a valid PDF" << std::endl;
                throw std::exception();
            }
        }

        // check input font map
        if (!fontmap.empty()) {
            boost::filesystem::path mapfile(map_config_path());
            mapfile.append(fontmap).replace_extension(FONT_MAP_FILE_EXT);

            // set the absolute path to the font map, then check it
            if (util::fs::check_valid_input_file(mapfile)) { // throws if error
                font_map = mapfile.string();
            }
        }

        // check the destination
        boost::filesystem::path output_filepath(out_filename);
        boost::filesystem::path parent_path(output_filepath.parent_path());

        // output filen path is required so check that its parent path exists
        if (!parent_path.empty()) {
            if (!util::fs::directory_exists(parent_path)) {
                std::cerr << "destination folder '" << parent_path << "' does not exist" << std::endl;
                throw std::exception();
            }
            output_path = parent_path.string();
        }

        // check if the destination file exists
        if (boost::filesystem::exists(output_filepath))
        {
            // remove the file if asked to do so
            if (boost::filesystem::is_regular_file(output_filepath) && flags.force_output_write) {
                boost::filesystem::remove(output_filepath);
            } else {
                // something exists with the name
                std::cerr << output_filepath << " destination file exists" << std::endl;
                throw std::exception();
            }
        }

        // configure some useful paths, etc.
        doc_base_name = output_filepath.stem().string();

        // determine the resource directory based on the output path
        // but don't create it yet as some documents might not have
        // images, etc. that will need saving.
        resource_dir = (parent_path / doc_base_name).string();

        //        std::cerr << *this << std::endl;
    }

    //
    // absolute path to the map file
    std::string Options::get_absolute_map_path(const std::string& name) {
        boost::filesystem::path mapfile(options.map_config_path());
        mapfile.append(name).replace_extension(FONT_MAP_FILE_EXT);
        return mapfile.string();
    }

    const std::string& Options::map_config_path() const {
        // TODO: cleanup
        return DEFAULT_CONFIG_DIR;
    }

    //
    // create absolute and relative image paths
    bool Options::get_image_path(intmax_t img_id, std::string& image_path) const
    {
        boost::filesystem::path file_path(resource_dir);

        // ensure the resource dir exists
        if (!util::fs::create_fs_dir(file_path)) {
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
    // output op
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
            opt.get_image_path(1, test);
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
        if (opt.flags.omit_outline) opts.push_back("omit_outline");
        if (opt.flags.use_page_crop_box) opts.push_back("use_page_crop_box");
        if (opt.flags.crop_page) opts.push_back("crop_page");
        if (opt.flags.include_invisible_text) opts.push_back("invisible_text");
        if (opt.flags.link_output_only) opts.push_back("links_only");
        if (opt.flags.include_debug_info) opts.push_back("debug_info");
        if (opt.flags.libpng_use_best_compression) opts.push_back("png_best_comp");
        if (opt.flags.force_font_preprocess) opts.push_back("font_preprocess");
        if (opt.flags.force_output_write) opts.push_back("force_output_write");

        if (!opts.empty())
        {
            o << "   Flags:             ";
            std::for_each( opts.begin(), opts.end(),
                           [&](const std::string& s) { o << s << " "; } );
        }

        return o;
    }

} // namespace
