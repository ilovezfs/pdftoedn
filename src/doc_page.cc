#include <iostream>
#include <ostream>
#include <string>
#include <set>
#include <list>
#include <vector>

#ifdef EDSEL_RUBY_GEM
#include <rice/Array.hpp>
#include <rice/Hash.hpp>
#endif

#include <poppler/GfxState.h>

#include "color.h"
#include "text.h"
#include "graphics.h"
#include "pdf_error_tracker.h"
#include "doc_page.h"
#include "edsel_options.h"
#include "util.h"
#include "util_fs.h"
#include "util_versions.h"
#include "util_edn.h"

#undef EDSEL_INCLUDE_DEBUG_INFO

namespace pdftoedn
{
    // static initializers
    const pdftoedn::Symbol PdfPage::SYMBOL_PAGE_TEXT_SPANS     = "text_spans";
    const pdftoedn::Symbol PdfPage::SYMBOL_PAGE_GFX_CMDS       = "graphics";

    const pdftoedn::Symbol PdfPage::SYMBOL_FONT_IDX            = "font_idx";
    const pdftoedn::Symbol PdfPage::SYMBOL_COLOR_IDX           = "color_idx";
    const pdftoedn::Symbol PdfPage::SYMBOL_OPACITY             = "opacity";


    static const pdftoedn::Symbol SYMBOL_PAGE_OK               = "is_ok";
    static const pdftoedn::Symbol SYMBOL_PAGE_NUMBER           = "pgnum";
    static const pdftoedn::Symbol SYMBOL_PAGE_WIDTH            = "width";
    static const pdftoedn::Symbol SYMBOL_PAGE_HEIGHT           = "height";
    static const pdftoedn::Symbol SYMBOL_PAGE_ROTATION         = "rotation";
    static const pdftoedn::Symbol SYMBOL_PAGE_HAS_INVISIBLES   = "has_invisible_text";
    static const pdftoedn::Symbol SYMBOL_PAGE_TEXT_BOUNDS      = "text_bounds";
    static const pdftoedn::Symbol SYMBOL_PAGE_GFX_BOUNDS       = "gfx_bounds";
    static const pdftoedn::Symbol SYMBOL_PAGE_BOUNDS           = "bounds";
    static const pdftoedn::Symbol SYMBOL_PAGE_LINKS            = "links";

    static const pdftoedn::Symbol SYMBOL_RESOURCES             = "resources";
    static const pdftoedn::Symbol SYMBOL_RES_COLOR_LIST        = "colors";
    static const pdftoedn::Symbol SYMBOL_RES_FONT_LIST         = "fonts";
    static const pdftoedn::Symbol SYMBOL_RES_IMAGE_BLOBS       = "images";
    static const pdftoedn::Symbol SYMBOL_RES_GLYPHS            = "glyphs";

    static const pdftoedn::Symbol SYMBOL_COLOR_IDX             = "color_idx";
    static const pdftoedn::Symbol SYMBOL_OPACITY               = "opacity";

    static const pdftoedn::Symbol SYMBOL_EQUIVALENT_FONTS      = "equivalent_doc_fonts";

    // ==================================================================
    //
    //

    //
    // destructor
    PdfPage::~PdfPage()
    {
        // pdf text spans & images are tracked as pointers sorted in a
        // set; there are auto_ptr types that let you manage this but,
        // for now, we just track them directly as pointer and delete
        // them at the end
        util::delete_ptr_container_elems(text_spans);
        util::delete_ptr_container_elems(images);

        // everything else is either tracked as pointers in a list /
        // vector.. we can delete them the same way as the set data
        util::delete_ptr_container_elems(fonts);
        util::delete_ptr_container_elems(colors);
        util::delete_ptr_container_elems(glyphs);
        util::delete_ptr_container_elems(clip_paths);
        util::delete_ptr_container_elems(graphics);
        util::delete_ptr_container_elems(links);
    }


    //
    // helper to check if a color has been registered. Returns index
    // into color vector; -1 if not found
    intmax_t PdfPage::get_color_index(color_comp_t r, color_comp_t g, color_comp_t b) const
    {
        auto ii = std::find_if( colors.begin(), colors.end(),
                                [=](const RGBColor* c) { return c->equals(r, g, b); }
                                );
        if (ii != colors.end()) {
            // match.. return the index
            return ii - colors.begin();
        }
        // not found
        return -1;
    }

    //
    // register a color triplet; adds it to the vector if not there;
    // return its index
    uintmax_t PdfPage::register_color(color_comp_t r, color_comp_t g, color_comp_t b)
    {
        intmax_t idx = get_color_index(r, g, b);

        if (idx == -1) {
            colors.push_back( new RGBColor(r, g, b) );
            idx = colors.size() - 1;
        }
        return idx;
    }


