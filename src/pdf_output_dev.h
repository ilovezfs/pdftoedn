#pragma once

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <queue>

#include <poppler/GfxState.h>

#include "eng_output_dev.h"
#include "graphics.h"

namespace pdftoedn
{
    class FontEngine;
    class StreamProps;

    //------------------------------------------------------------------------
    // pdftoedn::OutputDev
    //------------------------------------------------------------------------
    class OutputDev : public EngOutputDev {
    public:
        // for inlined images (do not have a ref id)
        enum { IMG_RES_ID_UNDEF = -1 };

        // constructor takes reference to object that will store
        // extracted data
        OutputDev(Catalog* doc_cat, pdftoedn::FontEngine& fnt_engine) :
            EngOutputDev(doc_cat),
            font_engine(fnt_engine),
            inline_img_id(IMG_RES_ID_UNDEF - 1)
        { }
        virtual ~OutputDev() { }

        // set up font manager, etc.
        bool init();

        // POPPLER virtual interface
        // =========================
        // Does this device use upside-down coordinates?
        // (Upside-down means (0,0) is the top left corner of the page.)
        virtual GBool upsideDown() { return gTrue; }

        virtual GBool needNonText() { return gTrue; }
        virtual GBool needCharCount() { return gTrue; }

        // Does this device use drawChar() or drawString()?
        virtual GBool useDrawChar() { return gTrue; }

        virtual GBool useTilingPatternFill() { return gTrue; }

        // Does this device use beginType3Char/endType3Char?  Otherwise,
        // text in Type 3 fonts will be drawn with drawChar/drawString.
        virtual GBool interpretType3Chars() { return gTrue; }

        // This device now supports text in pattern colorspace!
        virtual GBool supportTextCSPattern(GfxState *state) {
            return state->getFillColorSpace()->getMode() == csPattern;
        }

        //----- initialization and control
        virtual void startPage(int pageNum, GfxState *state, XRef *xref);
        virtual void endPage();

        //----- update graphics state
        virtual void saveState(GfxState* state);
        virtual void restoreState(GfxState* state);

        virtual void updateAll(GfxState *state);
        virtual void updateLineDash(GfxState* state);
        virtual void updateFlatness(GfxState * /*state*/) {}
        virtual void updateLineJoin(GfxState *state);
        virtual void updateLineCap(GfxState *state);
        virtual void updateMiterLimit(GfxState *state);
        virtual void updateLineWidth(GfxState *state);
        virtual void updateAlphaIsShape(GfxState *state);
        virtual void updateTextKnockout(GfxState *state);
        virtual void updateFillColorSpace(GfxState * /*state*/) {}
        virtual void updateStrokeColorSpace(GfxState * /*state*/) {}
        virtual void updateFillColor(GfxState *state);
        virtual void updateStrokeColor(GfxState *state);
        virtual void updateBlendMode(GfxState *state);
        virtual void updateFillOpacity(GfxState *state);
        virtual void updateStrokeOpacity(GfxState *state);
        virtual void updateFillOverprint(GfxState *state);
        virtual void updateStrokeOverprint(GfxState *state);
        virtual void updateOverprintMode(GfxState* state);
        virtual void updateTransfer(GfxState * /*state*/) {}
        virtual void updateFillColorStop(GfxState * /*state*/, double /*offset*/) {}

        //----- text state
        virtual void updateFont(GfxState* state);
        virtual void updateTextShift(GfxState* state, double shift);

        //----- text drawing
        virtual void drawChar(GfxState *state, double x, double y,
                              double dx, double dy,
                              double originX, double originY,
                              CharCode code, int nBytes, Unicode *u, int uLen);
        virtual void beginActualText(GfxState* state, GooString *text );
        virtual void endActualText(GfxState * /*state*/) { }

        //----- paths
        virtual void stroke(GfxState *state);
        virtual void fill(GfxState *state);
        virtual void eoFill(GfxState *state);

        //----- patterns
        virtual GBool tilingPatternFill(GfxState * /*state*/, Gfx * /*gfx*/, Catalog * /*cat*/, Object * /*str*/,
                                        double * /*pmat*/, int /*paintType*/, int /*tilingType*/, Dict * /*resDict*/,
                                        double * /*mat*/, double * /*bbox*/,
                                        int /*x0*/, int /*y0*/, int /*x1*/, int /*y1*/,
                                        double /*xStep*/, double /*yStep*/);

        //----- clipping
        virtual void clip(GfxState *state);
        virtual void eoClip(GfxState *state);
        virtual void clipToStrokePath(GfxState *state);

        //----- images
        virtual void drawImageMask(GfxState *state, Object *ref, Stream *str,
                                   int width, int height, GBool invert, GBool interpolate,
                                   GBool inlineImg);
        virtual void drawImage(GfxState *state, Object *ref, Stream *str,
                               int width, int height, GfxImageColorMap *colorMap,
                               GBool interpolate, int *maskColors, GBool inlineImg);

        virtual void setSoftMaskFromImageMask(GfxState *state,
                                              Object *ref, Stream *str,
                                              int width, int height, GBool invert,
                                              GBool inlineImg, double *baseMatrix);
        virtual void unsetSoftMaskFromImageMask(GfxState *state, double *baseMatrix);
        virtual void drawMaskedImage(GfxState *state, Object *ref, Stream *str,
                                     int width, int height,
                                     GfxImageColorMap *colorMap, GBool interpolate,
                                     Stream *maskStr, int maskWidth, int maskHeight,
                                     GBool maskInvert, GBool maskInterpolate);
        virtual void drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str,
                                         int width, int height,
                                         GfxImageColorMap *colorMap,
                                         GBool interpolate,
                                         Stream *maskStr,
                                         int maskWidth, int maskHeight,
                                         GfxImageColorMap *maskColorMap,
                                         GBool maskInterpolate);

        //----- grouping operators
        virtual void endMarkedContent(GfxState *state);
        virtual void beginMarkedContent(char *name, Dict *properties);
        virtual void markPoint(char *name);
        virtual void markPoint(char *name, Dict *properties);

        //----- transparency groups and soft masks
        virtual GBool checkTransparencyGroup(GfxState * /*state*/, GBool /*knockout*/) { return gTrue; }
        virtual void beginTransparencyGroup(GfxState * /*state*/, double * /*bbox*/,
                                            GfxColorSpace * /*blendingColorSpace*/,
                                            GBool /*isolated*/, GBool /*knockout*/,
                                            GBool /*forSoftMask*/);
        virtual void endTransparencyGroup(GfxState * /*state*/);
        virtual void paintTransparencyGroup(GfxState * /*state*/, double * /*bbox*/);
        virtual void setSoftMask(GfxState * /*state*/, double * /*bbox*/, GBool /*alpha*/,
                                 Function * /*transferFunc*/, GfxColor * /*backdropColor*/);
        virtual void clearSoftMask(GfxState * /*state*/);

    private:
        pdftoedn::FontEngine& font_engine;
        PdfTM text_tm;
        std::queue<Unicode> actual_text;
        int inline_img_id;

        // non-virtual methods; helpers
        bool process_image_blob(const std::ostringstream* blob, const PdfTM& ctm,
                                const BoundingBox& bbox, const StreamProps& properties,
                                int width, int height,
                                intmax_t& ref_num);
        void build_path_command(GfxState* state, PdfDocPath::Type type,
                                PdfDocPath::EvenOddRule eo_rule = PdfDocPath::EVEN_ODD_RULE_DISABLED);
    };

} // namespace
