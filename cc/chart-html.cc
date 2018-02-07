#include <iostream>
#include <fstream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/quicklook.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

static void contents(std::ostream& output, const acmacs::chart::Chart& chart);
static void header(std::ostream& output, std::string table_name);
static void footer(std::ostream& output);
static std::string html_escape(std::string source);

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    using namespace std::string_literals;

    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--open", false},
                {"--verbose", false},
                {"--time", false, "report time of loading chart"},
                {"-h", false},
                {"--help", false},
                {"-v", false},
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> [<output.html>]\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const report_time report = args["--time"] ? report_time::Yes : report_time::No;
            acmacs::file::temp temp_file(".html");
            const std::string output_filename = args.number_of_arguments() > 1 ? std::string{args[1]} : static_cast<std::string>(temp_file);
            std::ofstream output{output_filename};

            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            header(output, chart->make_name());
            contents(output, *chart);
            footer(output);
            output.close();

            if (args["--open"] || args.number_of_arguments() < 2)
                acmacs::quicklook(output_filename, 2);
        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void contents(std::ostream& output, const acmacs::chart::Chart& chart)
{
    output << "<h3>" << chart.make_name() << "</h3>\n";
    auto field_present = [](const std::vector<std::string>& src) -> bool { return std::find_if(src.begin(), src.end(), [](const auto& s) -> bool { return !s.empty(); }) != src.end(); };
    auto antigens = chart.antigens();
    auto sera = chart.sera();

    std::vector<std::string> annotations(antigens->size()), reassortants(antigens->size()), passages(antigens->size()), dates(antigens->size()), lab_ids(antigens->size());
    for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
        annotations[ag_no] = string::join(" ", antigen->annotations());
        reassortants[ag_no] = antigen->reassortant();
        passages[ag_no] = antigen->passage();
        dates[ag_no] = antigen->date();
        lab_ids[ag_no] = string::join(" ", antigen->lab_ids());
    }
    std::vector<std::string> serum_annotations(sera->size());
    for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
        serum_annotations[sr_no] = string::join(" ", serum->annotations());
    }
    const bool annotations_present{field_present(annotations)}, reassortants_present{field_present(reassortants)}, passages_present{field_present(passages)},
               dates_present{field_present(dates)}, lab_ids_present{field_present(lab_ids)}, serum_annotations_present{field_present(serum_annotations)};

    output << "<table><tr><td></td><td></td>";
    size_t skip_left = 0;
    if (annotations_present) {
        output << "<td></td>";
        ++skip_left;
    }
    if (reassortants_present) {
        output << "<td></td>";
        ++skip_left;
    }
    if (passages_present) {
        output << "<td></td>";
        ++skip_left;
    }
    for (auto sr_no : acmacs::range(sera->size()))
        output << "<td class=\"sr-no " << ("sr-" + std::to_string(sr_no)) << "\">" << (sr_no + 1) << "</td>";
    size_t skip_right = 0;
    if (dates_present) {
        output << "<td></td>";
        ++skip_right;
    }
    if (lab_ids_present) {
        output << "<td></td>";
        ++skip_right;
    }
    output << "</tr>\n";

      // serum names
    output << "<tr>";
    for ([[maybe_unused]] auto i : acmacs::range(skip_left + 2))
        output << "<td></td>";
    for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
        const auto sr_no_class = "sr-" + std::to_string(sr_no);
        output << "<td class=\"sr-name " << sr_no_class << "\">" << html_escape(serum->name_abbreviated()) << "</td>";
    }
    for ([[maybe_unused]] auto i : acmacs::range(skip_right))
        output << "<td></td>";
    output << "</tr>\n";

      // serum annotations (e.g. CONC)
    if (serum_annotations_present) {
        output << "<tr>";
        for ([[maybe_unused]] auto i : acmacs::range(skip_left + 2))
            output << "<td></td>";
        for (auto [sr_no, serum] : acmacs::enumerate(*sera))
            output << "<td class=\"sr-annotations " << ("sr-" + std::to_string(sr_no)) << "\">" << html_escape(serum_annotations[sr_no]) << "</td>";
        for ([[maybe_unused]] auto i : acmacs::range(skip_right))
            output << "<td></td>";
        output << "</tr>\n";
    }

      // serum_ids
    output << "<tr>";
    for ([[maybe_unused]] auto i : acmacs::range(skip_left + 2))
        output << "<td></td>";
    for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
        const auto sr_no_class = "sr-" + std::to_string(sr_no);
        output << "<td class=\"sr-id " << sr_no_class << "\">" << html_escape(serum->serum_id()) << "</td>";
    }
    for ([[maybe_unused]] auto i : acmacs::range(skip_right))
        output << "<td></td>";
    output << "</tr>\n";

    auto titers = chart.titers();
      // antigens and titers
    for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
        output << "<tr class=\"" << ((ag_no % 2) == 0 ? "even" : "odd") << ' ' << ("ag-" + std::to_string(ag_no)) << "\"><td class=\"ag-no\">" << (ag_no + 1) << "</td>"
               << "<td class=\"ag-name\">" << html_escape(antigen->name()) << "</td>";
        if (annotations_present)
            output << "<td class=\"auto ag-annotations\">" << html_escape(annotations[ag_no]) << "</td>";
        if (reassortants_present)
            output << "<td class=\"ag-reassortant\">" << html_escape(reassortants[ag_no]) << "</td>";
        if (passages_present)
            output << "<td class=\"ag-passage\">" << html_escape(passages[ag_no]) << "</td>";

          // titers
        for (auto sr_no : acmacs::range(sera->size())) {
            const auto titer = titers->titer(ag_no, sr_no);
            const char* titer_class = "regular";
            switch (titer.type()) {
              case acmacs::chart::Titer::Invalid: titer_class = "titer-invalid"; break;
              case acmacs::chart::Titer::Regular: titer_class = "titer-regular"; break;
              case acmacs::chart::Titer::DontCare: titer_class = "titer-dont-care"; break;
              case acmacs::chart::Titer::LessThan: titer_class = "titer-less-than"; break;
              case acmacs::chart::Titer::MoreThan: titer_class = "titer-more-than"; break;
              case acmacs::chart::Titer::Dodgy: titer_class = "titer-dodgy"; break;
            }
            const char* titer_pos_class = sr_no == 0 ? "titer-pos-left" : (sr_no == (sera->size() - 1) ? "titer-pos-right" : "titer-pos-middle");
            const auto sr_no_class = "sr-" + std::to_string(sr_no);
            output << "<td class=\"titer " << titer_class << ' ' << titer_pos_class << ' ' << sr_no_class << "\">" << html_escape(acmacs::to_string(titer)) << "</td>";
        }

        if (dates_present)
            output << "<td class=\"ag-date\">" << dates[ag_no] << "</td>";
        if (lab_ids_present)
            output << "<td class=\"ag-lab-id\">" << lab_ids[ag_no] << "</td>";
        output << "</tr>\n";
    }

    output << "</table>\n";

            // const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;
            // for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
            //     std::cout << "AG " << std::setw(num_digits) << ag_no << " " << string::join(" ", {antigen->name(), string::join(" ", antigen->annotations()), antigen->reassortant(), antigen->passage(), "[" + static_cast<std::string>(antigen->date()) + "]", string::join(" ", antigen->lab_ids())}) << (antigen->reference() ? " Ref" : "") << '\n';
            // }
            // for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
            //     std::cout << "SR " << std::setw(num_digits) << sr_no << " " << string::join(" ", {serum->name(), string::join(" ", serum->annotations()), serum->reassortant(), serum->passage(), serum->serum_id(), serum->serum_species()}) << '\n';
            // }

} // contents