    //
    // looks up the index of a font by family & style
    intmax_t PdfPage::get_font_index(const PdfFont& font) const
    {
        auto ii = std::find_if( fonts.begin(), fonts.end(),
                                [&](const PageFont* f) { return f->is_equivalent_to(font); }
                                );
        if (ii != fonts.end()) {
            // match.. return the index
            return ii - fonts.begin();
        }
        // not found
        return -1;
    }

    //
    // a new font was found in the PDF. Look it up to see if we've
    // added it. If not, do so. Update the current font index & size
    // in the tracked state in either case
    void PdfPage::update_font(const PdfFont* font, double size)
    {
        // look it up in the cache
        intmax_t font_idx = get_font_index(*font);

        if (font_idx == -1) {
            // not found.. store it temporarily until we try to use it
            if (!in_pending_list(font)) {
                pending_font.push(font);
            }
        }

        // update font state
        cur_text.attribs.update_font(font_idx, size);
    }


    bool PdfPage::in_pending_list(const PdfFont* f) const
    {
        return (!pending_font.empty() && (f->equals(*pending_font.top())));
    }


    //
    // checks if an image has been cached - based on resource id
    bool PdfPage::image_is_cached(intmax_t res_id) const
    {
        if (res_id != -1) {
            // image has a valid res id
            auto ii = std::find_if( images.begin(), images.end(),
                                    [=](const ImageData* i) { return i->equals(res_id); }
                                    );
            if (ii != images.end()) {
                // increase ref count
                (*ii)->ref();
                return true;
            }
        }

        return false;
    }

    //
    // checks if an image has been cached based on md5 (for inlined
    // images w/out resource id)
    bool PdfPage::image_is_cached(const std::string& md5, intmax_t& res_id) const
    {
        auto ii = find_if( images.begin(), images.end(),
                           [&](const ImageData* i) { return (i->md5() == md5); }
                           );
        if (ii != images.end()) {
            res_id = (*ii)->id();
            // increase ref count
            (*ii)->ref();
            return true;
        }
        return false;
    }

    //
    // adds an image blob to the table
    bool PdfPage::cache_image(intmax_t res_id, const BoundingBox& bbox,
                              int width, int height,
                              const StreamProps& properties,
                              const std::string& data,
                              intmax_t& ret_img_id)
    {
        std::string data_md5( util::md5(data) );

        // we've seen instances of repeated usage of inlined streams
        // so we must check if an entry already exists for it (using
        // md5)
        if (properties.is_inlined()) {
            intmax_t cached_res_id;
            if (image_is_cached(data_md5, cached_res_id)) {
                // return the resource id found
                ret_img_id = cached_res_id;
                return true;
            }
        }

        // determine a file name for the image within the resource
        // directory and write it
        std::string img_file_path;
        if (!pdftoedn::options.get_image_path(res_id, img_file_path)) {
            et.log_error( ErrorTracker::ERROR_PAGE_DATA, MODULE,
                          "failed to determine absolute file path to write image data to disk");
            return false;
        }

        if (!util::fs::write_image_to_disk(img_file_path, data)) {
            std::stringstream err;
            err << "Error writing '" << img_file_path << "' to disk";
            et.log_error( ErrorTracker::ERROR_PAGE_DATA, MODULE, err.str());
            return false;
        }

        // image is written. Save info in an ImageData for object
        // output but use the relative path name in the output
        ImageData* image = new ImageData(res_id, bbox, width, height,
                                         properties, data, data_md5,
                                         pdftoedn::options.get_image_rel_path(img_file_path));

        // cache meta and return the used resource id
        images.insert( images.end(), image );
        ret_img_id = res_id;
        return true;
    }


