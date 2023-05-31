/* Copyright (c) 2016 The Authors of Polymorphic. All Rights Reserved.

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

#include "polymorphic.h"

#include <new>
#include <stdexcept>
#include <utility>

#undef CATCH_CONFIG_WINDOWS_SEH
#include "catch2/catch_test_macros.hpp"

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

TEST_CASE("Support for incomplete types", "[polymorphic.class]") {
  class Incomplete;
  polymorphic<Incomplete> p;

  REQUIRE_FALSE(bool(p));
}

TEST_CASE("Default constructor", "[polymorphic.constructors]") {
  GIVEN("A default constructed polymorphic to BaseType") {
    polymorphic<BaseType> cptr;

    THEN("operator bool returns false") { REQUIRE((bool)cptr == false); }
  }

  GIVEN("A default constructed const polymorphic to BaseType") {
    const polymorphic<BaseType> ccptr;

    THEN("operator bool returns false") { REQUIRE((bool)ccptr == false); }
  }
}

TEST_CASE("Value constructor", "[polymorphic.constructors]") {
  DerivedType d(7);

  polymorphic<BaseType> i(std::in_place_type<DerivedType>, d);

  REQUIRE(i->value() == 7);
}

TEST_CASE("Value constructor rvalue", "[polymorphic.constructors]") {
  polymorphic<BaseType> i(std::in_place_type<DerivedType>, 7);

  REQUIRE(i->value() == 7);
}

TEST_CASE("Value move-constructor", "[polymorphic.constructors]") {
  DerivedType d(7);

  polymorphic<BaseType> i(std::in_place_type<DerivedType>, std::move(d));

  REQUIRE(i->value() == 7);
}

TEST_CASE("Pointer constructor", "[polymorphic.constructors]") {
  GIVEN("A pointer-constructed polymorphic") {
    int v = 7;
    polymorphic<BaseType> cptr(new DerivedType(v));

    THEN("Operator-> calls the pointee method") { REQUIRE(cptr->value() == v); }

    THEN("operator bool returns true") { REQUIRE((bool)cptr == true); }
  }
  GIVEN("A pointer-constructed const polymorphic") {
    int v = 7;
    const polymorphic<BaseType> ccptr(new DerivedType(v));

    THEN("Operator-> calls the pointee method") {
      REQUIRE(ccptr->value() == v);
    }

    THEN("operator bool returns true") { REQUIRE((bool)ccptr == true); }
  }
  GIVEN(
      "Ensure nullptr pointer-constructed const polymorphic construct "
      "to be default initialised") {
    DerivedType* null_derived_ptr = nullptr;
    const polymorphic<BaseType> ccptr(null_derived_ptr);

    THEN("operator bool returns true") { REQUIRE((bool)ccptr == false); }
  }
  GIVEN("Ensure polymorphic supports construction from nullptr") {
    const polymorphic<BaseType> ccptr(nullptr);

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

TEST_CASE("polymorphic constructed with copier and deleter",
          "[polymorphic.constructor]") {
  size_t copy_count = 0;
  size_t deletion_count = 0;
  auto cp = polymorphic<DerivedType>(
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

TEST_CASE("polymorphic destructor", "[polymorphic.destructor]") {
  GIVEN("No derived objects") {
    REQUIRE(DerivedType::object_count == 0);

    THEN(
        "Object count is increased on construction and decreased on "
        "destruction") {
      // begin and end scope to force destruction
      {
        polymorphic<BaseType> tmp(std::in_place_type<DerivedType>);
        REQUIRE(DerivedType::object_count == 1);
      }
      REQUIRE(DerivedType::object_count == 0);
    }
  }
}

TEST_CASE("polymorphic copy constructor",
          "[polymorphic.constructors]") {
  GIVEN(
      "A polymorphic copied from a default-constructed "
      "polymorphic") {
    polymorphic<BaseType> original_cptr;
    polymorphic<BaseType> cptr(original_cptr);

    THEN("operator bool returns false") { REQUIRE((bool)cptr == false); }
  }

  GIVEN(
      "A polymorphic copied from a in-place-constructed "
      "polymorphic") {
    REQUIRE(DerivedType::object_count == 0);

    int v = 7;
    polymorphic<BaseType> original_cptr(std::in_place_type<DerivedType>,
                                              v);
    polymorphic<BaseType> cptr(original_cptr);

    THEN("values are distinct") {
      REQUIRE(cptr.operator->() != original_cptr.operator->());
    }

    THEN("Operator-> calls the pointee method") { REQUIRE(cptr->value() == v); }

    THEN("operator bool returns true") { REQUIRE((bool)cptr == true); }

    THEN("object count is two") { REQUIRE(DerivedType::object_count == 2); }

    WHEN("Changes are made to the original cloning pointer after copying") {
      int updated_value = 99;
      original_cptr->set_value(updated_value);
      REQUIRE(original_cptr->value() == updated_value);
      THEN("They are not reflected in the copy (copy is distinct)") {
        REQUIRE(cptr->value() != updated_value);
        REQUIRE(cptr->value() == v);
      }
    }
  }
}

TEST_CASE("polymorphic move constructor",
          "[polymorphic.constructors]") {
  GIVEN(
      "A polymorphic move-constructed from a default-constructed "
      "polymorphic") {
    polymorphic<BaseType> original_cptr;
    polymorphic<BaseType> cptr(std::move(original_cptr));

    THEN("The original polymorphic is empty") {
      REQUIRE(!(bool)original_cptr);
    }

    THEN("The move-constructed polymorphic is empty") {
      REQUIRE(!(bool)cptr);
    }
  }

  GIVEN(
      "A polymorphic move-constructed from a default-constructed "
      "polymorphic") {
    int v = 7;
    polymorphic<BaseType> original_cptr(std::in_place_type<DerivedType>,
                                              v);
    auto original_pointer = original_cptr.operator->();
    CHECK(DerivedType::object_count == 1);

    polymorphic<BaseType> cptr(std::move(original_cptr));
    CHECK(DerivedType::object_count == 1);

    THEN("The original polymorphic is empty") {
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

TEST_CASE("polymorphic assignment", "[polymorphic.assignment]") {
  GIVEN(
      "A default-constructed polymorphic assigned-to a "
      "default-constructed polymorphic") {
    polymorphic<BaseType> cptr1;
    const polymorphic<BaseType> cptr2;

    REQUIRE(DerivedType::object_count == 0);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 0);

    THEN("The assigned-to object is empty") { REQUIRE(!cptr1); }
  }

  GIVEN(
      "A default-constructed polymorphic assigned to a "
      "pointer-constructed polymorphic") {
    int v1 = 7;

    polymorphic<BaseType> cptr1(std::in_place_type<DerivedType>, v1);
    const polymorphic<BaseType> cptr2;

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 0);

    THEN("The assigned-to object is empty") { REQUIRE(!cptr1); }
  }

  GIVEN(
      "A default-constructed polymorphic assigned to an "
      "in-place-constructed polymorphic") {
    int v1 = 7;

    polymorphic<BaseType> cptr1;
    const polymorphic<BaseType> cptr2(std::in_place_type<DerivedType>,
                                            v1);
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
      "An in-place-constructed polymorphic assigned to an "
      "in-place-constructed polymorphic") {
    int v1 = 7;
    int v2 = 87;

    polymorphic<BaseType> cptr1(std::in_place_type<DerivedType>, v1);
    const polymorphic<BaseType> cptr2(std::in_place_type<DerivedType>,
                                            v2);
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

  GIVEN("An on-place-constructed polymorphic assigned to itself") {
    int v1 = 7;

    polymorphic<BaseType> cptr1(std::in_place_type<DerivedType>, v1);
    const auto p = cptr1.operator->();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr1;

    REQUIRE(DerivedType::object_count == 1);

    THEN("The assigned-from object is unchanged") {
      REQUIRE(cptr1.operator->() == p);
    }
  }
}

TEST_CASE("polymorphic move-assignment",
          "[polymorphic.assignment]") {
  GIVEN(
      "A default-constructed polymorphic move-assigned-to a "
      "default-constructed polymorphic") {
    polymorphic<BaseType> cptr1;
    polymorphic<BaseType> cptr2;

    REQUIRE(DerivedType::object_count == 0);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 0);

    THEN("The move-assigned-from object is empty") { REQUIRE(!cptr2); }

    THEN("The move-assigned-to object is empty") { REQUIRE(!cptr1); }
  }

  GIVEN(
      "A default-constructed polymorphic move-assigned to a "
      "in-place-constructed polymorphic") {
    int v1 = 7;

    polymorphic<BaseType> cptr1(std::in_place_type<DerivedType>, v1);
    polymorphic<BaseType> cptr2;

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 0);

    THEN("The move-assigned-from object is empty") { REQUIRE(!cptr2); }

    THEN("The move-assigned-to object is empty") { REQUIRE(!cptr1); }
  }

  GIVEN(
      "A default-constructed polymorphic move-assigned to an "
      "in-place-constructed polymorphic") {
    int v1 = 7;

    polymorphic<BaseType> cptr1;
    polymorphic<BaseType> cptr2(std::in_place_type<DerivedType>, v1);
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
      "An in-place-constructed polymorphic move-assigned to a "
      "in-place-constructed polymorphic") {
    int v1 = 7;
    int v2 = 87;

    polymorphic<BaseType> cptr1(std::in_place_type<DerivedType>, v1);
    polymorphic<BaseType> cptr2(std::in_place_type<DerivedType>, v2);
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
      "Guard against self-assignment during move-assignment to an "
      "in-place-constructed polymorphic") {
    int v1 = 7;

    polymorphic<BaseType> cptr1(std::in_place_type<DerivedType>, v1);

    REQUIRE(DerivedType::object_count == 1);

    // Manually move to avoid issues caused by flags "-Wself-move":
    //      explicitly moving variable of type 'polymorphic<BaseType>' to
    //      itself
    cptr1 = static_cast<polymorphic<BaseType>&&>(cptr1);

    REQUIRE(DerivedType::object_count == 1);

    THEN("The move-assigned-to object is valid") { REQUIRE((bool)cptr1); }
  }
}

TEST_CASE("make_polymorphic with single template argument",
          "[polymorphic.make_polymorphic.single]") {
  auto pv = make_polymorphic<DerivedType>(7);
  static_assert(
      std::is_same<decltype(pv), polymorphic<DerivedType>>::value, "");
  REQUIRE(pv->value() == 7);
}

TEST_CASE("make_polymorphic with two template arguments",
          "[polymorphic.make_polymorphic.double]") {
  auto pv = make_polymorphic<BaseType, DerivedType>(7);
  static_assert(std::is_same<decltype(pv), polymorphic<BaseType>>::value,
                "");
  REQUIRE(pv->value() == 7);
}

struct B;
struct A {
  A(B);
  A() = default;
};
struct B : A {};

TEST_CASE("make_polymorphic ambiguity",
          "[polymorphic.make_polymorphic.ambiguity]") {
  B b;
  // This was ambiguous with two make_polymorphic functions.
  auto pv = make_polymorphic<A, B>(b);
}

TEST_CASE("Derived types", "[polymorphic.derived_types]") {
  GIVEN(
      "A polymorphic<BaseType> constructed from "
      "make_polymorphic<DerivedType>") {
    int v = 7;
    auto cptr = make_polymorphic<DerivedType>(v);

    WHEN("A polymorphic<BaseType> is copy-constructed") {
      polymorphic<BaseType> bptr(cptr);

      THEN("Operator-> calls the pointee method") {
        REQUIRE(bptr->value() == v);
      }

      THEN("operator bool returns true") { REQUIRE((bool)bptr == true); }
    }

    WHEN("A polymorphic<BaseType> is move-constructed") {
      polymorphic<BaseType> bptr(std::move(cptr));

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
          "[polymorphic.constructors]") {
  GIVEN("A value-constructed multiply-derived-class polymorphic") {
    int v = 7;
    polymorphic<MultiplyDerived> cptr(std::in_place_type<MultiplyDerived>,
                                            v);

    THEN(
        "When copied to a polymorphic to an intermediate base type, "
        "data is accessible as expected") {
      auto cptr_IA = polymorphic<IntermediateBaseA>(cptr);
      REQUIRE(cptr_IA->a_ == 3);
      REQUIRE(cptr_IA->v_ == 42);
    }

    THEN(
        "When copied to a polymorphic to an intermediate base type, "
        "data is accessible as expected") {
      auto cptr_IB = polymorphic<IntermediateBaseB>(cptr);
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
          "[polymorphic.exception_safety.copy]") {
  GIVEN("A value-constructed polymorphic to a ThrowsOnCopy") {
    const int v = 7;
    polymorphic<ThrowsOnCopy> cptr(std::in_place_type<ThrowsOnCopy>, v);

    THEN(
        "When copying to another polymorphic, after an exception, the "
        "source remains valid") {
      Tracked::reset_counts();
      polymorphic<ThrowsOnCopy> another;
      REQUIRE_THROWS_AS(another = cptr, std::runtime_error);
      REQUIRE(cptr->value_ == v);
      REQUIRE(Tracked::ctor_count_ - Tracked::dtor_count_ == 0);
    }

    THEN(
        "When copying to another polymorphic, after an exception, the "
        "destination is not changed") {
      const int v2 = 5;
      polymorphic<ThrowsOnCopy> another(std::in_place_type<ThrowsOnCopy>,
                                              v2);
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
          "[polymorphic.exception_safety.copier]") {
  GIVEN("A pointer-constructed polymorphic") {
    const int v = 7;
    polymorphic<TrackedValue> cptr(new TrackedValue(v),
                                         throwing_copier<TrackedValue>{});

    THEN("When an exception occurs in the copier, the source is unchanged") {
      polymorphic<TrackedValue> another;
      Tracked::reset_counts();
      REQUIRE_THROWS_AS(another = cptr, std::bad_alloc);
      REQUIRE(cptr->value_ == v);
      REQUIRE(Tracked::ctor_count_ - Tracked::dtor_count_ == 0);
    }

    THEN(
        "When an exception occurs in the copier, the destination is "
        "unchanged") {
      const int v2 = 5;
      polymorphic<TrackedValue> another(std::in_place_type<TrackedValue>,
                                              v2);
      Tracked::reset_counts();
      REQUIRE_THROWS_AS(another = cptr, std::bad_alloc);
      REQUIRE(another->value_ == v2);
      REQUIRE(Tracked::ctor_count_ - Tracked::dtor_count_ == 0);
    }
  }
}

TEST_CASE("polymorphic<const T>",
          "[polymorphic.compatible_types]") {
  polymorphic<const DerivedType> p(std::in_place_type<DerivedType>, 7);
  REQUIRE(p->value() == 7);
  // Will not compile as p is polymorphic<const DerivedType> not
  // polymorphic<DerivedType>
  // p->set_value(42);
}

TEST_CASE("Check exception object construction",
          "[polymorphic.construction.exception]") {
  bad_polymorphic_construction exception;
  // Should find an error message referencing polymorphic.
  CHECK(std::string(exception.what()).find("polymorphic") !=
        std::string::npos);
}

class DeeplyDerivedType : public DerivedType {
 public:
  DeeplyDerivedType() : DerivedType(0) {}
};

TEST_CASE("polymorphic dynamic and static type mismatch",
          "[polymorphic.construction]") {
  DeeplyDerivedType dd;
  DerivedType* p = &dd;

  CHECK(typeid(*p) != typeid(DerivedType));

  CHECK_THROWS_AS([p] { return polymorphic<BaseType>(p); }(),
                  bad_polymorphic_construction);
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
    "polymorphic dynamic and static type mismatch is not a problem "
    "with custom copier or deleter",
    "[polymorphic.construction]") {
  DeeplyDerivedType dd;
  DerivedType* p = &dd;

  CHECK(typeid(*p) != typeid(DerivedType));

  CHECK_NOTHROW([p] {
    return polymorphic<BaseType>(p, fake_copy{}, no_deletion{});
  }());
}

TEST_CASE("Dangling reference in forwarding constructor",
          "[polymorphic.constructors]") {
  auto d = DerivedType(7);
  auto& rd = d;
  polymorphic<DerivedType> p(std::in_place_type<DerivedType>, rd);

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
    polymorphic<DerivedType> p(mem, std::allocator_arg_t{}, alloc);
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
    polymorphic<DerivedType> p(mem, std::allocator_arg_t{}, alloc);
    polymorphic<DerivedType> p2(p);
    CHECK(allocs == 3);
    CHECK(deallocs == 0);
  }
  CHECK(allocs == 3);
  CHECK(deallocs == 4);
}

TEST_CASE("Allocator used to construct with allocate_polymorphic") {
  unsigned allocs = 0;
  unsigned deallocs = 0;

  tracking_allocator<DerivedType> alloc(&allocs, &deallocs);

  {
    unsigned const value = 99;
    polymorphic<DerivedType> p = allocate_polymorphic<DerivedType>(
        std::allocator_arg_t{}, alloc, value);
    CHECK(allocs == 2);
    CHECK(deallocs == 0);
  }
  CHECK(allocs == 2);
  CHECK(deallocs == 2);
}
