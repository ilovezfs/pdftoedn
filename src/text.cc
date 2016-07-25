#include <string>
#include <sstream>
#include <list>
#include <ostream>
#include <algorithm>
#include <complex>

#ifdef EDSEL_RUBY_GEM
#include <rice/Array.hpp>
#include <rice/Hash.hpp>
#endif

#include "doc_page.h"
#include "text.h"
#include "transforms.h"
#include "util.h"
#include "util_edn.h"

namespace pdftoedn
{
    // static initializers
    const pdftoedn::Symbol PdfGlyph::SYMBOL             = "glyph";

    const pdftoedn::Symbol PdfText::SYMBOL_TYPE_SPAN    = "span";
    const pdftoedn::Symbol PdfText::SYMBOL_ORIGIN       = "origin";
    const pdftoedn::Symbol PdfText::SYMBOL_GLYPH_IDX    = "glyph_idx";

    static const pdftoedn::Symbol SYMBOL_X_POS_VECTOR   = "x_vector";
    static const pdftoedn::Symbol SYMBOL_Y_POS_VECTOR   = "y_vector";
    static const pdftoedn::Symbol SYMBOL_TEXT           = "text";
    static const pdftoedn::Symbol SYMBOL_PT_SIZE        = "size";
    static const pdftoedn::Symbol SYMBOL_INVISIBLE      = "invisible";
    static const pdftoedn::Symbol SYMBOL_LINK_IDX       = "link_idx";

    // when two spans overlap, this is the minimum ratio between their
    // intersection and union to call it a full overlap
    const double PdfText::SPAN_OVERLAP_THRESHOLD = 0.75;

    // =============================================
    // TextAttribs
    //

    void TextAttribs::update_font(intmax_t font_index, double font_sz)
    {
        font_idx = font_index;
        font_size = font_sz;
        baseline_threshold = font_size * YPOS_THRESHOLD / 100;
    }

    // =============================================
    // PdfChar - a unicode character
    //

    //
    // check if this character looks to be adjacent to a previous one
    bool PdfChar::spans(const PdfChar& prev) const
    {
        // if we have rotation but are not rotation along 90, 180, or
        // 270, we don't bother computing span
        if (!ctm.is_rotation_orthogonal()) {
            return false;
        }

        // check that text looks like it might actually follow the
        // previous character
        if (prev.x1() > x2()) {
            return false;
        }

        double scaling = attribs.txt.font_size * metrics.horiz_scaling;
        double max_space = 2 * scaling;

        // found cases where a very large whitespace is used to
        // separate text spans in table cells, footers, etc.
        if (prev.is_space() && (prev.width() > max_space)) {
            return false;
        }

        double bbox_delta = left() - prev.right();
        double min_ws_space = 0.2 * scaling;

        if (ctm.rotation_deg() == 90) {
            // TESLA-7613: when text is rotated by 90, left -
            // prev.right results in a negative value due to inverted
            // y axis. Compensate here so the calculation below works
            bbox_delta = std::abs(bbox_delta);
        }

#if 0 // you don't really want to enable this
        cerr << " ----------------- comparing spans -------------- " << endl
             << " a: " << prev << endl
             << " b: " << *this << endl;

        cerr << boolalpha
             << "   rotated orth? " << (ctm.is_rotation_orthogonal()) << " (deg: " << ctm.rotation_deg() << ")" << endl
             << "   glyph idx is -1? " << ((int) glyph_idx == -1) << endl
             << "   attribs are equal? " <<  (attribs == prev.attribs) << endl
             << "   font_size equal? " <<  (attribs.font_size == prev.attribs.font_size) << " (font size: " << attribs.font_size << ")" <<  endl
             << "   top delta check: < 0.001: "
             << (std::abs(prev.top() - top()   )   < 0.001) << endl
             << "   left vs. prev right delta ("
             << bbox_delta
             << ") : < 0.2 * attribs.font_size * horiz_scaling (" << min_ws_space << ") ? "
             << (bbox_delta < min_ws_space) << endl
             << "  prev is space? " << prev.is_space() << ", cur is space? " << is_space()
             << endl;
#endif

        // if we're dealing with a glyph, or text attributes are not
        // equal or the position delta is not within some magic
        // numbers, it is not spannable
        if ( (glyph_idx != -1) ||
             (attribs != prev.attribs) ||
             (ctm.rotation() != prev.ctm.rotation()) ||
             (
              // different line?
              (std::abs(prev.top() - top()) > 0.001) ||
              (
               // if there's a gap, only break it up if they're both not whitespace
               ((bbox_delta > min_ws_space) && (!prev.is_space() && !is_space())) ||
               // or if it's a large gap, split it
               ( bbox_delta > max_space )
               ))
             ) {
            return false;
        }
        return true;
    }