    //
    // pops the current temporary span and pushes it into the list if
    // it's not 0-length or whitespace.
    bool PdfPage::insert_pending_span()
    {
        // retrieve the transient span, trim trailing whitespace and
        // store it if not empty or outside of the page view
        PdfText* span = cur_text.pop_text();
        span->finalize();

        if (span->length() == 0) {
            // don't store it
            delete span;
            return false;
        }

        BoundingBox span_bbox = span->bounding_box();

        // compute approximated bboxes on rotated text
        if (span->CTM().is_rotated())
        {
            // rotated text? another story..
            double angle = span->CTM().rotation();

            // apply inverse rotation to determine horizontal character width
            PdfTM rhctm(-angle, span_bbox.x1(), span_bbox.y1());

            Coord char_props = rhctm.transform_delta(span_bbox.width(), -span_bbox.height());

            // adjust height by font size and rotate to get actual dx/dy
            char_props.y = std::abs(char_props.y) + span->font_size();

            PdfTM hctm(-angle, span_bbox.x1(), span_bbox.y1());

            // compute rotated bbox dimensions
            Coord p2p = hctm.transform(char_props.x, -char_props.y);
            span_bbox = BoundingBox(span_bbox.p1(), p2p);
        }

        if (!inside_page(span_bbox)) {
            //            std::cerr << "span is outside of page view: " << *span << std::endl;
            delete span;
            return false;
        }

        // set the clipping path, if applicable
        intmax_t span_clip_path_id = span->clip_id();
        if (span_clip_path_id != -1) {
            // recalculate the span's clipped bbox - only used for computing text bounds
            span_bbox = span_bbox.clip( clip_paths[ span_clip_path_id ]->bounding_box() );
        }

        // remove any spans this one "overwrites"
        if (!span->CTM().is_rotated()) {
            remove_spans_overlapped_by_span( *span );
        }

        // insert it into the list
        text_spans.insert(text_spans.end(), span);

        // adjust the overall text bounds if needed
        cur_text.bounds.expand( span_bbox );

#if 0
#if 1
        std::cerr << "      - inserting " << *span << std::endl;
#else
        std::cerr << " ============================================== pushing: \""
                  << *span << "\" @ ["
                  << span->x1() << ", "
                  << span->y1() << "]"
                  << std::endl;

        std::cerr << " ------------- CUR SET START - "
                  << text_spans.size() << " text_spans -------------" << std::endl;

        std::for_each( text_spans.begin(), text_spans.end(),
                       [&](const PdfBoxedItem* t) {
                           (*i)->dump(std::cerr);
                           std::cerr << std::endl;
                       } );
        std::cerr << " ------------- CUR SET  END  -------------" << std::endl;
#endif
#endif

        return true;
    }

    //
    // checks if the pending span overlaps any already stored spans
    // and, if so, removes them
    void PdfPage::remove_spans_overlapped_by_span(const PdfText& pending_span)
    {
        std::multiset<PdfBoxedItem*>::const_iterator si, cur = text_spans.begin();

        // search until no matches are found
        while (1)
        {
            si = std::find_if( cur,
                               text_spans.end(),
                               pending_span.overlap_predicate() );

            if (si == text_spans.end()) {
                break;
            }

            // delete the text span and erase the container
            cur = std::next(si);
            delete *si;
            text_spans.erase(si);
        }
    }


    //
    // checks if the rectangular region overlaps any already stored
    // spans and, if so, removes them / truncates them
    void PdfPage::remove_spans_overlapped_by_region(const PdfPath& region)
    {
        BoundingBox path_bbox = region.bounding_box();
        std::multiset<PdfBoxedItem*>::iterator ti = text_spans.begin();
        while (ti != text_spans.end())
        {
            PdfBoxedItem* span = *ti;

            // TODO: re-work rotated text spans to let this work
            if (span->CTM().is_rotated()) {
                ++ti;
                continue;
            }

            const BoundingBox& sbbox = span->bounding_box();
            // TODO: FIX needs to determine if region is covering
            // bottom 80% of box since top 20% is likely blank
            double overlap_ratio = sbbox.intersection_area_ratio(path_bbox);

            // approx. rules for now. Anything that's covered less
            // than 25% we say is not covered
            if (overlap_ratio < 0.25) {
                ++ti;
                continue;
            }

            std::multiset<PdfBoxedItem*>::const_iterator tmp = ti++;

            // anything > 80% is fully covered. Might get some false
            // positives here because the bboxes are approximated
            if (overlap_ratio > 0.8) {
                delete *tmp;
                text_spans.erase(tmp);
            }
            else {
                // for ratios between 25% and 80%, check the bbox to
                // see if a chucnk of the span is covered; if so,
                // remove those characters from the span.. TODO: this
                // assumes horizontal spans so FIX
                if (sbbox.x_min() < path_bbox.x_min() ||
                    sbbox.x_max() > path_bbox.x_max()) {
                    PdfText* s = dynamic_cast<PdfText*>(span);
                    if (s) {
                        s->whiteout(path_bbox);

                        // if no chars are left, delete it
                        if (s->length() == 0) {
                            delete *tmp;
                            text_spans.erase(tmp);
                        }
                    }
                }
            }
        }
    }


