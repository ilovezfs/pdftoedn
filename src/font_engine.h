#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif
#include <poppler/poppler-config.h>
#include <poppler/OutputDev.h>
#include <poppler/GfxFont.h>
#ifdef __clang__
#pragma clang diagnostic push
#endif

#include "pdf_font_source.h"
#include "font_maps.h"
#include "util_versions.h"

//#define FE_PREPROCESS_TEXT // collect text usage while pre-processing doc

class GfxState;

namespace pdftoedn
{
    class PdfFont;
    class PdfPath;

    typedef std::map<PdfRef, pdftoedn::PdfFont *> FontList;

    // -------------------------------------------------------
    // Our FE - handles font creation (via FT) and caching
    //
    class FontEngine
    {
    public:
        // constructor / destructor
        FontEngine(XRef *doc_xref);
        virtual ~FontEngine();

        bool found_font_warnings() const { return has_font_warnings; }

        // poppler > 0.24.0 passes a doc Xref on every call to startPage
        void update_document_ref(XRef* doc_xref) { xref = doc_xref; }

        // load system / embedded document font using FT
        pdftoedn::PdfFont* load_font(GfxFont* gfx_font);

        //
        // some ops can be helped if we provide a sorted list of font
        // sizes used in a document. Since we now pre-process all text, we
        // can easily take care of this and return it as part of the
        // document meta
        void track_font_size(double font_size) { font_sizes.insert(font_size); }
        // retrieve a sorted list of the tracked font sizes
        const std::set<double>& get_font_size_list() const { return font_sizes; }
        const FontList& get_font_list() const { return fonts; }

        // remap a character code
        enum eCodeRemapStatus {
            CODE_REMAP_ERROR,
            CODE_REMAP_IGNORE,
            CODE_REMAPPED_BY_POPPLER,
            CODE_REMAPPED_BY_EDSEL
        };
        eCodeRemapStatus get_code_unicode(CharCode code, Unicode* const u, uintmax_t& unicode);

    private:
        XRef *xref; // PDF document ref for object lookup
        bool has_font_warnings;
        FontList fonts;
        std::set<double> font_sizes;
        FT_Library ft_lib;
        pdftoedn::PdfFont* cur_doc_font;

        pdftoedn::PdfFont* find_font(GfxFont* gfx_font) const;
        static std::string sanitize_font_name(const std::string& name);

        friend std::string util::version::freetype(const FontEngine&);
    };

    //
    // font engine device - used to pre-process the document and
    // extract the font data. This is needed because document
    // processing is done page by page so we won't know what fonts
    // used until all pages are done
    class FontEngDev : public ::OutputDev
    {
    public:
        FontEngDev(pdftoedn::FontEngine& fnt_engine) :
            font_engine(fnt_engine), cur_font(NULL)
        { }
        virtual ~FontEngDev() { }

        // need to define these
        virtual GBool upsideDown() { return gTrue; }
        virtual GBool useDrawChar() { return gTrue; }
        virtual GBool interpretType3Chars() { return gTrue; }
        virtual GBool needNonText() { return gFalse; }

        virtual void startPage(int pageNum, GfxState *state, XRef *xref) { }

        // but we only care about these here
        virtual void updateFont(GfxState *state);
#ifdef FE_PREPROCESS_TEXT
        virtual void drawChar(GfxState *state, double x, double y,
                              double dx, double dy,
                              double originX, double originY,
                              CharCode code, int nBytes, Unicode *u, int uLen);
#endif // FE_PREPROCESS_TEXT

    private:
        pdftoedn::FontEngine& font_engine;
        pdftoedn::PdfFont* cur_font;
    };

} // namespace

