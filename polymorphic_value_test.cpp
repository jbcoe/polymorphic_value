/* Copyright (c) 2016 The Polymorphic Value Authors. All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
==============================================================================*/

#include "polymorphic_value.h"

#include <new>
#include <stdexcept>
#include <utility>

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

using namespace isocpp_p0201;

namespace {
struct BaseType {
  virtual int value() const = 0;
  virtual void set_value(int) = 0;
  virtual ~BaseType() = default;
};

struct DerivedType : BaseType {
  int value_ = 0;

  DerivedType() { ++object_count; }

  DerivedType(const DerivedType& d) {
    value_ = d.value_;
    ++object_count;
  }

  DerivedType(int v) : value_(v) { ++object_count; }

  ~DerivedType() { --object_count; }

  int value() const override { return value_; }

  void set_value(int i) override { value_ = i; }

  static size_t object_count;
};

size_t DerivedType::object_count = 0;

}  // namespace

TEST_CASE("Support for incomplete types", "[polymorphic_value.class]") {
  class Incomplete;
  polymorphic_value<Incomplete> p;

  REQUIRE_FALSE(bool(p));
}

TEST_CASE("Default constructor", "[polymorphic_value.constructors]") {
  GIVEN("A default constructed polymorphic_value to BaseType") {
    polymorphic_value<BaseType> cptr;

    THEN("operator bool returns false") { REQUIRE((bool)cptr == false); }
  }

  GIVEN("A default constructed const polymorphic_value to BaseType") {
    const polymorphic_value<BaseType> ccptr;

    THEN("operator bool returns false") { REQUIRE((bool)ccptr == false); }
  }
}

TEST_CASE("Value constructor", "[polymorphic_value.constructors]") {
  DerivedType d(7);

  polymorphic_value<BaseType> i(std::in_place_type<DerivedType>, d);

  REQUIRE(i->value() == 7);
}

TEST_CASE("Value constructor rvalue", "[polymorphic_value.constructors]") {
  polymorphic_value<BaseType> i(std::in_place_type<DerivedType>, 7);

  REQUIRE(i->value() == 7);
}

TEST_CASE("Value move-constructor", "[polymorphic_value.constructors]") {
  DerivedType d(7);

  polymorphic_value<BaseType> i(std::in_place_type<DerivedType>, std::move(d));

  REQUIRE(i->value() == 7);
}

TEST_CASE("Pointer constructor", "[polymorphic_value.constructors]") {
  GIVEN("A pointer-constructed polymorphic_value") {
    int v = 7;
    polymorphic_value<BaseType> cptr(new DerivedType(v));

    THEN("Operator-> calls the pointee method") { REQUIRE(cptr->value() == v); }

    THEN("operator bool returns true") { REQUIRE((bool)cptr == true); }
  }
  GIVEN("A pointer-constructed const polymorphic_value") {
    int v = 7;
    const polymorphic_value<BaseType> ccptr(new DerivedType(v));

    THEN("Operator-> calls the pointee method") {
      REQUIRE(ccptr->value() == v);
    }

    THEN("operator bool returns true") { REQUIRE((bool)ccptr == true); }
  }
  GIVEN(
      "Ensure nullptr pointer-constructed const polymorphic_value construct "
      "to be default initialised") {
    DerivedType* null_derived_ptr = nullptr;
    const polymorphic_value<BaseType> ccptr(null_derived_ptr);

    THEN("operator bool returns true") { REQUIRE((bool)ccptr == false); }
  }
}

struct BaseCloneSelf {
  BaseCloneSelf() = default;
  virtual ~BaseCloneSelf() = default;
  BaseCloneSelf(const BaseCloneSelf&) = delete;
  virtual std::unique_ptr<BaseCloneSelf> clone() const = 0;
};

struct DerivedCloneSelf : BaseCloneSelf {
  static size_t object_count;
  std::unique_ptr<BaseCloneSelf> clone() const {
    return std::make_unique<DerivedCloneSelf>();
  }
  DerivedCloneSelf() { ++object_count; }
  ~DerivedCloneSelf() { --object_count; }
};

size_t DerivedCloneSelf::object_count = 0;

struct invoke_clone_member {
  template <typename T>
  T* operator()(const T& t) const {
    return static_cast<T*>(t.clone().release());
  }
};