    //
    // adds a new character found in the PDF
    void PdfPage::new_character(double x, double y, double w, double h, const PdfTM& ctm,
                                const TextMetrics& metrics, uintmax_t unicode_c,
                                intmax_t glyph_idx, bool invisible)
    {
        // copy the current text attribs - we'll modify the copy
        // until we know this character gets added
        TextAttribs ta(cur_text.attribs);
        bool used_pending_font = false;

        // need a valid font index to use. If not yet set and one is
        // pending, use its would-be index until we know we don't need
        // to discard the character
        if (ta.font_idx == -1 && !pending_font.empty()) {
            ta.font_idx = fonts.size();
            used_pending_font = true;
        }

        // a font & color must have been set by now otherwise we can't
        // do anything with this
        if (!ta.are_valid() || cur_gfx.attribs.fill.color_idx == -1)
        {
            et.log_error( ErrorTracker::ERROR_PAGE_DATA, MODULE, "attempted to add character but no font and/or color have been registered" );
            return;
        }

        if (invisible) {
            // mark the page as carrying invisible text
            has_invisible_text = true;
        }

        // new text span.. estimate the bbox using the font height
        // since poppler returns a 0 for horizontal text. If
        // rotated, we use the given height and try to compensate
        // later
        BoundingBox bbox(x, y, w, (ctm.is_rotated() ? h : -ta.font_size));

        ta.invisible = invisible;
        ta.link_idx = inside_link(bbox);
        PdfChar *c = new PdfChar(bbox, ctm, unicode_c, ta, cur_gfx.attribs,
                                 metrics, glyph_idx, cur_gfx.clip_path());

        // check if we've started a span already
        if (cur_text.span)
        {
            // yes, check if we can join them
            if (!cur_text.span->spans(*c)) {
                // no. push the existing span into the list of
                // text_spans - this creates a new empty span we'll
                // append to below
                insert_pending_span();
            }
        }

        // store the current character
        if (cur_text.push_char(c)) {

            if (used_pending_font) {
                // move from the pending list and update index
                fonts.push_back( new PageFont(*pending_font.top()) );
                pending_font.pop();
            }

            // and update the text attribs
            cur_text.attribs = ta;
            return;
        }

        // did not get appended for some reason so delete it
        delete c;
    }

    //
    // end-of-text span marker
    void PdfPage::mark_end_of_text()
    {
        // if there's a pending span, insert it into the list
        if (cur_text.span) {
            insert_pending_span();
        }
    }

    //
    // push the current state into the stack
    void PdfPage::push_gfx_state()
    {
        //        std::cerr << "     pushing gfx attribs " << std::endl << cur_gfx.attribs << std::endl;
        cur_gfx.attribs_stack.push( cur_gfx.attribs );
    }

    //
    // restore to previous state if applicable
    void PdfPage::pop_gfx_state()
    {
        if (!cur_gfx.attribs_stack.empty()) {
            //        std::cerr << "     popping gfx attribs: " << std::endl << cur_gfx.attribs << std::endl;
            cur_gfx.attribs = cur_gfx.attribs_stack.top();
            cur_gfx.attribs_stack.pop();
        }
    }


    //
    // page data collection is done
    void PdfPage::finalize()
    {
        // make sure to push the final span
        mark_end_of_text();

        if (pdftoedn::options.include_debug_info()) {
            // debug info to logs
            std::for_each( fonts.begin(), fonts.end(),
                           [](const PageFont* f) {
                               f->check_fonts();
                           }
                           );
        }
    }

    //
    // searches if a clip path has already been defined to avoid
    // duplicates
    intmax_t PdfPage::find_clip_path(PdfDocPath* const path)
    {
        auto ii = std::find_if( clip_paths.begin(), clip_paths.end(),
                                [&](const PdfDocPath* p) { return (p->equals(*path)); }
                                );
        if (ii != clip_paths.end()) {
            return ii - clip_paths.begin();
        }
        return -1;
    }


