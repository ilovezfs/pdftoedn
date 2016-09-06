#include <cstdlib>
#include <string>
#include <sstream>
#include <regex>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/version.hpp>
#include <png.h>
#include <leptonica/allheaders.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <poppler/cpp/poppler-version.h>
#include <rapidjson/rapidjson.h>

#ifdef HAVE_LIBOPENSSL
#include <openssl/crypto.h>
#endif

#include "font_engine.h"
#include "util_versions.h"
#include "util_edn.h"

namespace pdftoedn {
    namespace util {
        namespace version {

            const pdftoedn::Symbol SYMBOL_APP          = "pdftoedn";
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
                std::stringstream ver;
                ver << ft_maj << "." << ft_min << "." <<ft_patch;
                return ver.str();
            }

            std::string rapidjson() {
                return RAPIDJSON_VERSION_STRING;
            }

            //
            // return lib version numbers
            std::string info() {
                // create a dummy FE to get the runtime version of FT
                FontEngine fe(NULL);

                std::stringstream ver;
                ver << " poppler " << poppler() << std::endl
                    << " libpng " << libpng() << std::endl
                    << " boost " << boost() << std::endl
                    << " freetype " << freetype(fe) << std::endl
                    << " leptonica " << leptonica() << std::endl
                    << " rapidjson " << rapidjson() << std::endl;

#ifdef HAVE_LIBOPENSSL
                {
                    std::string v;
                    std::smatch match;

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
                    v = OpenSSL_version(0);
#else
                    v = SSLeay_version(0);
#endif

                    if (std::regex_search(v, match, std::regex("[0-9]+.[0-9]+.[0-9]+(^ )?"))) {
                        ver << " openssl " << match[0] << std::endl;
                    }
                }
#endif

                return ver.str();
            }

            // version EDN hash
            util::edn::Hash& libs(const pdftoedn::FontEngine& fe, util::edn::Hash& version_h)
            {
                version_h.reserve(7);
                version_h.push( SYMBOL_APP      , PDFTOEDN_VERSION );
                version_h.push( SYMBOL_POPPLER  , poppler() );
                version_h.push( SYMBOL_LIBPNG   , libpng() );
                version_h.push( SYMBOL_BOOST    , boost() );
                version_h.push( SYMBOL_FREETYPE , freetype(fe) );
                version_h.push( SYMBOL_LEPTONICA, leptonica() );
                version_h.push( SYMBOL_RAPIDJSON, rapidjson() );
                return version_h;
            }
        } // version
    } // util
} // namespace
