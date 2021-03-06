#include "acmacs-base/argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/procrustes.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str>  match{*this, "match", dflt{"auto"}, desc{"match level: \"strict\", \"relaxed\", \"ignored\", \"auto\""}};
    option<int>  master_projection_no{*this, 'm', "master-projection", dflt{0}, desc{"master projection no"}};
    option<int>  chart_projection_no{*this, 'p', "chart-projection", dflt{0}, desc{"chart projection no, -1 to reorient all"}};

    argument<str> master_chart{*this, arg_name{"master-chart"}, mandatory};
    argument<str_array> sources{*this, arg_name{"chart-to-reorient"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);

        const auto match_level = acmacs::chart::CommonAntigensSera::match_level(opt.match);
        auto master = acmacs::chart::import_from_file(opt.master_chart);
        fmt::print("master: {}\n", master->make_name());
        for (const auto& source : *opt.sources) {
            acmacs::chart::ChartModify to_reorient{acmacs::chart::import_from_file(source)};
            fmt::print("{}\n", to_reorient.make_name());
            acmacs::chart::CommonAntigensSera common(*master, to_reorient, match_level);
            if (common) {
                bool modified{false};
                fmt::print("    common antigens: {} sera: {}\n", common.common_antigens(), common.common_sera());
                const auto projections = *opt.chart_projection_no < 0 ? acmacs::filled_with_indexes(to_reorient.number_of_projections()) : std::vector<size_t>{static_cast<size_t>(*opt.chart_projection_no)};
                for (auto projection_no : projections) {
                    auto procrustes_data = acmacs::chart::procrustes(*master->projection(static_cast<size_t>(*opt.master_projection_no)), *to_reorient.projection(projection_no), common.points(), acmacs::chart::procrustes_scaling_t::no);
                    fmt::print("    projection: {}\n", projection_no);
                    if (to_reorient.projection_modify(projection_no)->transformation().difference(procrustes_data.transformation) > 1e-5) {
                        to_reorient.projection_modify(projection_no)->transformation(procrustes_data.transformation);
                        modified = true;
                        fmt::print("transformation: {}\nrms: {}\n", procrustes_data.transformation, procrustes_data.rms);
                    }
                    else
                        fmt::print("already oriented\n");
                }
                if (modified)
                    acmacs::chart::export_factory(to_reorient, source, opt.program_name());
            }
            else {
                AD_ERROR("no common antigens/sera");
            }
        }
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
