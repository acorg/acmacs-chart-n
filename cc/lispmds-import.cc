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

constexpr const double DS_SCALE{3.0};
constexpr const double NS_SCALE{0.5};

static std::vector<double> native_column_bases(const acmacs::lispmds::value& aData);
static std::vector<double> column_bases(const acmacs::lispmds::value& aData, size_t aProjectionNo);
static std::pair<std::shared_ptr<acmacs::chart::ForcedColumnBases>, acmacs::chart::MinimumColumnBasis> forced_column_bases(const acmacs::lispmds::value& aData, size_t aProjectionNo);

// ----------------------------------------------------------------------

static inline size_t number_of_antigens(const acmacs::lispmds::value& aData)
{
    return aData[0][1].size();

} // number_of_antigens

static inline size_t number_of_sera(const acmacs::lispmds::value& aData)
{
    return aData[0][2].size();

} // number_of_sera

// ----------------------------------------------------------------------

std::vector<double> native_column_bases(const acmacs::lispmds::value& aData)
{
    std::vector<double> cb(number_of_sera(aData), 0);
    for (const auto& row: std::get<acmacs::lispmds::list>(aData[0][3])) {
        for (auto [sr_no, titer_v]: acmacs::enumerate(std::get<acmacs::lispmds::list>(row))) {
            try {
                const double titer = std::get<acmacs::lispmds::number>(titer_v);
                if (titer > cb[sr_no])
                    cb[sr_no] = titer;
            }
            catch (std::bad_variant_access&) {
                if (const std::string titer_s = titer_v; !titer_s.empty()) {
                    double titer;
                    switch (titer_s[0]) {
                      case '<':
                          titer = std::stod(titer_s.substr(1));
                          if (titer > cb[sr_no])
                              cb[sr_no] = titer;
                          break;
                      case '>':
                          titer = std::stod(titer_s.substr(1)) + 1;
                          if (titer > cb[sr_no])
                              cb[sr_no] = titer;
                          break;
                      default:
                          break;
                    }
                }
            }
        }
    }
    return cb;

} // native_column_bases

// ----------------------------------------------------------------------

std::vector<double> column_bases(const acmacs::lispmds::value& aData, size_t aProjectionNo)
{
    const auto num_antigens = number_of_antigens(aData);
    const auto num_sera = number_of_sera(aData);
    const auto number_of_points = num_antigens + num_sera;
    const acmacs::lispmds::list& cb = aProjectionNo == 0
            ? aData[":STARTING-COORDSS"][number_of_points][0][1]
            : aData[":BATCH-RUNS"][aProjectionNo - 1][0][number_of_points][0][1];
    std::vector<double> result(num_sera);
    using diff_t = decltype(cb.end() - cb.begin());
    std::transform(cb.begin() + static_cast<diff_t>(num_antigens), cb.begin() + static_cast<diff_t>(number_of_points), result.begin(), [](const auto& val) -> double { return std::get<acmacs::lispmds::number>(val); });
    return result;

} // column_bases

// ----------------------------------------------------------------------

std::pair<std::shared_ptr<acmacs::chart::ForcedColumnBases>, acmacs::chart::MinimumColumnBasis> forced_column_bases(const acmacs::lispmds::value& aData, size_t aProjectionNo)
{
    try {
        const auto native_cb = native_column_bases(aData);
        const auto cb = column_bases(aData, aProjectionNo);
        if (native_cb == cb) {
            return {std::make_shared<LispmdsNoColumnBases>(), acmacs::chart::MinimumColumnBasis()};
        }
        else {
            const double min_forced = *std::min_element(cb.begin(), cb.end());
            std::decay_t<decltype(native_cb)> native_upgraded(native_cb.size());
            std::transform(native_cb.begin(), native_cb.end(), native_upgraded.begin(), [min_forced](double b) -> double { return std::max(b, min_forced); });
            // std::cerr << "INFO: native: " << native_cb << '\n';
            // std::cerr << "INFO: forced: " << cb << '\n';
            // std::cerr << "INFO: upgrad: " << native_upgraded << '\n';
            if (native_upgraded == cb)
                return {std::make_shared<LispmdsNoColumnBases>(), acmacs::chart::MinimumColumnBasis(min_forced)};
            else
                return {std::make_shared<LispmdsForcedColumnBases>(cb), acmacs::chart::MinimumColumnBasis()};
        }
    }
    catch (acmacs::lispmds::keyword_no_found&) {
        return {std::make_shared<LispmdsNoColumnBases>(), acmacs::chart::MinimumColumnBasis()};
    }

} // forced_column_bases

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
    return ::forced_column_bases(mData, 0).first;

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

