//
// Copyright 2016-2017 Ed Porras
//
// This file is part of pdftoedn.
//
// pdftoedn is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pdftoedn is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pdftoedn.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "eng_output_dev.h"

namespace pdftoedn
{
    //------------------------------------------------------------------------
    // LinkOutputDev
    //------------------------------------------------------------------------
    class LinkOutputDev : public EngOutputDev
    {
    public:
        // constructor takes reference to object that will store
        // extracted data
        LinkOutputDev(Catalog* doc_cat) :
            EngOutputDev(doc_cat) { }
        virtual ~LinkOutputDev() { }

        // POPPLER virtual interface
        // =========================
        // Does this device use upside-down coordinates?
        // (Upside-down means (0,0) is the top left corner of the page.)
        virtual GBool upsideDown() { return gTrue; }

        virtual GBool needNonText() { return gFalse; }
        virtual GBool needCharCount() { return gFalse; }

        // Does this device use drawChar() or drawString()?
        virtual GBool useDrawChar() { return gFalse; }
        virtual GBool useTilingPatternFill() { return gFalse; }

        // Does this device use beginType3Char/endType3Char?  Otherwise,
        // text in Type 3 fonts will be drawn with drawChar/drawString.
        virtual GBool interpretType3Chars() { return gFalse; }

        // This device now supports text in pattern colorspace!
        virtual GBool supportTextCSPattern(GfxState *state) { return gFalse; }

        //----- initialization and control
        virtual void startPage(int pageNum, GfxState *state, XRef *xref);

        //----- text drawing
        virtual void beginString(GfxState * /*state*/, GooString * /*s*/) {}
        virtual void endString(GfxState * /*state*/) {}
    };

} // namespace