    //
    // create and add a new path type
    void PdfPage::add_path(GfxState* state, PdfDocPath::Type type, PdfDocPath::EvenOddRule eo_flag)
    {
        // convert the poppler path to our own type
        PdfDocPath* edsel_path = new PdfDocPath(type, cur_gfx.attribs, eo_flag);
        Coord c1, c2, c3;
        GfxPath* poppler_path = state->getPath();

        for (intmax_t i = 0; i < poppler_path->getNumSubpaths(); ++i)
        {
            GfxSubpath *subpath = poppler_path->getSubpath(i);

            if (subpath->getNumPoints() > 0)
            {
                // register a new move_to command
                state->transform(subpath->getX(0), subpath->getY(0), &c1.x, &c1.y);
                edsel_path->move_to(c1);

                for (intmax_t j = 1; j < subpath->getNumPoints(); j++)
                {
                    if (subpath->getCurve(j)) {
                        // and either a curve_to or
                        state->transform(subpath->getX(j), subpath->getY(j), &c1.x, &c1.y);
                        j++;
                        state->transform(subpath->getX(j), subpath->getY(j), &c2.x, &c2.y);
                        j++;
                        state->transform(subpath->getX(j), subpath->getY(j), &c3.x, &c3.y);
                        edsel_path->curve_to(c1, c2, c3);
                    }
                    else {
                        // a line to
                        state->transform(subpath->getX(j), subpath->getY(j), &c1.x, &c1.y);
                        edsel_path->line_to(c1);
                    }
                }
                // if the path is closed, mark it as so
                if (subpath->isClosed()) {
                    edsel_path->close();
                }
            }
        }

        //
        // if this is a clip path, we must check if we've already
        // created and stored it. Don't want duplicate clip paths
        // stored
        if (edsel_path->type() == PdfDocPath::CLIP) {
            intmax_t cur_path_idx = find_clip_path(edsel_path);

            if (cur_path_idx == -1) {
                // not found.. assign a unique id and store it
                cur_path_idx = clip_paths.size();
                edsel_path->set_clip_id( cur_path_idx );
                clip_paths.push_back( edsel_path );
                //                std::cerr << " --- new clip path: " << *edsel_path << std::endl;
            } else {
                // found.. discard the incoming path
                delete edsel_path;
                //                std::cerr << " --- clip path already exists with id: " << cur_path_idx << std::endl;
            }

            // and set the active clip path
            cur_gfx.attribs.clip_idx = cur_path_idx;
        }
        else {
            // stroke / fill paths might be clipped
            if (cur_gfx.clip_path_set()) {
                // there's a clip path.. Some diagrams contain paths
                // outside of the clip.
                const BoundingBox& cb = clip_paths[ cur_gfx.clip_path() ]->bounding_box();

                BoundingBox::eClipState clip_state = edsel_path->bounding_box().is_clipped_by(cb);
                if (clip_state == BoundingBox::FULLY_CLIPPED) {
                    // We don't want to store these so delete it and break out
                    delete edsel_path;
                    return;
                }

                // if the path is partially clipped, we'll adjust the
                // bounds to match the clip's
                if (clip_state == BoundingBox::PARTIALLY_CLIPPED) {
                    edsel_path->clip_bounds(cb); // TODO: unify w/ text + images
                }

                // and set the clip id for the SVG output
                edsel_path->set_clip_id( cur_gfx.clip_path() );
            }

            // finally don't bother with paths outside of the page
            // bounds
            if (!inside_page(edsel_path->bounding_box())) {
                delete edsel_path;
                return;
            }

            //
            // cleanup text painted over by opaque fill regions
            if ( edsel_path->type() == PdfDocPath::FILL &&
                 cur_gfx.attribs.fill.opacity == 1.0 &&
                 edsel_path->is_rectangular() )
            {
                remove_spans_overlapped_by_region( *edsel_path );
            }

            // all other paths get stored in the graphics list
            graphics.push_back( edsel_path );

            // update the total graphics bounds
            cur_gfx.bounds.expand( edsel_path->bounding_box() );
        }
    }


    //
    // adds an image entry to the list
    void PdfPage::new_image(int resource_id, const BoundingBox& bbox)
    {
        PdfImage* img = new pdftoedn::PdfImage(resource_id, bbox);

        BoundingBox img_bbox(bbox);
        if (cur_gfx.clip_path_set()) {
            img->set_clip_id( cur_gfx.clip_path() );

            // clip the image's bbox
            img_bbox = img_bbox.clip( clip_paths[cur_gfx.clip_path()]->bounding_box() );
        }

        graphics.push_back( img );

        // update bounds tracking for graphics elements
        cur_gfx.bounds.expand( img_bbox );
    }