    double PdfChar::left() const
    {
        if (!ctm.is_rotated()) {
            return bbox.x_min();
        }
        else {
            double rotation = ctm.rotation_deg();
            if (rotation == 90) {
                return bbox.y_max();
            } else if (rotation == 180) {
                return bbox.y_min();
            } else if (rotation == 270) {
                return bbox.x_max();
            }
            // don't span for arbitrary angles
            return 0;
        }
    }

    double PdfChar::right() const
    {
        if (!ctm.is_rotated()) {
            return bbox.x_max();
        }
        else {
            double rotation = ctm.rotation_deg();
            if (rotation == 90) {
                return bbox.y_min();
            } else if (rotation == 180) {
                return bbox.y_max();
            } else if (rotation == 270) {
                return bbox.x_min();
            }
            // don't span for arbitrary angles
            return 0;
        }
    }

    double PdfChar::top() const
    {
        if (!ctm.is_rotated()) {
            return bbox.y_min();
        }
        else {
            double rotation = ctm.rotation_deg();
            if (rotation == 90) {
                return bbox.x_min();
            } else if (rotation == 180) {
                return bbox.x_max();
            } else if (rotation == 270) {
                return bbox.y_max();
            }
            // don't span for arbitrary angles
            return 0;
        }
    }

    double PdfChar::bottom() const
    {
        if (!ctm.is_rotated()) {
            return bbox.y_max();
        }
        else {
            double rotation = ctm.rotation_deg();
            if (rotation == 90) {
                return bbox.x_max();
            } else if (rotation == 180) {
                return bbox.x_min();
            } else if (rotation == 270) {
                return bbox.y_min();
            }
            // don't span for arbitrary angles
            return 0;
        }
    }


    // =============================================
    // PdfText - a span of PdfChars
    //

    bool PdfText::OverlapPred::operator()(PdfBoxedItem* const bi)
    {
        // TODO: re-work rotated text spans to let this work
        if (bi->CTM().is_rotated()) {
            return false;
        }
        return (bi->bounding_box().intersection_area_ratio(ps_bbox) > SPAN_OVERLAP_THRESHOLD);
    }

    //
    // TESLA-7177: give a small amount of play when comparing vertical
    // position of text spans
    bool PdfText::is_positioned_before(const PdfBoxedItem* bi) const
    {
        if (std::abs(y_max() - bi->y_max()) < attribs.txt.baseline_threshold) {
            return (x_min() < bi->x_min());
        }

        return (y_max() < bi->y_max());
    }

    //
    // removes whitespace (trailing for now as no spans are created
    // with leading ws already)
    void PdfText::trim()
    {
        while (!chars.empty())
        {
            if (!chars.back()->is_space()) {
                break;
            }
            delete chars.back();
            chars.pop_back();
        }
    }

    //
    // store / append a new character
    bool PdfText::push_back(PdfChar* c)
    {
        // is there a span created?
        if (chars.empty())
        {
            // no. get the char data and set it for the span
            ctm = c->ctm;
            attribs = c->attribs; // all other common attributes
        }
        else
        {
            // don't append back to back white-space
            if (chars.back()->is_space() && c->is_space()) {
                return false;
            }
        }

        // add the character now
        chars.push_back(c);
        return true;
    }

    const PdfText::OverlapPred& PdfText::overlap_predicate() const
    {
        if (!overlap_pred) {
            overlap_pred = new OverlapPred(*this);
        }
        return *overlap_pred;
    }


