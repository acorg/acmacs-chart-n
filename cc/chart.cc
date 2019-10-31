#include <regex>
#include <algorithm>

#include "acmacs-base/string.hh"
#include "acmacs-virus/virus-name.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/range.hh"
#include "locationdb/locdb.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/serum-circle.hh"

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"
static const std::regex sDate{"[12][90][0-9][0-9]-[0-2][0-9]-[0-3][0-9]"};
#include "acmacs-base/diagnostics-pop.hh"

void acmacs::chart::Date::check() const
{
    if (!empty() && !std::regex_match(std::begin(this->get()), std::end(this->get()), sDate))
        throw invalid_data{fmt::format("invalid date (YYYY-MM-DD expected): {}", **this)};

} // acmacs::chart::Date::check

// ----------------------------------------------------------------------

std::string acmacs::chart::Chart::make_info(size_t max_number_of_projections_to_show) const
{
    return string::join("\n", {info()->make_info(),
                    "Antigens:" + std::to_string(number_of_antigens()) + " Sera:" + std::to_string(number_of_sera()),
                    projections()->make_info(max_number_of_projections_to_show)
                    });

} // acmacs::chart::Chart::make_info

// ----------------------------------------------------------------------

std::string acmacs::chart::Chart::make_name(std::optional<size_t> aProjectionNo) const
{
    std::string n = info()->make_name();
    if (auto prjs = projections(); !prjs->empty() && (!aProjectionNo || *aProjectionNo < prjs->size())) {
        auto prj = (*prjs)[aProjectionNo ? *aProjectionNo : 0];
        n += " >=" + static_cast<std::string>(prj->minimum_column_basis()) + " " + std::to_string(prj->stress());
    }
    return n;

} // acmacs::chart::Chart::make_name

// ----------------------------------------------------------------------

std::string acmacs::chart::Chart::description() const
{
    auto n = info()->make_name();
    if (auto prjs = projections(); !prjs->empty()) {
        auto prj = (*prjs)[0];
        n += string::concat(" >=", prj->minimum_column_basis(), " ", prj->stress());
    }
    if (info()->virus_type() == acmacs::virus::type_subtype_t{"B"})
        n += string::concat(' ', lineage());
    n += string::concat(" Ag:", number_of_antigens(), " Sr:", number_of_sera());
    if (const auto layers = titers()->number_of_layers(); layers > 1)
        n += string::concat(" (", layers, " source tables)");
    return n;

} // acmacs::chart::Chart::description

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::ColumnBases> acmacs::chart::Chart::computed_column_bases(acmacs::chart::MinimumColumnBasis aMinimumColumnBasis, use_cache a_use_cache) const
{
    if (a_use_cache == use_cache::yes) {
        if (auto found = computed_column_bases_.find(aMinimumColumnBasis); found != computed_column_bases_.end())
            return found->second;
    }
    return computed_column_bases_[aMinimumColumnBasis] = titers()->computed_column_bases(aMinimumColumnBasis);

} // acmacs::chart::Chart::computed_column_bases

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::ColumnBases> acmacs::chart::Chart::column_bases(acmacs::chart::MinimumColumnBasis aMinimumColumnBasis) const
{
    if (auto cb = forced_column_bases(aMinimumColumnBasis); cb)
        return cb;
    return computed_column_bases(aMinimumColumnBasis);

} // acmacs::chart::Chart::column_bases

// ----------------------------------------------------------------------

double acmacs::chart::Chart::column_basis(size_t serum_no, size_t projection_no) const
{
    auto prj = projection(projection_no);
    if (auto forced = prj->forced_column_bases(); forced)
        return forced->column_basis(serum_no);
    else
        return computed_column_bases(prj->minimum_column_basis(), use_cache::yes)->column_basis(serum_no);

} // acmacs::chart::Chart::column_basis

// ----------------------------------------------------------------------

acmacs::virus::lineage_t acmacs::chart::Chart::lineage() const
{
    std::map<BLineage, size_t> lineages;
    auto ags = antigens();
    for (auto antigen: *ags) {
        if (const auto lineage = antigen->lineage(); lineage != BLineage::Unknown)
            ++lineages[lineage];
    }
    switch (lineages.size()) {
      case 0:
          return {};
      case 1:
          return lineages.begin()->first;
      default:
          return std::max_element(lineages.begin(), lineages.end(), [](const auto& a, const auto& b) -> bool { return a.second < b.second; })->first;
    }
    // return {};

} // acmacs::chart::Chart::lineage

