#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/csv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

static void write_csv(std::string aFilename, std::shared_ptr<acmacs::chart::Antigens> antigens, std::shared_ptr<acmacs::chart::Sera> sera, std::shared_ptr<acmacs::chart::Layout> layout);
static void write_text(std::string aFilename, std::string aFieldSeparator, std::shared_ptr<acmacs::chart::Antigens> antigens, std::shared_ptr<acmacs::chart::Sera> sera, std::shared_ptr<acmacs::chart::Layout> layout);
static std::string encode_name(std::string aName, std::string aFieldSeparator);

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--projection", 0L},
                {"--field-separator", " "},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                {"--verbose", false}
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 2) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> <output-layout.{txt,csv}[.xz]>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            auto chart = acmacs::chart::import_factory(args[0], acmacs::chart::Verify::None);
            auto antigens = chart->antigens();
            auto sera = chart->sera();
            auto layout = chart->projection(args["--projection"])->layout();
            const std::string filename = args[1];
            if (string::ends_with(filename, ".csv") || string::ends_with(filename, ".csv.xz"))
                write_csv(filename, antigens, sera, layout);
            else
                write_text(filename, args["--field-separator"], antigens, sera, layout);
                // const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;
        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void write_csv(std::string aFilename, std::shared_ptr<acmacs::chart::Antigens> antigens, std::shared_ptr<acmacs::chart::Sera> sera, std::shared_ptr<acmacs::chart::Layout> layout)
{
    acmacs::CsvWriter writer;
    const auto number_of_dimensions = layout->number_of_dimensions();
    for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
        writer.add_field("AG");
        writer.add_field(antigen->full_name());
        for (size_t dim = 0; dim < number_of_dimensions; ++dim)
            writer.add_field(acmacs::to_string(layout->coordinate(ag_no, dim)));
        writer.new_row();
    }
    const auto number_of_antigens = antigens->size();
    for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
        writer.add_field("SR");
        writer.add_field(serum->full_name());
        for (size_t dim = 0; dim < number_of_dimensions; ++dim)
            writer.add_field(acmacs::to_string(layout->coordinate(sr_no + number_of_antigens, dim)));
        writer.new_row();
    }
    acmacs::file::write(aFilename, writer);

} // write_csv

// ----------------------------------------------------------------------

void write_text(std::string aFilename, std::string aFieldSeparator, std::shared_ptr<acmacs::chart::Antigens> antigens, std::shared_ptr<acmacs::chart::Sera> sera, std::shared_ptr<acmacs::chart::Layout> layout)
{
    std::string result;
    const auto number_of_dimensions = layout->number_of_dimensions();
    for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
        result += "AG" + aFieldSeparator + encode_name(antigen->full_name(), aFieldSeparator);
        for (size_t dim = 0; dim < number_of_dimensions; ++dim)
            result += aFieldSeparator + acmacs::to_string(layout->coordinate(ag_no, dim));
        result += '\n';
    }
    const auto number_of_antigens = antigens->size();
    for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
        result += "SR" + aFieldSeparator + encode_name(serum->full_name(), aFieldSeparator);
        for (size_t dim = 0; dim < number_of_dimensions; ++dim)
            result += aFieldSeparator + acmacs::to_string(layout->coordinate(sr_no + number_of_antigens, dim));
        result += '\n';
    }
    acmacs::file::write(aFilename, result);

} // write_text

// ----------------------------------------------------------------------

std::string encode_name(std::string aName, std::string aFieldSeparator)
{
    if (!aFieldSeparator.empty() && aFieldSeparator[0] == ' ')
        aName = string::replace(aName, " ", "_");
    return aName;

} // encode_name

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End: