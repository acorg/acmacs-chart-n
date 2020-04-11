#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-chart-2/text-export.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

static std::string export_forced_column_bases_to_text(const acmacs::chart::Chart& chart);
static std::string export_projections_to_text(const acmacs::chart::Chart& chart);
static std::string export_plot_spec_to_text(const acmacs::chart::Chart& chart);
static std::string export_extensions_to_text(const acmacs::chart::Chart& chart);
static std::string export_style_to_text(const acmacs::PointStyle& aStyle);

// ----------------------------------------------------------------------

std::string acmacs::chart::export_text(const Chart& chart)
{
    fmt::memory_buffer result;
    fmt::format_to(result, "{}",
                   acmacs::string::join("\n\n", export_info_to_text(chart), export_table_to_text(chart), export_forced_column_bases_to_text(chart), export_projections_to_text(chart),
                                        export_plot_spec_to_text(chart), export_extensions_to_text(chart)),
                   "\n", acmacs::string::Split::StripKeepEmpty);
    return fmt::to_string(result);

    // // Timeit ti_plot_spec("export plot_spec ");
    // if (auto plot_spec = aChart.plot_spec(); !plot_spec->empty())
    //     export_plot_spec(ace["c"]["p"], plot_spec);
    //   // ti_plot_spec.report();
    // if (const auto& ext = aChart.extension_fields(); ext.is_object())
    //      ace["c"]["x"] = ext;
} // acmacs::chart::export_text

// ----------------------------------------------------------------------

std::string acmacs::chart::export_table_to_text(const Chart& chart, std::optional<size_t> just_layer)
{
    fmt::memory_buffer result;
    auto antigens = chart.antigens();
    auto sera = chart.sera();
    auto titers = chart.titers();
    const auto max_antigen_name = antigens->max_full_name();

    if (just_layer.has_value()) {
        if (*just_layer >= titers->number_of_layers())
            throw std::runtime_error(fmt::format("Invalid layer: {}, number of layers in the chart: {}", *just_layer, titers->number_of_layers()));
        fmt::format_to(result, "Layer {}   {}\n\n", *just_layer, chart.info()->source(*just_layer)->make_name());
    }

    const auto column_width = 8;
    const auto table_prefix = 5;
    fmt::format_to(result, "{: >{}s}  ", "", max_antigen_name + table_prefix);
    for (auto serum_no : acmacs::range(sera->size()))
        fmt::format_to(result, "{: ^{}d}", serum_no, column_width);
    fmt::format_to(result, "\n");
    fmt::format_to(result, "{: >{}s}  ", "", max_antigen_name + table_prefix);
    for (auto serum : *sera)
        fmt::format_to(result, "{: ^8s}", serum->abbreviated_location_year(), column_width);
    fmt::format_to(result, "\n\n");

    const auto ag_no_num_digits = static_cast<int>(std::log10(antigens->size())) + 1;
    if (!just_layer.has_value()) {
        // merged table
        for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
            fmt::format_to(result, "{:{}d} {: <{}s} ", ag_no, ag_no_num_digits, antigen->full_name(), max_antigen_name);
            for (auto serum_no : acmacs::range(sera->size()))
                fmt::format_to(result, "{: >{}s}", *titers->titer(ag_no, serum_no), column_width);
            fmt::format_to(result, "\n");
        }
    }
    else {
        // just layer
        const auto [antigens_of_layer, sera_of_layer] = titers->antigens_sera_of_layer(*just_layer);
        for (auto ag_no : antigens_of_layer) {
            auto antigen = antigens->at(ag_no);
            fmt::format_to(result, "{:{}d} {: <{}s} ", ag_no, ag_no_num_digits, antigen->full_name(), max_antigen_name);
            for (auto serum_no : acmacs::range(sera->size()))
                fmt::format_to(result, "{: >{}s}", *titers->titer_of_layer(*just_layer, ag_no, serum_no), column_width);
            fmt::format_to(result, "\n");
        }
    }

    fmt::format_to(result, "\n");
    for (auto [sr_no, serum] : acmacs::enumerate(*sera))
        fmt::format_to(result, "{: >{}s} {:3d} {}\n", "", max_antigen_name + table_prefix, sr_no, serum->full_name());

    return fmt::to_string(result);

} // acmacs::chart::export_table_to_text