    //
    // check if a character is within any of the link bboxes
    intmax_t PdfPage::inside_link(const BoundingBox& bbox) const
    {
        Coord bbox_center = bbox.center();
        auto ii = std::find_if( links.begin(), links.end(),
                                [&](const PdfAnnotLink* l) { return l->encloses(bbox_center); }
                                );
        if (ii != links.end()) {
            return (ii - links.begin());
        }
        return -1;
    }

#ifdef EDSEL_RUBY_GEM
    //
    // rubify the PDF page data
    Rice::Object PdfPage::to_ruby() const
    {
        Rice::Hash page_h;
        page_h[ util::version::SYMBOL_DATA_FORMAT_VERSION ] = util::version::data_format_version();
        page_h[ SYMBOL_PAGE_NUMBER ]                        = number;
        page_h[ SYMBOL_PAGE_OK ]                            = !et.errors_reported();

        // text spans, graphics, links

        // an array for the text spans - note: must call to_ruby()
        // method explicitly so the correct one is invoked as we're
        // iterating through polymorphic types
        Rice::Array text_a;
        std::for_each( text_spans.begin(), text_spans.end(),
                       [&](const PdfBoxedItem* t) { text_a.push( t->to_ruby() ); }
                       );

        // an array for the graphics with clip paths first
        Rice::Array gfx_a;
        std::for_each( clip_paths.begin(), clip_paths.end(),
                       [&](const PdfDocPath *p) { gfx_a.push( p->to_ruby() ); }
                       );
        std::for_each( graphics.begin(), graphics.end(),
                       [&](const PdfGfxCmd *c) { gfx_a.push( c->to_ruby() ); }
                       );

        // an array for links
        Rice::Array links_a;
        std::for_each( links.begin(), links.end(),
                       [&](const PdfAnnotLink* l) { links_a.push( l->to_ruby() ); }
                       );

        Rice::Object resources = resource_output();

        // populate hash with the data to be returned
        page_h[ SYMBOL_PAGE_WIDTH ]                   = width();
        page_h[ SYMBOL_PAGE_HEIGHT ]                  = height();
        page_h[ SYMBOL_PAGE_ROTATION ]                = rotation;
        page_h[ SYMBOL_PAGE_HAS_INVISIBLES ]          = has_invisible_text;

        // compute the page's bbox based on the text and gfx bounds as
        // we add them to the output to prevent infinite bounds
        // (TESLA-7137)
        Bounds page_bounds;
        if (!text_spans.empty()) {
            page_h[ SYMBOL_PAGE_TEXT_BOUNDS ]         = cur_text.bounds.to_ruby();
            page_bounds.expand(cur_text.bounds.bounding_box());
        }
        if (!graphics.empty()) {
            page_h[ SYMBOL_PAGE_GFX_BOUNDS ]          = cur_gfx.bounds.to_ruby();
            page_bounds.expand(cur_gfx.bounds.bounding_box());
        }
        page_h[ SYMBOL_PAGE_BOUNDS ]                  = page_bounds.to_ruby();

        page_h[ SYMBOL_RESOURCES ]                    = resources;

#ifdef EDSEL_INCLUDE_DEBUG_INFO
        page_h[ pdftoedn::Symbol( "num_text_spans" ) ]    = text_spans.size();
#endif
        page_h[ SYMBOL_PAGE_TEXT_SPANS ]              = text_a;
        page_h[ SYMBOL_PAGE_GFX_CMDS ]                = gfx_a;
        page_h[ SYMBOL_PAGE_LINKS ]                   = links_a;

        // warnings / errors encountered
        if (et.errors_or_warnings_reported()) {
            page_h[ ErrorTracker::SYMBOL_ERRORS ]     = et.to_ruby();
        }

        return page_h;
    }


    //
    // builds the resource hash data
    Rice::Hash PdfPage::resource_output() const
    {
        // color list
        Rice::Array color_a;
        std::for_each( colors.begin(), colors.end(),
                       [&](const RGBColor* c) { color_a.push( c->to_ruby() ); }
                       );

        // create and add an array for the font list
        Rice::Array font_a;
        std::for_each( fonts.begin(), fonts.end(),
                       [&](const PageFont* f) { font_a.push( f->to_ruby() ); }
                       );

        // image blobs
        Rice::Hash image_h;
        std::for_each( images.begin(), images.end(),
                       [&](const ImageData* i) { image_h[ i->id() ] = i->to_ruby(); }
                       );

        // glyphs
        Rice::Array glyph_a;
        std::for_each( glyphs.begin(), glyphs.end(),
                       [&](const PdfGlyph* g) { glyph_a.push( g->to_ruby() ); }
                       );

        // collected resources
        Rice::Hash resource_h;
        resource_h[ SYMBOL_RES_COLOR_LIST ]           = color_a;
        resource_h[ SYMBOL_RES_FONT_LIST ]            = font_a;
        resource_h[ SYMBOL_RES_IMAGE_BLOBS ]          = image_h;
        resource_h[ SYMBOL_RES_GLYPHS ]               = glyph_a;
        return resource_h;
    }

#else

