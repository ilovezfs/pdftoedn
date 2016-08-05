#pragma once

namespace pdftoedn
{
    class DocFontMaps;

    namespace util
    {
        namespace config {

            bool read_map_config(const char* config_data, pdftoedn::DocFontMaps& maps);
        }
    }

    extern const char* DEFAULT_FONT_MAP;
}
