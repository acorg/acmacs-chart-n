#pragma once

#include <variant>
#include <string>
#include <vector>
#include <stdexcept>
#include <typeinfo>

#include "acmacs-base/string.hh"

// ----------------------------------------------------------------------

namespace acmacs::lispmds
{
    class error : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class type_mismatch : public error { public: using error::error; };
    class keyword_no_found : public error { public: using error::error; };
    class keyword_has_no_value : public error { public: using error::error; };

    class nil
    {
     public:
        inline nil() = default;

    }; // class nil

    class boolean
    {
     public:
        inline boolean(bool aValue = false) : mValue{aValue} {}

        inline operator bool() const { return mValue; }

     private:
        bool mValue;

    }; // class boolean

    class number
    {
     public:
        inline number() = default;
        inline number(std::string aValue) : mValue(aValue)
            {
                for (auto& c: mValue) {
                    switch (c) {
                      case 'd': case 'D':
                          c = 'e';
                          break;
                      default:
                          break;
                    }
                }
            }
        inline number(const std::string_view& aValue) : number(std::string{aValue}) {}

        inline operator double() const { return std::stod(mValue); }
        inline operator float() const { return std::stof(mValue); }
        inline operator unsigned long() const { return std::stoul(mValue); }
        inline operator long() const { return std::stol(mValue); }

     private:
        std::string mValue;

    }; // class number

    namespace detail
    {
        class string
        {
         public:
            inline string() = default;
            inline string(std::string aValue) : mValue(aValue) {}
            inline string(const std::string_view& aValue) : mValue(aValue) {}

            inline operator std::string() const { return mValue; }
            inline char operator[](size_t index) const { return mValue.at(index); }
            inline bool operator==(const string& s) const { return mValue == s.mValue; }
            inline bool operator!=(const string& s) const { return mValue != s.mValue; }
            inline bool operator==(std::string s) const { return mValue == s; }
            inline bool operator!=(std::string s) const { return mValue != s; }

         private:
            std::string mValue;

        }; // class string

    } // namespace detail

    class string : public detail::string { public: using detail::string::string; };

    class symbol : public detail::string { public: using detail::string::string; };

    class keyword : public detail::string { public: using detail::string::string; };

    class list;

    using value = std::variant<nil, boolean, number, string, symbol, keyword, list>; // nil must be the first alternative, it is the default value;

    class list
    {
     public:
        inline list() = default;

        using iterator = decltype(std::declval<const std::vector<value>>().begin());
        using reverse_iterator = decltype(std::declval<const std::vector<value>>().rbegin());
        inline iterator begin() const { return mContent.begin(); }
        inline iterator end() const { return mContent.end(); }
        inline iterator begin() { return mContent.begin(); }
        inline iterator end() { return mContent.end(); }
        inline reverse_iterator rbegin() const { return mContent.rbegin(); }

        inline value& append(value&& to_add)
            {
                mContent.push_back(std::move(to_add));
                return mContent.back();
            }

        inline const value& operator[](size_t aIndex) const
            {
                return mContent.at(aIndex);
            }

        const value& operator[](std::string_view aKeyword) const;

        inline size_t size() const { return mContent.size(); }
        inline bool empty() const { return mContent.empty(); }

     private:
        std::vector<value> mContent;

    }; // class list


// ----------------------------------------------------------------------

    inline value& append(value& target, value&& to_add)
    {
        return std::visit([&](auto&& arg) -> value& {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg.append(std::move(to_add));
            else
                throw type_mismatch{"not a lispmds::list, cannot append value"};
        }, target);
    }

    inline const value& get_(const value& val, size_t aIndex)
    {
        using namespace std::string_literals;
        return std::visit([aIndex](const auto& arg) -> const value& {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg[aIndex];
            else
                throw type_mismatch{"not a lispmds::list, cannot use [index]: "s + typeid(arg).name()};
        }, val);
    }

    inline const value& get_(const value& val, int aIndex)
    {
        return get_(val, static_cast<size_t>(aIndex));
    }

    inline const value& get_(const value& val, std::string aKeyword)
    {
        return std::visit([aKeyword](auto&& arg) -> const value& {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg[aKeyword];
            else
                throw type_mismatch{"not a lispmds::list, cannot use [keyword]"};
        }, val);
    }

    template <typename Arg, typename... Args> inline const value& get(const value& val, Arg&& arg, Args&&... args)
    {
        if constexpr (sizeof...(args) == 0) {
            return get_(val, std::forward<Arg>(arg));
        }
        else {
            return get(get_(val, std::forward<Arg>(arg)), args...);
        }
    }

    inline size_t size(const value& val)
    {
        return std::visit([](auto&& arg) -> size_t {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg.size();
            else if constexpr (std::is_same_v<T, nil>)
                return 0;
            else
                throw type_mismatch{"not a lispmds::list, cannot use size()"};
        }, val);
    }

    template <typename... Args> inline size_t size(const value& val, Args&&... args)
    {
        return size(get(val, args...));
    }

    inline bool empty(const value& val)
    {
        return std::visit([](auto&& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg.empty();
            else if constexpr (std::is_same_v<T, nil>)
                return true;
            else
                throw type_mismatch{"not a lispmds::list, cannot use empty()"};
        }, val);
    }

    template <typename... Args> inline bool empty(const value& val, Args&&... args)
    {
        return empty(get(val, args...));
    }

// ----------------------------------------------------------------------

    inline const value& list::operator[](std::string_view aKeyword) const
    {
        auto p = mContent.begin();
        while (p != mContent.end() && (!std::get_if<keyword>(&*p) || std::get<keyword>(*p) != aKeyword))
            ++p;
        if (p == mContent.end())
            throw keyword_no_found{std::string{aKeyword}};
        ++p;
        if (p == mContent.end())
            throw keyword_has_no_value{std::string{aKeyword}};
        return *p;
    }

// ----------------------------------------------------------------------

    value parse_string(const std::string_view& aData);

} // namespace acmacs::lispmds

// ----------------------------------------------------------------------

namespace acmacs
{
    inline std::string to_string(const lispmds::nil&)
    {
        return "nil";
    }

    inline std::string to_string(const lispmds::boolean& val)
    {
        return val ? "t" : "f";
    }

    inline std::string to_string(const lispmds::number& val)
    {
        return acmacs::to_string(static_cast<double>(val));
    }

    inline std::string to_string(const lispmds::string& val)
    {
        return '"' + static_cast<std::string>(val) + '"';
    }

    inline std::string to_string(const lispmds::symbol& val)
    {
        return '\'' + static_cast<std::string>(val);
    }

    inline std::string to_string(const lispmds::keyword& val)
    {
        return static_cast<std::string>(val);
    }

    std::string to_string(const lispmds::value& val);

    inline std::string to_string(const lispmds::list& list)
    {
        std::string result{"(\n"};
        for (const lispmds::value& val: list) {
            result.append(to_string(val));
            result.append(1, '\n');
        }
        result.append(")\n");
        return result;
    }

    inline std::string to_string(const lispmds::value& val)
    {
        return std::visit([](auto&& arg) -> std::string { return to_string(arg); }, val);
    }

} // namespace acmacs

// ----------------------------------------------------------------------

inline std::ostream& operator<<(std::ostream& s, const acmacs::lispmds::value& val)
{
    return s << acmacs::to_string(val);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
