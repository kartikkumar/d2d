/*
 * Copyright (c) 2015 K. Kumar (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#define CATCH_CONFIG_MAIN

#include <catch.hpp>

namespace d2d
{
namespace tests
{

TEST_CASE( "Dummy", "[dummy]" )
{
    REQUIRE( 1 == 1 );
}
} // namespace tests
} // namespace d2d