#pragma once

#include <ostream>
#include <set>
#include <stack>
#include <list>
#include <vector>

#include <poppler/GfxState.h>

#include "base_types.h"
#include "font.h"
#include "text.h"
#include "graphics.h"
#include "image.h"
#include "pdf_links.h"

namespace pdftoedn
{
    // ---------------------------------------------------------
    // tracks the data read from a page in the PDF doc.
    //
    class PdfPage : public pdftoedn::gemable {
    public:

        // constructor / destructor
        PdfPage(uintmax_t page_number, double page_width, double page_height, intmax_t page_rotation) :
            number(page_number), bbox(0, 0, page_width, page_height), rotation(page_rotation),
            has_invisible_text(false)
        {}
        virtual ~PdfPage();

        // accessors
        double width() const { return bbox.width(); }
        double height() const { return bbox.height(); }
        bool is_rotated() const { return (rotation != 0); }

        // resource-related methods --
        // adds an entry to the color table if it doesn't exist;
        // returns the index in the table
        uintmax_t register_color(color_comp_t r, color_comp_t g, color_comp_t b);

        // path ops --
        // creates a path, inserts it into the path list and returns a
        // reference to it
        void add_path(GfxState* state, PdfDocPath::Type type, PdfDocPath::EvenOddRule eo_flag);

        // image blob manipulations
        bool image_is_cached(intmax_t resource_id) const;
        bool cache_image(intmax_t resource_id, const BoundingBox& bbox,
                         int width, int height,
                         const StreamProps& properties,
                         const std::string& data,
                         intmax_t& id);

        // text-related methods --
        //
        // adds an entry into the font list if it isn't in there already
        void update_font(const PdfFont* font, double size);

        // add a unicode character entry.. if glyph_idx != -1,
        // character is to be drawn via a path
        void new_character(double x, double y, double width, double height, const PdfTM& ctm,
                           const TextMetrics& metrics, uintmax_t unicode_c, intmax_t glyph_idx,
                           bool invisible);

        // graphics-related methods --
        //
        void push_gfx_state();
        void pop_gfx_state();

        void update_line_dash(const std::list<double>& l_dash) { cur_gfx.attribs.line_dash = l_dash; }
        void update_line_join(int8_t line_join) { cur_gfx.attribs.line_join = line_join; }
        void update_line_cap(int8_t line_cap) { cur_gfx.attribs.line_cap = line_cap; }
        void update_miter_limit(double miter_limit) { cur_gfx.attribs.miter_limit = miter_limit; }
        void update_line_width(double line_width) { cur_gfx.attribs.line_width = line_width; }
        void update_stroke_color(color_comp_t r, color_comp_t g, color_comp_t b) {
            cur_gfx.attribs.stroke.color_idx = register_color(r, g, b);
        }
        void update_stroke_opacity(double opacity) { cur_gfx.attribs.stroke.opacity = opacity; }
        void update_stroke_overprint(bool overprint) { cur_gfx.attribs.stroke.overprint = overprint; }

        void update_fill_color(color_comp_t r, color_comp_t g, color_comp_t b) {
            cur_gfx.attribs.fill.color_idx = register_color(r, g, b);
        }
        void update_fill_opacity(double opacity) { cur_gfx.attribs.fill.opacity = opacity; }
        void update_fill_overprint(bool overprint) { cur_gfx.attribs.fill.overprint = overprint; }
        void update_overprint_mode(uint8_t mode) { cur_gfx.attribs.overprint_mode = mode; }
        void update_blend_mode(uint8_t blend_mode) { cur_gfx.attribs.blend_mode = blend_mode; }

        // images --
        //
        // creates and inserts a new image metadata container into the list
        void new_image(int resource_id, const BoundingBox& bbox);

        // links --
        void new_annot_link(pdftoedn::PdfAnnotLink* const annot_link) {
            links.push_back(annot_link);
        }

