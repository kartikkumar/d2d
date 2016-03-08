/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#ifndef D2D_TYPEDEFS_HPP
#define D2D_TYPEDEFS_HPP

#include <map>

#include <boost/array.hpp>

#include <rapidjson/document.h>

namespace d2d
{

//! 3-Vector.
typedef boost::array< double, 3 > Vector3;

//! 6-Vector.
typedef boost::array< double, 6 > Vector6;

//! State history.
typedef std::map< double, Vector6 > StateHistory;

//! JSON config iterator.
typedef rapidjson::Value::ConstMemberIterator ConfigIterator;

} // namespace d2d

#endif // D2D_CATALOG_PRUNER_HPP