// RangesTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <concepts>
#include <experimental/coroutine>
#include <experimental/generator>
#include <iostream>
#include <json/json.hpp>
#include <pipe/algorithm.h>
#include <range/v3/view.hpp>
#include <string>
#include <type_traits>
#include <vector>

#include <gentools.h>

void print(const nlohmann::json& js)
{
    for (auto& [key, value] : js.items())
    {
        if (value.is_object())
        {
            std::cout << "object: ";
            print(value);
        }
        else
        {
            std::cout << "k: " << key << " - v: " << value << '\n';
        }
    }
}

using json_ref = std::reference_wrapper<nlohmann::json>;
using json_value = std::variant<double, uint64_t, int64_t, bool, nullptr_t, std::string, json_ref>;

json_value convert(nlohmann::json& json)
{
    if (json.is_structured()) return {std::ref(json)};
    if (json.is_null()) return {nullptr};
    if (json.is_boolean()) return {json.get<bool>()};
    if (json.is_number_float()) return {json.get<double>()};
    if (json.is_number_unsigned()) return {json.get<uint64_t>()};
    if (json.is_number_integer()) return {json.get<int64_t>()};
    if (json.is_string()) return {json.get<std::string>()}; //{std::string_view{json.get_ref<std::string&>()}};
    return {nullptr};
}

struct traverse_recursive_fn
{
    auto operator()(nlohmann::json& jsonRoot) const -> gentools::generator<std::pair<std::string, json_value>>
    {
        for (auto&& item : jsonRoot.items())
        {
            co_yield std::make_pair(item.key(), convert(item.value()));

            if (item.value().is_object())
            {
                for (auto&& [key, value] : (*this)(item.value()))
                {
                    co_yield std::make_pair(std::move(key), std::move(value));
                }
            }
        }
    }
};

inline constexpr traverse_recursive_fn traverse_recursive{};

struct traverse_fn
{
    auto operator()(nlohmann::json& jsonRoot) const -> gentools::generator<std::pair<std::string, json_value>>
    {
        using namespace nlohmann;
        std::vector<std::pair<std::string, json_ref>> stack{};
        stack.reserve(jsonRoot.size());
    
        auto pushToStack = [&stack](json& jsonObject)
        {
            //std::cout << " - pustToStack ";
            for (auto&& item : jsonObject.items())
            {
                //std::cout << item.key() << ' ' << item.value();
                stack.emplace_back(item.key(), item.value());
            }
            //std::cout << '\n';
        };
        
        pushToStack(jsonRoot);
    
        while (!stack.empty())
        {
            auto [jsonKey, jsonRef] = stack.back();
            stack.pop_back();
    
            if (jsonRef.get().is_object())
            {
                //std::cout << jsonValue;
                pushToStack(jsonRef.get());
            }
            
            co_yield std::make_pair(/*std::move*/(jsonKey), convert(jsonRef.get()));
        }
    }
};

inline constexpr traverse_fn traverse{};

namespace gentools
{
    template <typename Range>
    using gen_value_t = typename Range::iterator::value_type;
    
    struct take_gen
    {
        size_t mCount = 0;
    
        template <typename Range>
        gentools::generator<gen_value_t<Range>> operator()(Range range)
        {
            size_t i = 0;
            for (auto&& value : range)
            {
                if (i++ < mCount)
                {
                    co_yield value;
                }
                else
                {
                    break;
                }
            }
        }
    };
    
    auto take(size_t count)
    {
        return take_gen{count};
    }
    
    template <typename Range, typename Func>
    auto operator|(Range range, Func&& func)
    {
        return func(std::move(range));
    }
}


int main()
{
    nlohmann::json jdata = {
        {"pi", 3.141},
        {"happy", true},
        {"name", "Niels"},
        {"nothing", nullptr},
        {"answer", {
            {"everything", 42}
        }},
        {"list", {1, 0, 2}},
        {"object", {
            {"currency", "USD"},
            {"value", 42.99}
        }}
    };

    //print(jdata);
    
    //for (auto&& [jsonKey, jsonValue] : jdata | ranges::make_view_closure(traverse) | ranges::views::take(2))
    auto gen = traverse_recursive(jdata);
    for (auto&& [jsonKey, jsonValue] : traverse_recursive(jdata) | gentools::take(2))
    {
        std::cout << jsonKey << " asd "/* << value.second*/ << '\n';
        //std::cout << (jsonItem.value().is_object() ? jsonItem.key() : "k") << " - " << jsonItem.value() << '\n';
    }

    //auto query = from(jdata).where(KEY, equals, "pi").select(ALL);
    //auto result = query();
    //// result == {"pi", 3.141}
    //
    //auto query = from(jdata).where(KEY, equals, "happy").select(VALUE);
    //auto result = query();
    //// result == true

    return 0;
}

