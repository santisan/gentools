#include <doctest/doctest.h>
#include <gentools.h>
#include <numeric>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <vector>

#if defined(_WIN32) || defined(WIN32)
// apparently this is required to compile in MSVC++
#include <sstream>
#endif

template <typename T>
auto genToVec(T&& gen)
{
	return ranges::make_subrange(std::forward<T>(gen)) | ranges::to<std::vector>;
}

TEST_SUITE("compress")
{
	TEST_CASE("compress with no selectors")
	{
		auto gen = gentools::compress(ranges::views::iota(1, 5), ranges::views::empty<bool>);
		const auto results = genToVec(gen);

		CHECK(results.empty());
	}

	TEST_CASE("compress with bool selectors")
	{
		const bool selectors[2] = { true, false };
		constexpr int size = 10;
		auto gen = gentools::compress(ranges::views::iota(1, size), selectors | ranges::views::cycle | ranges::views::take(size * 2));
		const auto results = genToVec(gen);

		const int expected[size / 2] = { 1, 3, 5, 7, 9 };
		CHECK(ranges::equal(results, expected));
	}

	TEST_CASE("compress with int selectors")
	{
		const int selectors[2] = { 0, 1};
		constexpr int size = 10;
		auto gen = gentools::compress(ranges::views::iota(1, size * 2), selectors | ranges::views::cycle | ranges::views::take(size));
		const auto results = genToVec(gen);

		const int expected[size / 2] = { 2, 4, 6, 8, 10 };
		CHECK(ranges::equal(results, expected));

		// This should fail to compile because compress requires the selector range's value type to be convertible to bool
		/*
		struct T {};
		static_assert(!std::is_convertible_v<T, bool>);
		std::vector<T> selectors2(10);
		for (int i : gentools::compress(ranges::views::iota(1, 10), selectors2))
		{
			results.push_back(i);
		}
		*/
	}
}

TEST_SUITE("accumulate")
{
	TEST_CASE("accumulate empty range")
	{
		auto gen = gentools::accumulate(ranges::views::empty<int>);
		const auto results = genToVec(gen);
		CHECK(results.empty());
	}

	TEST_CASE("accumulate default args")
	{
		auto gen = gentools::accumulate(ranges::views::iota(5, 10));
		const auto results = genToVec(gen);

		const int expected[5] = { 5, 11, 18, 26, 35 };
		CHECK(ranges::equal(results, expected));
	}

	TEST_CASE("accumulate with function and initial value")
	{
		const int initialValue = 1;
		auto gen = gentools::accumulate(ranges::views::iota(5, 9), [](auto x, auto y) { return x * y; }, initialValue);
		const auto results = genToVec(gen);

		const int expected[5] = { initialValue, 5, 30, 210, 1680 };
		CHECK(ranges::equal(results, expected));
	}

	TEST_CASE("accumulate string")
	{
		const std::string seed[2] = { "AB", "CD" };
		const std::string expected[4] = { "AB", "ABCD", "ABCDAB", "ABCDABCD" };
		{
			auto gen = gentools::accumulate(seed | ranges::views::cycle | ranges::views::take(4));
			const auto results = genToVec(gen);

			CHECK(ranges::equal(results, expected));
		}
		{
			auto gen = gentools::accumulate(seed | ranges::views::cycle);
			const auto results = ranges::make_subrange(gen) | ranges::views::take(4) | ranges::to<std::vector>;
			
			CHECK(ranges::equal(results, expected));
		}
	}
}

TEST_SUITE("repeat")
{
	TEST_CASE("repeat")
	{
		auto r1 = gentools::repeat(2.4, 3);
		std::vector<double> v1{r1.begin(), r1.end()};

		const double expected[3] = { 2.4, 2.4, 2.4 };
		CHECK(ranges::equal(v1, expected));
	}

	TEST_CASE("repeat infinite")
	{
		auto gen = gentools::repeat(2.4);
		auto results = ranges::make_subrange(gen) | ranges::views::take(3) | ranges::to<std::vector>;

		const double expected[3] = { 2.4, 2.4, 2.4 };
		CHECK(ranges::equal(results, expected));
	}
}

TEST_SUITE("count")
{
	TEST_CASE("count default args")
	{
		auto gen = gentools::count<int>();
		const auto results = ranges::make_subrange(gen) | ranges::views::take(5) | ranges::to<std::vector>;

		const int expected[5] = { 0, 1, 2, 3, 4 };
		CHECK(ranges::equal(results, expected));
	}

	TEST_CASE("count")
	{
		auto gen = gentools::count(1.f, 2.f);
		const auto results = ranges::make_subrange(gen) | ranges::views::take(4) | ranges::to<std::vector>;

		const float expected[4] = { 1.f, 3.f, 5.f, 7.f };
		CHECK(ranges::equal(results, expected));
	}
}

TEST_SUITE("cycle")
{
	// calling cycle causes a weird compilation error in range/v3/iterator/concepts.hpp
	/*TEST_CASE("cycle empty range")
	{
		std::vector<int> results;
		auto gen = gentools::cycle(ranges::views::iota(1, 3));
		for (int i : ranges::make_subrange(gen) | ranges::views::take(6))
		{
			results.push_back(i);
		}
		const int expected[6] = { 1, 2, 1, 2, 1, 2 };
		CHECK(ranges::equal(results, expected));
	}*/
}

TEST_SUITE("chain")
{
	TEST_CASE("chain")
	{
		auto gen = gentools::chain(ranges::views::iota(1, 3), ranges::views::iota(3, 5), ranges::views::iota(5, 7));
		const auto results = genToVec(gen);
		
		const int expected[6] = { 1, 2, 3, 4, 5, 6 };
		CHECK(ranges::equal(results, expected));

	}
}
