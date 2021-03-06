#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/log.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/randomizer.hh"

// ----------------------------------------------------------------------

static void test_rough(acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, acmacs::number_of_dimensions_t num_dims);
static void test_randomization(acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, acmacs::number_of_dimensions_t num_dims);
static void test_dimension(acmacs::chart::ChartModify& chart, std::string min_col_basis);
static void test_lbfgs_cg(acmacs::chart::ChartModify& chart, std::string min_col_basis, const acmacs::chart::dimension_schedule& schedule, acmacs::chart::optimization_precision precision);
static void optimize_n(acmacs::chart::optimization_method method, acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, acmacs::number_of_dimensions_t num_dims, acmacs::chart::optimization_precision precision);
static void optimize_n(acmacs::chart::optimization_method method, acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, const acmacs::chart::dimension_schedule& schedule, acmacs::chart::optimization_precision precision);

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"-n", 1, "number of optimizations"},
                {"-d", "2", "number of dimensions (comma or space separated values for dimension annealing)"},
                {"-m", "none", "minimum column basis"},
                {"--rough", false},
                {"--method", "cg", "method: lbfgs, cg"},
                {"--md", 1.0, "max distance multiplier"},
                {"--rough-test", false},
                {"--test-randomization", false},
                {"--test-dimension", false},
                {"--test-lbfgs-cg", false},
                {"--time", false, "report time of loading chart"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 1) {
            fmt::print(stderr, "Usage: {} [options] <chart-file> [<output-chart-file>]\n{}\n", args.program(), args.usage_options());
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            const acmacs::chart::dimension_schedule schedule{acmacs::string::split_into_uint<acmacs::number_of_dimensions_t>(args["-d"].str())};
            const auto number_of_dimensions = schedule.final();
            const auto precision = args["--rough"] ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;
            const acmacs::chart::optimization_method method{acmacs::chart::optimization_method_from_string(args["--method"])};

            if (args["--rough-test"]) {
                test_rough(chart, args["-n"], args["-m"].str(), number_of_dimensions);
            }
            else if (args["--test-randomization"]) {
                test_randomization(chart, args["-n"], args["-m"].str(), number_of_dimensions);
            }
            else if (args["--test-dimension"]) {
                test_dimension(chart, args["-m"].str());
            }
            else if (args["--test-lbfgs-cg"]) {
                test_lbfgs_cg(chart, args["-m"].str(), schedule, precision);
            }
            else {
                optimize_n(method, chart, args["-n"], args["-m"].str(), schedule, precision);
                chart.projections_modify().sort();
                std::cout << chart.make_info() << '\n';
                if (args.number_of_arguments() > 1)
                    acmacs::chart::export_factory(chart, args[1], args.program(), report);
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

void test_randomization(acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, acmacs::number_of_dimensions_t num_dims)
{
    const size_t randomizations = 100;
    auto projection = chart.projections_modify().new_from_scratch(num_dims, min_col_basis);
    auto randomizer = randomizer_plain_with_table_max_distance(*projection);
    for (size_t no = 0; no < attempts; ++no) {
        Timeit ti("randomize and relax: ");
        projection->randomize_layout(randomizer);
        double best_stress = projection->stress();
        acmacs::Layout best_layout{*projection->layout()};
        for (size_t rnd_no = 0; rnd_no < randomizations; ++rnd_no) {
            projection->randomize_layout(randomizer);
            const double stress = projection->stress();
            // std::cout << "  s: " << stress << '\n';
            if (stress < best_stress) {
                best_stress = stress;
                best_layout = *projection->layout();
            }
        }
        const auto status = projection->relax(acmacs::chart::optimization_options(acmacs::chart::optimization_method::alglib_lbfgs_pca));
        fmt::print("final {:.12f} time: {} iters: {} nstress: {} {}\n", status.final_stress, status.time, status.number_of_iterations, status.number_of_stress_calculations, status.termination_report);
    }

} // test_randomization

// ----------------------------------------------------------------------

void test_rough(acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, acmacs::number_of_dimensions_t num_dims)
{
    auto projection = chart.projections_modify().new_from_scratch(num_dims, min_col_basis);
    auto randomizer = randomizer_plain_with_table_max_distance(*projection);

    std::vector<std::tuple<size_t, double, double>> stresses;

    double speed_ratio_sum = 0;
    for (size_t no = 0; no < attempts; ++no) {
        projection->randomize_layout(randomizer);
        const auto status = projection->relax(acmacs::chart::optimization_options(acmacs::chart::optimization_method::alglib_lbfgs_pca, acmacs::chart::optimization_precision::rough));
        std::cout << "rough " << std::setprecision(12) << status.final_stress << " time: " << acmacs::format_duration(status.time) << " iters: " << status.number_of_iterations << " nstress: " << status.number_of_stress_calculations << ' ' << status.termination_report << '\n';
        const auto status2 = projection->relax(acmacs::chart::optimization_options(acmacs::chart::optimization_method::alglib_lbfgs_pca, acmacs::chart::optimization_precision::fine));
        std::cout << "fin   " << std::setprecision(12) << status2.final_stress << " time: " << acmacs::format_duration(status2.time) << " iters: " << status2.number_of_iterations << " nstress: " << status2.number_of_stress_calculations << ' ' << status.termination_report << '\n';
        const double speed_ratio = static_cast<double>(status.time.count()) / static_cast<double>((status.time + status2.time).count());
          // std::cout << "  rough speed ratio: " << speed_ratio << '\n';
        speed_ratio_sum += speed_ratio;
        stresses.emplace_back(no, status.final_stress, status2.final_stress);
    }
    std::cout << "average speed ratio: " << (speed_ratio_sum / static_cast<double>(attempts)) << '\n';

    auto report = [&stresses](std::string header) {
                      std::cout << header << '\n';
                      for (const auto& entry: stresses)
                          std::cout << "  " << std::setw(3) << std::get<0>(entry) << ' ' << std::get<1>(entry) << ' ' << std::get<2>(entry) << ' ' << (std::get<1>(entry) - std::get<2>(entry)) << '\n';
                  };

    std::sort(stresses.begin(), stresses.end(), [](const auto& a, const auto& b) { return std::get<1>(a) < std::get<1>(b); });
    std::vector<size_t> order_rough(stresses.size());
    std::transform(stresses.begin(), stresses.end(), order_rough.begin(), [](const auto& entry) { return std::get<0>(entry); });
    report("by rough");
    std::sort(stresses.begin(), stresses.end(), [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });
    std::vector<size_t> order_final(stresses.size());
    std::transform(stresses.begin(), stresses.end(), order_final.begin(), [](const auto& entry) { return std::get<0>(entry); });
    report("by final");
    fmt::print("order_rough: {}\n", order_rough);
    if (order_rough == order_final)
        fmt::print("orders are the same\n");
    else
        fmt::print("orders are DIFFERENT\norder_final: {}\n", order_final);

} // test_rough

// ----------------------------------------------------------------------

void test_dimension(acmacs::chart::ChartModify& chart, std::string min_col_basis)
{
    for (auto dims : acmacs::range<acmacs::number_of_dimensions_t>(1UL, chart.number_of_antigens())) {
        auto projection = chart.projections_modify().new_from_scratch(dims, min_col_basis);
        projection->randomize_layout(randomizer_plain_with_table_max_distance(*projection));
        const auto status = projection->relax(acmacs::chart::optimization_options(acmacs::chart::optimization_method::alglib_lbfgs_pca, acmacs::chart::optimization_precision::rough));
        std::cout << std::setw(3) << acmacs::to_string(dims) << " " << std::setprecision(12) << status.final_stress << " time: " << acmacs::format_duration(status.time) << " iters: " << status.number_of_iterations << " nstress: " << status.number_of_stress_calculations << '\n';
    }

} // test_dimension

// ----------------------------------------------------------------------

void test_lbfgs_cg(acmacs::chart::ChartModify& chart, std::string min_col_basis, const acmacs::chart::dimension_schedule& schedule, acmacs::chart::optimization_precision precision)
{
    auto projection = chart.projections_modify().new_from_scratch(schedule.initial(), min_col_basis);
    projection->randomize_layout(randomizer_plain_with_table_max_distance(*projection));
    auto layout = projection->layout_modified();
    acmacs::Layout starting{*layout};

    const auto status1 = acmacs::chart::optimize(*projection, schedule, acmacs::chart::optimization_options(acmacs::chart::optimization_method::alglib_lbfgs_pca, precision));
    fmt::print("{}\n", status1);

    projection->set_layout(starting, true);

    const auto status2 = acmacs::chart::optimize(*projection, schedule, acmacs::chart::optimization_options(acmacs::chart::optimization_method::alglib_cg_pca, precision));
    fmt::print("{}\n", status2);

} // test_lbfgs_cg

// ----------------------------------------------------------------------

void optimize_n(acmacs::chart::optimization_method method, acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, acmacs::number_of_dimensions_t num_dims, acmacs::chart::optimization_precision precision)
{
    for (size_t no = 0; no < attempts; ++no) {
          // Timeit ti("randomize and relax: ");
        auto projection = chart.projections_modify().new_from_scratch(num_dims, min_col_basis);
        projection->randomize_layout(randomizer_plain_with_table_max_distance(*projection));
        const auto status = projection->relax(acmacs::chart::optimization_options(method, precision));
        fmt::print("{}\n", status);
    }

} // optimize_n

// ----------------------------------------------------------------------

void optimize_n(acmacs::chart::optimization_method method, acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, const acmacs::chart::dimension_schedule& schedule, acmacs::chart::optimization_precision precision)
{
    if (schedule.size() == 1) {
        optimize_n(method, chart, attempts, min_col_basis, schedule.initial(), precision);
        return;
    }

    for (size_t no = 0; no < attempts; ++no) {
        auto projection = chart.projections_modify().new_from_scratch(schedule.initial(), min_col_basis);
        projection->randomize_layout(randomizer_plain_with_table_max_distance(*projection));
        auto layout = projection->layout_modified();
        auto stress = acmacs::chart::stress_factory(*projection, acmacs::chart::multiply_antigen_titer_until_column_adjust::yes);
        for (auto num_dims : schedule) {
            if (num_dims > projection->number_of_dimensions())
                throw std::runtime_error("invalid schedule: " + acmacs::to_string(schedule));
            if (num_dims < projection->number_of_dimensions()) {
                acmacs::chart::dimension_annealing(method, stress, projection->number_of_dimensions(), num_dims, layout->data(), layout->data() + layout->size());
                layout->change_number_of_dimensions(num_dims);
                stress.change_number_of_dimensions(num_dims);
            }
            const auto status = acmacs::chart::optimize(method, stress, layout->data(), layout->data() + layout->size(), precision);
            std::cout << std::setprecision(12)
                      // << status.initial_stress << " --> "
                      << status.final_stress << " dims: " << acmacs::to_string(layout->number_of_dimensions())
                      << " time: " << acmacs::format_duration(status.time) << " iters: " << status.number_of_iterations << " nstress: " << status.number_of_stress_calculations << '\n';
        }
    }

} // optimize_n

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
