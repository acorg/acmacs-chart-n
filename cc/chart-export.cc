#include "acmacs-base/argv.hh"
#include "acmacs-base/fmt.hh"
// #include "acmacs-base/to-json-v1.hh"
#include "acmacs-base/to-json.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool>   names{*this, "names", desc{"output antigen and serum names"}};
    option<bool>   titers{*this, "titers", desc{"output titers"}};
    option<bool>   logged_titers{*this, "logged-titers", desc{"output logged titers"}};
    option<bool>   map_distances{*this, "map-distances", desc{"output map distances"}};
    option<bool>   column_bases{*this, "column-bases", desc{"output column bases"}};
    option<size_t> projection{*this, "projection", dflt{0UL}};

    argument<str>  chart{*this, arg_name{"chart"}, mandatory};
    argument<str>  output{*this, arg_name{"output"}, dflt{"-"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        auto chart = acmacs::chart::import_from_file(opt.chart, acmacs::chart::Verify::None);
        auto json = to_json::object();
        if (opt.names) {
            auto antigens = chart->antigens();
            auto sera = chart->sera();
            json << to_json::key_val{"antigens", to_json::array(antigens->begin(), antigens->end(), [](const auto& antigen) { return antigen->format("{name_full}"); })}
                 << to_json::key_val{"sera", to_json::array(sera->begin(), sera->end(), [](const auto& serum) { return serum->format("{name_full}"); })};
        }
        if (opt.titers) {
            auto titers = chart->titers();
            std::vector<std::vector<size_t>> values(titers->number_of_antigens(), std::vector<size_t>(titers->number_of_sera(), 0UL));
            for (const auto& titer : titers->titers_existing())
                values[titer.antigen][titer.serum] = titer.titer.value_with_thresholded();
            json << to_json::key_val{"titers", to_json::array(values.begin(), values.end(), [](const auto& row) { return to_json::array(row.begin(), row.end()); })};
        }
        if (opt.logged_titers) {
            auto titers = chart->titers();
            std::vector<std::vector<double>> values(titers->number_of_antigens(), std::vector<double>(titers->number_of_sera(), -1));
            for (const auto& titer : titers->titers_existing())
                values[titer.antigen][titer.serum] = titer.titer.logged_with_thresholded();
            json << to_json::key_val{"logged_titers", to_json::array(values.begin(), values.end(), [](const auto& row) { return to_json::array(row.begin(), row.end()); })};
        }
        if (opt.map_distances) {
            auto layout = chart->projection(opt.projection)->layout();
            const auto number_of_antigens = chart->number_of_antigens(), number_of_sera = chart->number_of_sera();
            std::vector<std::vector<double>> distances(number_of_antigens, std::vector<double>(number_of_sera, -1));
            for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no)
                for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no)
                    distances[ag_no][sr_no] = layout->distance(ag_no, sr_no + number_of_antigens, -1); // -1 for no distance
            json << to_json::key_val{"map_distances", to_json::array(distances.begin(), distances.end(), [](const auto& row) { return to_json::array(row.begin(), row.end()); })};
        }
        if (opt.column_bases) {
            if (auto fcb = chart->projection(opt.projection)->forced_column_bases(); fcb) {
                const auto fcb_data = fcb->data();
                json << to_json::key_val{"column_bases", to_json::array(fcb_data.begin(), fcb_data.end())};
            }
            else {
                const auto cb_data = chart->computed_column_bases(acmacs::chart::MinimumColumnBasis{})->data();
                json << to_json::key_val{"column_bases", to_json::array(cb_data.begin(), cb_data.end())};
            }
        }
        acmacs::file::write(opt.output, json.pretty(2));
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
