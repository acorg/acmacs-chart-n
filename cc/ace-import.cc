#include <set>
#include <vector>
#include <limits>

#include "acmacs-base/stream.hh"
#include "acmacs-base/string.hh"
#include "acmacs-chart/ace-import.hh"

using namespace std::string_literals;
using namespace acmacs::chart;

// ----------------------------------------------------------------------

std::shared_ptr<Chart> acmacs::chart::ace_import(const std::string_view& aData, Verify aVerify)
{
    auto chart = std::make_shared<AceChart>(rjson::parse_string(aData));
    chart->verify_data(aVerify);
    return chart;

} // acmacs::chart::ace_import

// ----------------------------------------------------------------------

void AceChart::verify_data(Verify aVerify) const
{
    try {
        const auto& antigens = mData["c"].get_or_empty_array("a");
        if (antigens.empty())
            throw import_error("[ace]: no antigens");
        const auto& sera = mData["c"].get_or_empty_array("s");
        if (sera.empty())
            throw import_error("[ace]: no sera");
        const auto& titers = mData["c"].get_or_empty_object("t");
        if (titers.empty())
            throw import_error("[ace]: no titers");
        if (auto [ll_present, ll] = titers.get_array_if("l"); ll_present) {
            if (ll.size() != antigens.size())
                throw import_error("[ace]: number of the titer rows (" + acmacs::to_string(ll.size()) + ") does not correspond to the number of antigens (" + acmacs::to_string(antigens.size()) + ")");
        }
        else if (auto [dd_present, dd] = titers.get_array_if("d"); dd_present) {
            if (dd.size() != antigens.size())
                throw import_error("[ace]: number of the titer rows (" + acmacs::to_string(dd.size()) + ") does not correspond to the number of antigens (" + acmacs::to_string(antigens.size()) + ")");
        }
        else
            throw import_error("[ace]: no titers (neither \"l\" nor \"d\" present)");
        if (aVerify != Verify::None) {
            std::cerr << "WARNING: AceChart::verify_data not implemented\n";
        }
    }
    catch (std::exception& err) {
        throw import_error("[ace]: structure verification failed: "s + err.what());
    }

} // AceChart::verify_data

// ----------------------------------------------------------------------

std::shared_ptr<Info> AceChart::info() const
{
    return std::make_shared<AceInfo>(mData["c"]["i"]);

} // AceChart::info

// ----------------------------------------------------------------------

std::shared_ptr<Antigens> AceChart::antigens() const
{
    return std::make_shared<AceAntigens>(mData["c"].get_or_empty_array("a"));

} // AceChart::antigens

// ----------------------------------------------------------------------

std::shared_ptr<Sera> AceChart::sera() const
{
    return std::make_shared<AceSera>(mData["c"].get_or_empty_array("s"));

} // AceChart::sera

// ----------------------------------------------------------------------

std::shared_ptr<Titers> AceChart::titers() const
{
    return std::make_shared<AceTiters>(mData["c"].get_or_empty_object("t"));

} // AceChart::titers

// ----------------------------------------------------------------------

std::shared_ptr<ForcedColumnBases> AceChart::forced_column_bases() const
{
    return std::make_shared<AceForcedColumnBases>(mData["c"].get_or_empty_array("C"));

} // AceChart::forced_column_bases

// ----------------------------------------------------------------------

std::shared_ptr<Projections> AceChart::projections() const
{
    return std::make_shared<AceProjections>(mData["c"].get_or_empty_array("P"));

} // AceChart::projections

// ----------------------------------------------------------------------

std::shared_ptr<PlotSpec> AceChart::plot_spec() const
{
    return std::make_shared<AcePlotSpec>(mData["c"].get_or_empty_object("p"));

} // AceChart::plot_spec

// ----------------------------------------------------------------------

std::string AceInfo::make_info() const
{
    const auto n_sources = number_of_sources();
    return string::join(" ", {name(),
                    virus(Compute::Yes),
                    lab(Compute::Yes),
                    virus_type(Compute::Yes),
                    subset(Compute::Yes),
                    assay(Compute::Yes),
                    rbc_species(Compute::Yes),
                    date(Compute::Yes),
                    n_sources ? ("(" + std::to_string(n_sources) + " tables)") : std::string{}
                             });

} // AceInfo::make_info

// ----------------------------------------------------------------------