// ----------------------------------------------------------------------

void acmacs::chart::Chart::serum_coverage(Titer aHomologousTiter, size_t aSerumNo, Indexes& aWithinFold, Indexes& aOutsideFold, double aFold) const
{
    if (!aHomologousTiter.is_regular())
        throw serum_coverage_error(fmt::format("cannot handle non-regular homologous titer: {}", *aHomologousTiter));
    const double titer_threshold = aHomologousTiter.logged() - aFold;
    if (titer_threshold <= 0)
        throw serum_coverage_error(fmt::format("homologous titer is too low: {}", *aHomologousTiter));
    auto tts = titers();
    for (size_t ag_no = 0; ag_no < number_of_antigens(); ++ag_no) {
        const Titer titer = tts->titer(ag_no, aSerumNo);
        const double value = titer.is_dont_care() ? -1 : titer.logged_for_column_bases();
        if (value >= titer_threshold)
            aWithinFold.insert(ag_no);
        else if (value >= 0 && value < titer_threshold)
            aOutsideFold.insert(ag_no);
    }
    if (aWithinFold->empty())
        throw serum_coverage_error("no antigens within 4fold from homologous titer (for serum coverage)"); // BUG? at least homologous antigen must be there!

} // acmacs::chart::Chart::serum_coverage

// ----------------------------------------------------------------------

void acmacs::chart::Chart::serum_coverage(size_t aAntigenNo, size_t aSerumNo, Indexes& aWithinFold, Indexes& aOutsideFold, double aFold) const
{
    serum_coverage(titers()->titer(aAntigenNo, aSerumNo), aSerumNo, aWithinFold, aOutsideFold, aFold);

} // acmacs::chart::Chart::serum_coverage

// ----------------------------------------------------------------------

void acmacs::chart::Chart::set_homologous(find_homologous options, SeraP aSera, acmacs::debug dbg) const
{
    if (!aSera)
        aSera = sera();
    aSera->set_homologous(options, *antigens(), dbg);

} // acmacs::chart::Chart::set_homologous

// ----------------------------------------------------------------------

acmacs::PointStyle acmacs::chart::Chart::default_style(acmacs::chart::Chart::PointType aPointType) const
{
    acmacs::PointStyle style;
    style.outline = BLACK;
    switch (aPointType) {
      case PointType::TestAntigen:
          style.shape = acmacs::PointShape::Circle;
          style.size = Pixels{5.0};
          style.fill = GREEN;
          break;
      case PointType::ReferenceAntigen:
          style.shape = acmacs::PointShape::Circle;
          style.size = Pixels{8.0};
          style.fill = TRANSPARENT;
          break;
      case PointType::Serum:
          style.shape = acmacs::PointShape::Box;
          style.size = Pixels{6.5};
          style.fill = TRANSPARENT;
          break;
    }
    return style;

} // acmacs::chart::Chart::default_style

// ----------------------------------------------------------------------

std::vector<acmacs::PointStyle> acmacs::chart::Chart::default_all_styles() const
{
    auto ags = antigens();
    auto srs = sera();
    std::vector<acmacs::PointStyle> result(ags->size() + srs->size());
    for (size_t ag_no = 0; ag_no < ags->size(); ++ag_no)
        result[ag_no] = default_style((*ags)[ag_no]->reference() ? PointType::ReferenceAntigen : PointType::TestAntigen);
    for (auto ps = result.begin() + static_cast<typename decltype(result.begin())::difference_type>(ags->size()); ps != result.end(); ++ps)
        *ps = default_style(PointType::Serum);
    return result;

} // acmacs::chart::Chart::default_all_styles

// ----------------------------------------------------------------------

