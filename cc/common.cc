#include "acmacs-chart-2/common.hh"
#include "acmacs-chart-2/chart.hh"

using namespace acmacs::chart;

// ----------------------------------------------------------------------

class CommonAntigensSera::Impl
{
 public:
    Impl(const Chart& primary, const Chart& secondary, match_level_t match_level);

    struct CoreEntry
    {
        CoreEntry() = default;
        CoreEntry(CoreEntry&&) = default;
        template <typename AgSr> CoreEntry(size_t a_index, const AgSr& ag_sr)
            : index(a_index), name(ag_sr.name()), reassortant(ag_sr.reassortant()), annotations(ag_sr.annotations()) {}
        CoreEntry& operator=(CoreEntry&&) = default;

        static int compare(const CoreEntry& lhs, const CoreEntry& rhs)
            {
                if (auto n_c = lhs.name.compare(rhs.name); n_c != 0)
                    return n_c;
                if (auto r_c = lhs.reassortant.compare(rhs.reassortant); r_c != 0)
                    return r_c;
                return string::compare(lhs.annotations.join(), rhs.annotations.join());
            }

        static bool less(const CoreEntry& lhs, const CoreEntry& rhs) { return compare(lhs, rhs) < 0; }

        size_t index;
        Name name;
        Reassortant reassortant;
        Annotations annotations;

    }; // struct CoreEntry

    struct AntigenEntry : public CoreEntry
    {
        AntigenEntry() = default;
        AntigenEntry(AntigenEntry&&) = default;
        AntigenEntry(size_t a_index, const Antigen& antigen) : CoreEntry(a_index, antigen), passage(antigen.passage()) {}
        AntigenEntry& operator=(AntigenEntry&&) = default;

        std::string full_name() const { return ::string::join(" ", {name, reassortant, ::string::join(" ", annotations), passage}); }
        size_t full_name_length() const { return name.size() + reassortant.size() + annotations.total_length() + passage.size() + 1 + (reassortant.empty() ? 0 : 1) + annotations.size(); }
        bool operator<(const AntigenEntry& rhs) const { return compare(*this, rhs) < 0; }

        static int compare(const AntigenEntry& lhs, const AntigenEntry& rhs)
            {
                if (auto np_c = CoreEntry::compare(lhs, rhs); np_c != 0)
                    return np_c;
                return lhs.passage.compare(rhs.passage);
            }

        Passage passage;

    }; // class AntigenEntry

    struct SerumEntry : public CoreEntry
    {
        SerumEntry() = default;
        SerumEntry(SerumEntry&&) = default;
        SerumEntry(size_t a_index, const Serum& serum) : CoreEntry(a_index, serum), serum_id(serum.serum_id()), passage(serum.passage()) {}
        SerumEntry& operator=(SerumEntry&&) = default;

        std::string full_name() const { return ::string::join(" ", {name, reassortant, ::string::join(" ", annotations), serum_id, passage}); }
        size_t full_name_length() const { return name.size() + reassortant.size() + annotations.total_length() + serum_id.size() + 1 + (reassortant.empty() ? 0 : 1) + annotations.size() + passage.size() + (passage.empty() ? 0 : 1); }
        bool operator<(const SerumEntry& rhs) const { return compare(*this, rhs) < 0; }

        static int compare(const SerumEntry& lhs, const SerumEntry& rhs)
            {
                if (auto np_c = CoreEntry::compare(lhs, rhs); np_c != 0)
                    return np_c;
                return lhs.serum_id.compare(rhs.serum_id);
            }

        SerumId serum_id;
        Passage passage;

    }; // class SerumEntry

    enum class score_t : size_t { no_match = 0, passage_serum_id_ignored = 1, egg = 2, without_date = 3, full_match = 4 };

    struct MatchEntry
    {
        size_t primary_index;
        size_t secondary_index;
        score_t score;
        bool use = false;

    }; // class MatchEntry

