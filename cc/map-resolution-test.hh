#pragma once

#include <vector>
#include <iostream>

#include "acmacs-base/statistics.hh"
#include "acmacs-chart-2/optimize-options.hh"
#include "acmacs-chart-2/column-bases.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    class Layout;

    namespace chart
    {
        class ChartModify;

        namespace map_resolution_test_data
        {
            //  additional projection in each replicate, first full table is
            //  relaxed, then titers dont-cared and the best projection
            //  relaxed again from already found starting coordinates.
            enum class relax_from_full_table { no, yes };

            // converting titers to dont-care may change column bases, force
            // master chart column bases
            enum class column_bases_from_master { no, yes };

            struct Parameters
            {
                std::vector<number_of_dimensions_t> number_of_dimensions{number_of_dimensions_t{1}, number_of_dimensions_t{2}, number_of_dimensions_t{3}, number_of_dimensions_t{4},
                                                                         number_of_dimensions_t{5}};
                number_of_optimizations_t number_of_optimizations{100};
                size_t number_of_random_replicates_for_each_proportion{25};
                std::vector<double> proportions_to_dont_care{0.1, 0.2, 0.3};
                acmacs::chart::MinimumColumnBasis minimum_column_basis{};
                enum column_bases_from_master column_bases_from_master { column_bases_from_master::yes };
                enum optimization_precision optimization_precision { optimization_precision::rough };
                enum relax_from_full_table relax_from_full_table { relax_from_full_table::no };
            };

            struct PredictionErrorForTiter
            {
                PredictionErrorForTiter(size_t ag, size_t sr, double er) : antigen{ag}, serum{sr}, error{er} {}
                size_t antigen;
                size_t serum;
                double error;
            };

            struct Predictions
            {
                double av_abs_error;
                double sd_error;
                double correlation;
                statistics::SimpleLinearRegression linear_regression;
                size_t number_of_samples;
            };

            std::ostream& operator << (std::ostream& out, const Predictions& predictions);

            struct PredictionsSummary
            {
                statistics::StandardDeviation av_abs_error;
                statistics::StandardDeviation sd_error;
                statistics::StandardDeviation correlations;
                statistics::StandardDeviation r2;
                size_t number_of_samples;
            };

            std::ostream& operator << (std::ostream& out, const PredictionsSummary& predictions_summary);

            struct ReplicateStat
            {
                std::vector<double> master_distances;
                std::vector<double> predicted_distances;
                std::vector<PredictionErrorForTiter> prediction_errors_for_titers;
            };

            class Results
            {
            };

        } // namespace map_resolution_test_data

        map_resolution_test_data::Results map_resolution_test(ChartModify& chart, const map_resolution_test_data::Parameters& parameters);


    } // namespace chart

} // namespace acmacs

/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
