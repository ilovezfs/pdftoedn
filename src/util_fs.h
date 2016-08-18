#pragma once

#include <string>
#include <boost/filesystem.hpp>

namespace pdftoedn
{
    namespace util
    {
        namespace fs {
            bool create_fs_dir(const boost::filesystem::path& path);
            void expand_path(std::string& path);
            bool check_valid_input_file(const boost::filesystem::path& infile);
            bool write_image_to_disk(const std::string& filename, const std::string& blob,
                                     bool overwrite = false);
            bool read_text_file(const std::string& filename, char** data);
        }
    }
}
