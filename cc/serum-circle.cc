#include <cmath>

#include "acmacs-base/layout.hh"
#include "acmacs-chart-2/serum-circle.hh"
#include "acmacs-chart-2/point-index-list.hh"

// ----------------------------------------------------------------------

acmacs::chart::SerumCircle::SerumCircle(size_t antigen_no, size_t serum_no, double column_basis, Titer homologous_titer)
    : serum_no_{serum_no}, column_basis_{column_basis}, per_antigen_(1)
{
    per_antigen_.front() = detail::SerumCirclePerAntigen(antigen_no, homologous_titer);

} // acmacs::chart::SerumCircle::SerumCircle

// ----------------------------------------------------------------------

acmacs::chart::SerumCircle::SerumCircle(const PointIndexList& antigens, size_t serum_no, double column_basis, const Titers& titers)
    : serum_no_{serum_no}, column_basis_{column_basis}, per_antigen_(antigens.size())
{
    std::transform(std::begin(antigens), std::end(antigens), per_antigen_.begin(), [serum_no,&titers](size_t ag_no) {
        return detail::SerumCirclePerAntigen(ag_no, titers.titer(ag_no, serum_no));
    });

} // acmacs::chart::SerumCircle::SerumCircle

// ----------------------------------------------------------------------

acmacs::chart::SerumCircle::SerumCircle(const PointIndexList& antigens, size_t serum_no, double column_basis, Titer homologous_titer)
    : serum_no_{serum_no}, column_basis_{column_basis}, per_antigen_(antigens.size())
{
    std::transform(std::begin(antigens), std::end(antigens), per_antigen_.begin(), [&homologous_titer](size_t ag_no) {
        return detail::SerumCirclePerAntigen(ag_no, homologous_titer);
    });

} // acmacs::chart::SerumCircle::SerumCircle

// ----------------------------------------------------------------------

const char* acmacs::chart::detail::SerumCirclePerAntigen::report_reason() const
{
    switch (failure_reason) {
        case serum_circle_failure_reason::not_calculated:
            if (valid())
                return "SUCCESS";
            else
                return "not calculated";
        case serum_circle_failure_reason::non_regular_homologous_titer:
            return "non-regular homologous titer";
        case serum_circle_failure_reason::titer_too_low:
            return "protectionboundary titer is too low, protects everything";
    }

} // acmacs::chart::detail::SerumCirclePerAntigen::report_reason

// ----------------------------------------------------------------------

class TiterDistance
{
 public:
    inline TiterDistance(acmacs::chart::Titer aTiter, double aColumnBase, double aDistance)
        : titer(aTiter), similarity(aTiter.is_dont_care() ? 0.0 : aTiter.logged_for_column_bases()),
          final_similarity(std::min(aColumnBase, similarity)), distance(aDistance) {}
    inline TiterDistance() : similarity(0), final_similarity(0), distance(std::numeric_limits<double>::quiet_NaN()) {}
    inline operator bool() const { return !titer.is_dont_care(); }

    acmacs::chart::Titer titer;
    double similarity;
    double final_similarity;
    double distance;
};

inline std::ostream& operator << (std::ostream& out, const TiterDistance& td)
{
    if (td)
        return out << "t:" << td.titer << " s:" << td.similarity << " f:" << td.final_similarity << " d:" << td.distance << std::endl;
    else
        return out << "dont-care" << std::endl;
}

// ----------------------------------------------------------------------

// Description of empirical radius calculation found in my message to Derek 2015-09-21 12:03 Subject: Serum protection radius
//
// Program "draws" some circle around a serum with some radius. Then for
// each antigen having titer with that serum program calculates:
// 1. Theoretical protection, i.e. if titer for antigen and serum is more
// or equal than (homologous-titer - 2)
// 2. Empirical protection, i.e. if antigen is inside the drawn circle,
// i.e. if distance between antigen and serum is less or equal than the
// circle radius.
//
// As the result for a circle we have four numbers:
// 1. Number of antigens both theoretically and empirically protected;
// 2. Number of antigens just theoretically protected;
// 3. Number of antigens just empirically protected;
// 4. Number of antigens not protected at all.
//
// Then the program optimizes the circle radius to minimize 2 and 3,
// i.e. the sum of number of antigens protected only theoretically and
// only empirically.
//
// Practically program first calculates stress for the radius equal to
// the distance of the closest antigen. Then it takes the radius as
// average between closest antigen distance and the second closest
// antigen distance and gets stress. Then it takes the radius as
// closest antigen and gets stress. And so on, the stress increases
// with each antigen included into the circle.
//
// If there are multiple optima with equal sums of 2 and 3, then the
// radius is a mean of optimal radii.

