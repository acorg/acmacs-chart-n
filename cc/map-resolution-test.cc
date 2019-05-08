#include "acmacs-base/range.hh"
#include "acmacs-chart-2/map-resolution-test.hh"
#include "acmacs-chart-2/chart-modify.hh"

static void relax(acmacs::chart::ChartModify& chart, acmacs::number_of_dimensions_t number_of_dimensions, const acmacs::chart::map_resolution_test_data::Parameters& parameters);
static void relax_with_proportion_dontcared(acmacs::chart::ChartModify& chart, acmacs::number_of_dimensions_t number_of_dimensions, double proportion_to_dont_care, size_t replicate_no, const acmacs::chart::map_resolution_test_data::Parameters& parameters);
static acmacs::chart::map_resolution_test_data::ReplicateStat collect_errors(acmacs::chart::ChartModify& master_chart, acmacs::chart::ChartModify& prediction_chart, const acmacs::chart::map_resolution_test_data::Parameters& parameters);
// ----------------------------------------------------------------------

acmacs::chart::map_resolution_test_data::Results acmacs::chart::map_resolution_test(ChartModify& chart, const map_resolution_test_data::Parameters& parameters)
{
    chart.projections_modify()->remove_all();
    for (auto number_of_dimensions : parameters.number_of_dimensions) {
        for (auto proportion_to_dont_care : parameters.proportions_to_dont_care) {
            if (parameters.relax_from_full_table == map_resolution_test_data::relax_from_full_table::yes)
                relax(chart, number_of_dimensions, parameters);
            for (auto replicate_no : range(parameters.number_of_random_replicates_for_each_proportion))
                relax_with_proportion_dontcared(chart, number_of_dimensions, proportion_to_dont_care, replicate_no + 1, parameters);
        }
    }

} // acmacs::chart::map_resolution_test

// ----------------------------------------------------------------------

void relax(acmacs::chart::ChartModify& chart, acmacs::number_of_dimensions_t number_of_dimensions, const acmacs::chart::map_resolution_test_data::Parameters& parameters)
{
    chart.relax(parameters.number_of_optimizations, parameters.minimum_column_basis, number_of_dimensions, acmacs::chart::use_dimension_annealing::yes, {parameters.optimization_precision}, acmacs::chart::report_stresses::no);

} // relax

// ----------------------------------------------------------------------

void relax_with_proportion_dontcared(acmacs::chart::ChartModify& master_chart, acmacs::number_of_dimensions_t number_of_dimensions, double proportion_to_dont_care, size_t replicate_no, const acmacs::chart::map_resolution_test_data::Parameters& parameters)
{
    acmacs::chart::ChartClone chart(master_chart, acmacs::chart::ChartClone::clone_data::titers);
    chart.info_modify()->name_append(string::concat(proportion_to_dont_care, "-dont-cared"));
    chart.titers_modify()->remove_layers();
    chart.titers_modify()->set_proportion_of_titers_to_dont_care(proportion_to_dont_care);
    if (parameters.column_bases_from_master == acmacs::chart::map_resolution_test_data::column_bases_from_master::yes)
        chart.forced_column_bases_modify(*master_chart.column_bases(parameters.minimum_column_basis));
    if (parameters.relax_from_full_table == acmacs::chart::map_resolution_test_data::relax_from_full_table::yes) {
        auto projection = chart.projections_modify()->new_by_cloning(*master_chart.projections_modify()->at(0), true);
        projection->set_forced_column_bases(master_chart.column_bases(parameters.minimum_column_basis));
        projection->comment("relaxed-from-full-table-best");
        projection->relax(acmacs::chart::optimization_options{parameters.optimization_precision});
    }
    relax(chart, number_of_dimensions, parameters);
    chart.projections_modify()->sort();

    std::cout << "replicate:" << replicate_no << " dim:" << number_of_dimensions << " prop:" << proportion_to_dont_care << '\n'
              << chart.make_info(*parameters.number_of_optimizations + 10) << "\n\n";

    const auto replicate_stat = collect_errors(master_chart, chart, parameters);
    // collect statistics

} // relax_with_proportion_dontcared

// ----------------------------------------------------------------------

acmacs::chart::map_resolution_test_data::ReplicateStat collect_errors(acmacs::chart::ChartModify& master_chart, acmacs::chart::ChartModify& prediction_chart, const acmacs::chart::map_resolution_test_data::Parameters& parameters)
{
    const auto number_of_antigens = master_chart.number_of_antigens();
    auto prediction_projection = prediction_chart.projection(0);
    auto prediction_layout = prediction_projection->layout();
    auto prediction_chart_titers = prediction_chart.titers();
    auto master_column_bases = master_chart.column_bases(parameters.minimum_column_basis);

    acmacs::chart::map_resolution_test_data::ReplicateStat replicate_stat;

    for (const auto& titer_ref : *prediction_chart_titers) {
        if (const auto master_titer = master_chart.titers()->titer(titer_ref.antigen, titer_ref.serum); master_titer.is_regular()) {
            const auto predicted_distance = prediction_layout->distance(titer_ref.antigen, titer_ref.serum + number_of_antigens);
            const auto master_distance = master_column_bases->column_basis(titer_ref.serum) - master_titer.logged();
            replicate_stat.prediction_errors_for_titers.emplace_back(titer_ref.antigen, titer_ref.serum, master_distance - predicted_distance);
            replicate_stat.master_distances.push_back(master_distance);
            replicate_stat.predicted_distances.push_back(predicted_distance);
        }
    }

    return replicate_stat;

} // collect_errors

// ----------------------------------------------------------------------


/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