void acmacs::chart::Chart::show_table(std::ostream& output, std::optional<size_t> layer_no) const
{
    auto sr_label = [](size_t sr_no) -> char { return static_cast<char>('A' + sr_no); };

    auto ags = antigens();
    auto srs = sera();
    auto tt = titers();
    PointIndexList antigen_indexes, serum_indexes;
    if (layer_no) {
        std::tie(antigen_indexes, serum_indexes) = tt->antigens_sera_of_layer(*layer_no);
    }
    else {
        antigen_indexes = PointIndexList{filled_with_indexes(ags->size())};
        serum_indexes = PointIndexList{filled_with_indexes(srs->size())};
    }

    const auto max_ag_name = static_cast<int>(ags->max_full_name());

    output << std::setw(max_ag_name + 6) << std::right << ' ' << "Serum full names are under the table\n";
    output << std::setw(max_ag_name) << ' ';
    for (auto sr_ind : acmacs::range(serum_indexes->size()))
        output << std::setw(7) << std::right << sr_label(sr_ind);
    output << '\n';

    output << std::setw(max_ag_name + 2) << ' ';
    for (auto sr_no : serum_indexes)
        output << std::setw(7) << std::right << srs->at(sr_no)->abbreviated_location_year();
    output << '\n';

    for (auto ag_no : antigen_indexes) {
        output << std::setw(max_ag_name + 2) << std::left << ags->at(ag_no)->full_name();
        for (auto sr_no : serum_indexes)
            output << std::setw(7) << std::right << *tt->titer(ag_no, sr_no);
        output << '\n';
    }
    output << '\n';

    for (auto [sr_ind, sr_no] : acmacs::enumerate(serum_indexes))
        output << sr_label(sr_ind) << std::setw(7) << std::right << srs->at(sr_no)->abbreviated_location_year() << "  " << srs->at(sr_no)->full_name() << '\n';

} // acmacs::chart::Chart::show_table

// ----------------------------------------------------------------------

bool acmacs::chart::same_tables(const Chart& c1, const Chart& c2, bool verbose)
{
    if (!equal(*c1.antigens(), *c2.antigens(), verbose)) {
        if (verbose)
            fmt::print(stderr, "WARNING: antigen sets are different\n");
        return false;
    }

    if (!equal(*c1.sera(), *c2.sera(), verbose)) {
        if (verbose)
            fmt::print(stderr, "WARNING: serum sets are different\n");
        return false;
    }

    if (!equal(*c1.titers(), *c2.titers(), verbose)) {
        if (verbose)
            fmt::print(stderr, "WARNING: titers are different\n");
        return false;
    }

    return true;

} // acmacs::chart::same_tables

// ----------------------------------------------------------------------

acmacs::chart::BLineage::Lineage acmacs::chart::BLineage::from(char aSource)
{
    switch (aSource) {
        case 'Y':
            return Yamagata;
        case 'V':
            return Victoria;
    }
    return Unknown;

} // acmacs::chart::BLineage::from

// ----------------------------------------------------------------------

acmacs::chart::BLineage::Lineage acmacs::chart::BLineage::from(std::string_view aSource)
{
    return aSource.empty() ? Unknown : from(aSource[0]);

} // acmacs::chart::BLineage::from

// ----------------------------------------------------------------------

std::string acmacs::chart::Info::make_info() const
{
    const auto n_sources = number_of_sources();
    return string::join(" ", {name(),
                    *virus(Compute::Yes),
                    lab(Compute::Yes),
                    virus_type(Compute::Yes),
                    subset(Compute::Yes),
                    assay(Compute::Yes),
                    rbc_species(Compute::Yes),
                    date(Compute::Yes),
                    n_sources ? ("(" + std::to_string(n_sources) + " tables)") : std::string{}
                             });

} // acmacs::chart::Info::make_info

// ----------------------------------------------------------------------

std::string acmacs::chart::Info::make_name() const
{
    std::string n = name(Compute::No);
    if (n.empty())
        n = string::join({lab(Compute::Yes), *virus_not_influenza(Compute::Yes), virus_type(Compute::Yes), subset(Compute::Yes), assay(Compute::Yes), rbc_species(Compute::Yes), date(Compute::Yes)});
    return n;

} // acmacs::chart::Info::make_name

// ----------------------------------------------------------------------

size_t acmacs::chart::Info::max_source_name() const
{
    if (number_of_sources() < 2)
        return 0;
    size_t msn = 0;
    for (auto s_no : acmacs::range(number_of_sources()))
        msn = std::max(msn, source(s_no)->name().size());
    return msn;

} // acmacs::chart::Info::max_source_name

// ----------------------------------------------------------------------

acmacs::chart::Lab acmacs::chart::Info::fix_lab_name(Lab source, FixLab fix) const
{
    switch (fix) {
        case FixLab::no:
            break;
        case FixLab::yes:
            source = Lab{::string::replace(source, "NIMR", "Crick")};
            source = Lab{::string::replace(source, "MELB", "VIDRL")};
            break;
        case FixLab::reverse:
            source = Lab{::string::replace(source, "Crick", "NIMR")};
            source = Lab{::string::replace(source, "VIDRL", "MELB")};
            break;
    }
    return source;

} // acmacs::chart::Info::fix_lab_name

// ----------------------------------------------------------------------

