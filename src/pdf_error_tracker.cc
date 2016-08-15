#include <iostream>
#include <string>
#include <list>

#include <poppler/Error.h>

#include "pdf_error_tracker.h"
#include "util.h"
#include "util_edn.h"

namespace pdftoedn
{
    const Symbol ErrorTracker::SYMBOL_ERROR_TYPES[]      = {
        "syntax_warning",
        "syntax_error",
        "config",
        "cli",
        "io",
        "not_allowed",
        "unimplemented",
        "internal",

        // graphics libraries
        "png_warning",
        "png_error",

        // our errors from here on
        "", // entry for the delimiter

        "invalid_args",
        "unhandled_link_action",
        "fe_font_freetype",
        "fe_font_read",
        "fe_font_read_unsupported_type",
        "fe_font_map",
        "fe_font_map_dup",
        "od_runtime_error",
        "od_unimplemented_cb",
        "page_data",
        "ut_image_encode",
        "ut_image_xform",
    };

    const Symbol ErrorTracker::SYMBOL_ERROR_LEVELS[]     = {
        "info",
        "warning",
        "error",
        "critical",
    };
    const Symbol ErrorTracker::SYMBOL_ERRORS             = "errors";
    const Symbol ErrorTracker::error::SYMBOL_TYPE        = "type";
    const Symbol ErrorTracker::error::SYMBOL_LEVEL       = "level";
    const Symbol ErrorTracker::error::SYMBOL_MODULE      = "module";
    const Symbol ErrorTracker::error::SYMBOL_DESC        = "desc";
    const Symbol ErrorTracker::error::SYMBOL_COUNT       = "count";


    //
    // output error as EDN
    std::ostream& ErrorTracker::error::to_edn(std::ostream& o) const
    {
        util::edn::Hash h(5);
        h.push( SYMBOL_TYPE, SYMBOL_ERROR_TYPES[ type ] );
        h.push( SYMBOL_LEVEL, SYMBOL_ERROR_LEVELS[ lvl ] );
        h.push( SYMBOL_MODULE, mod );
        h.push( SYMBOL_DESC, msg );
        if (count > 1) {
            h.push( SYMBOL_COUNT, count );
        }
        o << h;
        return o;
    }


    //
    // has this error been marked to be ignored already?
    bool ErrorTracker::error_muted(error_type e) const
    {
        if ( std::any_of( ignore_errors.begin(), ignore_errors.end(),
                          [&](const error_type& err) { return (err == e); }
                          ) ) {
            return true;
        }
        return false;
    }

    //
    // marks this error type so we don't report it in the output
    void ErrorTracker::ignore_error(error_type e)
    {
        if (!error_muted(e)) {
            ignore_errors.push_back(e);
        }
    }

    //
    // add an entry to the list
    void ErrorTracker::log(error_type e, error::level l, const std::string& module, const std::string& msg)
    {
        if (error_muted(e)) {
            // don't log if it has been muted
            return;
        }

        // check if it's already been logged
        if (std::any_of( errors.begin(), errors.end(),
                         [&](const error* err) { return (err->matches(e, module, msg)); }
                         )) {
            return;
        }

        // report it
        errors.push_back( new error(e, l, module, msg) );
    }

    //
    // purge the list of errors
    void ErrorTracker::flush_errors()
    {
        if (!errors.empty()) {
            util::delete_ptr_container_elems(errors);
            errors.clear();
        }
    }

    //
    // checks if there's anything more serious than warnings
    bool ErrorTracker::errors_reported() const
    {
        return std::any_of( errors.begin(), errors.end(),
                            [](const error* e) { return (e->higher_than(error::L_WARNING)); }
                            );
    }


    // outputs the list of errors
    std::ostream& ErrorTracker::to_edn(std::ostream& o) const
    {
        util::edn::Vector v(errors.size());
        std::for_each( errors.begin(), errors.end(),
                       [&](const error* e) { v.push( e ); }
                       );
        o << v;
        return o;
    }


    //
    // static function for registering with poppler's error handler
    void ErrorTracker::error_handler(void *data, ErrorCategory category, Goffset pos, char *msg)
    {
        if (!msg) {
            std::cerr << __FUNCTION__ << " - NULL error message" << std::endl;
            return;
        }

        ErrorTracker* et = static_cast<ErrorTracker*>(data);

        if (et)
        {
            ErrorTracker::error_type e;
            ErrorTracker::error::level l;

            if (util::poppler_error_to_edsel(category, msg, pos, e, l)) {
                et->log(e, l, "poppler", msg);
            }
        }
    }

} // namespace
