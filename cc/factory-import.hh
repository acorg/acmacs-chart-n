#pragma once

#include <string>
#include <string_view>
#include <memory>

#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/verify.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;
    using ChartP = std::shared_ptr<Chart>;

    ChartP import_from_file(std::string aFilename, Verify aVerify = Verify::None, report_time aReport = report_time::no);
    inline ChartP import_from_file(std::string_view aFilename, Verify aVerify = Verify::None, report_time aReport = report_time::no) { return import_from_file(std::string(aFilename), aVerify, aReport); }
    inline ChartP import_from_file(const char* aFilename, Verify aVerify = Verify::None, report_time aReport = report_time::no) { return import_from_file(std::string(aFilename), aVerify, aReport); }
    ChartP import_from_data(std::string aData, Verify aVerify, report_time aReport);
    ChartP import_from_data(std::string_view aData, Verify aVerify, report_time aReport);
    ChartP import_from_decompressed_data(std::string aData, Verify aVerify, report_time aReport);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
