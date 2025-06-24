
#include <catch2/catch_amalgamated.hpp>

#include "../vmlib/mat44.hpp"

// Test case to verify matrix translations are working as expected
TEST_CASE("Translations", "[translation][mat44]")
{
	static constexpr float kEps_ = 1e-6f;

	using namespace Catch::Matchers;

	// Verify that matrix is translated along the x-axis as intended
	SECTION("Translate through X")
	{
		auto const val = make_translation({ 10.f, 0.f, 0.f });

		REQUIRE_THAT(val(0, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(val(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(0, 3), WithinAbs(10.f, kEps_));

		REQUIRE_THAT(val(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(1, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(val(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(val(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(2, 2), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(val(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(val(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(3, 3), WithinAbs(1.f, kEps_));
	}

	// Verify that matrix is translated along the y-axis as intended
	SECTION("Translate through Y")
	{
		auto const val = make_translation({ 0.f, 10.f, 0.f });

		REQUIRE_THAT(val(0, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(val(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(val(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(1, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(val(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(1, 3), WithinAbs(10.f, kEps_));

		REQUIRE_THAT(val(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(2, 2), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(val(2, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(val(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(3, 3), WithinAbs(1.f, kEps_));
	}

	// Verify that matrix is translated along the y-axis as intended
	SECTION("Translate through Z")
	{
		auto const val = make_translation({ 0.f, 0.f, 15.f });

		REQUIRE_THAT(val(0, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(val(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(0, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(val(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(1, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(val(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(1, 3), WithinAbs(0.f, kEps_));

		REQUIRE_THAT(val(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(2, 2), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(val(2, 3), WithinAbs(15.f, kEps_));

		REQUIRE_THAT(val(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(3, 3), WithinAbs(1.f, kEps_));
	}

	// Check that matrix is tranlated along all axes properly
	SECTION("Translate through XYZ ")
	{
		auto const val = make_translation({ 4.f, 5.f, 6.f });

		REQUIRE_THAT(val(0, 0), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(val(0, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(0, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(0, 3), WithinAbs(4.f, kEps_));

		REQUIRE_THAT(val(1, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(1, 1), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(val(1, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(1, 3), WithinAbs(5.f, kEps_));

		REQUIRE_THAT(val(2, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(2, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(2, 2), WithinAbs(1.f, kEps_));
		REQUIRE_THAT(val(2, 3), WithinAbs(6.f, kEps_));

		REQUIRE_THAT(val(3, 0), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(3, 1), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(3, 2), WithinAbs(0.f, kEps_));
		REQUIRE_THAT(val(3, 3), WithinAbs(1.f, kEps_));
	}
}