TEST_CASE("polymorphic_value constructed with copier and deleter",
          "[polymorphic_value.constructor]") {
  size_t copy_count = 0;
  size_t deletion_count = 0;
  auto cp = polymorphic_value<DerivedType>(
      new DerivedType(),
      [&](const DerivedType& d) {
        ++copy_count;
        return new DerivedType(d);
      },
      [&](const DerivedType* d) {
        ++deletion_count;
        delete d;
      });
  {
    auto cp2 = cp;
    REQUIRE(copy_count == 1);
  }
  REQUIRE(deletion_count == 1);
}

TEST_CASE("polymorphic_value destructor", "[polymorphic_value.destructor]") {
  GIVEN("No derived objects") {
    REQUIRE(DerivedType::object_count == 0);

    THEN(
        "Object count is increased on construction and decreased on "
        "destruction") {
      // begin and end scope to force destruction
      {
        polymorphic_value<BaseType> tmp(new DerivedType());
        REQUIRE(DerivedType::object_count == 1);
      }
      REQUIRE(DerivedType::object_count == 0);
    }
  }
}

TEST_CASE("polymorphic_value copy constructor",
          "[polymorphic_value.constructors]") {
  GIVEN(
      "A polymorphic_value copied from a default-constructed "
      "polymorphic_value") {
    polymorphic_value<BaseType> original_cptr;
    polymorphic_value<BaseType> cptr(original_cptr);

    THEN("operator bool returns false") { REQUIRE((bool)cptr == false); }
  }

  GIVEN(
      "A polymorphic_value copied from a pointer-constructed "
      "polymorphic_value") {
    REQUIRE(DerivedType::object_count == 0);

    int v = 7;
    polymorphic_value<BaseType> original_cptr(new DerivedType(v));
    polymorphic_value<BaseType> cptr(original_cptr);

    THEN("values are distinct") {
      REQUIRE(cptr.operator->() != original_cptr.operator->());
    }

    THEN("Operator-> calls the pointee method") { REQUIRE(cptr->value() == v); }

    THEN("operator bool returns true") { REQUIRE((bool)cptr == true); }

    THEN("object count is two") { REQUIRE(DerivedType::object_count == 2); }

    WHEN("Changes are made to the original cloning pointer after copying") {
      int new_value = 99;
      original_cptr->set_value(new_value);
      REQUIRE(original_cptr->value() == new_value);
      THEN("They are not reflected in the copy (copy is distinct)") {
        REQUIRE(cptr->value() != new_value);
        REQUIRE(cptr->value() == v);
      }
    }
  }
}

TEST_CASE("polymorphic_value move constructor",
          "[polymorphic_value.constructors]") {
  GIVEN(
      "A polymorphic_value move-constructed from a default-constructed "
      "polymorphic_value") {
    polymorphic_value<BaseType> original_cptr;
    polymorphic_value<BaseType> cptr(std::move(original_cptr));

    THEN("The original polymorphic_value is empty") {
      REQUIRE(!(bool)original_cptr);
    }

    THEN("The move-constructed polymorphic_value is empty") {
      REQUIRE(!(bool)cptr);
    }
  }

  GIVEN(
      "A polymorphic_value move-constructed from a default-constructed "
      "polymorphic_value") {
    int v = 7;
    polymorphic_value<BaseType> original_cptr(new DerivedType(v));
    auto original_pointer = original_cptr.operator->();
    CHECK(DerivedType::object_count == 1);

    polymorphic_value<BaseType> cptr(std::move(original_cptr));
    CHECK(DerivedType::object_count == 1);

    THEN("The original polymorphic_value is empty") {
      REQUIRE(!(bool)original_cptr);
    }

    THEN("The move-constructed pointer is the original pointer") {
      REQUIRE(cptr.operator->() == original_pointer);
      REQUIRE(cptr.operator->() == original_pointer);
      REQUIRE((bool)cptr);
    }

    THEN("The move-constructed pointer value is the constructed value") {
      REQUIRE(cptr->value() == v);
    }
  }
}

