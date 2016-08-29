#pragma once

#ifdef FE_PREPROCESS_TEXT

#include "font.h"

namespace pdftoedn
{
    class FontEngine;

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
        virtual void drawChar(GfxState *state, double x, double y,
                              double dx, double dy,
                              double originX, double originY,
                              CharCode code, int nBytes, Unicode *u, int uLen);

    private:
        pdftoedn::FontEngine& font_engine;
        pdftoedn::PdfFont* cur_font;
    };

} // namespace

#endif // FE_PREPROCESS_TEXT