// ----------------------------------------------------------------------

std::string acmacs::chart::export_info_to_text(const Chart& chart)
{
    fmt::memory_buffer result;

    const auto do_export = [&result](acmacs::chart::InfoP info) {
        fmt::format_to(result, "{}", acmacs::string::join(" ", info->virus(), info->virus_type(), info->assay(), info->date(), info->name(), info->lab(), info->rbc_species(), info->subset()));
        // info->table_type()
    };

    auto info = chart.info();
    do_export(info);
    if (const auto number_of_sources = info->number_of_sources(); number_of_sources) {
        for (size_t source_no = 0; source_no < number_of_sources; ++source_no) {
            fmt::format_to(result, "\n{:2d}  ", source_no);
            do_export(info->source(source_no));
        }
    }
    return fmt::to_string(result);

} // acmacs::chart::export_info_to_text

// ----------------------------------------------------------------------

std::string export_forced_column_bases_to_text(const acmacs::chart::Chart& chart)
{
    fmt::memory_buffer result;
    if (const auto column_bases = chart.forced_column_bases(acmacs::chart::MinimumColumnBasis{}); column_bases) {
        fmt::format_to(result, "forced-column-bases:");
        for (size_t sr_no = 0; sr_no < column_bases->size(); ++sr_no)
            fmt::format_to(result, "{}", column_bases->column_basis(sr_no));
    }
    return fmt::to_string(result);

} // export_forced_column_bases_to_text

// ----------------------------------------------------------------------

std::string export_projections_to_text(const acmacs::chart::Chart& chart)
{
    fmt::memory_buffer result;
    if (auto projections = chart.projections(); !projections->empty()) {
        fmt::format_to(result, "projections: {}\n\n", projections->size());
        for (size_t projection_no = 0; projection_no < projections->size(); ++projection_no) {
            fmt::format_to(result, "projection {}\n", projection_no);
            auto projection = (*projections)[projection_no];
            if (const auto comment = projection->comment(); !comment.empty())
                fmt::format_to(result, "  comment: {}\n", comment);
            if (const auto stress = projection->stress(); !std::isnan(stress) && stress >= 0)
                fmt::format_to(result, "  stress: {}\n", stress);
            if (const auto minimum_column_basis = projection->minimum_column_basis(); !minimum_column_basis.is_none())
                fmt::format_to(result, "  minimum-column-basis: {}\n", minimum_column_basis);
            if (const auto column_bases = projection->forced_column_bases(); column_bases) {
                fmt::format_to(result, "  forced-column-bases:");
                for (size_t sr_no = 0; sr_no < column_bases->size(); ++sr_no)
                    fmt::format_to(result, "{}", column_bases->column_basis(sr_no));
                fmt::format_to(result, "\n");
            }
            if (const auto transformation = projection->transformation(); transformation != acmacs::Transformation{} && transformation.valid())
                fmt::format_to(result, "  transformation: {}\n", transformation.as_vector());
            if (projection->dodgy_titer_is_regular() == acmacs::chart::dodgy_titer_is_regular::yes)
                fmt::format_to(result, "  dodgy-titer-is-regular: {}\n", true);
            if (projection->stress_diff_to_stop() > 0)
                fmt::format_to(result, "  stress-diff-to-stop: {}\n", projection->stress_diff_to_stop());
            if (const auto unmovable = projection->unmovable(); !unmovable->empty())
                fmt::format_to(result, "  unmovable: {}\n", unmovable);
            if (const auto disconnected = projection->disconnected(); !disconnected->empty())
                fmt::format_to(result, "  disconnected: {}\n", disconnected);
            if (const auto unmovable_in_the_last_dimension = projection->unmovable_in_the_last_dimension(); !unmovable_in_the_last_dimension->empty())
                fmt::format_to(result, "  unmovable-in-the-last-dimension: {}\n", unmovable_in_the_last_dimension);
            if (const auto avidity_adjusts = projection->avidity_adjusts(); !avidity_adjusts.empty())
                fmt::format_to(result, "  avidity-adjusts: {}\n", avidity_adjusts);

            auto layout = projection->layout();
            const auto number_of_dimensions = layout->number_of_dimensions();
            fmt::format_to(result, "  layout {} x {}\n", layout->number_of_points(), number_of_dimensions);
            if (const auto number_of_points = layout->number_of_points(); number_of_points && acmacs::valid(number_of_dimensions)) {
                for (size_t p_no = 0; p_no < number_of_points; ++p_no)
                    fmt::format_to(result, "    {:4d} {:20.17f}\n", p_no, layout->at(p_no));
            }
        }
    }
    return fmt::to_string(result);

} // export_projections_to_text

