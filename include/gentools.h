#include <array>
#include <concepts>
#include <experimental/coroutine>
#include <experimental/generator>
#include <iostream>
#include <functional>
#include <optional>
#include <numeric>
#include <range/v3/range/primitives.hpp>
#include <range/v3/view/drop_while.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/generate.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/reverse.hpp>
#include <range/v3/view/subrange.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/take_while.hpp>
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

    template <ranges::range T>
    generator<range_value_t<T>> to_generator(T&& range)
    {
        for (auto&& value : range)
        {
            co_yield value;
        }
    }

    template <ranges::range T, invocable F>
    using range_value_invoke_result_t = std::invoke_result_t<F, range_value_t<T>>;

    template <ranges::range T, invocable F>
    generator<range_value_invoke_result_t<T, F>> transform(T&& range, F&& func)
    {
        for (auto&& value : range)
        {
            co_yield func(value);
        }
    }

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

    template <int N, typename ... Ts>
    using ranges_element_value_t = range_value_t<param_list_element_t<N, Ts...>>;

    // TODO: require the same range_value_t for all input ranges (with concepts)
    template <ranges::range ... Ts>
    generator<ranges_element_value_t<0, Ts...>> chain(Ts&& ... ranges)
    {
        using gen_t = generator<ranges_element_value_t<0, Ts...>>;
        using gen_list_t = std::vector<gen_t>;
        constexpr size_t rangesSize = std::tuple_size_v<std::tuple<Ts...>>;

        gen_list_t generators;
        generators.reserve(rangesSize);

        (generators.push_back(std::move(to_generator(ranges))), ...);

        for (auto&& gen : generators)
        {
            for (auto&& v : gen)
            {
                co_yield v;
            }
        }
    }

    template <typename ... Ts>
    struct typelist {};

    template <typename A, template <typename ...> typename B>
    struct rename_impl;

    template <template <typename ...> typename A, typename ... Ts, template <typename ...> typename B>
    struct rename_impl<A<Ts...>, B>
    {
        using type = B<Ts...>;
    };

    template <typename A, template <typename ...> typename B>
    using rename_t = typename rename_impl<A, B>::type;

    template <template<typename> typename MetaFunc, typename ... Ts>
    struct transform_impl;

    template <template<typename> typename MetaFunc, typename ... Ts>
    struct transform_impl<MetaFunc, Ts...>
    {
        using type = typelist<MetaFunc<Ts>...>;
    };

    template <template<typename> typename MetaFunc, typename ... Ts>
    using transform_t = typename transform_impl<MetaFunc, Ts...>::type;

    // TODO: name this just 'chain' like the one above
    template <ranges::range ... Ts>
    auto chain_heterogeneous(Ts&& ... ranges) -> generator<rename_t<transform_t<range_value_t, Ts...>, std::variant>>
    {
        using ranges_value_type_list_t = transform_t<range_value_t, Ts...>;
        using gen_value_t = rename_t<ranges_value_type_list_t, std::variant>;
        using gen_list_t = std::vector<generator<gen_value_t>>;
        constexpr size_t rangesSize = std::tuple_size_v<std::tuple<Ts...>>;

        gen_list_t generators;
        generators.reserve(rangesSize);
        
        auto rangeValueToVariantFunc = [](auto&& v) { return gen_value_t(v); };
        (generators.push_back(std::move(transform(ranges, rangeValueToVariantFunc))), ...);

        for (auto&& gen : generators)
        {
            for (auto&& v : gen)
            {
                co_yield v;
            }
        }
    }

    template <ranges::range T, invocable F>
    generator<range_value_t<T>> take_while(T&& range, F&& pred)
    {
        for (auto&& value : ranges::views::take_while(range, pred))
        {
            co_yield value;
        }
    }

    template <ranges::range T, invocable F>
    generator<range_value_t<T>> drop_while(T&& range, F&& pred)
    {
        for (auto&& value : ranges::views::drop_while(range, pred))
        {
            co_yield value;
        }
    }

    template <ranges::range T, invocable F>
    generator<range_value_t<T>> filter(T&& range, F&& pred)
    {
        for (auto&& value : ranges::views::filter(range, pred))
        {
            co_yield value;
        }
    }

    template <ranges::range T, invocable F>
    using group_key_t = range_value_invoke_result_t<T, F>;

    template <ranges::range T, invocable F>
    using group_t = std::pair<group_key_t<T, F>, ranges::safe_subrange_t<T>>;

    template <ranges::range T>
    struct Identity
    {
        range_value_t<T> operator()(range_value_t<T>&& value)
        {
            return value;
        }
    };

    // Input range should be already ordered by the key
    template <ranges::range T, invocable F>
    generator<group_t<T, F>> group_by(T&& range, F&& keyFunc = Identity<T>{})
    {
        auto iter = ranges::begin(range);
        const auto rangeEnd = ranges::end(range);
        if (iter == rangeEnd)
        {
            return;
        }

        auto groupStartIter = iter;
        auto currentKey = keyFunc(*iter);

        while ((++iter) != rangeEnd)
        {
            const auto newKey = keyFunc(*iter);
            if (newKey != currentKey)
            {
                co_yield std::make_pair(currentKey, ranges::make_subrange(groupStartIter, iter));
                
                currentKey = newKey;
                groupStartIter = iter;
            }
        }

        co_yield std::make_pair(currentKey, ranges::make_subrange(groupStartIter, iter));
    }

    template <ranges::range T, invocable F>
    generator<range_value_invoke_result_t<T, F>> star_transform(T&& range, F&& func)
        requires ranges::is_invocable_v<F, range_value_t<T>>
    {
        for (auto&& value : range)
        {
            co_yield func(value);
        }
    }

} //namespace gentools
