#pragma once

#include <ostream>
#include <climits>
#include <cstdlib>
#include <list>
#include "base_types.h"

namespace pdftoedn
{
    // -------------------------------------------------------
    // abstract gfx-type for output commands
    //
    class PdfGfxCmd : public gemable
    {
    public:
        static const pdftoedn::Symbol SYMBOL_TYPE;

        PdfGfxCmd(const pdftoedn::Symbol& gfx_cmd) :
            cmd(gfx_cmd) { }

#ifdef EDSEL_RUBY_GEM
        virtual Rice::Object to_ruby() const;
#else
        virtual std::ostream& to_edn(std::ostream&) const;
#endif
    protected:
        const pdftoedn::Symbol& cmd;

#ifndef EDSEL_RUBY_GEM
        virtual util::edn::Vector& to_edn_vector(util::edn::Vector&) const;
#endif
    };


    // -------------------------------------------------------
    // subpath commands (commands with coordinates)
    //
    class PdfSubPathCmd : public PdfGfxCmd
    {
    public:
        PdfSubPathCmd(const pdftoedn::Symbol& cmd_symbol) :
            PdfGfxCmd(cmd_symbol) {}
        PdfSubPathCmd(const pdftoedn::Symbol& cmd_symbol,
                      const Coord& c) :
            PdfGfxCmd(cmd_symbol) {
            coords.push_back(c);
        }
        PdfSubPathCmd(const pdftoedn::Symbol& cmd_name,
                      const Coord& c1,
                      const Coord& c2,
                      const Coord& c3);

        virtual bool is_curved() const { return false; }
        bool get_cur_pt(Coord& c) const;
        bool equals(const PdfSubPathCmd& c) const { return (coords == c.coords); }

#ifdef EDSEL_RUBY_GEM
        virtual Rice::Object to_ruby() const;
#else
        virtual std::ostream& to_edn(std::ostream&) const;
#endif
    protected:
        std::list<Coord> coords;
    };



    // -------------------------------------------------------
    // path building
    //
    class PdfPath : public PdfGfxCmd {
    public:
        enum eShape { UNKNOWN, IRREGULAR, RECTANGULAR };

        static const pdftoedn::Symbol SYMBOL_TYPE_PATH;
        static const pdftoedn::Symbol SYMBOL_COMMAND_LIST;

        // constructors - stroke or fill paths
        PdfPath() : PdfGfxCmd(SYMBOL_TYPE_PATH), shape(UNKNOWN) { }
        PdfPath(const pdftoedn::Symbol& cmd_name) : PdfGfxCmd(cmd_name), shape(UNKNOWN) { }
        virtual ~PdfPath();

        bool equals(const PdfPath& p2) const;

        // subpath commands
        void move_to(const Coord& c);
        void curve_to(const Coord& c1, const Coord& c2, const Coord& c3);
        void line_to(const Coord& c);
        void close();

        uintmax_t length() const { return cmds.size(); }
        bool is_rectangular() const { return (shape == RECTANGULAR); }
        bool get_cur_pt(Coord& c) const;
        BoundingBox bounding_box() const { return bounds.bounding_box(); }

#ifdef EDSEL_RUBY_GEM
        // rubify
        virtual Rice::Object to_ruby() const;
#else
        virtual std::ostream& to_edn(std::ostream&) const;
#endif
    protected:
        Bounds bounds;
        eShape shape;
        std::list<PdfSubPathCmd *> cmds;

#ifndef EDSEL_RUBY_GEM
        virtual util::edn::Hash& to_edn_hash(util::edn::Hash& h) const;
#endif
    };



    // -------------------------------------------------------
    // graphics attributes
    //
    struct GfxAttribs
    {
        static const pdftoedn::Symbol SYMBOL_STROKE_COLOR_IDX;
        static const pdftoedn::Symbol SYMBOL_STROKE_OPACITY;
        static const pdftoedn::Symbol SYMBOL_FILL_COLOR_IDX;
        static const pdftoedn::Symbol SYMBOL_FILL_OPACITY;
        static const pdftoedn::Symbol SYMBOL_STROKE_OVERPRINT;
        static const pdftoedn::Symbol SYMBOL_FILL_OVERPRINT;
        static const pdftoedn::Symbol SYMBOL_LINE_WIDTH;
        static const pdftoedn::Symbol SYMBOL_MITER_LIMIT;
        static const pdftoedn::Symbol SYMBOL_LINE_CAP;
        static const pdftoedn::Symbol SYMBOL_LINE_CAP_STYLE[];
        static const pdftoedn::Symbol SYMBOL_LINE_JOIN;
        static const pdftoedn::Symbol SYMBOL_LINE_JOIN_STYLE[];
        static const pdftoedn::Symbol SYMBOL_DASH_VECTOR;
        static const pdftoedn::Symbol SYMBOL_BLEND_MODE;
        static const pdftoedn::Symbol SYMBOL_BLEND_MODE_TYPES[];
        static const pdftoedn::Symbol SYMBOL_OVERPRINT_MODE_TYPES[];