size_t LispmdsChart::number_of_antigens() const
{
    return ::number_of_antigens(mData);

} // LispmdsChart::number_of_antigens

// ----------------------------------------------------------------------

size_t LispmdsChart::number_of_sera() const
{
    return ::number_of_sera(mData);

} // LispmdsChart::number_of_sera

// ----------------------------------------------------------------------

std::string LispmdsInfo::name(Compute) const
{
    if (mData[0].size() >= 5)
        return std::get<acmacs::lispmds::symbol>(mData[0][4]);
    else
        return {};

} // LispmdsInfo::name

// ----------------------------------------------------------------------

Name LispmdsAntigen::name() const
{
    return static_cast<std::string>(std::get<acmacs::lispmds::symbol>(mData[0][1][mIndex]));

} // LispmdsAntigen::name

Name LispmdsSerum::name() const
{
    return static_cast<std::string>(std::get<acmacs::lispmds::symbol>(mData[0][2][mIndex]));

} // LispmdsSerum::name

// ----------------------------------------------------------------------

bool LispmdsAntigen::reference() const
{
    try {
        const auto& val = mData[":REFERENCE-ANTIGENS"];
        if (val.empty())
            return false;
        const auto& name = std::get<acmacs::lispmds::symbol>(mData[0][1][mIndex]);
        const auto& val_l = std::get<acmacs::lispmds::list>(val);
        return std::find_if(val_l.begin(), val_l.end(), [&name](const auto& ev) -> bool { return std::get<acmacs::lispmds::symbol>(ev) == name; }) != val_l.end();
    }
    catch (acmacs::lispmds::keyword_no_found&) {
        return false;
    }

} // LispmdsAntigen::reference

// ----------------------------------------------------------------------

size_t LispmdsAntigens::size() const
{
    return number_of_antigens(mData);

} // LispmdsAntigens::size

// ----------------------------------------------------------------------

std::shared_ptr<Antigen> LispmdsAntigens::operator[](size_t aIndex) const
{
    return std::make_shared<LispmdsAntigen>(mData, aIndex);

} // LispmdsAntigens::operator[]

// ----------------------------------------------------------------------

size_t LispmdsSera::size() const
{
    return number_of_sera(mData);

} // LispmdsSera::size

// ----------------------------------------------------------------------

std::shared_ptr<Serum> LispmdsSera::operator[](size_t aIndex) const
{
    return std::make_shared<LispmdsSerum>(mData, aIndex);

} // LispmdsSera::operator[]

// ----------------------------------------------------------------------

Titer LispmdsTiters::titer(size_t aAntigenNo, size_t aSerumNo) const
{
    const auto& titer_v = mData[0][3][aAntigenNo][aSerumNo];
    std::string prefix;
    double titer = 0;
    try {
        titer = std::get<acmacs::lispmds::number>(titer_v);
    }
    catch (std::bad_variant_access&) {
        const std::string sym = std::get<acmacs::lispmds::symbol>(titer_v);
        prefix.append(1, sym[0]);
        if (prefix == "*")
            return prefix;
        titer = std::stod(sym.substr(1));
    }
    return prefix + acmacs::to_string(std::lround(std::exp2(titer) * 10));

} // LispmdsTiters::titer

// ----------------------------------------------------------------------

size_t LispmdsTiters::number_of_antigens() const
{
    return mData[0][3].size();

} // LispmdsTiters::number_of_antigens

// ----------------------------------------------------------------------

size_t LispmdsTiters::number_of_sera() const
{
    return mData[0][3][0].size();

} // LispmdsTiters::number_of_sera

// ----------------------------------------------------------------------

size_t LispmdsTiters::number_of_non_dont_cares() const
{
    size_t result = 0;
    for (const auto& row: std::get<acmacs::lispmds::list>(mData[0][3])) {
        for (const auto& titer: std::get<acmacs::lispmds::list>(row)) {
            try {
                if (static_cast<std::string>(std::get<acmacs::lispmds::symbol>(titer))[0] != '*')
                    ++result;
            }
            catch (std::bad_variant_access&) {
                ++result;
            }
        }
    }
    return result;

} // LispmdsTiters::number_of_non_dont_cares

// ----------------------------------------------------------------------

// double LispmdsForcedColumnBases::column_basis(size_t aSerumNo) const
// {
//     return std::get<acmacs::lispmds::number>(mData[mNumberOfAntigens + aSerumNo]);

// } // LispmdsForcedColumnBases::column_basis

// ----------------------------------------------------------------------