    //
    // builds the resource hash data
    util::edn::Hash& PdfPage::resource_to_edn_hash(util::edn::Hash& resource_h) const
    {
        resource_h.reserve(4);

        // color list
        util::edn::Vector color_a(colors.size());
        std::for_each( colors.begin(), colors.end(),
                       [&](const RGBColor* c) { color_a.push( c ); }
                       );

        // create and add an array for the font list
        util::edn::Vector font_a(fonts.size());
        std::for_each( fonts.begin(), fonts.end(),
                       [&](const PageFont* f) { font_a.push( f ); }
                       );

        // image blobs
        util::edn::Hash image_h(images.size());
        std::for_each( images.begin(), images.end(),
                       [&](const ImageData* i) { image_h.push( i->id(), i); }
                       );

        // glyphs
        util::edn::Vector glyph_a(glyphs.size());
        std::for_each( glyphs.begin(), glyphs.end(),
                       [&](const PdfGlyph* g) { glyph_a.push( g ); }
                       );

        // collected resources
        resource_h.push( SYMBOL_RES_COLOR_LIST,           color_a );
        resource_h.push( SYMBOL_RES_FONT_LIST,            font_a );
        resource_h.push( SYMBOL_RES_IMAGE_BLOBS,          image_h );
        resource_h.push( SYMBOL_RES_GLYPHS,               glyph_a );
        return resource_h;
    }


    std::ostream& PdfPage::to_edn(std::ostream& o) const
    {
        util::edn::Hash page_h(15);
        page_h.push( util::version::SYMBOL_DATA_FORMAT_VERSION, util::version::data_format_version() );
        page_h.push( SYMBOL_PAGE_NUMBER,                        number );
        page_h.push( SYMBOL_PAGE_OK,                            !et.errors_reported() );

        // text spans, graphics, links

        // an array for the text spans - note: must call to_ruby()
        // method explicitly so the correct one is invoked as we're
        // iterating through polymorphic types
        util::edn::Vector text_a(text_spans.size());
        std::for_each( text_spans.begin(), text_spans.end(),
                       [&](const PdfBoxedItem* t) { text_a.push( t ); }
                       );

        // an array for the graphics with clip paths first
        util::edn::Vector gfx_a(clip_paths.size() + graphics.size());
        std::for_each( clip_paths.begin(), clip_paths.end(),
                       [&](const PdfDocPath *p) { gfx_a.push( p ); }
                       );
        std::for_each( graphics.begin(), graphics.end(),
                       [&](const PdfGfxCmd *c) { gfx_a.push( c ); }
                       );

        // an array for links
        util::edn::Vector links_a(links.size());
        std::for_each( links.begin(), links.end(),
                       [&](const PdfAnnotLink* l) { links_a.push( l ); }
                       );

        util::edn::Hash resources;
        resource_to_edn_hash(resources);

        // populate hash with the data to be returned
        page_h.push( SYMBOL_PAGE_WIDTH,                   width() );
        page_h.push( SYMBOL_PAGE_HEIGHT,                  height() );
        page_h.push( SYMBOL_PAGE_ROTATION,                rotation );
        page_h.push( SYMBOL_PAGE_HAS_INVISIBLES,          has_invisible_text );

        // compute the page's bbox based on the text and gfx bounds as
        // we add them to the output to prevent infinite bounds
        // (TESLA-7137)
        Bounds page_bounds;
        if (!text_spans.empty()) {
            page_h.push( SYMBOL_PAGE_TEXT_BOUNDS,         cur_text.bounds );
            page_bounds.expand(cur_text.bounds.bounding_box());
        }
        if (!graphics.empty()) {
            page_h.push( SYMBOL_PAGE_GFX_BOUNDS,          cur_gfx.bounds );
            page_bounds.expand(cur_gfx.bounds.bounding_box());
        }
        page_h.push( SYMBOL_PAGE_BOUNDS,                  page_bounds );

        page_h.push( SYMBOL_RESOURCES,                    resources );

        page_h.push( SYMBOL_PAGE_TEXT_SPANS,              text_a );
        page_h.push( SYMBOL_PAGE_GFX_CMDS,                gfx_a );
        page_h.push( SYMBOL_PAGE_LINKS,                   links_a );

        // warnings / errors encountered
        if (et.errors_or_warnings_reported()) {
            page_h.push( ErrorTracker::SYMBOL_ERRORS,     &et );
        }

        o << page_h;
        return o;
    }
#endif

    // ==================================================================
    // private struct to track transient collection state
    //