    template <typename AgSrEntry> class ChartData
    {
     public:
        ChartData(const acmacs::chart::Chart& primary, const acmacs::chart::Chart& secondary, match_level_t match_level);
        void report(std::ostream& stream, const char* prefix, const char* ignored_key) const;
        std::vector<CommonAntigensSera::common_t> common() const;

        std::vector<AgSrEntry> primary_;
        std::vector<AgSrEntry> secondary_;
        std::vector<MatchEntry> match_;
        size_t number_of_common_ = 0;
        const size_t primary_base_, secondary_base_; // 0 for antigens, number_of_antigens for sera
        const size_t min_number_;   // for match_level_t::automatic threshold

     private:
        template <typename AgSr> static void make(std::vector<AgSrEntry>& target, const AgSr& source)
            {
                for (size_t index = 0; index < target.size(); ++index) {
                    target[index] = AgSrEntry(index, *source[index]);
                }
            }

        void match(match_level_t match_level);
        score_t match(const AgSrEntry& primary, const AgSrEntry& secondary, match_level_t match_level) const;
        score_t match_not_ignored(const AgSrEntry& primary, const AgSrEntry& secondary) const;

    }; // class ChartData<AgSrEntry>

    ChartData<AntigenEntry> antigens_;
    ChartData<SerumEntry> sera_;

}; // class Impl

// ----------------------------------------------------------------------

template <> CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::AntigenEntry>::ChartData(const acmacs::chart::Chart& primary, const acmacs::chart::Chart& secondary, match_level_t match_level)
    : primary_(primary.number_of_antigens()), secondary_(secondary.number_of_antigens()),
      primary_base_{0}, secondary_base_{0},
      min_number_{std::min(primary_.size(), secondary_.size())}
{
    make(primary_, *primary.antigens());
    std::sort(primary_.begin(), primary_.end());
    make(secondary_, *secondary.antigens());
    match(match_level);

} // CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::AntigenEntry>::ChartData

// ----------------------------------------------------------------------

template <> CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::SerumEntry>::ChartData(const acmacs::chart::Chart& primary, const acmacs::chart::Chart& secondary, match_level_t match_level)
    : primary_(primary.number_of_sera()), secondary_(secondary.number_of_sera()),
      primary_base_{primary.number_of_antigens()}, secondary_base_{secondary.number_of_antigens()},
      min_number_{std::min(primary_.size(), secondary_.size())}
{
    make(primary_, *primary.sera());
    std::sort(primary_.begin(), primary_.end());
    make(secondary_, *secondary.sera());
    match(match_level);

} // CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::SerumEntry>::ChartData

// ----------------------------------------------------------------------

template <typename AgSrEntry> void CommonAntigensSera::Impl::ChartData<AgSrEntry>::match(CommonAntigensSera::match_level_t match_level)
{
    for (const auto& secondary: secondary_) {
        const auto [first, last] = std::equal_range(primary_.begin(), primary_.end(), secondary, AgSrEntry::less);
        for (auto p_e = first; p_e != last; ++p_e) {
            if (const auto score = match(*p_e, secondary, match_level); score != score_t::no_match)
                  //match_.emplace_back(p_e->index, secondary.index, score);
                match_.push_back({p_e->index, secondary.index, score});
        }
    }
    auto order = [](const auto& a, const auto& b) {
                     if (a.score != b.score)
                         return a.score > b.score;
                     else if (a.primary_index != b.primary_index)
                         return a.primary_index < b.primary_index;
                     return a.secondary_index < b.secondary_index;
                 };
    std::sort(match_.begin(), match_.end(), order);

    std::set<size_t> primary_used, secondary_used;
    auto use_entry = [this,&primary_used,&secondary_used](auto& match_entry) {
        if (primary_used.find(match_entry.primary_index) == primary_used.end() && secondary_used.find(match_entry.secondary_index) == secondary_used.end()) {
            match_entry.use = true;
            primary_used.insert(match_entry.primary_index);
            secondary_used.insert(match_entry.secondary_index);
            ++this->number_of_common_;
        }
    };

    const size_t automatic_level_threshold = std::max(3UL, min_number_ / 10);
    score_t previous_score{score_t::no_match};
    bool stop = false;
    for (auto& match_entry: match_) {
        switch (match_level) {
          case match_level_t::strict:
              if (match_entry.score == score_t::full_match)
                  use_entry(match_entry);
              else
                  stop = true;
              break;
          case match_level_t::relaxed:
              if (match_entry.score >= score_t::egg)
                  use_entry(match_entry);
              else
                  stop = true;
              break;
          case match_level_t::ignored:
              if (match_entry.score >= score_t::passage_serum_id_ignored)
                  use_entry(match_entry);
              else
                  stop = true;
              break;
          case match_level_t::automatic:
              if (previous_score == match_entry.score || number_of_common_ < automatic_level_threshold) {
                  // std::cout << match_entry.primary_index << ' ' << match_entry.secondary_index << ' ' << static_cast<size_t>(match_entry.score) << ' ' << static_cast<size_t>(previous_score) << number_of_common_ << ' ' << automatic_level_threshold << '\n';
                  use_entry(match_entry);
                  previous_score = match_entry.score;
              }
              else
                  stop = true;
              break;
        }
        if (stop)
            break;
    }

} // CommonAntigensSera::Impl::ChartData::match

