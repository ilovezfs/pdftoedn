#pragma once

//
// subclass of Poppler's PdfDoc for reading the contents of a
// PDF
//

#include <string>
#include <list>

#include <poppler/PDFDoc.h>

#include "font_engine.h"
#include "pdf_doc_outline.h"
#include "pdf_output_dev.h"

class LinkGoTo;
class LinkGoToR;
class LinkURI;
class GooList;

namespace pdftoedn
{
    //
    // Poppler PDF Doc reader
    class PDFReader : public PDFDoc
    {
    public:
        static const double DPI_72; // 72.0

        PDFReader();
        virtual ~PDFReader() { delete eng_odev; }

        bool pre_process_fonts();
        std::ostream& process(std::ostream& o);

        friend std::ostream& operator<<(std::ostream& o, PDFReader& doc) {
            return doc.process(o);
        }

    private:
        pdftoedn::FontEngine font_engine;
        pdftoedn::EngOutputDev* eng_odev;
        pdftoedn::PdfOutline outline_output;
        bool use_page_media_box;

        bool init_font_engine();
        bool process_outline(pdftoedn::PdfOutline& outline_output);
        void outline_level(GooList* items, int level, std::list<PdfOutline::Entry *>& entry_list);
        void outline_action_goto(LinkGoTo *link, PdfOutline::Entry& entry);
        void outline_action_goto_r(LinkGoToR *link, PdfOutline::Entry& entry);
        void outline_action_uri(LinkURI *link, PdfOutline::Entry& entry);
        void outline_link_dest(LinkDest* dest, PdfOutline::Entry& entry);
        uintmax_t get_link_page_num(LinkDest* link);

        void process_page(::OutputDev* dev, uintmax_t page);

        // returns document metadata
        std::ostream& output_meta(std::ostream& o);
        std::ostream& output_page(uintmax_t page_num, std::ostream& o);
    };

} // namespace

