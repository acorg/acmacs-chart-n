#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/rjson.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/randomizer.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"-d", 2U, "number of dimensions"},
                {"-m", "none", "minimum column basis"},
                {"--rough", false},
                {"--method", "cg", "method: lbfgs, cg"},
                {"--md", 2.0, "randomization diameter multiplier"},
                {"--no-disconnect-having-few-titers", false, "do not disconnect points having too few numeric titers"},
                {"--time", false, "report time of loading chart"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 2) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> <output-layouts.json>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            const auto precision = args["--rough"] ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;
            const auto method{acmacs::chart::optimization_method_from_string(args["--method"])};
            acmacs::chart::PointIndexList disconnected;
            if (!args["--no-disconnect-having-few-titers"])
                disconnected.extend(chart.titers()->having_too_few_numeric_titers());

            auto projection = chart.projections_modify()->new_from_scratch(args["-d"], static_cast<std::string_view>(args["-m"]));
            projection->randomize_layout(acmacs::chart::randomizer_plain_with_table_max_distance(*projection));
            acmacs::chart::IntermediateLayouts intermediate_layouts;
            const auto status = projection->relax(acmacs::chart::optimization_options(method, precision, args["--md"]), intermediate_layouts);
            std::cout << "INFO: " << status << '\n';
            std::cout << "INFO: intermediate_layouts: " << intermediate_layouts.size() << '\n';
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
