#pragma once

#include <memory>

#include "acmacs-base/color.hh"
#include "acmacs-chart/base.hh"
#include "acmacs-chart/passage.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    namespace internal
    {
        template <typename Parent, typename Reference> class iterator
        {
         public:
            using reference = Reference;

            inline iterator& operator++() { ++mIndex; return *this; }
            inline bool operator!=(const iterator& other) const { return &mParent != &other.mParent || mIndex != other.mIndex; }
            inline reference operator*() { return mParent[mIndex]; }

         private:
            inline iterator(const Parent& aParent, size_t aIndex) : mParent{aParent}, mIndex{aIndex} {}

            const Parent& mParent;
            size_t mIndex;

            friend Parent;
        };

    } // namespace internal

// ----------------------------------------------------------------------

    class Info
    {
      public:
        virtual ~Info();

        virtual std::string make_info() const = 0;

    }; // class Info

// ----------------------------------------------------------------------

    class Name : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

    }; // class Name

    class Date : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

    }; // class Date

    enum class BLineage { Unknown, Victoria, Yamagata };

    class LabIds : public internal::string_list_data
    {
     public:
        using internal::string_list_data::string_list_data;

    }; // class LabIds

    class Annotations : public internal::string_list_data
    {
     public:
        using internal::string_list_data::string_list_data;

    }; // class Annotations

    class Clades : public internal::string_list_data
    {
     public:
        using internal::string_list_data::string_list_data;

    }; // class Clades

    class SerumId : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

    }; // class SerumId

    class SerumSpecies : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

    }; // class SerumSpecies

    class Titer : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

    }; // class Titer

    class DrawingOrder : public internal::index_list_data
    {
     public:
        using internal::index_list_data::index_list_data;

    }; // class DrawingOrder

// ----------------------------------------------------------------------

    class Antigen
    {
      public:
        virtual ~Antigen();

        virtual Name name() const = 0;
        virtual Date date() const = 0;
        virtual Passage passage() const = 0;
        virtual BLineage lineage() const = 0;
        virtual Reassortant reassortant() const = 0;
        virtual LabIds lab_ids() const = 0;
        virtual Clades clades() const = 0;
        virtual Annotations annotations() const = 0;
        virtual bool reference() const = 0;

    }; // class Antigen

// ----------------------------------------------------------------------

    class Serum
    {
      public:
        virtual ~Serum();

        virtual Name name() const = 0;
        virtual Passage passage() const = 0;
        virtual BLineage lineage() const = 0;
        virtual Reassortant reassortant() const = 0;
        virtual Annotations annotations() const = 0;
        virtual SerumId serum_id() const = 0;
        virtual SerumSpecies serum_species() const = 0;

    }; // class Serum

// ----------------------------------------------------------------------

    class Antigens
    {
      public:
        virtual ~Antigens();

        virtual size_t size() const = 0;
        virtual std::shared_ptr<Antigen> operator[](size_t aIndex) const = 0;
        using iterator = internal::iterator<Antigens, std::shared_ptr<Antigen>>;
        inline iterator begin() const { return {*this, 0}; }
        inline iterator end() const { return {*this, size()}; }

    }; // class Antigens

// ----------------------------------------------------------------------

    class Sera
    {
      public:
        virtual ~Sera();

        virtual size_t size() const = 0;
        virtual std::shared_ptr<Serum> operator[](size_t aIndex) const = 0;
        using iterator = internal::iterator<Sera, std::shared_ptr<Serum>>;
        inline iterator begin() const { return {*this, 0}; }
        inline iterator end() const { return {*this, size()}; }

    }; // class Sera

// ----------------------------------------------------------------------

    class Titers
    {
      public:
        virtual ~Titers();

        virtual Titer titer(size_t aAntigenNo, size_t aSerumNo) const = 0;
        virtual Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const = 0;
        virtual size_t number_of_layers() const = 0;

    }; // class Titers

// ----------------------------------------------------------------------

    class ForcedColumnBases
    {
      public:
        virtual ~ForcedColumnBases();

        virtual bool exists() const = 0;
        inline operator bool() const { return exists(); }
        virtual double column_basis(size_t aSerumNo) const = 0;

    }; // class ForcedColumnBases

// ----------------------------------------------------------------------

    class Projection
    {
      public:
        virtual ~Projection();

        virtual std::string make_info() const;
        virtual double stress() const = 0;
        virtual size_t number_of_dimensions() const = 0;

    }; // class Projection

// ----------------------------------------------------------------------

    class Projections
    {
      public:
        virtual ~Projections();

        virtual bool empty() const = 0;
        virtual size_t size() const = 0;
        virtual std::shared_ptr<Projection> operator[](size_t aIndex) const = 0;
        using iterator = internal::iterator<Projections, std::shared_ptr<Projection>>;
        inline iterator begin() const { return {*this, 0}; }
        inline iterator end() const { return {*this, size()}; }

        virtual std::string make_info() const;

    }; // class Projections

// ----------------------------------------------------------------------

    class PointStyle
    {
      public:
        virtual ~PointStyle();

        virtual bool shown() const = 0;
        virtual Color fill() const = 0;
        virtual Color outline() const = 0;
        virtual double outline_width() const = 0;
        virtual double size() const = 0;
        // virtual Shape shape() const = 0;
        // virtual Rotation rotation() const = 0;
        // virtual Aspect aspect() const = 0;
        // virtual bool label_shown() const = 0;
        // virtual Offset label_offset() const = 0;
        // virtual std::string label_text() const = 0;
        // virtual TextStyle label_style() const = 0;
        // virtual double label_size() const = 0;
        // virtual Color label_color() const = 0;
        // virtual Rotation label_rotation() const = 0;
        // virtual double label_interline() const = 0;

    }; // class PointStyle

// ----------------------------------------------------------------------

    class PlotSpec
    {
      public:
        virtual ~PlotSpec();

        virtual DrawingOrder drawing_order() const = 0;
        virtual Color error_line_positive_color() const = 0;
        virtual Color error_line_negative_color() const = 0;
        virtual std::shared_ptr<PointStyle> style(size_t aPointNo) const = 0;

    }; // class PlotSpec

// ----------------------------------------------------------------------

    class Chart
    {
      public:
        virtual ~Chart();

        virtual std::shared_ptr<Info> info() const = 0;
        virtual std::shared_ptr<Antigens> antigens() const = 0;
        virtual std::shared_ptr<Sera> sera() const = 0;
        virtual std::shared_ptr<Titers> titers() const = 0;
        virtual std::shared_ptr<ForcedColumnBases> forced_column_bases() const = 0;
        virtual std::shared_ptr<Projections> projections() const = 0;
        virtual std::shared_ptr<PlotSpec> plot_spec() const = 0;

        inline size_t number_of_antigens() const { return antigens()->size(); }
        inline size_t number_of_sera() const { return sera()->size(); }
        inline size_t number_of_projections() const { return projections()->size(); }

        std::string make_info() const;

    }; // class Chart

} // namespace acmacs::chart

// ----------------------------------------------------------------------

template <> struct std::iterator_traits<acmacs::chart::Antigens::iterator> { using reference = typename acmacs::chart::Antigens::iterator::reference; };
template <> struct std::iterator_traits<acmacs::chart::Sera::iterator> { using reference = typename acmacs::chart::Sera::iterator::reference; };
template <> struct std::iterator_traits<acmacs::chart::Projections::iterator> { using reference = typename acmacs::chart::Projections::iterator::reference; };

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
