#include <iostream>
#include <algorithm>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/grid-test.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/procrustes.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv,
                       {
                           {"--no-omp", false, "single thread test"},
                           {"--point", "all", "point number to test"},
                           {"--step", 0.1, "grid step"},
                           {"--relax", false, "move trapped points and relax, test again, repeat while there are trapped points"},
                           {"--projection", 0, "projection number to test"},
                           {"--verbose", false},
                           {"--time", false, "report time of loading chart"},
                           {"-h", false},
                           {"--help", false},
                           {"-v", false},
                       });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 1 || args.number_of_arguments() > 2) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> [<output-chart>]\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            const bool verbose = args["--verbose"] || args["-v"];
            const std::string to_test(args["--point"]);
            const size_t projection_no = args["--projection"];
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};

            // std::cout << test.initial_report() << '\n';
            if (to_test == "all") {
                std::vector<size_t> projections_to_reorient;
                size_t projection_no_to_test = projection_no;
                for (auto attempt = 1; attempt < 10; ++attempt) {
                    acmacs::chart::GridTest test(chart, projection_no_to_test, args["--step"]);
                    const auto results = args["--no-omp"] ? test.test_all() : test.test_all_parallel();
                    std::cout << results.report() << '\n';
                    if (verbose || !args["--relax"]) {
                        for (const auto& entry : results) {
                            if (entry)
                                std::cout << entry.report() << '\n';
                        }
                    }
                    if (!args["--relax"])
                        break;
                    auto projection = test.make_new_projection_and_relax(results, true);
                    projection->comment("grid-test-" + acmacs::to_string(attempt));
                    projection_no_to_test = projection->projection_no();
                    projections_to_reorient.push_back(projection_no_to_test);
                    if (std::all_of(results.begin(), results.end(), [](const auto& result) { return result.diagnosis != acmacs::chart::GridTest::Result::trapped; }))
                        break;
                }
                if (args.number_of_arguments() > 1) {
                    if (!projections_to_reorient.empty()) {
                        acmacs::chart::CommonAntigensSera common(chart);
                        auto master_projection = chart.projection(projection_no);
                        for (auto projection_to_reorient : projections_to_reorient) {
                            const auto procrustes_data = acmacs::chart::procrustes(*master_projection, *chart.projection(projection_to_reorient), common.points(), acmacs::chart::procrustes_scaling_t::no);
                            chart.projection_modify(projection_to_reorient)->transformation(procrustes_data.transformation);
                        }
                    }
                    chart.projections_modify()->sort();
                    acmacs::chart::export_factory(chart, args[1], fs::path(args.program()).filename(), report);
                }
                std::cerr << chart.make_info() << '\n';
            }
            else {
                acmacs::chart::GridTest test(chart, projection_no, args["--step"]);
                if (const auto result = test.test_point(std::stoul(to_test)); result)
                    std::cout << result.report() << '\n';
            }

            // if (point_no >= chart.number_of_points())
            //     throw std::runtime_error("invalid point number");
        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