TEST_CASE("polymorphic_value assignment", "[polymorphic_value.assignment]") {
  GIVEN(
      "A default-constructed polymorphic_value assigned-to a "
      "default-constructed polymorphic_value") {
    polymorphic_value<BaseType> cptr1;
    const polymorphic_value<BaseType> cptr2;

    REQUIRE(DerivedType::object_count == 0);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 0);

    THEN("The assigned-to object is empty") { REQUIRE(!cptr1); }
  }

  GIVEN(
      "A default-constructed polymorphic_value assigned to a "
      "pointer-constructed polymorphic_value") {
    int v1 = 7;

    polymorphic_value<BaseType> cptr1(new DerivedType(v1));
    const polymorphic_value<BaseType> cptr2;

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 0);

    THEN("The assigned-to object is empty") { REQUIRE(!cptr1); }
  }

  GIVEN(
      "A pointer-constructed polymorphic_value assigned to a "
      "default-constructed polymorphic_value") {
    int v1 = 7;

    polymorphic_value<BaseType> cptr1;
    const polymorphic_value<BaseType> cptr2(new DerivedType(v1));
    const auto p = cptr2.operator->();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 2);

    THEN("The assigned-from object is unchanged") {
      REQUIRE(cptr2.operator->() == p);
    }

    THEN("The assigned-to object is non-empty") { REQUIRE((bool)cptr1); }

    THEN("The assigned-from object 'value' is the assigned-to object value") {
      REQUIRE(cptr1->value() == cptr2->value());
    }

    THEN(
        "The assigned-from object pointer and the assigned-to object pointer "
        "are distinct") {
      REQUIRE(cptr1.operator->() != cptr2.operator->());
    }
  }

  GIVEN(
      "A pointer-constructed polymorphic_value assigned to a "
      "pointer-constructed polymorphic_value") {
    int v1 = 7;
    int v2 = 87;

    polymorphic_value<BaseType> cptr1(new DerivedType(v1));
    const polymorphic_value<BaseType> cptr2(new DerivedType(v2));
    const auto p = cptr2.operator->();

    REQUIRE(DerivedType::object_count == 2);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 2);

    THEN("The assigned-from object is unchanged") {
      REQUIRE(cptr2.operator->() == p);
    }

    THEN("The assigned-to object is non-empty") { REQUIRE((bool)cptr1); }

    THEN("The assigned-from object 'value' is the assigned-to object value") {
      REQUIRE(cptr1->value() == cptr2->value());
    }

    THEN(
        "The assigned-from object pointer and the assigned-to object pointer "
        "are distinct") {
      REQUIRE(cptr1.operator->() != cptr2.operator->());
    }
  }

  GIVEN("A pointer-constructed polymorphic_value assigned to itself") {
    int v1 = 7;

    polymorphic_value<BaseType> cptr1(new DerivedType(v1));
    const auto p = cptr1.operator->();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr1;

    REQUIRE(DerivedType::object_count == 1);

    THEN("The assigned-from object is unchanged") {
      REQUIRE(cptr1.operator->() == p);
    }
  }
}

TEST_CASE("polymorphic_value move-assignment",
          "[polymorphic_value.assignment]") {
  GIVEN(
      "A default-constructed polymorphic_value move-assigned-to a "
      "default-constructed polymorphic_value") {
    polymorphic_value<BaseType> cptr1;
    polymorphic_value<BaseType> cptr2;

    REQUIRE(DerivedType::object_count == 0);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 0);

    THEN("The move-assigned-from object is empty") { REQUIRE(!cptr2); }

    THEN("The move-assigned-to object is empty") { REQUIRE(!cptr1); }
  }

  GIVEN(
      "A default-constructed polymorphic_value move-assigned to a "
      "pointer-constructed polymorphic_value") {
    int v1 = 7;

    polymorphic_value<BaseType> cptr1(new DerivedType(v1));
    polymorphic_value<BaseType> cptr2;

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 0);

    THEN("The move-assigned-from object is empty") { REQUIRE(!cptr2); }

    THEN("The move-assigned-to object is empty") { REQUIRE(!cptr1); }
  }

  GIVEN(
      "A pointer-constructed polymorphic_value move-assigned to a "
      "default-constructed polymorphic_value") {
    int v1 = 7;

    polymorphic_value<BaseType> cptr1;
    polymorphic_value<BaseType> cptr2(new DerivedType(v1));
    const auto p = cptr2.operator->();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 1);

    THEN("The move-assigned-from object is empty") { REQUIRE(!cptr2); }

    THEN(
        "The move-assigned-to object pointer is the move-assigned-from "
        "pointer") {
      REQUIRE(cptr1.operator->() == p);
    }
  }

  GIVEN(
      "A pointer-constructed polymorphic_value move-assigned to a "
      "pointer-constructed polymorphic_value") {
    int v1 = 7;
    int v2 = 87;

    polymorphic_value<BaseType> cptr1(new DerivedType(v1));
    polymorphic_value<BaseType> cptr2(new DerivedType(v2));
    const auto p = cptr2.operator->();

    REQUIRE(DerivedType::object_count == 2);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 1);

    THEN("The move-assigned-from object is empty") { REQUIRE(!cptr2); }

    THEN(
        "The move-assigned-to object pointer is the move-assigned-from "
        "pointer") {
      REQUIRE(cptr1.operator->() == p);
    }
  }

  GIVEN(
      "Guard against self-assignment during move-assigned to a "
      "pointer-constructed polymorphic_value") {
    int v1 = 7;

    polymorphic_value<BaseType> cptr1(new DerivedType(v1));

    REQUIRE(DerivedType::object_count == 1);

    // Manually move to avoid issues caused by flags "-Wself-move":
    //      explicitly moving variable of type 'polymorphic_value<BaseType>' to
    //      itself
    cptr1 = static_cast<polymorphic_value<BaseType>&&>(cptr1);

    REQUIRE(DerivedType::object_count == 1);

    THEN("The move-assigned-to object is valid") { REQUIRE((bool)cptr1); }
  }
}

