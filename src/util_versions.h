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
#ifdef EDSEL_RUBY_GEM
            void set_ext_version(const std::string& version);
            Rice::Object libs(const pdftoedn::FontEngine& fe);
#else
            util::edn::Hash& libs(const pdftoedn::FontEngine& fe, util::edn::Hash& h);
#endif
            // font engine initializes freetype so we need this kludgy crap
            std::string freetype(const pdftoedn::FontEngine&);
        }
    }
}