void acmacs::chart::detail::serum_circle_empirical(const SerumCircle& circle_data, detail::SerumCirclePerAntigen& per_antigen, const Layout& layout, const Titers& titers)
{
    std::vector<TiterDistance> titers_and_distances(titers.number_of_antigens());
    size_t max_titer_for_serum_ag_no = 0;
    for (size_t ag_no = 0; ag_no < titers.number_of_antigens(); ++ag_no) {
        const auto titer = titers.titer(ag_no, circle_data.serum_no());
        if (!titer.is_dont_care()) {
            // TODO: antigensSeraTitersMultipliers (acmacs/plot/serum_circle.py:113)
            titers_and_distances[ag_no] = TiterDistance(titer, circle_data.column_basis(), layout.distance(ag_no, circle_data.serum_no() + titers.number_of_antigens()));
            if (max_titer_for_serum_ag_no != ag_no && titers_and_distances[max_titer_for_serum_ag_no].final_similarity < titers_and_distances[ag_no].final_similarity)
                max_titer_for_serum_ag_no = ag_no;
        }
        // else if (ag_no == per_antigen.antigen_no)
        //     throw serum_circle_radius_calculation_error("no homologous titer");
    }
    const double protection_boundary_titer = titers_and_distances[per_antigen.antigen_no].final_similarity - 2.0;
    if (protection_boundary_titer < 1.0) {
        per_antigen.failure_reason = serum_circle_failure_reason::titer_too_low;
        return;
    }

    // if (aVerbose) {
    //     std::cerr << ">>> titers_and_distances: " << titers_and_distances << '\n';
    //     std::cerr << ">>> serum_circle_radius_empirical protection_boundary_titer: " << protection_boundary_titer << '\n';
    // }

    // sort antigen indices by antigen distance from serum, closest first
    auto antigens_by_distances_sorting = [&titers_and_distances](size_t a, size_t b) -> bool {
        const auto& aa = titers_and_distances[a];
        if (aa) {
            const auto& bb = titers_and_distances[b];
            return bb ? aa.distance < bb.distance : true;
        }
        else
            return false;
    };
    Indexes antigens_by_distances(acmacs::index_iterator(0UL), acmacs::index_iterator(titers.number_of_antigens()));
    std::sort(antigens_by_distances.begin(), antigens_by_distances.end(), antigens_by_distances_sorting);
    // if (aVerbose) std::cerr << "DEBUG: antigens_by_distances " << antigens_by_distances << std::endl;

    constexpr const size_t None = static_cast<size_t>(-1);
    size_t best_sum = None;
    size_t previous = None;
    double sum_radii = 0;
    size_t num_radii = 0;
    for (size_t ag_no : antigens_by_distances) {
        if (!titers_and_distances[ag_no])
            break;
        if (!std::isnan(titers_and_distances[ag_no].distance)) {
            const double radius = previous == None ? titers_and_distances[ag_no].distance : (titers_and_distances[ag_no].distance + titers_and_distances[previous].distance) / 2.0;
            size_t protected_outside = 0, not_protected_inside = 0; // , protected_inside = 0, not_protected_outside = 0;
            for (const auto& protection_data : titers_and_distances) {
                if (protection_data) {
                    const bool inside = protection_data.distance <= radius;
                    const bool protectd =
                        protection_data.titer.is_regular() ? protection_data.final_similarity >= protection_boundary_titer : protection_data.final_similarity > protection_boundary_titer;
                    if (protectd && !inside)
                        ++protected_outside;
                    else if (!protectd && inside)
                        ++not_protected_inside;
                }
            }
            const size_t summa = protected_outside + not_protected_inside;
            if (best_sum == None || best_sum >= summa) { // if sums are the same, choose the smaller radius (found earlier)
                if (best_sum == summa) {
                    // if (aVerbose)
                    //     std::cerr << "DEBUG: AG " << ag_no << " radius:" << radius << " distance:" << titers_and_distances[ag_no].distance << " prev:" << static_cast<int>(previous) << "
                    //     protected_outside:" << protected_outside << " not_protected_inside:" << not_protected_inside << " best_sum:" << best_sum << std::endl;
                    sum_radii += radius;
                    ++num_radii;
                }
                else {
                    // if (aVerbose)
                    //     std::cerr << "======================================================================" << std::endl
                    //               << "DEBUG: AG " << ag_no << " radius:" << radius << " distance:" << titers_and_distances[ag_no].distance << " prev:" << static_cast<int>(previous) << "
                    //               protected_outside:" << protected_outside << " not_protected_inside:" << not_protected_inside << " best_sum:" << best_sum << std::endl;
                    sum_radii = radius;
                    num_radii = 1;
                    best_sum = summa;
                }
            }
            // std::cerr << "AG " << ag_no << " radius:" << radius << " protected_outside:" << protected_outside << " not_protected_inside:" << not_protected_inside << " best_sum:" << best_sum <<
            // std::endl;
            previous = ag_no;
        }
    }
    per_antigen.radius = sum_radii / num_radii;

} // acmacs::chart::detail::serum_circle_empirical