    //
    // span is ready for insertion - compute its bbox
    void PdfText::finalize()
    {
        trim();
        if (!chars.empty()) {
            Bounds b;
            b = chars.front()->bounding_box();
            if (chars.size() > 1) {
                b.expand(chars.back()->bounding_box());
            }
            bbox = b.bounding_box();
        }
    }


    //
    // remove characters from the span covered by the region
    void PdfText::whiteout(const BoundingBox& wo_region)
    {
        std::list<PdfChar *>::iterator ci = chars.begin();

        while (ci != chars.end())
        {
            std::list<PdfChar *>::iterator cur = ci++;
            PdfChar* c = *cur;

            if (c->right() < wo_region.x_min()) {
                continue;
            }

            if (c->left() > wo_region.x_max()) {
                break;
            }

            delete c;
            chars.erase(cur);
        }

        finalize();
    }

#ifdef EDSEL_RUBY_GEM
    //
    // rubify the span
    Rice::Object PdfText::to_ruby() const
    {
        Rice::Hash text_h;

        // add a type identifier. TODO: This needs some cleaning up
        text_h[ PdfGfxCmd::SYMBOL_TYPE ]           = PdfText::SYMBOL_TYPE_SPAN;

        double font_size = chars.front()->attribs.txt.font_size;

        // poppler doesn't produce proper bounding boxes; compensate here
        if (!ctm.is_rotated()) {
            // non-rotated text simply sets the height to be the font size
            text_h[ BoundingBox::SYMBOL ]          = bbox.to_ruby();
        }
        else {
            // rotated text? another story..
            double angle = ctm.rotation();

            // apply inverse rotation to determine horizontal character width
            PdfTM rhctm(-angle, bbox.x1(), bbox.y1());

            Coord char_props = rhctm.transform_delta(bbox.width(), -bbox.height());

            // adjust height by font size and rotate to get actual dx/dy
            char_props.y = std::abs(char_props.y) + font_size;

            PdfTM hctm(-angle, bbox.x1(), bbox.y1());

            // compute rotated bbox dimensions
            Coord p2p = hctm.transform(char_props.x, -char_props.y);
            BoundingBox bboxp(bbox.p1(), p2p);

            // determine origin for SVG
            Coord origin(bbox.x1(), bbox.y2());

            // set the transform needed for the SVG side
            angle = ctm.rotation_deg();
            double w = -bbox.width();
            double h = -bbox.height();
            double x1 = origin.x;
            double x2 = origin.x + bbox.width();
            double y1 = origin.y;
            double y2 = origin.y - bbox.height();

            std::list<Transform *> transforms;

            // something is off with these but I need more examples to test with
            if (angle < 45) {
                transforms.push_back(new Rotate(-angle, x1, y1));
                w = 0;
            }
            else if ((int) angle == 45) {
                transforms.push_back(new Rotate(-angle, x1, y1));
                w = h = 0;
            }
            else if (angle < 90) {
                transforms.push_back(new Rotate(-angle, x2, y2));
            }
            else if ((int) angle == 90) {
                transforms.push_back(new Rotate(-angle, x1, bbox.y2()));
                h = 0;
            }
            else if (angle < 135) {
                transforms.push_back(new Rotate(-angle, x1, y2));
            }
            else if (angle < 180) {
                transforms.push_back(new Rotate(-angle, x1, y2));
            }
            else if (angle < 225) {
                transforms.push_back(new Rotate(-angle, x1, y2));
            }
            else if (angle < 270) {
                transforms.push_back(new Rotate(-angle, x1, y2));
            }
            else if ((int) angle == 270) {
                transforms.push_back(new Translate(-h, w));
                transforms.push_back(new Rotate(-angle, x1, y2));
            }
            else if (angle < 315) {
                transforms.push_back(new Rotate(-angle, x1, y2));
            }
            else if (angle < 360) {
                w = 0;
                transforms.push_back(new Rotate(-angle, x2, y2));
            }

            if (std::abs(w) != 0.0 && std::abs(h) != 0.0) {
                transforms.push_back(new Translate(w, h));
            }

            text_h[ PdfBoxedItem::SYMBOL_ROTATION ] = ctm.rotation_deg();
            text_h[ PdfText::SYMBOL_ORIGIN ]        = origin.to_ruby();
            text_h[ BoundingBox::SYMBOL ]           = bboxp.to_ruby();
            text_h[ PdfBoxedItem::SYMBOL_XFORM ]    = Transform::list_to_ruby(transforms);

            // cleanup
            util::delete_ptr_container_elems(transforms);
        }

        // run through the list of characters to build the string and
        // the x-position vector..
        Rice::Array x_vector_a;
        std::string str;
        intmax_t glyph_idx = -1;

        std::for_each ( chars.begin(), chars.end(),
                        [&](const PdfChar* c) {
                            str += util::wstring_to_utfstring( c->wstr() );

                            if (!ctm.is_rotated()) {
                                x_vector_a.push( c->bounding_box().x1() );
                            }

                            // if a glyph was encountered in the stream, length will
                            // be 1 always since they're not spannable
                            glyph_idx = c->get_glyph_index();
                        } );

        text_h[ SYMBOL_TEXT ]                      = util::string_to_ruby(str);

        // font and color data
        text_h[ PdfPage::SYMBOL_FONT_IDX ]         = attribs.txt.font_idx;
        text_h[ SYMBOL_PT_SIZE ]                   = attribs.txt.font_size;

        text_h[ PdfPage::SYMBOL_COLOR_IDX ]        = attribs.gfx.fill.color_idx;
        if (attribs.gfx.fill.opacity != 1.0) {
            text_h[ PdfPage::SYMBOL_OPACITY ]      = attribs.gfx.fill.opacity;
        }

        text_h[ SYMBOL_X_POS_VECTOR ]              = x_vector_a;

        if (glyph_idx != -1) {
            text_h[ PdfText::SYMBOL_GLYPH_IDX ]    = glyph_idx;
        }

        if (attribs.clip_path_id != -1) {
            text_h[ PdfDocPath::SYMBOL_CLIP_TO ]   = attribs.clip_path_id;
        }

        if (attribs.txt.link_idx != -1) {
            text_h[ SYMBOL_LINK_IDX ]              = attribs.txt.link_idx;
        }

        if (ctm.is_sheared()) {
            text_h[ SYMBOL_SHEARED ]               = true;
        }

        if (attribs.txt.invisible) {
            text_h[ SYMBOL_INVISIBLE ]             = true;
        }

        return text_h;
    }
#endif

