/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <typeinfo>

#include <catch.hpp>

#include "D2D/typedefs.hpp"

namespace d2d
{
namespace tests
{

TEST_CASE( "Test typedefs", "[typedef]" )
{
    REQUIRE( typeid( Vector3 )          == typeid( boost::array< double, 3 > ) );
    REQUIRE( typeid( Vector6 )          == typeid( boost::array< double, 6 > ) );
    REQUIRE( typeid( StateHistory )     == typeid( std::map< double, Vector6 > ) );
    REQUIRE( typeid( ConfigIterator )   == typeid( rapidjson::Value::ConstMemberIterator ) );
}

} // namespace tests
} // namespace d2d