std::string acmacs::chart::Projection::make_info() const
{
    fmt::memory_buffer result;
    auto lt = layout();
    fmt::format_to(result, "{} {}d", stress(), lt->number_of_dimensions());
    if (auto cmt = comment(); !cmt.empty())
        fmt::format_to(result, " <{}>", cmt);
    if (auto fcb = forced_column_bases(); fcb)
        fmt::format_to(result, " forced-column-bases"); // fcb
    else
        fmt::format_to(result, " >={}", minimum_column_basis());
    return fmt::to_string(result);

} // acmacs::chart::Projection::make_info

// ----------------------------------------------------------------------

double acmacs::chart::Projection::stress(acmacs::chart::RecalculateStress recalculate) const
{
    switch (recalculate) {
      case RecalculateStress::yes:
          return recalculate_stress();
      case RecalculateStress::if_necessary:
          if (const auto s = stored_stress(); s)
              return *s;
          else
              return recalculate_stress();
      case RecalculateStress::no:
          if (const auto s = stored_stress(); s)
              return *s;
          else
              return InvalidStress;
    }
    throw invalid_data("Projection::stress: internal");

} // acmacs::chart::Projection::stress

// ----------------------------------------------------------------------

double acmacs::chart::Projection::stress_with_moved_point(size_t point_no, const PointCoordinates& move_to) const
{
    acmacs::Layout new_layout(*layout());
    new_layout[point_no] = move_to;
    return stress_factory(*this, multiply_antigen_titer_until_column_adjust::yes).value(new_layout);

} // acmacs::chart::Projection::stress_with_moved_point

// ----------------------------------------------------------------------

acmacs::chart::Blobs acmacs::chart::Projection::blobs(double stress_diff, size_t number_of_drections, double stress_diff_precision) const
{
    Blobs blobs(stress_diff, number_of_drections, stress_diff_precision);
    blobs.calculate(*layout(), stress_factory(*this, multiply_antigen_titer_until_column_adjust::yes));
    return blobs;

} // acmacs::chart::Projection::blobs

// ----------------------------------------------------------------------

acmacs::chart::Blobs acmacs::chart::Projection::blobs(double stress_diff, const PointIndexList& points, size_t number_of_drections, double stress_diff_precision) const
{
    Blobs blobs(stress_diff, number_of_drections, stress_diff_precision);
    blobs.calculate(*layout(), points, stress_factory(*this, multiply_antigen_titer_until_column_adjust::yes));
    return blobs;

} // acmacs::chart::Projection::blobs

// ----------------------------------------------------------------------

std::string acmacs::chart::Projections::make_info(size_t max_number_of_projections_to_show) const
{
    std::string result = "Projections: " + std::to_string(size());
    for (auto projection_no: acmacs::range(0UL, std::min(max_number_of_projections_to_show, size())))
        result += "\n  " + std::to_string(projection_no) + ' ' + operator[](projection_no)->make_info();
    return result;

} // acmacs::chart::Projections::make_info

// ----------------------------------------------------------------------

// size_t acmacs::chart::Projections::projection_no(const acmacs::chart::Projection* projection) const
// {
//     std::cerr << "projection_no " << projection << '\n';
//     for (size_t index = 0; index < size(); ++index) {
//         std::cerr << "p " << index << ' ' << operator[](index).get() << '\n';
//         if (operator[](index).get() == projection)
//             return index;
//     }
//     throw invalid_data("cannot find projection_no, total projections: " + std::to_string(size()));

// } // acmacs::chart::Projections::projection_no

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::full_name_with_fields() const
{
    std::string r{name()};
    if (const auto value = reassortant(); !value.empty())
        r += " reassortant=\"" + *value + '"';
    if (const auto value = ::string::join(" ", annotations()); !value.empty())
        r += " annotations=\"" + value + '"';
    if (const auto value = passage(); !value.empty())
        r += " passage=\"" + *value + "\" ptype=" + value.passage_type();
    if (const auto value = date(); !value.empty())
        r += " date=" + *value;
    if (const auto value = lineage(); value != BLineage::Unknown)
        r += fmt::format(" lineage={}", value);
    if (reference())
        r += " reference";
    if (const auto value = ::string::join(" ", lab_ids()); !value.empty())
        r += " lab_ids=\"" + value + '"';
    return r;

} // acmacs::chart::Antigen::full_name_with_fields

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::full_name_with_fields() const
{
    std::string r{name()};
    if (const auto value = reassortant(); !value.empty())
        r += " reassortant=\"" + *value + '"';
    if (const auto value = ::string::join(" ", annotations()); !value.empty())
        r += " annotations=\"" + value + '"';
    if (const auto value = serum_id(); !value.empty())
        r += " serum_id=\"" + *value + '"';
    if (const auto value = passage(); !value.empty())
        r += " passage=\"" + *value + "\" ptype=" + value.passage_type();
    if (const auto value = serum_species(); !value.empty())
        r += " serum_species=\"" + *value + '"';
    if (const auto value = lineage(); value != BLineage::Unknown)
        r += fmt::format(" lineage={}", value);
    return r;

} // acmacs::chart::Serum::full_name_with_fields

