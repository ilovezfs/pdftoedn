#include <poppler/GfxState.h>

#include "link_output_dev.h"
#include "doc_page.h"

namespace pdftoedn
{
    //------------------------------------------------------------------------
    // LinkOutputDev
    //------------------------------------------------------------------------
    void LinkOutputDev::startPage(int pageNum, GfxState *state, XRef *xref)
    {
        int w, h, rot;

        // set up graphics output for page
        if (state) {
            w = (int)(state->getPageWidth() + 0.5);
            if (w <= 0) {
                w = 1;
            }
            h = (int)(state->getPageHeight() + 0.5);
            if (h <= 0) {
                h = 1;
            }
            rot = state->getRotate();
        } else {
            w = h = 1;
            rot = 0;
        }

        // allocate a new page in the collector doc. and register the
        // error callback
        if (page_out) {
            delete page_out;
        }

        page_out = new pdftoedn::PdfPage(pageNum, w, h, rot);

        // links
        process_page_links( pageNum );
    }

} // namespace