TEST_CASE("make_polymorphic_value with single template argument",
          "[polymorphic_value.make_polymorphic_value.single]") {
  auto pv = make_polymorphic_value<DerivedType>(7);
  static_assert(
      std::is_same<decltype(pv), polymorphic_value<DerivedType>>::value, "");
  REQUIRE(pv->value() == 7);
}

TEST_CASE("make_polymorphic_value with two template arguments",
          "[polymorphic_value.make_polymorphic_value.double]") {
  auto pv = make_polymorphic_value<BaseType, DerivedType>(7);
  static_assert(std::is_same<decltype(pv), polymorphic_value<BaseType>>::value,
                "");
  REQUIRE(pv->value() == 7);
}

struct B;
struct A {
  A(B);
  A() = default;
};
struct B : A {};

TEST_CASE("make_polymorphic_value ambiguity",
          "[polymorphic_value.make_polymorphic_value.ambiguity]") {
  B b;
  // This was ambiguous with two make_polymorphic_value functions.
  auto pv = make_polymorphic_value<A, B>(b);
}

TEST_CASE("Derived types", "[polymorphic_value.derived_types]") {
  GIVEN(
      "A polymorphic_value<BaseType> constructed from "
      "make_polymorphic_value<DerivedType>") {
    int v = 7;
    auto cptr = make_polymorphic_value<DerivedType>(v);

    WHEN("A polymorphic_value<BaseType> is copy-constructed") {
      polymorphic_value<BaseType> bptr(cptr);

      THEN("Operator-> calls the pointee method") {
        REQUIRE(bptr->value() == v);
      }

      THEN("operator bool returns true") { REQUIRE((bool)bptr == true); }
    }

    WHEN("A polymorphic_value<BaseType> is move-constructed") {
      polymorphic_value<BaseType> bptr(std::move(cptr));

      THEN("Operator-> calls the pointee method") {
        REQUIRE(bptr->value() == v);
      }

      THEN("operator bool returns true") { REQUIRE((bool)bptr == true); }
    }
  }
}

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

TEST_CASE("Gustafsson's dilemma: multiple (virtual) base classes",
          "[polymorphic_value.constructors]") {
  GIVEN("A value-constructed multiply-derived-class polymorphic_value") {
    int v = 7;
    polymorphic_value<MultiplyDerived> cptr(new MultiplyDerived(v));

    THEN(
        "When copied to a polymorphic_value to an intermediate base type, "
        "data is accessible as expected") {
      auto cptr_IA = polymorphic_value<IntermediateBaseA>(cptr);
      REQUIRE(cptr_IA->a_ == 3);
      REQUIRE(cptr_IA->v_ == 42);
    }

    THEN(
        "When copied to a polymorphic_value to an intermediate base type, "
        "data is accessible as expected") {
      auto cptr_IB = polymorphic_value<IntermediateBaseB>(cptr);
      REQUIRE(cptr_IB->b_ == 101);
      REQUIRE(cptr_IB->v_ == 42);
    }
  }
}

struct Tracked {
  static int ctor_count_;
  static int dtor_count_;
  static int assignment_count_;