// ----------------------------------------------------------------------

acmacs::chart::SerumCircle acmacs::chart::serum_circle_empirical(const PointIndexList& antigens, Titer homologous_titer, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers)
{
    SerumCircle circle_data(antigens, serum_no, column_basis, homologous_titer);
    if (circle_data.failure_reason() == serum_circle_failure_reason::not_calculated)
        serum_circle_empirical(circle_data, circle_data.per_antigen_.front(), layout, titers);
    return circle_data;

} // acmacs::chart::serum_circle_empirical

// ----------------------------------------------------------------------

acmacs::chart::SerumCircle acmacs::chart::serum_circle_empirical(size_t antigen_no, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers)
{
    SerumCircle circle_data(antigen_no, serum_no, column_basis, titers.titer(antigen_no, serum_no));
    if (circle_data.failure_reason() == serum_circle_failure_reason::not_calculated)
        serum_circle_empirical(circle_data, circle_data.per_antigen_.front(), layout, titers);
    return circle_data;

} // acmacs::chart::serum_circle_empirical

// ----------------------------------------------------------------------

acmacs::chart::SerumCircle acmacs::chart::serum_circle_empirical(const PointIndexList& antigens, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers)
{
    SerumCircle circle_data(antigens, serum_no, column_basis, titers);
    for (auto& per_antigen : circle_data.per_antigen_) {
        if (per_antigen.failure_reason == serum_circle_failure_reason::not_calculated)
            serum_circle_empirical(circle_data, per_antigen, layout, titers);
    }
    return circle_data;

} // acmacs::chart::serum_circle_empirical

// ----------------------------------------------------------------------

// Low reactors are defined as >4-fold from the homologous titer,
// hence the theoretical radius is 2 units plus the number of 2-folds
// between max titer and the homologous titer for a serum. Saying the
// same thing mathematically the theoretical radius for a serum circle
// is 2 + log2(max titer for serum S against any antigen A) - log2(homologous titer for serum S).

void acmacs::chart::detail::serum_circle_theoretical(const SerumCircle& circle_data, detail::SerumCirclePerAntigen& per_antigen)
{
    per_antigen.radius = 2.0 + circle_data.column_basis() - per_antigen.titer.logged_for_column_bases();

} // acmacs::chart::detail::serum_circle_theoretical

// ----------------------------------------------------------------------

acmacs::chart::SerumCircle acmacs::chart::serum_circle_theoretical(Titer homologous_titer, size_t serum_no, double column_basis)
{
    SerumCircle circle_data(static_cast<size_t>(-1), serum_no, column_basis, homologous_titer);
    if (circle_data.failure_reason() == serum_circle_failure_reason::not_calculated)
        serum_circle_theoretical(circle_data, circle_data.per_antigen_.front());
    return circle_data;

} // acmacs::chart::serum_circle_theoretical

// ----------------------------------------------------------------------

acmacs::chart::SerumCircle acmacs::chart::serum_circle_theoretical(size_t antigen_no, size_t serum_no, double column_basis, const Titers& titers)
{
    SerumCircle circle_data(antigen_no, serum_no, column_basis, titers.titer(antigen_no, serum_no));
    if (circle_data.failure_reason() == serum_circle_failure_reason::not_calculated)
        serum_circle_theoretical(circle_data, circle_data.per_antigen_.front());
    return circle_data;

} // acmacs::chart::serum_circle_theoretical

// ----------------------------------------------------------------------

acmacs::chart::SerumCircle acmacs::chart::serum_circle_theoretical(const PointIndexList& antigens, size_t serum_no, double column_basis, const Titers& titers)
{
    SerumCircle circle_data(antigens, serum_no, column_basis, titers);
    for (auto& per_antigen : circle_data.per_antigen_) {
        if (per_antigen.failure_reason == serum_circle_failure_reason::not_calculated)
            serum_circle_theoretical(circle_data, per_antigen);
    }
    return circle_data;

} // acmacs::chart::serum_circle_theoretical

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End: