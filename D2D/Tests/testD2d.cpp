/*    
 * Copyright (c) 2014, K. Kumar (me@kartikkumar.com)
 * All rights reserved.
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