        //
        // line cap styles
        enum LineCapStyles {
            BUTT_CAP,
            ROUND_CAP,
            PROJECTING_SQUARE_CAP,

            LINE_CAP_STYLE_COUNT // delimiter
        };

        enum LineJoinStyles {
            MITER_JOIN,
            ROUND_JOIN,
            BEVEL_JOIN,

            LINE_JOIN_STYLE_COUNT // delim
        };

        enum BlendModes {
            NORMAL_BLEND,
            MULTIPLY_BLEND,
            SCREEN_BLEND,
            DARKEN_BLEND,
            LIGHTEN_BLEND,

            BLEND_MODE_COUNT // delim
        };

        enum OverprintMode {
            STANDARD_OVERPRINT,
            ILLUSTRATOR_OVERPRINT,

            OVERPRINT_MODE_COUNT // delim
        };

        //
        // stroke / fill common attributes
        struct StrokeFill {

            // constructor
            StrokeFill(intmax_t color_index = -1, double _opacity = 0) :
                color_idx(color_index), opacity(_opacity),
                overprint(false)
            { }

            bool operator==(const StrokeFill& sf) const {
                return ( (color_idx == sf.color_idx) &&
                         (opacity == sf.opacity) &&
                         (overprint == sf.overprint) );
            }
            bool operator!=(const StrokeFill& sf) const {
                return ( (color_idx != sf.color_idx) ||
                         (opacity != sf.opacity) ||
                         (overprint != sf.overprint) );
            }

            intmax_t color_idx;
            // int gray_idx; // ??
            double opacity;
            bool overprint;

            std::ostream& dump(std::ostream& o) const;
            friend std::ostream& operator<<(std::ostream& o, const StrokeFill& sf);
        }; // StrokeFill


        //
        // GfxAttribs constructor
        //
        GfxAttribs() :
            line_width(0), miter_limit(0),
            line_cap(-1), line_join(-1),
            blend_mode(NORMAL_BLEND),
            overprint_mode(STANDARD_OVERPRINT),
            clip_idx(-1)
        { }


        bool operator==(const GfxAttribs& a2) const { return equals(a2); }
        bool operator!=(const GfxAttribs& a2) const { return !equals(a2); }
        bool equals(const GfxAttribs& a2) const;

        // data
        StrokeFill stroke;
        StrokeFill fill;
        std::list<double> line_dash;
        double line_width;
        double miter_limit;
        int8_t line_cap;
        int8_t line_join;
        uint8_t blend_mode;
        uint8_t overprint_mode;
        intmax_t clip_idx;

        std::ostream& dump(std::ostream& o) const;
        friend std::ostream& operator<<(std::ostream& o, const GfxAttribs& a);
    };


    // -------------------------------------------------------
    // path found in PDF content. Carries graphic attribs
    //
    class PdfDocPath : public PdfPath
    {
    public:

        static const pdftoedn::Symbol SYMBOL_PATH_TYPE;
        static const pdftoedn::Symbol SYMBOL_PATH_TYPES[]; // clip, fill, stroke
        static const pdftoedn::Symbol SYMBOL_ATTRIBS;
        static const pdftoedn::Symbol SYMBOL_EVEN_ODD;
        static const pdftoedn::Symbol SYMBOL_ID;
        static const pdftoedn::Symbol SYMBOL_CLIP_TO;

        enum Type {
            STROKE,
            FILL,
            CLIP,
            //            CLIP_TO_STROKE // ??
        };

        enum EvenOddRule {
            EVEN_ODD_RULE_DISABLED,
            EVEN_ODD_RULE_ENABLED
        };

        // constructor
        PdfDocPath(Type doc_path_type,
                   const GfxAttribs& gfx_attribs,
                   EvenOddRule even_odd_flag) :
            path_type(doc_path_type),
            attribs(gfx_attribs),
            even_odd(even_odd_flag),
            clip_id(-1)
        { }

        // accessors
        Type type() const { return path_type; }
        intmax_t id() const { return clip_id; }
        bool equals(const PdfDocPath& p2) const;

        void set_clip_id(intmax_t id) { clip_id = id; }
        void clip_bounds(const BoundingBox& clip_bbox) { bounds.clip(clip_bbox); }

#ifdef EDSEL_RUBY_GEM
        virtual Rice::Object to_ruby() const;
#else
        virtual std::ostream& to_edn(std::ostream& o) const;
#endif

    protected:
        Type path_type;
        GfxAttribs attribs;
        EvenOddRule even_odd; // applies only to fill & clip

        // if this is a CLIP Path, this is the id to be used in
        // output; if STROKE or FILL path, this is the id of the clip
        // path to clip to (when != -1)
        intmax_t clip_id;

#ifdef EDSEL_RUBY_GEM
        Rice::Object attribs_to_ruby() const;
#else
        util::edn::Hash& attribs_to_edn_hash(util::edn::Hash& h) const;
#endif
    };

} // namespace