  static void reset_counts() {
    ctor_count_ = 0;
    dtor_count_ = 0;
    assignment_count_ = 0;
  }

  Tracked() { ++ctor_count_; }
  ~Tracked() { ++dtor_count_; }
  Tracked(const Tracked&) { ++ctor_count_; }
  Tracked(Tracked&&) { ++ctor_count_; }
  Tracked& operator=(const Tracked&) {
    ++assignment_count_;
    return *this;
  }
  Tracked& operator=(Tracked&&) {
    ++assignment_count_;
    return *this;
  }
};

int Tracked::ctor_count_ = 0;
int Tracked::dtor_count_ = 0;
int Tracked::assignment_count_ = 0;

struct ThrowsOnCopy : Tracked {
  int value_ = 0;

  ThrowsOnCopy() = default;

  explicit ThrowsOnCopy(const int v) : value_(v) {}

  ThrowsOnCopy(const ThrowsOnCopy&) {
    throw std::runtime_error("something went wrong during copy");
  }

  ThrowsOnCopy& operator=(const ThrowsOnCopy& rhs) = default;
};

TEST_CASE("Exception safety: throw in copy constructor",
          "[polymorphic_value.exception_safety.copy]") {
  GIVEN("A value-constructed polymorphic_value to a ThrowsOnCopy") {
    const int v = 7;
    polymorphic_value<ThrowsOnCopy> cptr(new ThrowsOnCopy(v));

    THEN(
        "When copying to another polymorphic_value, after an exception, the "
        "source remains valid") {
      Tracked::reset_counts();
      polymorphic_value<ThrowsOnCopy> another;
      REQUIRE_THROWS_AS(another = cptr, std::runtime_error);
      REQUIRE(cptr->value_ == v);
      REQUIRE(Tracked::ctor_count_ - Tracked::dtor_count_ == 0);
    }

    THEN(
        "When copying to another polymorphic_value, after an exception, the "
        "destination is not changed") {
      const int v2 = 5;
      polymorphic_value<ThrowsOnCopy> another(new ThrowsOnCopy(v2));
      Tracked::reset_counts();
      REQUIRE_THROWS_AS(another = cptr, std::runtime_error);
      REQUIRE(another->value_ == v2);
      REQUIRE(Tracked::ctor_count_ - Tracked::dtor_count_ == 0);
    }
  }
}

template <typename T>
struct throwing_copier {
  using deleter_type = std::default_delete<T>;
  T* operator()(const T& t) const { throw std::bad_alloc{}; }
};

struct TrackedValue : Tracked {
  int value_ = 0;
  explicit TrackedValue(const int v) : value_(v) {}
};

TEST_CASE("Exception safety: throw in copier",
          "[polymorphic_value.exception_safety.copier]") {
  GIVEN("A value-constructed polymorphic_value") {
    const int v = 7;
    polymorphic_value<TrackedValue> cptr(new TrackedValue(v),
                                         throwing_copier<TrackedValue>{});

    THEN("When an exception occurs in the copier, the source is unchanged") {
      polymorphic_value<TrackedValue> another;
      Tracked::reset_counts();
      REQUIRE_THROWS_AS(another = cptr, std::bad_alloc);
      REQUIRE(cptr->value_ == v);
      REQUIRE(Tracked::ctor_count_ - Tracked::dtor_count_ == 0);
    }

    THEN(
        "When an exception occurs in the copier, the destination is "
        "unchanged") {
      const int v2 = 5;
      polymorphic_value<TrackedValue> another(new TrackedValue(v2));
      Tracked::reset_counts();
      REQUIRE_THROWS_AS(another = cptr, std::bad_alloc);
      REQUIRE(another->value_ == v2);
      REQUIRE(Tracked::ctor_count_ - Tracked::dtor_count_ == 0);
    }
  }
}

TEST_CASE("polymorphic_value<const T>",
          "[polymorphic_value.compatible_types]") {
  polymorphic_value<const DerivedType> p(std::in_place_type<DerivedType>, 7);
  REQUIRE(p->value() == 7);
  // Will not compile as p is polymorphic_value<const DerivedType> not
  // polymorphic_value<DerivedType>
  // p->set_value(42);
}

TEST_CASE("Check exception object construction",
          "[polymorphic_value.construction.exception]") {
  bad_polymorphic_value_construction exception;
  // Should find an error message referencing polymorphic_value.
  CHECK(std::string(exception.what()).find("polymorphic_value") !=
        std::string::npos);
}

