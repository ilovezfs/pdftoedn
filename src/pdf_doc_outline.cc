#include <list>

#include "util.h"
#include "util_edn.h"
#include "pdf_doc_outline.h"

namespace pdftoedn
{
    static const pdftoedn::Symbol SYMBOL_ENTRIES       = "entries";
    static const pdftoedn::Symbol SYMBOL_TITLE         = "title";
    static const pdftoedn::Symbol SYMBOL_PAGE_NUM      = "page";
    static const pdftoedn::Symbol SYMBOL_LINK          = "link";
    static const pdftoedn::Symbol SYMBOL_DEST          = "dest";

    // ==================================================================
    //
    //

    static util::edn::Vector& entry_list_to_edn_vector(const std::list<PdfOutline::Entry *>& l, util::edn::Vector& entries_a)
    {
        std::for_each( l.begin(), l.end(),
                       [&](const PdfOutline::Entry* e) { entries_a.push(e); }
                       );
        return entries_a;
    }

    // ==================================================================
    // internal Entry type
    //
    std::ostream& PdfOutline::to_edn(std::ostream& o) const {
        util::edn::Vector entries_v(entries.size());
        entry_list_to_edn_vector(entries, entries_v);
        o << entries_v;
        return o;
    }

    util::edn::Hash& PdfOutline::Entry::to_edn_hash(util::edn::Hash& entry_h) const
    {
        entry_h.push( SYMBOL_TITLE,    pdftoedn::util::wstring_to_utfstring(title) );
        entry_h.push( SYMBOL_PAGE_NUM, page );

        if (dest.length() > 0) {
            entry_h.push( SYMBOL_DEST, dest );
        }

        // add the link data if it was set
        if (link_meta.is_set()) {
            entry_h.push( SYMBOL_LINK, &link_meta );
        }

        // and any child entries if they exist
        if (!entries.empty()) {
            util::edn::Vector v(entries.size());
            entry_h.push( SYMBOL_ENTRIES, entry_list_to_edn_vector(entries, v) );
        }
        return entry_h;
    }

    std::ostream& PdfOutline::Entry::to_edn(std::ostream& o) const
    {
        util::edn::Hash entry_h(5);
        to_edn_hash(entry_h);
        o << entry_h;
        return o;
    }
} // namespace