    //
    // as new characters are added, a span is tracked in the page's
    // current text state. This ensures a Text entry has been
    // allocated and then the character is pushed into it. NOTE:
    // leading whitespace are ignored
    bool PdfPage::TextState::push_char(PdfChar* const c)
    {
        if (!span) {
            // if we're trying to insert a space and there's no span, drop it
            if (c->is_space()) {
                return false;
            }
            span = new PdfText;
        }

        return span->push_back( c );
    }


    //
    // called when the span is complete. Returns the text span and
    // clears the span pointer
    PdfText* PdfPage::TextState::pop_text()
    {
        PdfText* t = span;
        span = NULL;
        return t;
    }


    // ==================================================================
    // optimal output of a page's font
    //

    //
    // "equivalent" means matching family & style
    bool PdfPage::PageFont::is_equivalent_to(const PdfFont& font) const
    {
        const PdfFont* f = *(matching_doc_fonts.begin());

        if (font.is_bold()   == f->is_bold() &&
            font.is_italic() == f->is_italic() &&
            font.family()    == f->family()) {

            if (pdftoedn::options.include_debug_info()) {
                matching_doc_fonts.insert(&font);
            }
            return true;
        }
        return false;
    }

#ifdef EDSEL_RUBY_GEM
    //
    //
    Rice::Object PdfPage::PageFont::to_ruby() const
    {
        Rice::Hash font_h;

        // get the family & style from the first entry
        std::set<const PdfFont*>::const_iterator fi = matching_doc_fonts.begin();

        font_h[ PdfFont::SYMBOL_FAMILY ]             = (*fi)->family();

        // font style attributes, if present
        if ((*fi)->is_bold()) {
            font_h[ PdfFont::SYMBOL_STYLE_BOLD ]     = true;
        }

        if ((*fi)->is_italic()) {
            font_h[ PdfFont::SYMBOL_STYLE_ITALIC ]   = true;
        }

        if (pdftoedn::options.include_debug_info())
        {
            // list equivalent fonts
            Rice::Array refs_a;

            while (fi != matching_doc_fonts.end()) {
                const PdfFont* f = *fi;

                // add the name to the array
                refs_a.push( f->name() );

                // clear unmapped list so we only record the ones for
                // this page
                f->clear_unmapped_codes();

                ++fi;
            }
            font_h[ SYMBOL_EQUIVALENT_FONTS ] = refs_a;
        }
        return font_h;
    }
#else
    std::ostream& PdfPage::PageFont::to_edn(std::ostream& o) const
    {
        util::edn::Hash font_h(4);

        // get the family & style from the first entry
        std::set<const PdfFont*>::const_iterator fi = matching_doc_fonts.begin();

        font_h.push( PdfFont::SYMBOL_FAMILY,             (*fi)->family() );

        // font style attributes, if present
        if ((*fi)->is_bold()) {
            font_h.push( PdfFont::SYMBOL_STYLE_BOLD,     true );
        }

        if ((*fi)->is_italic()) {
            font_h.push( PdfFont::SYMBOL_STYLE_ITALIC,   true );
        }

        if (pdftoedn::options.include_debug_info())
        {
            // list equivalent fonts
            util::edn::Vector refs_a(matching_doc_fonts.size());

            while (fi != matching_doc_fonts.end()) {
                const PdfFont* f = *fi;

                // add the name to the array
                refs_a.push( f->name() );

                // clear unmapped list so we only record the ones for
                // this page
                f->clear_unmapped_codes();

                ++fi;
            }
            font_h.push( SYMBOL_EQUIVALENT_FONTS, refs_a );
        }
        o << font_h;
        return o;
    }
#endif

    //
    // debug info about font map errors
    void PdfPage::PageFont::check_fonts() const
    {
        std::for_each( matching_doc_fonts.begin(), matching_doc_fonts.end(),
                       [](const PdfFont* f) {
                           if (f->is_unknown()) {
                               // report an error
                               std::stringstream err;
                               err << "Failed to map font " << f->name() << " (" << f->type_str();
                               if (f->src()->has_std_encoding()) {
                                   err << ", " << f->src()->get_encoding()->name();
                               }
                               err << ")";
                               pdftoedn::et.log_error(ErrorTracker::ERROR_FE_FONT_MAPPING, MODULE, err.str());
                           }

                           // report a warning about unmapped codes if needed
                           if (f->has_unmapped_codes() &&
                               ( !f->src()->has_std_encoding() || !f->has_to_unicode()) ) {
                               // report the warning
                               std::stringstream s;
                               s << "Font '" << f->name() << "' has custom encoding w/ unmapped codes: " << f->get_unmapped_codes_str();
                               et.log_warn(ErrorTracker::ERROR_FE_FONT_MAPPING, MODULE, s.str());
                           }
                       } );
    }

} // namespace