// ----------------------------------------------------------------------

std::string export_plot_spec_to_text(const acmacs::chart::Chart& chart)
{
    fmt::memory_buffer result;

    if (auto plot_spec = chart.plot_spec(); !plot_spec->empty()) {
        fmt::format_to(result, "plot-spec:\n");
        if (const auto drawing_order = plot_spec->drawing_order(); !drawing_order->empty()) {
            fmt::format_to(result, "  drawing-order:");
            for (const auto& index : drawing_order)
                fmt::format_to(result, " {}", index);
            fmt::format_to(result, "\n");
        }
        if (const auto color = plot_spec->error_line_positive_color(); color != RED)
            fmt::format_to(result, "  error-line-positive-color: {}\n", color);
        if (const auto color = plot_spec->error_line_negative_color(); color != BLUE)
            fmt::format_to(result, "  error-line-negative-color: {}\n", color);

        const auto compacted = plot_spec->compacted();

        fmt::format_to(result, "  plot-style-per-point:");
        for (const auto& index : compacted.index)
            fmt::format_to(result, " {}", index);
        fmt::format_to(result, "\n");

        fmt::format_to(result, "  styles: {}\n", compacted.styles.size());
        for (size_t style_no = 0; style_no < compacted.styles.size(); ++style_no)
            fmt::format_to(result, "    {:2d} {}\n", style_no, export_style_to_text(compacted.styles[style_no]));

        // "g": {},                  // ? grid data
        // "l": [],                  // ? for each procrustes line, index in the "L" list
        // "L": []                    // ? list of procrustes lines styles
        // "s": [],                  // list of point indices for point shown on all maps in the time series
        // "t": {}                    // title style?
    }
    return fmt::to_string(result);

} // export_plot_spec_to_text

// ----------------------------------------------------------------------

std::string export_style_to_text(const acmacs::PointStyle& aStyle)
{
    fmt::memory_buffer result;
    fmt::format_to(result, "shown:{} fill:\"{}\" outline:\"{}\" outline_width:{} size:{} rotation:{} aspect:{} shape:{}", aStyle.shown, aStyle.fill, aStyle.outline, aStyle.outline_width.value(),
                   aStyle.size.value(), aStyle.rotation, aStyle.aspect, aStyle.shape);
    fmt::format_to(result, " label:{{ shown:{} text:\"{}\" font_family:\"{}\" slant:\"{}\" weight:\"{}\" size:{} color:\"{}\" rotation:{} interline:{} offset:{}",
                   aStyle.label.shown, aStyle.label_text, aStyle.label.style.font_family, aStyle.label.style.slant, aStyle.label.style.weight, aStyle.label.size.value(), aStyle.label.color,
                   aStyle.label.rotation, aStyle.label.interline, aStyle.label.offset);

    return fmt::to_string(result);

} // export_style_to_text

// ----------------------------------------------------------------------

std::string export_extensions_to_text(const acmacs::chart::Chart& chart)
{
    fmt::memory_buffer result;
    return fmt::to_string(result);

} // export_extensions_to_text

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
