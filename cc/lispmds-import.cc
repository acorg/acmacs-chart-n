#include <set>
#include <vector>
#include <limits>
#include <regex>

#include "acmacs-base/stream.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart/lispmds-import.hh"

using namespace std::string_literals;
using namespace acmacs::chart;

// ----------------------------------------------------------------------

std::shared_ptr<Chart> acmacs::chart::lispmds_import(const std::string_view& aData, Verify aVerify)
{
    try {
        auto chart = std::make_shared<LispmdsChart>(acmacs::lispmds::parse_string(aData));
        chart->verify_data(aVerify);
        return chart;
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        throw;
    }

} // acmacs::chart::lispmds_import

// ----------------------------------------------------------------------

void LispmdsChart::verify_data(Verify) const
{
    try {
    }
    catch (std::exception& err) {
        throw import_error("[lispmds]: structure verification failed: "s + err.what());
    }

} // LispmdsChart::verify_data

// ----------------------------------------------------------------------

std::shared_ptr<Info> LispmdsChart::info() const
{
    return std::make_shared<LispmdsInfo>(mData);

} // LispmdsChart::info

// ----------------------------------------------------------------------

std::shared_ptr<Antigens> LispmdsChart::antigens() const
{
    return std::make_shared<LispmdsAntigens>(mData);

} // LispmdsChart::antigens

// ----------------------------------------------------------------------

std::shared_ptr<Sera> LispmdsChart::sera() const
{
    return std::make_shared<LispmdsSera>(mData);

} // LispmdsChart::sera

// ----------------------------------------------------------------------

std::shared_ptr<Titers> LispmdsChart::titers() const
{
    return std::make_shared<LispmdsTiters>(mData);

} // LispmdsChart::titers

// ----------------------------------------------------------------------

std::shared_ptr<ForcedColumnBases> LispmdsChart::forced_column_bases() const
{
    return std::make_shared<LispmdsForcedColumnBases>(mData);

} // LispmdsChart::forced_column_bases

// ----------------------------------------------------------------------

std::shared_ptr<Projections> LispmdsChart::projections() const
{
    return std::make_shared<LispmdsProjections>(mData);

} // LispmdsChart::projections

// ----------------------------------------------------------------------

std::shared_ptr<PlotSpec> LispmdsChart::plot_spec() const
{
    return std::make_shared<LispmdsPlotSpec>(mData);

} // LispmdsChart::plot_spec

// ----------------------------------------------------------------------

std::string LispmdsInfo::name(Compute) const
{
    return "?LispmdsInfo::name";

} // LispmdsInfo::name

// ----------------------------------------------------------------------

Name LispmdsAntigen::name() const
{
    return "?LispmdsAntigen::name";

} // LispmdsAntigen::name

Name LispmdsSerum::name() const
{
    return "?LispmdsSerum::name";

} // LispmdsSerum::name

// ----------------------------------------------------------------------

bool LispmdsAntigen::reference() const
{
      //return false;

} // LispmdsAntigen::reference

// ----------------------------------------------------------------------

size_t LispmdsAntigens::size() const
{
    return mData[0][1].size();

} // LispmdsAntigens::size

// ----------------------------------------------------------------------

std::shared_ptr<Antigen> LispmdsAntigens::operator[](size_t aIndex) const
{

} // LispmdsAntigens::operator[]

// ----------------------------------------------------------------------

size_t LispmdsSera::size() const
{
    return mData[0][2].size();

} // LispmdsSera::size

// ----------------------------------------------------------------------

std::shared_ptr<Serum> LispmdsSera::operator[](size_t aIndex) const
{

} // LispmdsSera::operator[]

// ----------------------------------------------------------------------

Titer LispmdsTiters::titer(size_t aAntigenNo, size_t aSerumNo) const
{

} // LispmdsTiters::titer

// ----------------------------------------------------------------------

size_t LispmdsTiters::number_of_antigens() const
{

} // LispmdsTiters::number_of_antigens

// ----------------------------------------------------------------------

size_t LispmdsTiters::number_of_sera() const
{

} // LispmdsTiters::number_of_sera

// ----------------------------------------------------------------------

size_t LispmdsTiters::number_of_non_dont_cares() const
{

} // LispmdsTiters::number_of_non_dont_cares

// ----------------------------------------------------------------------

double LispmdsForcedColumnBases::column_basis(size_t aSerumNo) const
{

} // LispmdsForcedColumnBases::column_basis

// ----------------------------------------------------------------------

size_t LispmdsForcedColumnBases::size() const
{

} // LispmdsForcedColumnBases::size

// ----------------------------------------------------------------------

double LispmdsProjection::stress() const
{

} // LispmdsProjection::stress

// ----------------------------------------------------------------------

size_t LispmdsProjection::number_of_points() const
{

} // LispmdsProjection::number_of_points

// ----------------------------------------------------------------------

size_t LispmdsProjection::number_of_dimensions() const
{

} // LispmdsProjection::number_of_dimensions

// ----------------------------------------------------------------------

double LispmdsProjection::coordinate(size_t aPointNo, size_t aDimensionNo) const
{

} // LispmdsProjection::coordinate

// ----------------------------------------------------------------------

std::shared_ptr<ForcedColumnBases> LispmdsProjection::forced_column_bases() const
{

} // LispmdsProjection::forced_column_bases

// ----------------------------------------------------------------------

acmacs::Transformation LispmdsProjection::transformation() const
{
    acmacs::Transformation result;
    return result;

} // LispmdsProjection::transformation

// ----------------------------------------------------------------------

PointIndexList LispmdsProjection::unmovable() const
{
    return {};

} // LispmdsProjection::unmovable

// ----------------------------------------------------------------------

PointIndexList LispmdsProjection::disconnected() const
{
    return {};

} // LispmdsProjection::disconnected

// ----------------------------------------------------------------------

bool LispmdsProjections::empty() const
{

} // LispmdsProjections::empty

// ----------------------------------------------------------------------

size_t LispmdsProjections::size() const
{

} // LispmdsProjections::size

// ----------------------------------------------------------------------

std::shared_ptr<Projection> LispmdsProjections::operator[](size_t aIndex) const
{

} // LispmdsProjections::operator[]

// ----------------------------------------------------------------------

bool LispmdsPlotSpec::empty() const
{

} // LispmdsPlotSpec::empty

// ----------------------------------------------------------------------

DrawingOrder LispmdsPlotSpec::drawing_order() const
{
    DrawingOrder result;
    return result;

} // LispmdsPlotSpec::drawing_order

// ----------------------------------------------------------------------

Color LispmdsPlotSpec::error_line_positive_color() const
{
    return "red";

} // LispmdsPlotSpec::error_line_positive_color

// ----------------------------------------------------------------------

Color LispmdsPlotSpec::error_line_negative_color() const
{
    return "blue";

} // LispmdsPlotSpec::error_line_negative_color

// ----------------------------------------------------------------------

acmacs::PointStyle LispmdsPlotSpec::style(size_t aPointNo) const
{
    acmacs::PointStyle result;
    return result;

} // LispmdsPlotSpec::style

// ----------------------------------------------------------------------

std::vector<acmacs::PointStyle> LispmdsPlotSpec::all_styles() const
{
    return {};

} // LispmdsPlotSpec::all_styles

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End: