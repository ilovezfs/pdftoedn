#pragma once

#include <string>

#include <poppler/Error.h>
#include <poppler/GfxState.h>
#include <poppler/GfxFont.h>

namespace pdftoedn
{
    namespace util
    {
        namespace debug {
            std::string get_poppler_doc_error_str(int errCode);
            const char* get_poppler_error_str(ErrorCategory e);
            const char* get_stream_kind_str(StreamKind sk);
            const char* get_font_type_str(FontSource::FontType type);
            const char* get_font_type_str(GfxFontType type);
            const char* get_blend_mode_str(GfxBlendMode mode);
            void save_blob_to_disk(const std::ostringstream& blob, const std::string& name_pfx, int id);
            const char* get_font_file_extension(GfxFontType type);
        }

    }
}