inline const acmacs::lispmds::value& LispmdsProjection::data() const
{
    return mIndex == 0 ? mData[":STARTING-COORDSS"] : mData[":BATCH-RUNS"][mIndex - 1];

} // LispmdsProjection::data

// ----------------------------------------------------------------------

inline const acmacs::lispmds::value& LispmdsProjection::layout() const
{
    return mIndex == 0 ? data() : data()[0];

} // LispmdsProjection::layout

// ----------------------------------------------------------------------

double LispmdsProjection::stress() const
{
    if (mIndex == 0)
        return 0;
    return std::get<acmacs::lispmds::number>(data()[1]);

} // LispmdsProjection::stress

// ----------------------------------------------------------------------

size_t LispmdsProjection::number_of_points() const
{
    return mNumberOfAntigens + mNumberOfSera;

} // LispmdsProjection::number_of_points

// ----------------------------------------------------------------------

size_t LispmdsProjection::number_of_dimensions() const
{
    return layout()[0].size();

} // LispmdsProjection::number_of_dimensions

// ----------------------------------------------------------------------

double LispmdsProjection::coordinate(size_t aPointNo, size_t aDimensionNo) const
{
    return std::get<acmacs::lispmds::number>(layout()[aPointNo][aDimensionNo]);

} // LispmdsProjection::coordinate

// ----------------------------------------------------------------------

std::shared_ptr<ForcedColumnBases> LispmdsProjection::forced_column_bases() const
{
    return ::forced_column_bases(mData, mIndex).first;

} // LispmdsProjection::forced_column_bases

// ----------------------------------------------------------------------

acmacs::chart::MinimumColumnBasis LispmdsProjection::minimum_column_basis() const
{
    return ::forced_column_bases(mData, mIndex).second;

} // LispmdsProjection::minimum_column_basis

// ----------------------------------------------------------------------

acmacs::Transformation LispmdsProjection::transformation() const
{
    acmacs::Transformation result;
    try {
        if (const auto& coord_tr = mData[":CANVAS-COORD-TRANSFORMATIONS"]; !coord_tr.empty()) {
            try {
                if (const auto& v0 = coord_tr[":CANVAS-BASIS-VECTOR-0"]; !v0.empty()) {
                    result.a = std::get<acmacs::lispmds::number>(v0[0]);
                    result.c = std::get<acmacs::lispmds::number>(v0[1]);
                }
            }
            catch (std::exception&) {
            }
            try {
                if (const auto& v1 = coord_tr[":CANVAS-BASIS-VECTOR-1"]; !v1.empty()) {
                    result.b = std::get<acmacs::lispmds::number>(v1[0]);
                    result.d = std::get<acmacs::lispmds::number>(v1[1]);
                }
            }
            catch (std::exception&) {
            }
            try {
                if (static_cast<double>(std::get<acmacs::lispmds::number>(coord_tr[":CANVAS-X-COORD-SCALE"])) < 0) {
                    result.a = - result.a;
                    result.b = - result.b;
                }
            }
            catch (std::exception&) {
            }
            try {
                if (static_cast<double>(std::get<acmacs::lispmds::number>(coord_tr[":CANVAS-Y-COORD-SCALE"])) < 0) {
                    result.c = - result.c;
                    result.d = - result.d;
                }
            }
            catch (std::exception&) {
            }
        }
    }
    catch (acmacs::lispmds::keyword_no_found&) {
    }
    return result;

} // LispmdsProjection::transformation

// ----------------------------------------------------------------------

PointIndexList LispmdsProjection::unmovable() const
{
      //   :UNMOVEABLE-COORDS '(87 86 85 83 82 80 81)
    try {
        const acmacs::lispmds::list& val = mData[":UNMOVEABLE-COORDS"];
        return {val.begin(), val.end(), [](const auto& v) -> size_t { return std::get<acmacs::lispmds::number>(v); }};
    }
    catch (std::exception&) {
        return {};
    }

} // LispmdsProjection::unmovable

// ----------------------------------------------------------------------

PointIndexList LispmdsProjection::disconnected() const
{
      // std::cerr << "WARNING: LispmdsProjection::disconnected not implemented\n";
    return {};

} // LispmdsProjection::disconnected

// ----------------------------------------------------------------------

bool LispmdsProjections::empty() const
{
    try {
        const auto& val = mData[":STARTING-COORDSS"];
        return val.empty();
    }
    catch (acmacs::lispmds::keyword_no_found&) {
        return true;
    }

} // LispmdsProjections::empty

// ----------------------------------------------------------------------