// ----------------------------------------------------------------------

// std::string name_abbreviated(std::string aName);
static inline std::string name_abbreviated(std::string_view aName)
{
    try {
        std::string virus_type, host, location, isolation, year, passage, extra;
        virus_name::split_with_extra(aName, virus_type, host, location, isolation, year, passage, extra);
        return string::join("/", {get_locdb().abbreviation(location), isolation, year.substr(2)});
    }
    catch (virus_name::Unrecognized&) {
        return std::string{aName};
    }

} // name_abbreviated

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::name_abbreviated() const
{
    return ::name_abbreviated(name());

} // acmacs::chart::Antigen::name_abbreviated

// ----------------------------------------------------------------------

static inline std::string name_without_subtype(std::string_view aName)
{
    try {
        std::string virus_type, host, location, isolation, year, passage, extra;
        virus_name::split_with_extra(aName, virus_type, host, location, isolation, year, passage, extra);
        if (virus_type.size() > 1 && virus_type[0] == 'A' && virus_type[1] == '(')
            virus_type.resize(1);
        return string::join("/", {virus_type, host, location, isolation, year});
    }
    catch (virus_name::Unrecognized&) {
        return std::string{aName};
    }
}

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::name_without_subtype() const
{
    return ::name_without_subtype(name());

} // acmacs::chart::Antigen::name_without_subtype

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::location_abbreviated() const
{
    return get_locdb().abbreviation(::virus_name::location(name()));

} // acmacs::chart::Antigen::location_abbreviated

// ----------------------------------------------------------------------

static inline std::string abbreviated_location_year(std::string_view aName)
{
    try {
        std::string virus_type, host, location, isolation, year, passage, extra;
        virus_name::split_with_extra(aName, virus_type, host, location, isolation, year, passage, extra);
        return string::join("/", {get_locdb().abbreviation(location), year.substr(2, 2)});
    }
    catch (virus_name::Unrecognized&) {
        return std::string{aName};
    }
}

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::abbreviated_location_year() const
{
    return ::abbreviated_location_year(name());

} // acmacs::chart::Antigen::abbreviated_location_year

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::name_abbreviated() const
{
    return ::name_abbreviated(name());

} // acmacs::chart::Serum::name_abbreviated

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::name_without_subtype() const
{
    return ::name_without_subtype(name());

} // acmacs::chart::Serum::name_abbreviated

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::location_abbreviated() const
{
    return get_locdb().abbreviation(::virus_name::location(name()));

} // acmacs::chart::Serum::location_abbreviated

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::abbreviated_location_year() const
{
    return ::abbreviated_location_year(name());

} // acmacs::chart::Serum::abbreviated_location_year

// ----------------------------------------------------------------------

static inline bool not_in_country(std::string_view aName, std::string_view aCountry)
{
    try {
        return get_locdb().country(virus_name::location(aName)) != aCountry;
    }
    catch (virus_name::Unrecognized&) {
    }
    catch (LocationNotFound&) {
    }
    return true;

} // AntigensSera<AgSr>::filter_country

// ----------------------------------------------------------------------

static inline bool not_in_continent(std::string_view aName, std::string_view aContinent)
{
    try {
        return get_locdb().continent(virus_name::location(aName)) != aContinent;
    }
    catch (virus_name::Unrecognized&) {
    }
    catch (LocationNotFound&) {
    }
    return true;

} // AntigensSera<AgSr>::filter_continent

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::chart::Antigens::find_by_full_name(std::string_view aFullName) const
{
    const auto found = std::find_if(begin(), end(), [aFullName](auto antigen) -> bool { return antigen->full_name() == aFullName; });
    if (found == end())
        return {};
    else
        return found.index();

} // acmacs::chart::Antigens::find_by_full_name

// ----------------------------------------------------------------------

