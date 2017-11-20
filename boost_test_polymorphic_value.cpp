#define BOOST_TEST_MODULE polymorphic_value
#include "polymorphic_value.h"
#include <boost/test/included/unit_test.hpp>

#include <catch.hpp>
#include <new>
#include <stdexcept>

using boost::polymorphic_value;

class Shape {};

BOOST_AUTO_TEST_CASE(empty_upon_default_construction) {
  polymorphic_value<Shape> pv;

  BOOST_TEST(!bool(pv));
}