// ----------------------------------------------------------------------

void header(std::ostream& output, std::string table_name)
{
    const char* h1 = R"(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
table { border-collapse: collapse; border: 1px solid grey; border-spacing: 0; }
table.td { border: 1px solid grey; }
.sr-no { text-align: center; font-size: 0.8em; }
.sr-name { text-align: center; padding: 0 0.5em; white-space: nowrap; font-weight: bold; font-size: 0.8em; }
.sr-id { text-align: center; white-space: nowrap; font-size: 0.5em; border-bottom: 1px solid grey; }
.sr-annotations { text-align: center; white-space: nowrap; font-size: 0.5em; }
.ag-no { text-align: right; font-size: 0.8em; }
.ag-name { text-align: left; padding: 0 0.5em; white-space: nowrap; font-weight: bold; font-size: 0.8em; }
.ag-annotations { text-align: left; padding: 0 0.5em; white-space: nowrap; font-size: 0.8em; }
.ag-reassortant { text-align: left; padding: 0 0.5em; white-space: nowrap; font-size: 0.8em; }
.ag-passage { text-align: left; padding: 0 0.5em; white-space: nowrap; font-size: 0.8em; }
.ag-date { text-align: left; padding: 0 0.5em; white-space: nowrap; font-size: 0.8em; }
.ag-lab-id { text-align: left; padding: 0 0.5em; white-space: nowrap; font-size: 0.8em; }
.titer { text-align: right; white-space: nowrap; padding-right: 1em; font-size: 0.8em; }
.titer-pos-left { border-left: 1px solid grey; }
.titer-pos-right { border-right: 1px solid grey; }
.titer-invalid   { color: red; }
.titer-regular   { color: black; }
.titer-dont-care { color: grey; }
.titer-less-than { color: green; }
.titer-more-than { color: blue; }
.titer-dodgy     { color: brown; }
tr.even td { background-color: white; }
tr.odd td { background-color: #F0F0F0; }
</style>
<title>)";
    const char* h2 = R"(</title>
</head>
<body>
)";
    output << h1 << table_name << h2;

} // header

// ----------------------------------------------------------------------

void footer(std::ostream& output)
{
    const char* footer_data = R"(
</body>
</html>
)";
    output << footer_data;

} // footer

// ----------------------------------------------------------------------

std::string html_escape(std::string source)
{
    source = string::replace(source, "&", "&amp;");
    source = string::replace(source, "<", "&lt;");
    source = string::replace(source, ">", "&gt;");
    return source;

} // html_escape

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End: