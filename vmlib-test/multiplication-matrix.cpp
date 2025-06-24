#include <catch2/catch_amalgamated.hpp>

#include "../vmlib/mat44.hpp"

// Test case to check if multiplication between two 4x4 matrices is working as intended
TEST_CASE("Multiplication of 4x4 matrix by matrix", "[mat44]")
{
	static constexpr float kEps_ = 1e-6f;

	// Left matrix for multiplication
	constexpr Mat44f testLeft = { {
		1.f, 6.f, 8.f, 3.f,
		5.f, 3.f, 2.f, 5.f,
		4.f, 2.f, 8.f, 9.f,
		2.f, 9.f, 3.f, 4.f
	} };

	// Right matrix for multiplication
	constexpr Mat44f testRight = { {
		6.f, 7.f, 8.f, 3.f,
		5.f, 2.f, 1.f, 4.f,
		2.f, 8.f, 4.f, 7.f,
		8.f, 9.f, 5.f, 6.f
	} };

	using namespace Catch::Matchers;

	// Standard 4x4 matrix-matrix multiplication test
	SECTION("Standard matrix-matrix multiplication")
	{
		auto const result = testLeft * testRight;

		REQUIRE_THAT(result(0, 0), WithinAbs(76.f, kEps_));
		REQUIRE_THAT(result(0, 1), WithinAbs(110.f, kEps_));
		REQUIRE_THAT(result(0, 2), WithinAbs(61.f, kEps_));
		REQUIRE_THAT(result(0, 3), WithinAbs(101.f, kEps_));

		REQUIRE_THAT(result(1, 0), WithinAbs(89.f, kEps_));
		REQUIRE_THAT(result(1, 1), WithinAbs(102.f, kEps_));
		REQUIRE_THAT(result(1, 2), WithinAbs(76.f, kEps_));
		REQUIRE_THAT(result(1, 3), WithinAbs(71.f, kEps_));

		REQUIRE_THAT(result(2, 0), WithinAbs(122.f, kEps_));
		REQUIRE_THAT(result(2, 1), WithinAbs(177.f, kEps_));
		REQUIRE_THAT(result(2, 2), WithinAbs(111.f, kEps_));
		REQUIRE_THAT(result(2, 3), WithinAbs(130.f, kEps_));

		REQUIRE_THAT(result(3, 0), WithinAbs(95.f, kEps_));
		REQUIRE_THAT(result(3, 1), WithinAbs(92.f, kEps_));
		REQUIRE_THAT(result(3, 2), WithinAbs(57.f, kEps_));
		REQUIRE_THAT(result(3, 3), WithinAbs(87.f, kEps_));
	}
}

// Test case to check if multiplication between 4x4 matrix and vector is working as intended
TEST_CASE("4x4 matrix by vector multiplication", "[mat44][vec4]")
{
	static constexpr float kEps_ = 1e-6f;

	// 4x4 matrix for multiplication
	constexpr Mat44f testMatrix = { {
		1.f, 6.f, 8.f, 3.f,
		5.f, 3.f, 2.f, 5.f,
		4.f, 2.f, 8.f, 9.f,
		2.f, 9.f, 3.f, 4.f
	} };

	// 4 vector for multiplication
	constexpr Vec4f testVector = {
		3.f, 2.f, 7.f, 4.f
	};

	using namespace Catch::Matchers;

	// Standard 4x4 matrix-vector multiplication test
	SECTION("Standard matrix-vector multiplication")
	{
		auto const resultVector = testMatrix * testVector;

		REQUIRE_THAT(resultVector[0], WithinAbs(83.f, kEps_));
		REQUIRE_THAT(resultVector[1], WithinAbs(55.f, kEps_));
		REQUIRE_THAT(resultVector[2], WithinAbs(108.f, kEps_));
		REQUIRE_THAT(resultVector[3], WithinAbs(61.f, kEps_));
	}
}