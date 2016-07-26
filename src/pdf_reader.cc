#include <list>

#ifdef EDSEL_RUBY_GEM
// always include rice headers before ruby.h
#include <rice/Array.hpp>
#include <rice/Hash.hpp>
#endif

#include <goo/GooList.h>
#include <Outline.h>
#include <Link.h>
#include <ErrorCodes.h>

#include "util.h"
#include "util_edn.h"
#include "pdf_reader.h"
#include "pdf_output_dev.h"
#include "link_output_dev.h"
#include "font_engine.h"
#include "pdf_doc_outline.h"
#include "doc_page.h"
#include "edsel_options.h"

namespace pdftoedn
{
    static const pdftoedn::Symbol SYMBOL_PDF_FILENAME       = "filename";
    static const pdftoedn::Symbol SYMBOL_PDF_DOC_OK         = "is_ok";
    static const pdftoedn::Symbol SYMBOL_PDF_MAJ_VER        = "pdf_ver_major";
    static const pdftoedn::Symbol SYMBOL_PDF_MIN_VER        = "pdf_ver_minor";
    static const pdftoedn::Symbol SYMBOL_PDF_ENCRYPTED      = "encrypted";
    static const pdftoedn::Symbol SYMBOL_PDF_NUM_PAGES      = "num_pages";
    static const pdftoedn::Symbol SYMBOL_PDF_DOC_FONTS      = "doc_fonts";
    static const pdftoedn::Symbol SYMBOL_PDF_DOC_FONT_SIZES = "font_size_list";

    static const pdftoedn::Symbol SYMBOL_PDF_OUTLINE        = "outline";

    static const pdftoedn::Symbol SYMBOL_FONT_ENG_OK        = "font_engine_ok";
    static const pdftoedn::Symbol SYMBOL_FONT_ENG_FONT_WARN = "found_font_warnings";

    static const pdftoedn::Symbol SYMBOL_LIB_VERSIONS       = "lib_versions";

    const double PDFReader::DPI_72 = 72.0;

    //
    // opens the document and preps things for processing
    PDFReader::PDFReader() :
        PDFDoc(new GooString(pdftoedn::options.filename().c_str()), NULL, NULL),
        init_ok(false),
        font_engine(getXRef()),
        eng_odev(NULL),
        use_page_media_box(true)
    {
        if (isOk()) {

            // TESLA-6245: Mike P requested a way to extract only links
            // from a doc. To do this, we use a different type of
            // OutputDev that ignores everything but links
            if (pdftoedn::options.link_output_only()) {
                eng_odev = new pdftoedn::LinkOutputDev(getCatalog());
                init_ok = true;
            }
            else {

                // pre-process the doc to extract fonts first. Needed
                // if additional font data needs to be included in the
                // meta before pages are parsed
                if (pdftoedn::options.force_pre_process_fonts()) {
                    pre_process_fonts();
                }

                eng_odev = new pdftoedn::OutputDev(getCatalog(), font_engine);

                // initialize font engine (set up freetype, etc.)
                init_ok = font_engine.init();

                if (init_ok) {
                    // use page crop box if requested (page media box is the default)
                    if (pdftoedn::options.use_page_crop_box()) {
                        use_page_media_box = false;
                    }

                    // process outline, if needed
                    if (!(pdftoedn::options.omit_outline())) {
                        process_outline(outline_output);
                    }
                } else {
                    et.log_critical( ErrorTracker::ERROR_FE_INIT_FAILURE, MODULE, "Font Engine failed to initialize" );
                }
            }
        }
    }