    std::ostream& PdfText::to_edn(std::ostream& o) const
    {
        util::edn::Hash text_h(16);

        // add a type identifier. TODO: This needs some cleaning up
        text_h.push( PdfGfxCmd::SYMBOL_TYPE, PdfText::SYMBOL_TYPE_SPAN );

        double font_size = attribs.txt.font_size;
        std::list<Transform *> transforms;

        // poppler doesn't produce proper bounding boxes; compensate here
        if (!ctm.is_rotated()) {
            // non-rotated text simply sets the height to be the font size
            text_h.push( BoundingBox::SYMBOL, bbox );
        }
        else {
            // rotated text? another story..
            double angle = ctm.rotation();

            // apply inverse rotation to determine horizontal character width
            PdfTM rhctm(-angle, bbox.x1(), bbox.y1());

            Coord char_props = rhctm.transform_delta(bbox.width(), -bbox.height());

            // adjust height by font size and rotate to get actual dx/dy
            char_props.y = std::abs(char_props.y) + font_size;

            PdfTM hctm(-angle, bbox.x1(), bbox.y1());

            // compute rotated bbox dimensions
            Coord p2p = hctm.transform(char_props.x, -char_props.y);
            BoundingBox bboxp(bbox.p1(), p2p);

            // determine origin for SVG
            Coord origin(bbox.x1(), bbox.y2());

            // set the transform needed for the SVG side
            angle = ctm.rotation_deg();
            double w = -bbox.width();
            double h = -bbox.height();
            double x1 = origin.x;
            double x2 = origin.x + bbox.width();
            double y1 = origin.y;
            double y2 = origin.y - bbox.height();

            // something is off with these but I need more examples to test with
            if (angle < 45) {
                transforms.push_back(new Rotate(-angle, x1, y1));
                w = 0;
            }
            else if ((int) angle == 45) {
                transforms.push_back(new Rotate(-angle, x1, y1));
                w = h = 0;
            }
            else if (angle < 90) {
                transforms.push_back(new Rotate(-angle, x2, y2));
            }
            else if ((int) angle == 90) {
                transforms.push_back(new Rotate(-angle, x1, bbox.y2()));
                h = 0;
            }
            else if (angle < 135) {
                transforms.push_back(new Rotate(-angle, x1, y2));
            }
            else if (angle < 180) {
                transforms.push_back(new Rotate(-angle, x1, y2));
            }
            else if (angle < 225) {
                transforms.push_back(new Rotate(-angle, x1, y2));
            }
            else if (angle < 270) {
                transforms.push_back(new Rotate(-angle, x1, y2));
            }
            else if ((int) angle == 270) {
                transforms.push_back(new Translate(-h, w));
                transforms.push_back(new Rotate(-angle, x1, y2));
            }
            else if (angle < 315) {
                transforms.push_back(new Rotate(-angle, x1, y2));
            }
            else if (angle < 360) {
                w = 0;
                transforms.push_back(new Rotate(-angle, x2, y2));
            }

            if (std::abs(w) != 0.0 && std::abs(h) != 0.0) {
                transforms.push_back(new Translate(w, h));
            }

            text_h.push( PdfBoxedItem::SYMBOL_ROTATION, ctm.rotation_deg() );
            text_h.push( PdfText::SYMBOL_ORIGIN, origin );
            text_h.push( BoundingBox::SYMBOL, bboxp );

            util::edn::Vector transforms_v(transforms.size());
            text_h.push( PdfBoxedItem::SYMBOL_XFORM, Transform::list_to_edn(transforms, transforms_v) );
        }

        // run through the list of characters to build the string and
        // the x-position vector..
        util::edn::Vector x_vector_a(chars.size());
        std::string str;
        intmax_t glyph_idx = -1;

        std::for_each ( chars.begin(), chars.end(),
                        [&](const PdfChar* c) {
                            str += util::wstring_to_utfstring(c->wstr());

                            if (!ctm.is_rotated()) {
                                x_vector_a.push( c->bounding_box().x1() );
                            }

                            // if a glyph was encountered in the stream, length will
                            // be 1 always since they're not spannable
                            glyph_idx = c->get_glyph_index();
                        } );

        text_h.push( SYMBOL_TEXT,                    str );

        // font and color data
        text_h.push( PdfPage::SYMBOL_FONT_IDX,       attribs.txt.font_idx );
        text_h.push( SYMBOL_PT_SIZE,                 font_size );

        text_h.push( PdfPage::SYMBOL_COLOR_IDX,      attribs.gfx.fill.color_idx );
        if (attribs.gfx.fill.opacity != 1.0) {
            text_h.push( PdfPage::SYMBOL_OPACITY,    attribs.gfx.fill.opacity );
        }

        text_h.push( SYMBOL_X_POS_VECTOR,            x_vector_a );

        if (glyph_idx != -1) {
            text_h.push( PdfText::SYMBOL_GLYPH_IDX,  glyph_idx );
        }

        if (attribs.clip_path_id != -1) {
            text_h.push( PdfDocPath::SYMBOL_CLIP_TO, attribs.clip_path_id );
        }

        if (attribs.txt.link_idx != -1) {
            text_h.push( SYMBOL_LINK_IDX,            attribs.txt.link_idx );
        }

        if (ctm.is_sheared()) {
            text_h.push( SYMBOL_SHEARED,             true );
        }

        if (attribs.txt.invisible) {
            text_h.push( SYMBOL_INVISIBLE,           true );
        }

        o << text_h;

        // cleanup
        util::delete_ptr_container_elems(transforms);
        return o;
    }

    // =============================================
    // PdfGlyph - unmapped character to be represented via a path
    //
#if 0
    Rice::Object PdfGlyph::to_ruby() const
    {
        Rice::Hash path_h = PdfPath::to_ruby();

        return path_h;
    }
#endif

} // namespace