std::string AceInfo::make_field(const char* aField, const char* aSeparator, Compute aCompute) const
{
    std::string result{mData.get_or_default(aField, "")};
    if (result.empty() && aCompute == Compute::Yes) {
        const auto& sources{mData.get_or_empty_array("S")};
        if (!sources.empty()) {
            std::set<std::string> composition;
            std::transform(std::begin(sources), std::end(sources), std::inserter(composition, composition.begin()), [aField](const auto& sinfo) { return sinfo.get_or_default(aField, ""); });
            result = string::join(aSeparator, composition);
        }
    }
    return result;

} // AceInfo::make_field

// ----------------------------------------------------------------------

std::string AceInfo::date(Compute aCompute) const
{
    std::string result{mData.get_or_default("D", "")};
    if (result.empty() && aCompute == Compute::Yes) {
        const auto& sources{mData.get_or_empty_array("S")};
        if (!sources.empty()) {
            std::vector<std::string> composition{sources.size()};
            std::transform(std::begin(sources), std::end(sources), std::begin(composition), [](const auto& sinfo) { return sinfo.get_or_default("D", ""); });
            std::sort(std::begin(composition), std::end(composition));
            result = string::join("-", {composition.front(), composition.back()});
        }
    }
    return result;

} // AceInfo::date

// ----------------------------------------------------------------------

static inline BLineage b_lineage(std::string aLin)
{
    if (!aLin.empty()) {
        switch (aLin[0]) {
          case 'Y':
              return BLineage::Yamagata;
          case 'V':
              return BLineage::Victoria;
        }
    }
    return BLineage::Unknown;
}

BLineage AceAntigen::lineage() const
{
    return b_lineage(mData["L"]);

} // AceAntigen::lineage

BLineage AceSerum::lineage() const
{
    return b_lineage(mData["L"]);

} // AceSerum::lineage

// ----------------------------------------------------------------------

Titer AceTiters::titer(size_t aAntigenNo, size_t aSerumNo) const
{
    if (auto [present, list] = mData.get_array_if("l"); present) {
        return list[aAntigenNo][aSerumNo];
    }
    else {
        return titer_in_d(mData["d"], aAntigenNo, aSerumNo);
    }

} // AceTiters::titer

// ----------------------------------------------------------------------

Titer AceTiters::titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const
{
    return titer_in_d(mData["L"][aLayerNo], aAntigenNo, aSerumNo);

} // AceTiters::titer_of_layer

// ----------------------------------------------------------------------

size_t AceTiters::number_of_antigens() const
{
    if (auto [present, list] = mData.get_array_if("l"); present) {
        return list.size();
    }
    else {
        return static_cast<const rjson::array&>(mData["d"]).size();
    }

} // AceTiters::number_of_antigens

// ----------------------------------------------------------------------

size_t AceTiters::number_of_sera() const
{
    if (auto [present, list] = mData.get_array_if("l"); present) {
        return static_cast<const rjson::array&>(list[0]).size();
    }
    else {
        const rjson::array& d = mData["d"];
        auto max_index = [](const rjson::object& obj) -> size_t {
                             size_t result = 0;
                             for (auto [key, value]: obj) {
                                 if (const size_t ind = std::stoul(key); ind > result)
                                     result = ind;
                             }
                             return result;
                         };
        return max_index(*std::max_element(d.begin(), d.end(), [max_index](const rjson::object& a, const rjson::object& b) { return max_index(a) < max_index(b); })) + 1;
    }

} // AceTiters::number_of_sera

// ----------------------------------------------------------------------

size_t AceTiters::number_of_non_dont_cares() const
{
    size_t result = 0;
    if (auto [present, list] = mData.get_array_if("l"); present) {
        for (const rjson::array& row: list) {
            for (const Titer titer: row) {
                if (!titer.is_dont_care())
                    ++result;
            }
        }
    }
    else {
        const rjson::array& d = mData["d"];
        result = std::accumulate(d.begin(), d.end(), 0U, [](size_t a, const rjson::object& row) -> size_t { return a + row.size(); });
    }
    return result;

} // AceTiters::number_of_non_dont_cares

// ----------------------------------------------------------------------

size_t AceProjection::number_of_dimensions() const
{
    try {
        for (const rjson::array& row: static_cast<const rjson::array&>(mData["l"])) {
            if (!row.empty())
                return row.size();
        }
    }
    catch (rjson::field_not_found&) {
    }
    catch (std::bad_variant_access&) {
    }
    return 0;

} // AceProjection::number_of_dimensions

// ----------------------------------------------------------------------

