#pragma once

#include <string>
#include <list>
#include <stdexcept>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif
#include <poppler/Error.h>
#ifdef __clang__
#pragma clang diagnostic push
#endif

#include "base_types.h"

namespace pdftoedn
{
    // ----------------------------------
    //
    //
    struct ErrorTracker : public gemable {
        // runtime error codes - these map to enums in poppler/Error.h
        // plus we could add our own
        enum error_type {
            ERROR_SYNTAX_WARNING,
            ERROR_SYNTAX_ERROR,
            ERROR_CONFIG,
            ERROR_COMMAND_LINE,
            ERROR_IO,
            ERROR_NOT_ALLOWED,
            ERROR_UNIMPLEMENTED,
            ERROR_INTERNAL,

            // our own errors from here on
            ERROR_NONE, // not an error - used to separate poppler errors from our own

            ERROR_INVALID_ARGS,
            ERROR_UNHANDLED_LINK_ACTION,
            ERROR_FE_INIT_FAILURE,
            ERROR_FE_FONT_FT,
            ERROR_FE_FONT_READ,
            ERROR_FE_FONT_READ_UNSUPPORTED,
            ERROR_FE_FONT_MAPPING,
            ERROR_FE_FONT_MAPPING_DUPLICATE,
            ERROR_OD_RUNTIME,
            ERROR_OD_UNIMPLEMENTED_CB,
            ERROR_PAGE_DATA,
            ERROR_UT_IMAGE_ENCODE,
            ERROR_UT_IMAGE_XFORM,

            ERROR_TYPE_COUNT // delim
        };

        struct error : public gemable {
            enum level {
                L_INFO,
                L_WARNING,
                L_ERROR,
                L_CRITICAL
            };

            static const Symbol SYMBOL_TYPE;
            static const Symbol SYMBOL_LEVEL;
            static const Symbol SYMBOL_MODULE;
            static const Symbol SYMBOL_DESC;
            static const Symbol SYMBOL_COUNT;

            error(error_type e, level l, const std::string& m, const std::string& e_msg) :
                type(e), lvl(l), mod(m), msg(e_msg), count(1)
            { }

            bool matches(error_type e_type, const std::string& module, const std::string& message) const {
                // if it matches, increase the instance count
                if (e_type == type && module == mod && message == msg) {
                    count++;
                    return true;
                }
                return false;
            }
            bool higher_than(level floor) const { return (lvl > floor); }

#ifdef EDSEL_RUBY_GEM
            virtual Rice::Object to_ruby() const;
#else
            virtual std::ostream& to_edn(std::ostream& o) const;
#endif
        private:
            error_type type;
            level lvl;
            std::string mod;
            std::string msg;
            mutable uintmax_t count;
        };

        static const Symbol SYMBOL_ERROR_TYPES[];
        static const Symbol SYMBOL_ERROR_LEVELS[];
        static const Symbol SYMBOL_ERRORS;

        virtual ~ErrorTracker() { flush_errors(); }

        void ignore_error(error_type e); // don't report this error type

        void log(error_type e, error::level l, const std::string& mod, const std::string& msg);
        void log_info(error_type e, const std::string& mod, const std::string& msg) {
            log(e, error::L_INFO, mod, msg);
        }
        void log_warn(error_type e, const std::string& mod, const std::string& msg) {
            log(e, error::L_WARNING, mod, msg);
        }
        void log_error(error_type e, const std::string& mod, const std::string& msg) {
            log(e, error::L_ERROR, mod, msg);
        }
        void log_critical(error_type e, const std::string& mod, const std::string& msg) {
            log(e, error::L_CRITICAL, mod, msg);
        }

        bool errors_reported() const;
        bool errors_or_warnings_reported() const { return !errors.empty(); }
        void flush_errors();

#ifdef EDSEL_RUBY_GEM
        virtual Rice::Object to_ruby() const;
#else
        virtual std::ostream& to_edn(std::ostream& o) const;
#endif
        // method to register error handler w/ poppler
        static void error_handler(void *data, ErrorCategory category, Goffset pos, char *msg);

    private:
        std::list<error *> errors;
        std::list<error_type> ignore_errors;

        bool error_muted(error_type e) const;
    };

    struct invalid_pdf : public std::invalid_argument {
        invalid_pdf() : invalid_argument("Invalid PDF document") {}
    };

    extern pdftoedn::ErrorTracker et;
} // namespace