class DeeplyDerivedType : public DerivedType {
 public:
  DeeplyDerivedType() : DerivedType(0) {}
};

TEST_CASE("polymorphic_value dynamic and static type mismatch",
          "[polymorphic_value.construction]") {
  DeeplyDerivedType dd;
  DerivedType* p = &dd;

  CHECK(typeid(*p) != typeid(DerivedType));

  CHECK_THROWS_AS([p] { return polymorphic_value<BaseType>(p); }(),
                  bad_polymorphic_value_construction);
}

struct fake_copy {
  template <class T>
  DerivedType* operator()(const T& b) const {
    return nullptr;
  }
};

struct no_deletion {
  void operator()(const void*) const {}
};

TEST_CASE(
    "polymorphic_value dynamic and static type mismatch is not a problem "
    "with custom copier or deleter",
    "[polymorphic_value.construction]") {
  DeeplyDerivedType dd;
  DerivedType* p = &dd;

  CHECK(typeid(*p) != typeid(DerivedType));

  CHECK_NOTHROW([p] {
    return polymorphic_value<BaseType>(p, fake_copy{}, no_deletion{});
  }());
}

TEST_CASE("Dangling reference in forwarding constructor",
          "[polymorphic_value.constructors]") {
  auto d = DerivedType(7);
  auto& rd = d;
  polymorphic_value<DerivedType> p(std::in_place_type<DerivedType>, rd);

  d.set_value(6);
  CHECK(p->value() == 7);
}

namespace {
template <typename T>
struct tracking_allocator {
  unsigned* alloc_counter;
  unsigned* dealloc_counter;

  explicit tracking_allocator(unsigned* a, unsigned* d) noexcept
      : alloc_counter(a), dealloc_counter(d) {}

  template <typename U>
  tracking_allocator(const tracking_allocator<U>& other)
      : alloc_counter(other.alloc_counter),
        dealloc_counter(other.dealloc_counter) {}

  using value_type = T;

  template <typename Other>
  struct rebind {
    using other = tracking_allocator<Other>;
  };

  constexpr T* allocate(std::size_t n) {
    ++*alloc_counter;
    std::allocator<T> default_allocator{};
    return default_allocator.allocate(n);
  }
  constexpr void deallocate(T* p, std::size_t n) {
    ++*dealloc_counter;
    std::allocator<T> default_allocator{};
    default_allocator.deallocate(p, n);
  }
};
}  // namespace

TEST_CASE("Allocator used to construct control block") {
  unsigned allocs = 0;
  unsigned deallocs = 0;

  tracking_allocator<DerivedType> alloc(&allocs, &deallocs);
  std::allocator<DerivedType> default_allocator{};
  auto mem = default_allocator.allocate(1);
  const unsigned value = 42;
  new (mem) DerivedType(value);

  {
    polymorphic_value<DerivedType> p(mem, std::allocator_arg_t{}, alloc);
    CHECK(allocs == 1);
    CHECK(deallocs == 0);
  }
  CHECK(allocs == 1);
  CHECK(deallocs == 2);
}

TEST_CASE("Copying object with allocator allocates") {
  unsigned allocs = 0;
  unsigned deallocs = 0;

  tracking_allocator<DerivedType> alloc(&allocs, &deallocs);
  std::allocator<DerivedType> default_allocator{};
  auto mem = default_allocator.allocate(1);
  const unsigned value = 42;
  new (mem) DerivedType(value);

  {
    polymorphic_value<DerivedType> p(mem, std::allocator_arg_t{}, alloc);
    polymorphic_value<DerivedType> p2(p);
    CHECK(allocs == 3);
    CHECK(deallocs == 0);
  }
  CHECK(allocs == 3);
  CHECK(deallocs == 4);
}

TEST_CASE("Allocator used to construct with allocate_polymorphic_value") {
  unsigned allocs = 0;
  unsigned deallocs = 0;

  tracking_allocator<DerivedType> alloc(&allocs, &deallocs);

  {
    unsigned const value = 99;
    polymorphic_value<DerivedType> p = allocate_polymorphic_value<DerivedType>(
        std::allocator_arg_t{}, alloc, value);
    CHECK(allocs == 2);
    CHECK(deallocs == 0);
  }
  CHECK(allocs == 2);
  CHECK(deallocs == 2);
}
