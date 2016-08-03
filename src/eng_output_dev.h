#pragma once

#include <cmath>

#include <poppler/OutputDev.h>
#include <poppler/GfxState.h>

namespace pdftoedn
{
    class PdfPage;

    //------------------------------------------------------------------------
    // pdftoedn::EngOutputDev - base class for all our output devices
    //------------------------------------------------------------------------
    class EngOutputDev : public ::OutputDev {
    public:
        EngOutputDev(Catalog* doc_cat) :
            catalog(doc_cat), page_out(NULL) { }
        virtual ~EngOutputDev();

        // skip anything larger than 10 inches
        static bool state_font_size_not_sane(GfxState* state) {
            return (state->getTransformedFontSize() > 10 * (state->getHDPI() + state->getVDPI()));
        }

        static double get_transformed_font_size(GfxState* state) {
            return (std::ceil(state->getTransformedFontSize() * 100) / 100);
        }

        // returns the collected data after displayPage has been
        // called to process a page
        const PdfPage* page_output() const { return page_out; }

    protected:
        Catalog* catalog;
        pdftoedn::PdfPage* page_out;

        void process_page_links(int page_num);
        void create_annot_link(AnnotLink *link);
    };

} // namespace