// ----------------------------------------------------------------------

template <typename AgSrEntry> CommonAntigensSera::Impl::score_t CommonAntigensSera::Impl::ChartData<AgSrEntry>::match(const AgSrEntry& primary, const AgSrEntry& secondary, acmacs::chart::CommonAntigensSera::match_level_t match_level) const
{
    score_t result{score_t::no_match};
    if (primary.name == secondary.name && primary.reassortant == secondary.reassortant && primary.annotations == secondary.annotations && !primary.annotations.distinct()) {
        switch (match_level) {
          case match_level_t::ignored:
              result = score_t::passage_serum_id_ignored;
              break;
          case match_level_t::strict:
          case match_level_t::relaxed:
          case match_level_t::automatic:
              result = match_not_ignored(primary, secondary);
              break;
        }
    }
    return result;

} // CommonAntigensSera::Impl::ChartData::match

// ----------------------------------------------------------------------

template <> CommonAntigensSera::Impl::score_t CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::AntigenEntry>::match_not_ignored(const AntigenEntry& primary, const AntigenEntry& secondary) const
{
    auto result = score_t::passage_serum_id_ignored;
    if (primary.passage.empty() || secondary.passage.empty()) {
        if (primary.passage == secondary.passage && !primary.reassortant.empty() && !secondary.reassortant.empty()) // reassortant assumes egg passage
            result = score_t::egg;
    }
    else if (primary.passage == secondary.passage)
        result = score_t::full_match;
    else if (primary.passage.without_date() == secondary.passage.without_date())
        result = score_t::without_date;
    else if (primary.passage.is_egg() == secondary.passage.is_egg())
        result = score_t::egg;
    return result;

} // CommonAntigensSera::Impl::ChartData<AntigenEntry>::match_not_ignored

// ----------------------------------------------------------------------

template <> CommonAntigensSera::Impl::score_t CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::SerumEntry>::match_not_ignored(const SerumEntry& primary, const SerumEntry& secondary) const
{
    auto result = score_t::passage_serum_id_ignored;
    if (primary.serum_id == secondary.serum_id && !primary.serum_id.empty())
        result = score_t::full_match;
    else if (!primary.passage.empty() && !secondary.passage.empty() && primary.passage.is_egg() == secondary.passage.is_egg())
        result = score_t::egg;
    return result;

} // CommonAntigensSera::Impl::ChartData<AntigenEntry>::match_not_ignored

// ----------------------------------------------------------------------

