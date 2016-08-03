#pragma once

#include <string>
#include <set>
#include <boost/filesystem.hpp>

namespace pdftoedn
{
    namespace util
    {
        namespace fs {
            bool create_fs_dir(const boost::filesystem::path& path);
            bool check_valid_input_file(const boost::filesystem::path& infile);
            bool directory_exists(const  boost::filesystem::path& dir);
            bool directory_files(const std::string& dir, std::set<std::string>& file_list,
                                 const std::string& ext = "");
            bool write_image_to_disk(const std::string& filename, const std::string& blob,
                                     bool overwrite = false);
            bool read_text_file(const std::string& filename, char** data);
        }
    }
}
