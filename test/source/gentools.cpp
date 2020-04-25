#include <doctest/doctest.h>
#include <gentools.h>
#include <numeric>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/view.hpp>
#include <vector>

#if defined(_WIN32) || defined(WIN32)
// apparently this is required to compile in MSVC++
#include <sstream>
#endif

TEST_SUITE("compress")
{
	TEST_CASE("compress with no selectors")
	{
		std::vector<int> results;
		for (int i : gentools::compress(ranges::views::iota(1, 5), ranges::views::empty<bool>))
		{
			results.push_back(i);
		}
		CHECK(results.empty());
	}

	TEST_CASE("compress with bool selectors")
	{
		const bool selectors[2] = { true, false };
		constexpr int size = 10;
		std::vector<int> results;
		results.reserve(size / 2);

		for (int i : gentools::compress(ranges::views::iota(1, size), selectors | ranges::views::cycle | ranges::views::take(size * 2)))
		{
			results.push_back(i);
		}

		const int expected[size / 2] = { 1, 3, 5, 7, 9 };
		CHECK(ranges::equal(results, expected));
	}

	TEST_CASE("compress with int selectors")
	{
		const int selectors[2] = { 0, 1};
		constexpr int size = 10;
		std::vector<int> results;
		results.reserve(size / 2);

		for (int i : gentools::compress(ranges::views::iota(1, size * 2), selectors | ranges::views::cycle | ranges::views::take(size)))
		{
			results.push_back(i);	
		}
	
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
		std::vector<int> results;
		for (int i : gentools::accumulate(ranges::views::empty<int>))
		{
			results.push_back(i);
		}
		CHECK(results.empty());
	}

	TEST_CASE("accumulate default args")
	{
		std::vector<int> results;
		for (int i : gentools::accumulate(ranges::views::iota(5, 10)))
		{
			results.push_back(i);
		}

		const int expected[5] = { 5, 11, 18, 26, 35 };
		CHECK(ranges::equal(results, expected));
	}

	TEST_CASE("accumulate with function and initial value")
	{
		std::vector<int> results;
		const int initialValue = 1;

		for (int i : gentools::accumulate(ranges::views::iota(5, 9), [](auto x, auto y) { return x * y; }, initialValue))
		{
			results.push_back(i);
		}

		const int expected[5] = { initialValue, 5, 30, 210, 1680 };
		CHECK(ranges::equal(results, expected));
	}

	TEST_CASE("accumulate string")
	{
		std::vector<std::string> results;
		const std::string seed[2] = { "AB", "CD" };
		for (std::string i : gentools::accumulate(seed | ranges::views::cycle | ranges::views::take(4)))
		{
			results.push_back(i);
		}

		const std::string expected[4] = { "AB", "ABCD", "ABCDAB", "ABCDABCD" };
		CHECK(ranges::equal(results, expected));
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
		std::vector<double> results;
		auto r = gentools::repeat(2.4);
		for (double i : ranges::make_subrange(r.begin(), r.end()) | ranges::views::take(3))
		{
			results.push_back(i);
		}

		const double expected[3] = { 2.4, 2.4, 2.4 };
		CHECK(ranges::equal(results, expected));
	}
}
