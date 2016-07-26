#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>

#ifdef EDSEL_RUBY_GEM
#include <rice/Hash.hpp>
#else
#include "config.h"
#endif

#include <boost/version.hpp>
#include <png.h>
#include <leptonica/allheaders.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cpp/poppler-version.h>
#include <rapidjson/rapidjson.h>

#include "font_engine.h"
#include "util_versions.h"
#include "util_edn.h"

namespace pdftoedn {
    namespace util {
        namespace version {

            const pdftoedn::Symbol SYMBOL_APP          = "edsel";
            const pdftoedn::Symbol SYMBOL_POPPLER      = "poppler";
            const pdftoedn::Symbol SYMBOL_LIBPNG       = "libpng";
            const pdftoedn::Symbol SYMBOL_BOOST        = "boost";
            const pdftoedn::Symbol SYMBOL_FREETYPE     = "freetype";
            const pdftoedn::Symbol SYMBOL_LEPTONICA    = "leptonica";
            const pdftoedn::Symbol SYMBOL_RAPIDJSON    = "rapidjson";

            // get versions for all libs - will include these in meta
            // output
            std::string poppler() { return poppler::version_string(); }
            std::string libpng() { return png_libpng_ver; }
            std::string boost() {
                std::string b_v = BOOST_LIB_VERSION;
                int underscore = b_v.find('_');
                b_v.replace(underscore, 1, ".");
                return b_v;
            }
            std::string leptonica() {
                std::string l_v;

                { // leptonica requires we free the version string
                    const char* v = getLeptonicaVersion();
                    l_v = v;
                    free((void*) v);
                }

                int dash = l_v.find('-') + 1;
                return l_v.substr(dash);
            }
            std::string freetype(const FontEngine& fe) {
                FT_Int ft_maj, ft_min, ft_patch;
                FT_Library_Version(fe.ft_lib, &ft_maj, &ft_min, &ft_patch);
                std::stringstream ss;
                ss << ft_maj << "." << ft_min << "." <<ft_patch;
                return ss.str();
            }

            std::string rapidjson() {
                return RAPIDJSON_VERSION_STRING;
            }

            //
            // return lib version numbers
            std::string info() {
                // create a dummy FE to get the runtime version of FT
                FontEngine fe(NULL);
                fe.init();

                std::stringstream v;
                v << " poppler " << poppler() << std::endl
                  << " libpng " << libpng() << std::endl
                  << " boost " << boost() << std::endl
                  << " freetype " << freetype(fe) << std::endl
                  << " leptonica " << leptonica() << std::endl
                  << " rapidjson " << rapidjson() << std::endl;
                return v.str();
            }

#ifdef EDSEL_RUBY_GEM
            // ext version info - obtained from Ruby side
            static std::string ext_version;
            void set_ext_version(const std::string& version) { ext_version = version; }

            //
            // versions for returned meta
            Rice::Object libs(const FontEngine& fe)
            {
                Rice::Hash version_h;
                version_h[ SYMBOL_APP ]          = ext_version;
                version_h[ SYMBOL_POPPLER ]      = poppler();
                version_h[ SYMBOL_LIBPNG  ]      = libpng();
                version_h[ SYMBOL_BOOST ]        = boost();
                if (fe.is_ok()) {
                    version_h[ SYMBOL_FREETYPE ] = freetype(fe);
                }
                version_h[ SYMBOL_LEPTONICA ]    = leptonica();
                version_h[ SYMBOL_RAPIDJSON ]    = rapidjson();
                return version_h;
            }
#else
            util::edn::Hash& libs(const pdftoedn::FontEngine& fe, util::edn::Hash& version_h)
            {
                version_h.reserve(9);
                version_h.push( SYMBOL_APP         , PDFTOEDN_VERSION );
                version_h.push( SYMBOL_POPPLER     , poppler() );
                version_h.push( SYMBOL_LIBPNG      , libpng() );
                version_h.push( SYMBOL_BOOST       , boost() );
                if (fe.is_ok()) {
                    version_h.push( SYMBOL_FREETYPE, freetype(fe) );
                }
                version_h.push( SYMBOL_LEPTONICA   , leptonica() );
                version_h.push( SYMBOL_RAPIDJSON   , rapidjson() );
                return version_h;
            }
#endif
        } // version
    } // util
} // namespace