template <typename AgSrEntry> void CommonAntigensSera::Impl::ChartData<AgSrEntry>::report(std::ostream& stream, const char* prefix, const char* ignored_key) const
{
    const char* const score_names[] = {"no-match", ignored_key, "egg", "no-date", "full"};

    if (number_of_common_) {
        auto find_primary = [this](size_t index) -> const auto& { return *std::find_if(this->primary_.begin(), this->primary_.end(), [index](const auto& element) { return element.index == index; }); };
        size_t primary_name_size = 0,
                  // secondary_name_size = 0,
                max_number_primary = 0,
                max_number_secondary = 0;
        for (const auto& m: match_) {
            if (m.use) {
                primary_name_size = std::max(primary_name_size, find_primary(m.primary_index).full_name_length());
                  // secondary_name_size = std::max(secondary_name_size, secondary_[m.secondary_index].full_name_length());
                max_number_primary = std::max(max_number_primary, m.primary_index);
                max_number_secondary = std::max(max_number_secondary, m.secondary_index);
            }
        }
        const auto num_digits_primary = static_cast<int>(std::log10(max_number_primary)) + 1;
        const auto num_digits_secondary = static_cast<int>(std::log10(max_number_secondary)) + 1;

        stream << "common " << prefix << ": " << number_of_common_ << '\n';
        for (const auto& m: match_) {
            if (m.use)
                stream << std::setw(static_cast<int>(std::strlen(ignored_key) + 1)) << std::left << score_names[static_cast<size_t>(m.score)]
                       << std::setw(num_digits_primary) << std::right << m.primary_index << ' ' << std::setw(static_cast<int>(primary_name_size)) << std::left << find_primary(m.primary_index).full_name()
                       << "|" << std::setw(num_digits_secondary) << std::right << m.secondary_index << ' ' << /* std::setw(static_cast<int>(secondary_name_size)) << std::left << */ secondary_[m.secondary_index].full_name() << '\n';
        }
    }
    else {
        stream << "WARNING: no common " << prefix << '\n';
    }

} // CommonAntigensSera::Impl::ChartData<AgSrEntry>::report

// ----------------------------------------------------------------------

template <typename AgSrEntry> std::vector<CommonAntigensSera::common_t> CommonAntigensSera::Impl::ChartData<AgSrEntry>::common() const
{
    std::vector<CommonAntigensSera::common_t> result;
    for (const auto& m: match_) {
        if (m.use)
            result.emplace_back(m.primary_index, m.secondary_index);
    }
    return result;

} // CommonAntigensSera::Impl::ChartData<AgSrEntry>::common

// ----------------------------------------------------------------------

inline CommonAntigensSera::Impl::Impl(const Chart& primary, const Chart& secondary, match_level_t match_level)
    : antigens_(primary, secondary, match_level), sera_(primary, secondary, match_level)
{
}

// ----------------------------------------------------------------------

acmacs::chart::CommonAntigensSera::CommonAntigensSera(const Chart& primary, const Chart& secondary, match_level_t match_level)
    : impl_(std::make_unique<Impl>(primary, secondary, match_level))
{
}

// ----------------------------------------------------------------------

// must be here to allow std::unique_ptr<Impl> with incomplete Impl in .hh
acmacs::chart::CommonAntigensSera::~CommonAntigensSera()
{
}

// ----------------------------------------------------------------------

void acmacs::chart::CommonAntigensSera::report() const
{
    auto& stream = std::cout;
    impl_->antigens_.report(stream, "antigens", "no-passage");
    stream << '\n';
    impl_->sera_.report(stream, "sera", "no-serum-id");

} // CommonAntigensSera::report

// ----------------------------------------------------------------------

std::vector<acmacs::chart::CommonAntigensSera::common_t> acmacs::chart::CommonAntigensSera::antigens() const
{
    return impl_->antigens_.common();

} // CommonAntigensSera::antigens

// ----------------------------------------------------------------------

std::vector<acmacs::chart::CommonAntigensSera::common_t> acmacs::chart::CommonAntigensSera::sera() const
{
    return impl_->sera_.common();

} // CommonAntigensSera::sera

// ----------------------------------------------------------------------

std::vector<acmacs::chart::CommonAntigensSera::common_t> acmacs::chart::CommonAntigensSera::points() const
{
    auto result = impl_->antigens_.common();
    for (auto [primary, secondary]: impl_->sera_.common())
        result.emplace_back(primary + impl_->sera_.primary_base_, secondary + impl_->sera_.secondary_base_);
    return result;

} // CommonAntigensSera::points

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End: