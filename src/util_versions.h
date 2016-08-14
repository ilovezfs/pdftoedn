#pragma once

#include <string>
#include "base_types.h"

namespace pdftoedn
{
    class FontEngine;

    namespace util
    {
        namespace version {
            extern const pdftoedn::Symbol SYMBOL_DATA_FORMAT_VERSION;

            // see util_data_format_version.cc for info on version
            // meaning
            uintmax_t data_format_version();

            std::string info();
            util::edn::Hash& libs(const pdftoedn::FontEngine& fe, util::edn::Hash& h);

            // font engine initializes freetype so we need this kludgy crap
            std::string freetype(const pdftoedn::FontEngine&);
        }
    }
}
