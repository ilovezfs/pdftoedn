#pragma once

#include <algorithm>
#include <string>
#include <sstream>

#include <poppler/Error.h>
#include <poppler/CharTypes.h>
#include <poppler/GfxState.h>
#include <poppler/GfxFont.h>

#include "pdf_font_source.h"
#include "image.h"
#include "pdf_error_tracker.h"

class LinkDest;

namespace pdftoedn {

    class PdfLink;
    class FontEngine;

    namespace util {

        // not defined by poppler for whatever stupid reason
        enum TextRenderingModes {
            TEXT_RENDER_FILL,
            TEXT_RENDER_STROKE,
            TEXT_RENDER_FILL_THEN_STROKE,
            TEXT_RENDER_INVISIBLE,
            TEXT_RENDER_FILL_AND_PATH_CLIP,
            TEXT_RENDER_STROKE_AND_PATH_CLIP,
            TEXT_RENDER_FILL_THEN_STROKE_AND_PATH_CLIP,
            TEXT_RENDER_ADD_TO_PATH_CLIP,
        };

        template< typename T >
        void delete_ptr_container_elems(T& c) {
            typedef typename T::value_type E;
            std::for_each( c.begin(), c.end(), [](const E& e) { delete e; });
        }
        template< typename T >
        void delete_ptr_container_elems(std::list<T>& l) {
            while (!l.empty()) { delete l.back(); l.pop_back(); }
        }
        template< typename T >
        void delete_ptr_map_elems(T& c) {
            typedef typename T::value_type E;
            std::for_each( c.begin(), c.end(), [](const E& e) { delete e.second; });
        }

        //
        // defined in util.cc
        std::string md5(const std::string& blob);
        std::string string_to_utf(const std::string& str);
        std::string string_to_utf(const std::wstring& str);
        std::wstring string_to_iso8859(const char* str);
#ifdef EDSEL_RUBY_GEM
        Rice::String string_to_ruby(std::string const& s);
        Rice::String wstring_to_ruby(std::wstring const& w_str);
#endif
        std::string wstring_to_utfstring(std::wstring const& w_str);

        std::wstring unicode_to_wstring(const Unicode* const u, int len);
        uint8_t pdf_to_svg_blend_mode(GfxBlendMode mode);
        void copy_link_meta(PdfLink& link, LinkDest& ldest, double page_height);
        StreamProps::stream_type_e poppler_stream_type_to_edsel(StreamKind k);
        bool poppler_error_to_edsel(ErrorCategory category, const std::string& poppler_msg, int pos,
                                    ErrorTracker::error_type& err, ErrorTracker::error::level& level);
        FontSource::FontType poppler_gfx_font_type_to_edsel(GfxFontType t);

        // trim a wstring
        inline std::wstring &ltrim(std::wstring &s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
            return s;
        }
        inline std::wstring &rtrim(std::wstring &s) {
            s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
            return s;
        }
        inline std::wstring &trim(std::wstring &s) {
            return ltrim(rtrim(s));
        }

        // lower-case a string
        inline std::string& tolower(std::string& d) {
            std::transform(d.begin(), d.end(), d.begin(), ::tolower);
            return d;
        }

        std::string expand_environment_variables(const std::string& s);

    }
} // namespace