    //
    // use a custom OutputDev to only read fonts from the doc
    bool PDFReader::pre_process_fonts()
    {
        // don't run through pages if poppler wasn't able to open the doc
        if (!isOk()) {
            return false;
        }

        // pre-process doc for font data
        FontEngDev fe_dev(font_engine);
        uint32_t num_pages = getNumPages();

        uintmax_t first, last;

        if (pdftoedn::options.page_number() != -1) {
            first = pdftoedn::options.page_number();
            last = first + 1;
        } else {
            first = 1;
            last = num_pages;
        }

        for (uintmax_t page = first; page <= last; page++)
        {
            // process the PDF info on this page
            display_page(&fe_dev, page);
        }

#if 0
        font_engine.dump_font_info();
#endif
        return true;
    }

#ifdef EDSEL_RUBY_GEM
    //
    // basic PDF info, document number of pages, outline
    Rice::Object PDFReader::meta()
    {
        Rice::Hash meta_h;

        meta_h[ util::version::SYMBOL_DATA_FORMAT_VERSION ] = util::version::data_format_version();
        meta_h[ SYMBOL_PDF_FILENAME ]                       = pdftoedn::options.filename();
        meta_h[ SYMBOL_PDF_DOC_OK ]                         = isOk();
        meta_h[ SYMBOL_FONT_ENG_OK ]                        = font_engine.is_ok();

        if (font_engine.found_font_warnings()) {
            meta_h[ SYMBOL_FONT_ENG_FONT_WARN ]             = true;
        }

        if (isOk()) {

            meta_h[ SYMBOL_PDF_MAJ_VER ]                    = getPDFMajorVersion();
            meta_h[ SYMBOL_PDF_MIN_VER ]                    = getPDFMinorVersion();

            meta_h[ SYMBOL_PDF_NUM_PAGES ]                  = getNumPages();

            // outline - empty hash if none
            meta_h[ SYMBOL_PDF_OUTLINE ]                    = outline_output.to_ruby();

            // save the sorted list of font sizes read in the document
            // - to be used in case we need to generate an outline by
            // examining page content. This is only done if the
            // force_font_preprocess option was specified
            const std::set<double>& font_size_list = font_engine.get_font_size_list();
            if (!font_size_list.empty()) {
                Rice::Array font_size_a;
                std::for_each( font_size_list.rbegin(), font_size_list.rend(),
                               [&](const double& d) { font_size_a.push(d); }
                               );
                meta_h[ SYMBOL_PDF_DOC_FONT_SIZES ]     = font_size_a;
            }

            // include document fonts in meta if requested
            if (pdftoedn::options.include_debug_info())
            {
                // document font list
                Rice::Array font_a;

                const FontList& fonts = font_engine.get_font_list();

                uintmax_t idx = 0;
                std::for_each( fonts.begin(), fonts.end(),
                               [&](const std::pair<const pdftoedn::PdfRef, pdftoedn::PdfFont *>& p) {
                                   Rice::Hash font_h = p.second->to_ruby();
                                   font_h[ PdfPage::SYMBOL_FONT_IDX ] = idx++;
                                   font_a.push(font_h);
                               } );
                meta_h[ SYMBOL_PDF_DOC_FONTS ] = font_a;
            }
        }
        else if (getErrorCode() == errEncrypted) {
            // if we're not able to process the doc because it was
            // encrypted, we'll indicate so here.
            meta_h[ SYMBOL_PDF_ENCRYPTED ]                  = true;
        }

        meta_h[ SYMBOL_LIB_VERSIONS ]                       = util::version::libs(font_engine);

        // if we caught errors, include them
        if (et.errors_reported()) {
            meta_h[ ErrorTracker::SYMBOL_ERRORS ]           = et.to_ruby();
        }

        return meta_h;
    }
#endif