size_t LispmdsProjections::size() const
{
    size_t result = 0;
    try {
        const auto& starting_coordss = mData[":STARTING-COORDSS"];
        if (!starting_coordss.empty())
            ++result;
        const auto& batch_runs = mData[":BATCH-RUNS"];
        result += batch_runs.size();
    }
    catch (acmacs::lispmds::keyword_no_found&) {
    }
    return result;

} // LispmdsProjections::size

// ----------------------------------------------------------------------

std::shared_ptr<Projection> LispmdsProjections::operator[](size_t aIndex) const
{
    return std::make_shared<LispmdsProjection>(mData, aIndex, number_of_antigens(mData), number_of_sera(mData));

} // LispmdsProjections::operator[]

// ----------------------------------------------------------------------

bool LispmdsPlotSpec::empty() const
{
    const auto& plot_spec = mData[":PLOT-SPEC"];
    return plot_spec.empty();

} // LispmdsPlotSpec::empty

// ----------------------------------------------------------------------

DrawingOrder LispmdsPlotSpec::drawing_order() const
{
      // :RAISE-POINTS 'NIL
      // :LOWER-POINTS 'NIL
      // don't know how drawing order is stored
    return {};

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
    extract_style(result, aPointNo);
    return result;

} // LispmdsPlotSpec::style

// ----------------------------------------------------------------------

std::vector<acmacs::PointStyle> LispmdsPlotSpec::all_styles() const
{
    try {
        const auto number_of_points = mData[0][1].size() + mData[0][2].size();
        std::vector<acmacs::PointStyle> result(number_of_points);
        for (size_t point_no = 0; point_no < number_of_points; ++point_no) {
            extract_style(result[point_no], point_no);
        }
        return result;
    }
    catch (std::exception& err) {
        std::cerr << "WARNING: [lispmds]: cannot get point styles: " << err.what() << '\n';
    }
    return {};

} // LispmdsPlotSpec::all_styles

// ----------------------------------------------------------------------

void LispmdsPlotSpec::extract_style(acmacs::PointStyle& aTarget, size_t aPointNo) const
{
    std::string name = aPointNo < number_of_antigens(mData)
                                  ? static_cast<std::string>(std::get<acmacs::lispmds::symbol>(mData[0][1][aPointNo])) + "-AG"
                                  : static_cast<std::string>(std::get<acmacs::lispmds::symbol>(mData[0][2][aPointNo - number_of_antigens(mData)])) + "-SR";
    const acmacs::lispmds::list& plot_spec = mData[":PLOT-SPEC"];
    for (const acmacs::lispmds::list& pstyle: plot_spec) {
        if (std::get<acmacs::lispmds::symbol>(pstyle[0]) == name) {
            extract_style(aTarget, pstyle);
            break;
        }
    }

} // LispmdsPlotSpec::extract_style

// ----------------------------------------------------------------------

void LispmdsPlotSpec::extract_style(acmacs::PointStyle& aTarget, const acmacs::lispmds::list& aSource) const
{
    try {
        aTarget.size = static_cast<double>(std::get<acmacs::lispmds::number>(aSource[":DS"])) / DS_SCALE;
          // if antigen also divide size by 2 ?
    }
    catch (std::exception&) {
    }

    try {
        aTarget.label_text = aSource[":WN"];
        aTarget.label.shown = !aTarget.label_text->empty();
    }
    catch (std::exception&) {
    }

    try {
        aTarget.shape = static_cast<std::string>(aSource[":SH"]);
    }
    catch (std::exception&) {
    }

    try {
        aTarget.label.size = static_cast<double>(std::get<acmacs::lispmds::number>(aSource[":NS"])) / NS_SCALE;
    }
    catch (std::exception&) {
    }

    try {
        if (const std::string label_color = aSource[":NC"]; label_color != "{}")
            aTarget.label.color = label_color;
    }
    catch (std::exception&) {
    }

    try {
        if (const std::string fill_color = aSource[":CO"]; fill_color != "{}")
            aTarget.fill = fill_color;
        else
            aTarget.fill = TRANSPARENT;
    }
    catch (std::exception&) {
    }

    try {
        if (const std::string outline_color = aSource[":CO"]; outline_color != "{}")
            aTarget.outline = outline_color;
        else
            aTarget.outline = BLACK;
    }
    catch (std::exception&) {
    }

    try {
        Color fill = aTarget.fill;
        fill.set_transparency(std::get<acmacs::lispmds::number>(aSource[":TR"]));
        aTarget.fill = fill;
    }
    catch (std::exception&) {
    }

} // LispmdsPlotSpec::extract_style

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