acmacs::chart::Indexes acmacs::chart::Antigens::find_by_name(std::string_view aName) const
{
    auto find = [this](auto name) -> Indexes {
        Indexes indexes;
        for (auto iter = this->begin(); iter != this->end(); ++iter) {
            if ((*iter)->name() == acmacs::virus::name_t{name})
                indexes.insert(iter.index());
        }
        return indexes;
    };

    Indexes indexes = find(aName);
    if (indexes->empty() && aName.size() > 2) {
        if (const auto first_name = (*begin())->name(); first_name.size() > 2) {
        // handle names with "A/" instead of "A(HxNx)/" or without subtype prefix (for A and B)
            if ((aName[0] == 'A' && aName[1] == '/' && first_name[0] == 'A' && first_name[1] == '(' && first_name.find(")/") != std::string::npos) || (aName[0] == 'B' && aName[1] == '/'))
                indexes = find(string::concat(first_name->substr(0, first_name.find('/')), aName.substr(1)));
            else if (aName[1] != '/' && aName[1] != '(')
                indexes = find(string::concat(first_name->substr(0, first_name.find('/') + 1), aName));
        }
    }
    return indexes;

} // acmacs::chart::Antigens::find_by_name

// ----------------------------------------------------------------------

template <typename AgSr> acmacs::chart::duplicates_t find_duplicates(const AgSr& ag_sr)
{
    std::map<std::string, std::vector<size_t>> designations_to_indexes;
    for (size_t index = 0; index < ag_sr.size(); ++index) {
        auto [pos, inserted] = designations_to_indexes.insert({ag_sr[index]->designation(), {}});
        pos->second.push_back(index);
    }

    acmacs::chart::duplicates_t result;
    for (auto [designation, indexes] : designations_to_indexes) {
        if (indexes.size() > 1 && designation.find(" DISTINCT") == std::string::npos) {
            result.push_back(indexes);
        }
    }
    return result;
}

// ----------------------------------------------------------------------

acmacs::chart::duplicates_t acmacs::chart::Antigens::find_duplicates() const
{
    return ::find_duplicates(*this);

} // acmacs::chart::Antigens::find_duplicates

// ----------------------------------------------------------------------

void acmacs::chart::Antigens::filter_country(Indexes& aIndexes, std::string_view aCountry) const
{
    remove(aIndexes, [aCountry](const auto& entry) { return not_in_country(entry.name(), aCountry); });

} // acmacs::chart::Antigens::filter_country

// ----------------------------------------------------------------------

void acmacs::chart::Antigens::filter_continent(Indexes& aIndexes, std::string_view aContinent) const
{
    remove(aIndexes, [aContinent](const auto& entry) { return not_in_continent(entry.name(), aContinent); });

} // acmacs::chart::Antigens::filter_continent

// ----------------------------------------------------------------------

size_t acmacs::chart::Antigens::max_full_name() const
{
    size_t max_name = 0;
    for (auto ag : *this)
        max_name = std::max(max_name, ag->full_name().size());
    return max_name;

} // acmacs::chart::Antigens::max_full_name

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"
static const std::regex sAnntotationToIgnore{"(CONC|RDE@|BOOST|BLEED|LAIV|^CDC$)"};
#include "acmacs-base/diagnostics-pop.hh"

bool acmacs::chart::Annotations::match_antigen_serum(const acmacs::chart::Annotations& antigen, const acmacs::chart::Annotations& serum)
{
    std::vector<std::string_view> antigen_fixed(antigen->size());
    auto antigen_fixed_end = antigen_fixed.begin();
    for (const auto& anno : antigen) {
        *antigen_fixed_end++ = anno;
    }
    antigen_fixed.erase(antigen_fixed_end, antigen_fixed.end());
    std::sort(antigen_fixed.begin(), antigen_fixed.end());

    std::vector<std::string_view> serum_fixed(serum->size());
    auto serum_fixed_end = serum_fixed.begin();
    for (const auto& anno : serum) {
        const std::string_view annos = static_cast<std::string_view>(anno);
        if (!std::regex_search(std::begin(annos), std::end(annos), sAnntotationToIgnore))
            *serum_fixed_end++ = anno;
    }
    serum_fixed.erase(serum_fixed_end, serum_fixed.end());
    std::sort(serum_fixed.begin(), serum_fixed.end());

    return antigen_fixed == serum_fixed;

} // acmacs::chart::Annotations::match_antigen_serum

// ----------------------------------------------------------------------