    std::ostream& PDFReader::meta(std::ostream& o) {
        util::edn::Hash meta_h(14);

        meta_h.push( util::version::SYMBOL_DATA_FORMAT_VERSION, util::version::data_format_version() );
        meta_h.push( SYMBOL_PDF_FILENAME                      , pdftoedn::options.filename() );
        meta_h.push( SYMBOL_PDF_DOC_OK                        , isOk() );
        meta_h.push( SYMBOL_FONT_ENG_OK                       , font_engine.is_ok() );

        if (font_engine.found_font_warnings()) {
            meta_h.push( SYMBOL_FONT_ENG_FONT_WARN            , true );
        }

        if (isOk()) {

            meta_h.push( SYMBOL_PDF_MAJ_VER                   , (uint8_t) getPDFMajorVersion() );
            meta_h.push( SYMBOL_PDF_MIN_VER                   , (uint8_t) getPDFMinorVersion() );

            meta_h.push( SYMBOL_PDF_NUM_PAGES                 , (uintmax_t) getNumPages() );

            // outline - empty hash if none
            meta_h.push( SYMBOL_PDF_OUTLINE                   , &outline_output );

            // save the sorted list of font sizes read in the
            // document - to be used in case we need to generate
            // an outline by examining page content
            const std::set<double>& font_size_list = font_engine.get_font_size_list();
            if (!font_size_list.empty()) {
                util::edn::Vector font_size_a(font_size_list.size());
                std::for_each( font_size_list.rbegin(), font_size_list.rend(),
                               [&](const double& d) { font_size_a.push(d); }
                               );
                meta_h.push( SYMBOL_PDF_DOC_FONT_SIZES       , font_size_a );
            }

            // include document fonts in meta if requested
            if (pdftoedn::options.include_debug_info())
            {
                // document font list
                const FontList& fonts = font_engine.get_font_list();
                util::edn::Vector font_a(fonts.size());

                uintmax_t idx = 0;
                std::for_each( fonts.begin(), fonts.end(),
                               [&](const std::pair<const pdftoedn::PdfRef, pdftoedn::PdfFont *>& p) {
                                   util::edn::Hash font_h(2);
                                   p.second->to_edn_hash(font_h);

                                   font_h.push( PdfPage::SYMBOL_FONT_IDX, idx++ );
                                   font_a.push(font_h);
                               } );
                meta_h.push( SYMBOL_PDF_DOC_FONTS, font_a );
            }
        }
        else if (getErrorCode() == errEncrypted) {
            // if we're not able to process the doc because it was
            // encrypted, we'll indicate so here.
            meta_h.push( SYMBOL_PDF_ENCRYPTED                 , true );
        }

#ifndef EDSEL_RUBY_GEM
        // TODO: remove #ifdef
        util::edn::Hash version_h;
        meta_h.push( SYMBOL_LIB_VERSIONS                      , util::version::libs(font_engine, version_h));
#endif

        // if we caught errors, include them
        if (et.errors_reported()) {
            meta_h.push( ErrorTracker::SYMBOL_ERRORS          , &et );
        }
        o << meta_h;
        return o;
    }


    //
    // calls poppler's display page with the necessary parameters
    void PDFReader::display_page(::OutputDev *dev, uintmax_t page_num) {
        // clear the current list of errors to only capture what is
        // generated by the page
        et.flush_errors();

        displayPage(dev, page_num, DPI_72, DPI_72, 0, gFalse, gTrue, gFalse);
    }


#ifdef EDSEL_RUBY_GEM
    //
    // extract the document page data
    Rice::Object PDFReader::process_page(uintmax_t page_num)
    {
        Rice::Hash page_h;
        uintmax_t num_pages = getNumPages();

        // poppler is 1-based so adjust
        page_num += 1;

        if (page_num <= num_pages) {

            // process the PDF info on this page
            display_page(eng_odev, page_num);

            const PdfPage* page_output = eng_odev->page_output();

            if (page_output) {
                page_h = page_output->to_ruby();
            }
        }

        return page_h;
    }
#endif

    //
    // extract the document page data
    std::ostream& PDFReader::process_page(uintmax_t page_num, std::ostream& o)
    {
        uintmax_t num_pages = getNumPages();

        // poppler is 1-based so adjust
        page_num += 1;

        if (page_num <= num_pages) {

            // process the PDF info on this page
            display_page(eng_odev, page_num);

            const PdfPage* page_output = eng_odev->page_output();

            if (page_output) {
                o << *page_output;
            }
        }

        return o;
    }


    std::ostream& operator<<(std::ostream& o, PDFReader& doc)
    {
        // return a hash with the data in the format
        // { :meta { <meta> }, :pages [ {<page1>} {<page2>} ... {<pageN>} ] }
        static const pdftoedn::Symbol Meta("meta");
        static const pdftoedn::Symbol Pages("pages");

        // but dont store it in a hash so we write a page at a time
        o << "{" << Meta << " ";
        doc.meta(o);
        o << ", " << Pages << " [";

        uintmax_t start_page, end_page;

        if (options.page_number() < 0) {
            start_page = 0;
            end_page = doc.getNumPages();
        } else {
            start_page = options.page_number();
            end_page = start_page + 1;
        }

        for (uintmax_t ii = start_page; ii < end_page; ++ii) {
            doc.process_page(ii, o);
        }

        o << "]}";
        return o;
    }