double AceProjection::coordinate(size_t aPointNo, size_t aDimensionNo) const
{
    const auto& layout = mData.get_or_empty_array("l");
    const auto& point = layout[aPointNo];
    try {
        return point[aDimensionNo];
    }
    catch (std::exception&) {
        return std::numeric_limits<double>::quiet_NaN();
    }

} // AceProjection::coordinate

// ----------------------------------------------------------------------

std::shared_ptr<ForcedColumnBases> AceProjection::forced_column_bases() const
{
    return std::make_shared<AceForcedColumnBases>(mData.get_or_empty_array("C"));

} // AceProjection::forced_column_bases

// ----------------------------------------------------------------------

acmacs::Transformation AceProjection::transformation() const
{
    acmacs::Transformation result;
    if (auto [present, array] = mData.get_array_if("t"); present) {
        result.set(array[0], array[1], array[2], array[3]);
    }
    return result;

} // AceProjection::transformation

// ----------------------------------------------------------------------

Color AcePlotSpec::error_line_positive_color() const
{
    try {
        return static_cast<std::string>(mData["E"]["c"]);
    }
    catch (rjson::field_not_found&) {
        return "red";
    }

} // AcePlotSpec::error_line_positive_color

// ----------------------------------------------------------------------

Color AcePlotSpec::error_line_negative_color() const
{
    try {
        return static_cast<std::string>(mData["e"]["c"]);
    }
    catch (rjson::field_not_found&) {
        return "blue";
    }

} // AcePlotSpec::error_line_negative_color

// ----------------------------------------------------------------------

acmacs::PointStyle AcePlotSpec::style(size_t aPointNo) const
{
    acmacs::PointStyle result;
    try {
        const rjson::array& indices = mData["p"];
        const size_t style_no = indices[aPointNo];
        const rjson::object& style = mData["P"][style_no];
        for (auto [field_name_v, field_value]: style) {
            const std::string field_name(field_name_v);
            if (!field_name.empty()) {
                try {
                    switch (field_name[0]) {
                      case '+':
                          result.shown = field_value;
                          break;
                      case 'F':
                          result.fill = Color(field_value);
                          break;
                      case 'O':
                          result.outline = Color(field_value);
                          break;
                      case 'o':
                          result.outline_width = Pixels{field_value};
                          break;
                      case 's':
                          result.size = field_value;
                          break;
                      case 'r':
                          result.rotation = Rotation{field_value};
                          break;
                      case 'a':
                          result.aspect = Aspect{field_value};
                          break;
                      case 'S':
                          result.shape = static_cast<std::string>(field_value);
                          break;
                      case 'l':
                          label_style(result, field_value);
                          break;
                    }
                }
                catch (std::exception& err) {
                    std::cerr << "WARNING: [ace]: point " << aPointNo << " style " << style_no << " field \"" << field_name << "\" value is wrong: " << err.what() << " value: " << field_value.to_json() << '\n';
                }
            }
        }
    }
    catch (std::exception& err) {
        std::cerr << "WARNING: [ace]: cannot get style for point " << aPointNo << ": " << err.what() << '\n';
    }
    return result;

} // AcePlotSpec::style

// ----------------------------------------------------------------------

void AcePlotSpec::label_style(acmacs::PointStyle& aStyle, const rjson::object& aData) const
{
    auto& label_style = aStyle.label;
    for (auto [field_name_v, field_value]: aData) {
        const std::string field_name(field_name_v);
        if (!field_name.empty()) {
            try {
                switch (field_name[0]) {
                  case '+':
                      label_style.shown = field_value;
                      break;
                  case 'p':
                      label_style.offset = acmacs::Offset(field_value[0], field_value[1]);
                      break;
                  case 's':
                      label_style.size = field_value;
                      break;
                  case 'c':
                      label_style.color = Color(field_value);
                      break;
                  case 'r':
                      label_style.rotation = Rotation{field_value};
                      break;
                  case 'i':
                      label_style.interline = field_value;
                      break;
                  case 'f':
                      label_style.style.font_family = static_cast<std::string>(field_value);
                      break;
                  case 'S':
                      label_style.style.slant = static_cast<std::string>(field_value);
                      break;
                  case 'W':
                      label_style.style.weight = static_cast<std::string>(field_value);
                      break;
                  case 't':
                      aStyle.label_text = static_cast<std::string>(field_value);
                      break;
                }
            }
            catch (std::exception& err) {
                std::cerr << "WARNING: [ace]: label style field \"" << field_name << "\" value is wrong: " << err.what() << " value: " << field_value.to_json() << '\n';
            }
        }
    }

} // AcePlotSpec::label_style

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
