#pragma once

#include <string>

namespace pdftoedn {

    class Options
    {
    public:
        // runtime options
        struct Flags {
            bool omit_outline;
            bool use_page_crop_box;
            bool crop_page;
            bool include_invisible_text;
            bool link_output_only;
            bool include_debug_info;
            bool libpng_use_best_compression;
            bool force_font_preprocess;
            bool force_output_write;
        };

        Options() : page_num(-1) {}
        Options(const std::string& pdf_filename, const std::string& edn_filename, const std::string& font_map,
                const Flags& f, intmax_t pg_num);

        const std::string& pdf_filename() const  { return src_pdf_filename; }
        const std::string& edn_filename() const  { return out_edn_filename; }
        const std::string& outputdir() const     { return output_path; }
        intmax_t page_number() const             { return page_num; }

        bool get_image_path(intmax_t id, std::string& abs_file_path, bool create_res_dir = true) const;
        std::string get_image_rel_path(const std::string& abs_path) const;

        bool omit_outline() const                { return flags.omit_outline; }
        bool use_page_crop_box() const           { return flags.use_page_crop_box; }
        bool crop_page() const                   { return flags.crop_page; }
        bool include_invisible_text() const      { return flags.include_invisible_text; }
        bool link_output_only() const            { return flags.link_output_only; }
        bool libpng_use_best_compression() const { return flags.libpng_use_best_compression; }
        bool include_debug_info() const          { return flags.include_debug_info; }
        bool force_pre_process_fonts() const     { return flags.force_font_preprocess; }
        bool force_output_write() const          { return flags.force_output_write; }

        friend std::ostream& operator<<(std::ostream& o, const Options& opt);

    private:
        std::string src_pdf_filename;
        std::string out_edn_filename;
        std::string font_map;
        Flags flags;
        intmax_t page_num;
        std::string output_path;
        std::string resource_dir;
        std::string doc_base_name;

        bool load_config(const std::string& new_font_map_file);
    };

    extern pdftoedn::Options options;

} // namespace