    //
    // extract the outline data
    bool PDFReader::process_outline(PdfOutline& outline_output)
    {
        Outline *outline = getOutline();

        if (outline && outline->getItems()) {
            outline_level(outline->getItems(), 0, outline_output.get_entry_list());
            return true;
        }
        return false;
    }


    //
    //
    // get link page number
    uintmax_t PDFReader::get_link_page_num( LinkDest* dest )
    {
        if (!dest)
            return 0;

        if (dest->isPageRef()) {
            Ref pageref = dest->getPageRef();
            return getCatalog()->findPage( pageref.num, pageref.gen);
        }

        return dest->getPageNum();
    }


    //
    //
    void PDFReader::outline_link_dest(LinkDest* dest, PdfOutline::Entry& entry)
    {
        uintmax_t page_num = get_link_page_num(dest);
        entry.set_page( page_num );
        util::copy_link_meta(entry.link(), *dest,
                             (use_page_media_box ?
                              getPageMediaHeight(page_num) :
                              getPageCropHeight(page_num)
                              )
                             );
    }


    //
    // PDF GoTo link types
    void PDFReader::outline_action_goto(LinkGoTo *link, PdfOutline::Entry& entry)
    {
        if (link && link->isOk()) {
            LinkDest* dest = NULL;

            if (link->getDest()) {
                dest = link->getDest()->copy();
            }
            else if (link->getNamedDest()) {
                dest = getCatalog()->findDest( link->getNamedDest() );
            }

            if (dest) {
                outline_link_dest(dest, entry);
                delete dest;
            }
        }
    }

    //
    // gotoR links pointing to other files in the FS
    void PDFReader::outline_action_goto_r(LinkGoToR *link, PdfOutline::Entry& entry)
    {
        if (link && link->isOk()) {

            // set the filename as the destination, then copy the meta info
            entry.set_dest( link->getFileName()->getCString() );

            LinkDest* dest = NULL;
            if (link->getDest()) {
                dest = link->getDest()->copy();
            }
            else if (link->getNamedDest()) {
                dest = getCatalog()->findDest( link->getNamedDest() );
            }

            if (dest) {
                outline_link_dest(dest, entry);
                delete dest;
            }
        }
    }

    //
    // URI links pointing to URI resources
    void PDFReader::outline_action_uri(LinkURI *link, PdfOutline::Entry& entry)
    {
        if (link && link->isOk()) {
            // set the URI as the destination
            entry.set_dest( link->getURI()->getCString() );
        }
    }

    //
    //
    // process the current outline entry in the list
    void PDFReader::outline_level(GooList* items, int level,
                                  std::list<PdfOutline::Entry *>& entry_list)
    {
        if (!items) {
            et.log_error(ErrorTracker::ERROR_INVALID_ARGS, MODULE, "PDFReader::outline_level was passed NULL items");
            return;
        }

        // run through the list
        for (int i = 0; i < items->getLength(); i++)
        {
            // get the current item
            OutlineItem* item = (OutlineItem*) items->get( i );

            if (!item) {
                continue;
            }

            // get & store the title, trimming whitespace
            std::wstring title = util::unicode_to_wstring(item->getTitle(), item->getTitleLength());
            PdfOutline::Entry* e = new PdfOutline::Entry( util::trim(title) );
            entry_list.push_back(e);

            // action shoud get LINK_GOTO
            LinkAction* link_action = item->getAction();

            if (link_action)
            {
                // select the link type
                if (link_action->getKind() == actionGoTo) {
                    outline_action_goto( dynamic_cast<LinkGoTo*>(link_action), *e );
                } else if (link_action->getKind() == actionGoToR) {
                    // TESLA-6168 - include GotoR link destination for Mike P.
                    outline_action_goto_r( dynamic_cast<LinkGoToR*>(link_action), *e );
                } else if (link_action->getKind() == actionURI) {
                    // (and adding URI link destination in case)
                    outline_action_uri( dynamic_cast<LinkURI*>(link_action), *e );
                } else {
                    std::stringstream s;
                    s << "link action kind: " << link_action->getKind();
                    et.log_warn(ErrorTracker::ERROR_UNHANDLED_LINK_ACTION, MODULE, s.str());
                }
            }

            // traverse the child nodes
            item->open();
            if (item->hasKids()) {
                outline_level(item->getKids(), level + 1, e->get_entry_list());
            }
            item->close();
        }
    }

} // namespace