        void finalize();

        virtual std::ostream& to_edn(std::ostream& o) const;

        static const pdftoedn::Symbol SYMBOL_PAGE_TEXT_SPANS;
        static const pdftoedn::Symbol SYMBOL_PAGE_GFX_CMDS;

        static const pdftoedn::Symbol SYMBOL_FONT_IDX;
        static const pdftoedn::Symbol SYMBOL_COLOR_IDX;
        static const pdftoedn::Symbol SYMBOL_OPACITY;

    private:

        // font class that tracks what we output on a page
        class PageFont : public gemable {
        public:
            PageFont(const PdfFont& font) {
                matching_doc_fonts.insert(&font);
            }

            bool is_equivalent_to(const PdfFont& font) const;
            void log_font_issues() const;

            virtual std::ostream& to_edn(std::ostream& o) const;

        private:
            mutable std::set<const PdfFont*, PdfFont::lt> matching_doc_fonts;
        };


        uintmax_t number;
        BoundingBox bbox;
        intmax_t rotation;
        bool has_invisible_text;

        // resources
        std::stack<const PdfFont*> pending_font;
        std::vector<PageFont *> fonts;
        std::vector<pdftoedn::RGBColor *> colors;
        std::set<pdftoedn::ImageData*, pdftoedn::ImageData::lt> images;
        std::vector<pdftoedn::PdfGlyph *> glyphs;

        // data
        std::multiset<pdftoedn::PdfBoxedItem *, pdftoedn::PdfBoxedItem::lt> text_spans;
        std::list<pdftoedn::PdfGfxCmd *> graphics;
        std::vector<pdftoedn::PdfDocPath *> clip_paths;
        std::vector<pdftoedn::PdfAnnotLink *> links;

        // transient state as text is collected
        struct TextState {
            TextState() : span(NULL) { }
            ~TextState() { delete span; }

            pdftoedn::TextAttribs attribs;
            pdftoedn::PdfText *span;
            pdftoedn::Bounds bounds;

            // helpers
            bool push_char(pdftoedn::PdfChar* const c);
            pdftoedn::PdfText* pop_text();
        } cur_text;

        // transient state of gfx as collected
        struct GraphicsState {
            GraphicsState() { }

            bool clip_path_set() const { return (attribs.clip_idx != -1); }
            intmax_t clip_path() const { return attribs.clip_idx; }

            pdftoedn::Bounds bounds;
            GfxAttribs attribs;

            // track the current gfx state as it is pushed / popped in
            // the PDF
            std::stack<GfxAttribs> attribs_stack;
        } cur_gfx;

        // helpers
        bool in_pending_list(const PdfFont* f) const;
        intmax_t get_color_index(color_comp_t r, color_comp_t g, color_comp_t b) const;
        intmax_t get_font_index(const PdfFont& font) const;
        intmax_t cur_font_index() const { return cur_text.attribs.font_idx; }

        bool inside_page(const BoundingBox& bbox) const { return bbox.is_inside( this->bbox ); }
        intmax_t inside_link(const BoundingBox& bbox) const;
        bool insert_pending_span();
        void remove_spans_overlapped_by_span(const PdfText& span);
        void remove_spans_overlapped_by_region(const PdfPath& region);
        intmax_t find_clip_path(PdfDocPath* const path);

        // private image blob search by md5 hash - for inlined images
        bool image_is_cached(const std::string& md5, intmax_t& res_id) const;

        // mark end of text object - triggers pushing of any pending spans
        void mark_end_of_text();

        util::edn::Hash& resource_to_edn_hash(util::edn::Hash& resource_h) const;

        // prohibit these cause we shouldn't be using them anyway
        PdfPage();
        PdfPage(const PdfPage&);
        PdfPage(PdfPage&);
        PdfPage& operator=(PdfPage&);
        PdfPage& operator=(const PdfPage&);
    };

} // namespace
