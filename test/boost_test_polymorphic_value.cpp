#define BOOST_TEST_MODULE polymorphic_value
#include "boost/polymorphic_value.hpp"
#include <boost/math/constants/constants.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/utility/string_view.hpp>

#include <new>
#include <stdexcept>

using boost::polymorphic_value;

namespace {

  struct Shape {
    bool moved_from = false;
    virtual boost::string_view name() const = 0;
    virtual double area() const = 0;

    Shape() = default;
    Shape(const Shape&) = default;
    // polymorphic_value does not require the destructor to be virtual.
    virtual ~Shape() = default; 
    Shape& operator=(const Shape&) = default;

    Shape(Shape&& s) { s.moved_from = true; }

    Shape& operator=(Shape&& s) {
      s.moved_from = true;
      return *this;
    }
  };

  class Square : public Shape {
    double side_;

  public:
    Square(double side) : side_(side) {}
    boost::string_view name() const override { return "square"; }
    double area() const override { return side_ * side_; }
  };

  class Circle : public Shape {
    double radius_;

  public:
    Circle(double radius) : radius_(radius) {}
    boost::string_view name() const override { return "circle"; }
    double area() const override {
      return boost::math::constants::pi<double>() * radius_ * radius_;
    }
  };


} // end namespace

BOOST_AUTO_TEST_CASE(empty_upon_default_construction) {
  polymorphic_value<Shape> pv;

  BOOST_TEST(!bool(pv));
}

BOOST_AUTO_TEST_CASE(support_for_incomplete_types) {
  class Foo;
  polymorphic_value<Foo> pv;

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

BOOST_AUTO_TEST_CASE(const_propagation) {
  polymorphic_value<Shape> pv(Square(2));
  static_assert(std::is_same<Shape*, decltype(pv.operator->())>::value, "");
  static_assert(std::is_same<Shape&, decltype(pv.operator*())>::value, "");

  const polymorphic_value<Shape> cpv(Square(2));
  static_assert(std::is_same<const Shape*, decltype(cpv.operator->())>::value,
                "");
  static_assert(std::is_same<const Shape&, decltype(cpv.operator*())>::value,
                "");
}

BOOST_AUTO_TEST_CASE(copy_constructor) {
  polymorphic_value<Square> pv(Square(2));
  auto pv2 = pv;

  BOOST_TEST(pv.operator->() != pv2.operator->());
  BOOST_TEST(pv2->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv2.operator->()));
}

BOOST_AUTO_TEST_CASE(copy_assignment) {
  polymorphic_value<Square> pv(Square(2));
  polymorphic_value<Square> pv2;
  pv2 = pv;

  BOOST_TEST(pv.operator->() != pv2.operator->());
  BOOST_TEST(pv2->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv2.operator->()));
}

BOOST_AUTO_TEST_CASE(move_constructor) {
  polymorphic_value<Square> pv(Square(2));
  const auto* p = pv.operator->();

  polymorphic_value<Square> pv2(std::move(pv));

  BOOST_TEST(!pv);
  BOOST_TEST(pv2.operator->() == p);
  BOOST_TEST(pv2->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv2.operator->()));
}

BOOST_AUTO_TEST_CASE(move_assignment) {
  polymorphic_value<Square> pv(Square(2));
  const auto* p = pv.operator->();

  polymorphic_value<Square> pv2;
  pv2 = std::move(pv);

  BOOST_TEST(!pv);
  BOOST_TEST(pv2.operator->() == p);
  BOOST_TEST(pv2->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv2.operator->()));
}

BOOST_AUTO_TEST_CASE(value_constructor) {
  Square s(2);
  polymorphic_value<Square> pv(s);

  BOOST_TEST(pv.operator->() != &s);
  BOOST_TEST(pv->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv.operator->()));
}

BOOST_AUTO_TEST_CASE(value_assignment) {
  Square s(2);
  polymorphic_value<Square> pv;
  pv = s;

  BOOST_TEST(pv.operator->() != &s);
  BOOST_TEST(pv->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv.operator->()));
}

BOOST_AUTO_TEST_CASE(value_move_constructor) {
  Square s(2);
  polymorphic_value<Square> pv(std::move(s));

  BOOST_TEST(s.moved_from);
  BOOST_TEST(pv.operator->() != &s);
  BOOST_TEST(pv->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv.operator->()));
}

BOOST_AUTO_TEST_CASE(value_move_assignment) {
  Square s(2);
  polymorphic_value<Square> pv;
  pv = std::move(s);

  BOOST_TEST(s.moved_from);
  BOOST_TEST(pv.operator->() != &s);
  BOOST_TEST(pv->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv.operator->()));
}

BOOST_AUTO_TEST_CASE(derived_type_copy_constructor) {
  polymorphic_value<Shape> pv(Square(2));
  auto pv2 = pv;

  BOOST_TEST(pv.operator->() != pv2.operator->());
  BOOST_TEST(pv2->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv2.operator->()));
}

BOOST_AUTO_TEST_CASE(derived_type_copy_assignment) {
  polymorphic_value<Shape> pv(Square(2));
  polymorphic_value<Shape> pv2;
  pv2 = pv;

  BOOST_TEST(pv.operator->() != pv2.operator->());
  BOOST_TEST(pv2->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv2.operator->()));
}

BOOST_AUTO_TEST_CASE(derived_type_move_constructor) {
  polymorphic_value<Shape> pv(Square(2));
  const auto* p = pv.operator->();

  polymorphic_value<Shape> pv2(std::move(pv));

  BOOST_TEST(!pv);
  BOOST_TEST(pv2.operator->() == p);
  BOOST_TEST(pv2->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv2.operator->()));
}

