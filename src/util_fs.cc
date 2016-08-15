#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif
#include <boost/filesystem.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "util_fs.h"
#include "pdf_error_tracker.h"

namespace pdftoedn
{
    namespace util
    {
        namespace fs
        {
            bool create_fs_dir(const boost::filesystem::path& dir)
            {
                namespace fs = boost::filesystem;

                // create it if it doesn't exist
                if (fs::exists(dir)) {
                    // if it exists, error out if it's NOT a directory
                    if (!fs::is_directory(dir)) {
                        std::cerr << "directory " << dir << " can't be created because it exists and is not a directory" << std::endl;
                        return false;
                    }
                }
                else {
                    // otherwise try to create it
                    return (fs::create_directories(dir));
                }
                return true;
            }


            //
            // check for valid input files
            bool check_valid_input_file(const boost::filesystem::path& infile)
            {
                namespace fs = boost::filesystem;

                // check input file
                if (!fs::exists(infile)) {
                    std::stringstream ss;
                    ss << infile << " does not exist" << std::endl;
                    throw invalid_file(ss.str());
                }

                if (!fs::is_regular_file(infile)) {
                    std::stringstream ss;
                    ss << infile << " is not a valid file" << std::endl;
                    throw invalid_file(ss.str());
                }
                return true;
            }


            bool directory_files(const std::string& dir, std::set<std::string>& file_list,
                                 const std::string& ext)
            {
                namespace fs = boost::filesystem;

                const fs::path& dir_path(dir);
                if (!exists(dir_path)) {
                    return false;
                }

                fs::directory_iterator end_itr; // default construction yields past-the-end
                std::string file;
                const uint32_t ext_len = ext.length();

                for ( fs::directory_iterator itr( dir_path );
                      itr != end_itr;
                      ++itr )
                {
                    // skip directories
                    if ( is_directory(itr->status()) ) {
                        continue;
                    }

                    bool match = true;
                    file = itr->path().filename().string();

                    // if there's an extension to match, check against it
                    if (ext_len > 0) {
                        uint32_t file_len = file.length();
                        if (file_len <= ext_len) {
                            continue;
                        }
                        uint32_t ii = ext_len;
                        while (ii) {
                            if (file[--file_len] != ext[--ii]) {
                                match = false;
                                break;
                            }
                        }
                    }

                    if (match) {
                        file_list.insert(file);
                    }
                }
                return true;
            }

            //
            // write image blob to disk
            bool write_image_to_disk(const std::string& filename, const std::string& blob,
                                     bool overwrite)
            {
                // TODO: overwrite is now false by default but maybe
                // check if destination is the same and overwrite?
                if (overwrite || !boost::filesystem::exists(filename))
                {
                    std::ofstream file;
                    file.open(filename.c_str());

                    if (file.is_open()) {
                        file << blob;
                        file.close();
                    }
                }
                return true;
            }


            //
            // opens file, reads size, allocates buffer for storage
            // (pointed to by *data), and copies content to it. NOTE:
            // caller must free buffer
            bool read_text_file(const std::string& filename, char** data)
            {
                *data = NULL;

                // open it for reading
                std::ifstream tmp(filename, std::ifstream::in);
                if (!tmp.is_open()) {
                    return false;
                }

                // determine the size
                tmp.seekg(0, std::ios::end);
                std::size_t size = tmp.tellg();

                // allocate and copy contents of file to the buffer
                // with room to NULL-terminate
                *data = new char[size + 1];
                tmp.seekg(0, std::ios::beg);
                tmp.read(*data, size);
                tmp.close();
                (*data)[size] = 0;
                return true;
            }

        } // fs
    } // util
} // namespace
