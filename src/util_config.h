#pragma once

namespace pdftoedn
{
    class DocFontMaps;

    namespace util
    {
        namespace config {
            bool read_map_config(pdftoedn::DocFontMaps& maps, const char* const data);
        }
    }

    extern const char* DEFAULT_FONT_MAP;
}
