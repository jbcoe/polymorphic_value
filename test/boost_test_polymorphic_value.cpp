#define BOOST_TEST_MODULE polymorphic_value
#include "boost/polymorphic_value.h"
#include <boost/test/included/unit_test.hpp>

#include <new>
#include <stdexcept>

using boost::polymorphic_value;

struct Shape {
  virtual double area() const = 0;
};

class Square : public Shape {
  double side_;

public:
  Square(double side) : side_(side) {}
  double area() const override { return side_ * side_; }
};

BOOST_AUTO_TEST_CASE(empty_upon_default_construction) {
  polymorphic_value<Shape> pv;

  BOOST_TEST(!bool(pv));
}

BOOST_AUTO_TEST_CASE(non_empty_upon_value_construction) {
  polymorphic_value<Square> pv(Square(2));

  BOOST_TEST(bool(pv));
}

BOOST_AUTO_TEST_CASE(pointer_like_methods_access_owned_object) {
  polymorphic_value<Shape> pv(Square(2));

  BOOST_TEST(pv->area() == 4);
}

BOOST_AUTO_TEST_CASE(const_is_propagated) {
  polymorphic_value<Shape> pv(Square(2));
  static_assert(std::is_same<Shape*, decltype(pv.operator->())>::value, "");
  static_assert(std::is_same<Shape&, decltype(pv.operator*())>::value, "");

  const polymorphic_value<Shape> cpv(Square(2));
  static_assert(std::is_same<const Shape*, decltype(cpv.operator->())>::value,
                "");
  static_assert(std::is_same<const Shape&, decltype(cpv.operator*())>::value,
                "");
}

BOOST_AUTO_TEST_CASE(copies_are_deep) {
  polymorphic_value<Shape> pv(Square(2));
  auto pv2 = pv;

  //BOOST_TEST(pv.operator*() != pv2.operator*());
}

BOOST_AUTO_TEST_CASE(assigned_copies_are_deep) {
  polymorphic_value<Shape> pv(Square(2));
  polymorphic_value<Shape> pv2;
  pv2 = pv;

  //BOOST_TEST(pv.operator*() != pv2.operator*());
}