acmacs::chart::Sera::homologous_canditates_t acmacs::chart::Sera::find_homologous_canditates(const Antigens& aAntigens, acmacs::debug dbg) const
{
    const auto match_passage = [](acmacs::virus::Passage antigen_passage, acmacs::virus::Passage serum_passage, const Serum& serum) -> bool {
        if (serum_passage.empty()) // NIID has passage type data in serum_id
            return antigen_passage.is_egg() == (serum.serum_id().find("EGG") != std::string::npos);
        else
            return antigen_passage.is_egg() == serum_passage.is_egg();
    };

    std::map<std::string, std::vector<size_t>, std::less<>> antigen_name_index;
    for (auto [ag_no, antigen] : acmacs::enumerate(aAntigens))
        antigen_name_index.emplace(antigen->name(), std::vector<size_t>{}).first->second.push_back(ag_no);

    acmacs::chart::Sera::homologous_canditates_t result(size());
    for (auto [sr_no, serum] : acmacs::enumerate(*this)) {
        if (auto ags = antigen_name_index.find(*serum->name()); ags != antigen_name_index.end()) {
            for (auto ag_no : ags->second) {
                auto antigen = aAntigens[ag_no];
                if (dbg == debug::yes)
                    fmt::print(stderr, "DEBUG: SR {} {} R:{} A:{} P:{} -- AG {} {} R:{} A:{} P:{} -- R_match: {} A_match:{} P_match:{}\n",
                               sr_no, *serum->name(), *serum->reassortant(), serum->annotations(), *serum->passage(),
                               ag_no, *antigen->name(), *antigen->reassortant(), antigen->annotations(), *antigen->passage(),
                               antigen->reassortant() == serum->reassortant(), Annotations::match_antigen_serum(antigen->annotations(), serum->annotations()),
                               match_passage(antigen->passage(), serum->passage(), *serum));
                if (antigen->reassortant() == serum->reassortant() && Annotations::match_antigen_serum(antigen->annotations(), serum->annotations()) &&
                    match_passage(antigen->passage(), serum->passage(), *serum)) {
                    result[sr_no].insert(ag_no);
                }
            }
        }
    }

    return result;

} // acmacs::chart::Sera::find_homologous_canditates

// ----------------------------------------------------------------------

void acmacs::chart::Sera::set_homologous(find_homologous options, const Antigens& aAntigens, acmacs::debug dbg)
{
    const auto match_passage_strict = [](acmacs::virus::Passage antigen_passage, acmacs::virus::Passage serum_passage, const Serum& serum) -> bool {
        if (serum_passage.empty()) // NIID has passage type data in serum_id
            return antigen_passage.is_egg() == (serum.serum_id().find("EGG") != std::string::npos);
        else
            return antigen_passage == serum_passage;
    };

    const auto match_passage_relaxed = [](acmacs::virus::Passage antigen_passage, acmacs::virus::Passage serum_passage, const Serum& serum) -> bool {
        if (serum_passage.empty()) // NIID has passage type data in serum_id
            return antigen_passage.is_egg() == (serum.serum_id().find("EGG") != std::string::npos);
        else
            return antigen_passage.is_egg() == serum_passage.is_egg();
    };

    const auto homologous_canditates = find_homologous_canditates(aAntigens, dbg);

    if (options == find_homologous::all) {
        for (auto [sr_no, serum] : acmacs::enumerate(*this))
            serum->set_homologous(*homologous_canditates[sr_no], dbg);
    }
    else {
        std::vector<std::optional<size_t>> homologous(size()); // for each serum
        for (auto [sr_no, serum] : acmacs::enumerate(*this)) {
            const auto& canditates = homologous_canditates[sr_no];
            for (auto canditate : canditates) {
                if (match_passage_strict(aAntigens[canditate]->passage(), serum->passage(), *serum)) {
                    homologous[sr_no] = canditate;
                    break;
                }
            }
        }

        if (options != find_homologous::strict) {
            for (auto [sr_no, serum] : acmacs::enumerate(*this)) {
                if (!homologous[sr_no]) {
                    const auto& canditates = homologous_canditates[sr_no];
                    for (auto canditate : canditates) {
                        const auto occupied = std::any_of(homologous.begin(), homologous.end(), [canditate](std::optional<size_t> ag_no) -> bool { return ag_no && *ag_no == canditate; });
                        if (!occupied && match_passage_relaxed(aAntigens[canditate]->passage(), serum->passage(), *serum)) {
                            homologous[sr_no] = canditate;
                            break;
                        }
                    }
                }
            }

            if (options != find_homologous::relaxed_strict) {
                for (auto [sr_no, serum] : acmacs::enumerate(*this)) {
                    if (!homologous[sr_no]) {
                        const auto& canditates = homologous_canditates[sr_no];
                        for (auto canditate : canditates) {
                            if (match_passage_relaxed(aAntigens[canditate]->passage(), serum->passage(), *serum)) {
                                homologous[sr_no] = canditate;
                                break;
                            }
                        }
                    }
                }
            }
        }

        for (auto [sr_no, serum] : acmacs::enumerate(*this))
            if (const auto homol = homologous[sr_no]; homol)
                serum->set_homologous({*homol}, dbg);
    }

} // acmacs::chart::Sera::set_homologous

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::chart::Sera::find_by_full_name(std::string_view aFullName) const
{
    const auto found = std::find_if(begin(), end(), [aFullName](auto serum) -> bool { return serum->full_name() == aFullName; });
    if (found == end())
        return {};
    else
        return found.index();

} // acmacs::chart::Sera::find_by_full_name

