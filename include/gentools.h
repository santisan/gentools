#include <array>
#include <concepts>
#include <experimental/coroutine>
#include <experimental/generator>
#include <iostream>
#include <optional>
#include <numeric>
#include <range/v3/range/primitives.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/generate.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/reverse.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace gentools
{
    template <typename T>
    using generator = std::experimental::generator<T>;

    template <typename T>
    using range_value_t = ranges::range_value_t<T>;

    template <typename Func, typename ... Args>
    concept invocable = requires { ranges::is_invocable_v<Func, Args ...>; };

    template <typename T>
    concept arithmetic = requires { std::is_arithmetic_v<T>; };

    template <arithmetic T>
    generator<T> count(T&& start = {}, T&& step = T{1})
    {
        for (auto value = start;; value = value + step)
        {
            co_yield value;
        }
    }

    template <ranges::range T>
    generator<range_value_t<T>> cycle(T&& range)
    {
        if (ranges::begin(range) == ranges::end(range))
        {
            return;
        }

        std::vector<range_value_t<T>> values;
        values.reserve(ranges::size(range));

        for (auto&& v : range)
        {
            co_yield v;
            values.push_back(v);
        }

        while (true)
        {
            for (auto&& v : values)
            {
                co_yield v;
            }
        }
    }

    /* 
       cycle function specialization for bidirectional ranges. 
       This implementation doesn't use an auxilliary buffer.
    */
    template <ranges::bidirectional_range T>
    generator<range_value_t<T>> cycle(T&& range)
    {
        while (true)
        {
            for (auto&& v : range)
            {
                co_yield v;
            }
        }
    }

    template <typename T>
    generator<T> repeat(T&& value)
    {
        while (true)
        {
            co_yield value;
        }
    }

    template <typename T>
    generator<T> repeat(T&& value, size_t times)
    {
        for (size_t i = 0; i < times; ++i)
        {
            co_yield value;
        }
    }

    template <ranges::range T, invocable F>
    inline constexpr generator<range_value_t<T>> accumulate(T&& range, F&& func, std::optional<range_value_t<T>> initial)
        //requires requires { ranges::is_invocable_v<F, range_value_t<T>, range_value_t<T>>; }
    {
        auto rangeStart = ranges::begin(range);
        const auto rangeEnd = ranges::end(range);

        if (rangeStart != rangeEnd)
        {
            range_value_t<T> accum = initial.value_or(*rangeStart);
            auto rangeIter = initial.has_value() ? rangeStart : ++rangeStart;

            for (; rangeIter != rangeEnd; ++rangeIter)
            {
                co_yield accum;
                accum = func(accum, *rangeIter);
            }

            co_yield accum;
        }
    }

    template <ranges::range T>
    inline constexpr generator<range_value_t<T>> accumulate(T&& range)
    {
        const auto initialValue = std::nullopt;
        return accumulate(range, [](auto x, auto y) { return x + y; }, initialValue);
    }

    template <ranges::range DataT, ranges::range SelectorsT>
    generator<range_value_t<DataT>> compress(DataT&& data, SelectorsT&& selectors)
        requires ranges::convertible_to<range_value_t<SelectorsT>, bool>
    {
        for (auto&& [value, _] : ranges::views::zip(data, selectors) | 
                                 ranges::views::filter([](auto&& pair) { return static_cast<bool>(pair.second); }))
        {
            co_yield value;
        }
    }

    template <int N, typename ... Ts>
    using param_list_element_t = std::tuple_element_t<N, std::tuple<Ts...>>;

    // TODO: require the same range_value_t for all input ranges
    // TODO 2: make a version without that restriction that returns a generator<variant<range_value_t<Ranges>..>> ???
    template <ranges::range ... Ranges>
    generator<range_value_t<param_list_element_t<0, Ranges...>>> chain(Ranges ... ranges)
    {
        using gen_list_t = std::vector<generator<range_value_t<param_list_element_t<0, Ranges...>>>>;
        constexpr size_t rangesSize = std::tuple_size_v<std::tuple<Ranges...>>;

        gen_list_t generators;
        generators.reserve(rangesSize);

        (generators.push_back(std::move(([](auto&& range) { for (auto&& v : range) co_yield v; })(ranges))), ...);

        for (auto&& gen : generators)
        {
            for (auto&& v : gen)
            {
                co_yield v;
            }
        }
    }

} //namespace gentools