BOOST_AUTO_TEST_CASE(derived_type_move_assignment) {
  polymorphic_value<Shape> pv(Square(2));
  const auto* p = pv.operator->();

  polymorphic_value<Shape> pv2;
  pv2 = std::move(pv);

  BOOST_TEST(!pv);
  BOOST_TEST(pv2.operator->() == p);
  BOOST_TEST(pv2->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv2.operator->()));
}

BOOST_AUTO_TEST_CASE(derived_type_value_constructor) {
  Square s(2);
  polymorphic_value<Shape> pv(s);

  BOOST_TEST(pv.operator->() != &s);
  BOOST_TEST(pv->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv.operator->()));
}

BOOST_AUTO_TEST_CASE(derived_type_value_assignment) {
  Square s(2);
  polymorphic_value<Shape> pv;
  pv = s;

  BOOST_TEST(pv.operator->() != &s);
  BOOST_TEST(pv->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv.operator->()));
}

BOOST_AUTO_TEST_CASE(derived_type_value_move_constructor) {
  Square s(2);
  polymorphic_value<Shape> pv(std::move(s));

  BOOST_TEST(s.moved_from);
  BOOST_TEST(pv.operator->() != &s);
  BOOST_TEST(pv->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv.operator->()));
}

BOOST_AUTO_TEST_CASE(derived_type_value_move_assignment) {
  Square s(2);
  polymorphic_value<Shape> pv;
  pv = std::move(s);

  BOOST_TEST(s.moved_from);
  BOOST_TEST(pv.operator->() != &s);
  BOOST_TEST(pv->area() == 4);
  BOOST_TEST(dynamic_cast<Square*>(pv.operator->()));
}

BOOST_AUTO_TEST_CASE(swap) {
  polymorphic_value<Shape> square(Square(2));
  polymorphic_value<Shape> circle(Circle(2));

  BOOST_TEST(square->name() == "square");
  BOOST_TEST(circle->name() == "circle");

  using std::swap;
  swap(square, circle);

  BOOST_TEST(square->name() == "circle");
  BOOST_TEST(circle->name() == "square");
}

BOOST_AUTO_TEST_CASE(member_swap) {
  polymorphic_value<Shape> square(Square(2));
  polymorphic_value<Shape> circle(Circle(2));

  BOOST_TEST(square->name() == "square");
  BOOST_TEST(circle->name() == "circle");

  using std::swap;
  square.swap(circle);

  BOOST_TEST(square->name() == "circle");
  BOOST_TEST(circle->name() == "square");
}

BOOST_AUTO_TEST_CASE(multiple_inheritance_with_virtual_base_classes) {
  // This is "Gustafsson's dilemma" and revealed a serious problem in an early
  // implementation.
  // Thanks go to Bengt Gustaffson.
  struct Base {
    int v_ = 42;
    virtual ~Base() = default;
  };
  struct IntermediateBaseA : virtual Base {
    int a_ = 3;
  };
  struct IntermediateBaseB : virtual Base {
    int b_ = 101;
  };
  struct MultiplyDerived : IntermediateBaseA, IntermediateBaseB {
    int value_ = 0;
    MultiplyDerived(int value) : value_(value){};
  };

  // Given a value-constructed multiply-derived-class polymorphic_value.
  int v = 7;
  polymorphic_value<MultiplyDerived> cptr(new MultiplyDerived(v));

  // Then when copied to a polymorphic_value to an intermediate base type, data
  // is accessible as expected.
  polymorphic_value<IntermediateBaseA> cptr_IA = cptr;
  BOOST_TEST(cptr_IA->a_ == 3);
  BOOST_TEST(cptr_IA->v_ == 42);

  // Then when copied to a polymorphic_value to an alternate intermediate base
  // type data is accessible as expected.
  polymorphic_value<IntermediateBaseB> cptr_IB = cptr;
  BOOST_TEST(cptr_IB->b_ == 101);
  BOOST_TEST(cptr_IB->v_ == 42);
}

BOOST_AUTO_TEST_CASE(reference_decay_in_forwarding_constructors) {
  // This test highlights a bug in earlier implementations where a
  // polymorphic_value<int&> could be erroneously created.
  // Thanks go to Matt Calabrese.
  int x = 7;
  int& rx = x;
  polymorphic_value<int> p(rx);

  x = 6;
  BOOST_TEST(*p == 7);
}

BOOST_AUTO_TEST_CASE(dynamic_and_static_type_mismatch_throws_exception) {

  class UnitSquare : public Square {
  public:
    UnitSquare() : Square(1) {}
    double area() const { return 1.0; }
    boost::string_view name() const { return "unit-square"; }
  };
  UnitSquare u;
  Square* s = &u;

  BOOST_CHECK_THROW([s] { return polymorphic_value<Shape>(s); }(),
                    boost::bad_polymorphic_value_construction);
}

BOOST_AUTO_TEST_CASE(custom_copy_and_delete) {
  size_t copy_count = 0;
  size_t deletion_count = 0;
  polymorphic_value<Square> pv(new Square(2),
                               [&](const Square& d) {
                                 ++copy_count;
                                 return new Square(d);
                               },
                               [&](const Square* d) {
                                 ++deletion_count;
                                 delete d;
                               });
  // Restrict scope.
  {
    auto pv2 = pv;
    BOOST_TEST(copy_count == 1);
  }
  BOOST_TEST(deletion_count == 1);
}