// ----------------------------------------------------------------------

acmacs::chart::Indexes acmacs::chart::Sera::find_by_name(std::string_view aName) const
{
    Indexes indexes;
    for (auto iter = begin(); iter != end(); ++iter) {
        if ((*iter)->name() == aName)
            indexes.insert(iter.index());
    }
    return indexes;

} // acmacs::chart::Sera::find_by_name

// ----------------------------------------------------------------------

acmacs::chart::duplicates_t acmacs::chart::Sera::find_duplicates() const
{
    return ::find_duplicates(*this);

} // acmacs::chart::Sera::find_duplicates

// ----------------------------------------------------------------------

void acmacs::chart::Sera::filter_country(Indexes& aIndexes, std::string_view aCountry) const
{
    remove(aIndexes, [aCountry](const auto& entry) { return not_in_country(entry.name(), aCountry); });

} // acmacs::chart::Sera::filter_country

// ----------------------------------------------------------------------

void acmacs::chart::Sera::filter_continent(Indexes& aIndexes, std::string_view aContinent) const
{
    remove(aIndexes, [aContinent](const auto& entry) { return not_in_continent(entry.name(), aContinent); });

} // acmacs::chart::Sera::filter_continent

// ----------------------------------------------------------------------

size_t acmacs::chart::Sera::max_full_name() const
{
    size_t max_name = 0;
    for (auto sr : *this)
        max_name = std::max(max_name, sr->full_name().size());
    return max_name;

} // acmacs::chart::Sera::max_full_name

// ----------------------------------------------------------------------

acmacs::PointStylesCompacted acmacs::chart::PlotSpec::compacted() const
{
    acmacs::PointStylesCompacted result;
    for (const auto& style: all_styles()) {
        if (auto found = std::find(result.styles.begin(), result.styles.end(), style); found == result.styles.end()) {
            result.styles.push_back(style);
            result.index.push_back(result.styles.size() - 1);
        }
        else {
            result.index.push_back(static_cast<size_t>(found - result.styles.begin()));
        }
    }
    return result;

} // acmacs::chart::PlotSpec::compacted

// ----------------------------------------------------------------------

// acmacs::chart::Chart::~Chart()
// {
// } // acmacs::chart::Chart::~Chart

// // ----------------------------------------------------------------------

// acmacs::chart::Info::~Info()
// {
// } // acmacs::chart::Info::~Info

// // ----------------------------------------------------------------------

// acmacs::chart::Antigen::~Antigen()
// {
// } // acmacs::chart::Antigen::~Antigen

// // ----------------------------------------------------------------------

// acmacs::chart::Serum::~Serum()
// {
// } // acmacs::chart::Serum::~Serum

// // ----------------------------------------------------------------------

// acmacs::chart::Antigens::~Antigens()
// {
// } // acmacs::chart::Antigens::~Antigens

// // ----------------------------------------------------------------------

// acmacs::chart::Sera::~Sera()
// {
// } // acmacs::chart::Sera::~Sera

// // ----------------------------------------------------------------------

// acmacs::chart::ColumnBases::~ColumnBases()
// {
// } // acmacs::chart::ColumnBases::~ColumnBases

// // ----------------------------------------------------------------------

// acmacs::chart::Projection::~Projection()
// {
// } // acmacs::chart::Projection::~Projection

// // ----------------------------------------------------------------------

// acmacs::chart::Projections::~Projections()
// {
// } // acmacs::chart::Projections::~Projections

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
