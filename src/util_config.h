#pragma once

#include <string>

namespace pdftoedn
{
    class DocFontMaps;

    namespace util
    {
        namespace config {

            bool read_map_config(const std::string& config_file, pdftoedn::DocFontMaps& maps);
        }
    }
}
