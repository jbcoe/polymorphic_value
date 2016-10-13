#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include "indirect.h"

struct BaseType
{
  virtual int value() const = 0;
  virtual void set_value(int) = 0;
  virtual ~BaseType() = default;
};

struct DerivedType : BaseType
{
  int value_ = 0;

  DerivedType()
  {
    ++object_count;
  }

  DerivedType(const DerivedType& d)
  {
    value_ = d.value_;
    ++object_count;
  }

  DerivedType(int v) : value_(v)
  {
    ++object_count;
  }

  ~DerivedType()
  {
    --object_count;
  }

  int value() const override { return value_; }

  void set_value(int i) override { value_ = i; }

  static size_t object_count;
};

size_t DerivedType::object_count = 0;

TEST_CASE("Default constructor","[indirect.constructors]")
{
  GIVEN("A default constructed indirect to BaseType")
  {
    indirect<BaseType> cptr;

    THEN("operator bool returns false")
    {
      REQUIRE((bool)cptr == false);
    }
  }

  GIVEN("A default constructed const indirect to BaseType")
  {
    const indirect<BaseType> ccptr;

    THEN("operator bool returns false")
    {
      REQUIRE((bool)ccptr == false);
    }
  }
}

TEST_CASE("Pointer constructor","[indirect.constructors]")
{
  GIVEN("A pointer-constructed indirect")
  {
    int v = 7;
    indirect<BaseType> cptr(new DerivedType(v));

    THEN("Operator-> calls the pointee method")
    {
      REQUIRE(cptr->value() == v);
    }

    THEN("operator bool returns true")
    {
      REQUIRE((bool)cptr == true);
    }
  }
  GIVEN("A pointer-constructed const indirect")
  {
    int v = 7;
    const indirect<BaseType> ccptr(new DerivedType(v));

    THEN("Operator-> calls the pointee method")
    {
      REQUIRE(ccptr->value() == v);
    }

    THEN("operator bool returns true")
    {
      REQUIRE((bool)ccptr == true);
    }
  }
}

struct BaseCloneSelf 
{
  BaseCloneSelf() = default;
  virtual ~BaseCloneSelf() = default;
  BaseCloneSelf(const BaseCloneSelf &) = delete;
  virtual std::unique_ptr<BaseCloneSelf> clone() const = 0;
};

struct DerivedCloneSelf : BaseCloneSelf
{
  static size_t object_count;
  std::unique_ptr<BaseCloneSelf> clone() const { return std::make_unique<DerivedCloneSelf>(); }
  DerivedCloneSelf() { ++object_count; }
  ~DerivedCloneSelf(){ --object_count; }
};

size_t DerivedCloneSelf::object_count = 0;

struct invoke_clone_member
{
  template <typename T> T *operator()(const T &t) const {
    return static_cast<T *>(t.clone().release());
  }
};

TEST_CASE("indirect constructed with copier and deleter",
          "[indirect.constructor]") {
  size_t copy_count = 0;
  size_t deletion_count = 0;
  auto cp = indirect<DerivedType>(new DerivedType(),
                                    [&](const DerivedType &d) {
                                      ++copy_count;
                                      return new DerivedType(d);
                                    },
                                    [&](const DerivedType *d) {
                                      ++deletion_count;
                                      delete d;
                                    });
  {
    auto cp2 = cp;
    REQUIRE(copy_count == 1);
  }
  REQUIRE(deletion_count == 1);
}

TEST_CASE("indirect destructor","[indirect.destructor]")
{
  GIVEN("No derived objects")
  {
    REQUIRE(DerivedType::object_count == 0);

    THEN("Object count is increased on construction and decreased on destruction")
    {
      // begin and end scope to force destruction
      {
        indirect<BaseType> tmp(new DerivedType());
        REQUIRE(DerivedType::object_count == 1);
      }
      REQUIRE(DerivedType::object_count == 0);
    }
  }
}

TEST_CASE("indirect copy constructor","[indirect.constructors]")
{
  GIVEN("A indirect copied from a default-constructed indirect")
  {
    indirect<BaseType> original_cptr;
    indirect<BaseType> cptr(original_cptr);

    THEN("operator bool returns false")
    {
      REQUIRE((bool)cptr == false);
    }
  }

  GIVEN("A indirect copied from a pointer-constructed indirect")
  {
    REQUIRE(DerivedType::object_count == 0);

    int v = 7;
    indirect<BaseType> original_cptr(new DerivedType(v));
    indirect<BaseType> cptr(original_cptr);

    THEN("values are distinct")
    {
      REQUIRE(&cptr.value() != &original_cptr.value());
    }

    THEN("Operator-> calls the pointee method")
    {
      REQUIRE(cptr->value() == v);
    }

    THEN("operator bool returns true")
    {
      REQUIRE((bool)cptr == true);
    }

    THEN("object count is two")
    {
      REQUIRE(DerivedType::object_count == 2);
    }

    WHEN("Changes are made to the original cloning pointer after copying")
    {
      int new_value = 99;
      original_cptr->set_value(new_value);
      REQUIRE(original_cptr->value() == new_value);
      THEN("They are not reflected in the copy (copy is distinct)")
      {
        REQUIRE(cptr->value() != new_value);
        REQUIRE(cptr->value() == v);
      }
    }
  }
}

TEST_CASE("indirect move constructor","[indirect.constructors]")
{
  GIVEN("A indirect move-constructed from a default-constructed indirect")
  {
    indirect<BaseType> original_cptr;
    indirect<BaseType> cptr(std::move(original_cptr));

    THEN("The original indirect is empty")
    {
      REQUIRE(!(bool)original_cptr);
    }

    THEN("The move-constructed indirect is empty")
    {
      REQUIRE(!(bool)cptr);
    }
  }

  GIVEN("A indirect move-constructed from a default-constructed indirect")
  {
    int v = 7;
    indirect<BaseType> original_cptr(new DerivedType(v));
    auto original_pointer = &original_cptr.value();
    CHECK(DerivedType::object_count == 1);

    indirect<BaseType> cptr(std::move(original_cptr));
    CHECK(DerivedType::object_count == 1);

    THEN("The original indirect is empty")
    {
      REQUIRE(!(bool)original_cptr);
    }

    THEN("The move-constructed pointer is the original pointer")
    {
      REQUIRE(&cptr.value()==original_pointer);
      REQUIRE(cptr.operator->()==original_pointer);
      REQUIRE((bool)cptr);
    }

    THEN("The move-constructed pointer value is the constructed value")
    {
      REQUIRE(cptr->value() == v);
    }
  }
}

TEST_CASE("indirect assignment","[indirect.assignment]")
{
  GIVEN("A default-constructed indirect assigned-to a default-constructed indirect")
  {
    indirect<BaseType> cptr1;
    const indirect<BaseType> cptr2;
    const auto p = &cptr2.value();

    REQUIRE(DerivedType::object_count == 0);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 0);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(&cptr2.value() == p);
    }

    THEN("The assigned-to object is empty")
    {
      REQUIRE(!cptr1);
    }
  }

  GIVEN("A default-constructed indirect assigned to a pointer-constructed indirect")
  {
    int v1 = 7;

    indirect<BaseType> cptr1(new DerivedType(v1));
    const indirect<BaseType> cptr2;
    const auto p = &cptr2.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 0);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(&cptr2.value() == p);
    }

    THEN("The assigned-to object is empty")
    {
      REQUIRE(!cptr1);
    }
  }

  GIVEN("A pointer-constructed indirect assigned to a default-constructed indirect")
  {
    int v1 = 7;

    indirect<BaseType> cptr1;
    const indirect<BaseType> cptr2(new DerivedType(v1));
    const auto p = &cptr2.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 2);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(&cptr2.value() == p);
    }

    THEN("The assigned-to object is non-empty")
    {
      REQUIRE((bool)cptr1);
    }

    THEN("The assigned-from object 'value' is the assigned-to object value")
    {
      REQUIRE(cptr1->value() == cptr2->value());
    }

    THEN("The assigned-from object pointer and the assigned-to object pointer are distinct")
    {
      REQUIRE(&cptr1.value() != &cptr2.value());
    }

  }

  GIVEN("A pointer-constructed indirect assigned to a pointer-constructed indirect")
  {
    int v1 = 7;
    int v2 = 87;

    indirect<BaseType> cptr1(new DerivedType(v1));
    const indirect<BaseType> cptr2(new DerivedType(v2));
    const auto p = &cptr2.value();

    REQUIRE(DerivedType::object_count == 2);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 2);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(&cptr2.value() == p);
    }

    THEN("The assigned-to object is non-empty")
    {
      REQUIRE((bool)cptr1);
    }

    THEN("The assigned-from object 'value' is the assigned-to object value")
    {
      REQUIRE(cptr1->value() == cptr2->value());
    }

    THEN("The assigned-from object pointer and the assigned-to object pointer are distinct")
    {
      REQUIRE(&cptr1.value() != &cptr2.value());
    }
  }

  GIVEN("A pointer-constructed indirect assigned to itself")
  {
    int v1 = 7;

    indirect<BaseType> cptr1(new DerivedType(v1));
    const auto p = &cptr1.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr1;

    REQUIRE(DerivedType::object_count == 1);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(&cptr1.value() == p);
    }
  }
}

TEST_CASE("indirect move-assignment","[indirect.assignment]")
{
  GIVEN("A default-constructed indirect move-assigned-to a default-constructed indirect")
  {
    indirect<BaseType> cptr1;
    indirect<BaseType> cptr2;
    const auto p = &cptr2.value();

    REQUIRE(DerivedType::object_count == 0);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 0);

    THEN("The move-assigned-from object is empty")
    {
      REQUIRE(!cptr2);
    }

    THEN("The move-assigned-to object is empty")
    {
      REQUIRE(!cptr1);
    }
  }

  GIVEN("A default-constructed indirect move-assigned to a pointer-constructed indirect")
  {
    int v1 = 7;

    indirect<BaseType> cptr1(new DerivedType(v1));
    indirect<BaseType> cptr2;
    const auto p = &cptr2.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 0);

    THEN("The move-assigned-from object is empty")
    {
      REQUIRE(!cptr2);
    }

    THEN("The move-assigned-to object is empty")
    {
      REQUIRE(!cptr1);
    }
  }

  GIVEN("A pointer-constructed indirect move-assigned to a default-constructed indirect")
  {
    int v1 = 7;

    indirect<BaseType> cptr1;
    indirect<BaseType> cptr2(new DerivedType(v1));
    const auto p = &cptr2.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 1);

    THEN("The move-assigned-from object is empty")
    {
      REQUIRE(!cptr2);
    }

    THEN("The move-assigned-to object pointer is the move-assigned-from pointer")
    {
      REQUIRE(&cptr1.value() == p);
    }
  }

  GIVEN("A pointer-constructed indirect move-assigned to a pointer-constructed indirect")
  {
    int v1 = 7;
    int v2 = 87;

    indirect<BaseType> cptr1(new DerivedType(v1));
    indirect<BaseType> cptr2(new DerivedType(v2));
    const auto p = &cptr2.value();

    REQUIRE(DerivedType::object_count == 2);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 1);

    THEN("The move-assigned-from object is empty")
    {
      REQUIRE(!cptr2);
    }

    THEN("The move-assigned-to object pointer is the move-assigned-from pointer")
    {
      REQUIRE(&cptr1.value() == p);
    }
  }
}

TEST_CASE("Derived types", "[indirect.derived_types]")
{
  GIVEN("A indirect<BaseType> constructed from make_indirect<DerivedType>")
  {
    int v = 7;
    auto cptr = make_indirect<DerivedType>(v);

    WHEN("A indirect<BaseType> is copy-constructed")
    {
      indirect<BaseType> bptr(cptr);

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->value() == v);
      }

      THEN("operator bool returns true")
      {
        REQUIRE((bool)bptr == true);
      }
    }

    WHEN("A indirect<BaseType> is assigned")
    {
      indirect<BaseType> bptr;
      bptr = cptr;

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->value() == v);
      }

      THEN("operator bool returns true")
      {
        REQUIRE((bool)bptr == true);
      }
    }

    WHEN("A indirect<BaseType> is move-constructed")
    {
      indirect<BaseType> bptr(std::move(cptr));

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->value() == v);
      }

      THEN("operator bool returns true")
      {
        REQUIRE((bool)bptr == true);
      }
    }

    WHEN("A indirect<BaseType> is move-assigned")
    {
      indirect<BaseType> bptr;
      bptr = std::move(cptr);

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->value() == v);
      }

      THEN("operator bool returns true")
      {
        REQUIRE((bool)bptr == true);
      }
    }
  }
}

TEST_CASE("make_indirect return type can be converted to base-type", "[indirect.make_indirect]")
{
  GIVEN("A indirect<BaseType> constructed from make_indirect<DerivedType>")
  {
    int v = 7;
    indirect<BaseType> cptr = make_indirect<DerivedType>(v);

    THEN("Operator-> calls the pointee method")
    {
      REQUIRE(cptr->value() == v);
    }

    THEN("operator bool returns true")
    {
      REQUIRE((bool)cptr == true);
    }
  }
}

struct Base { int v_ = 42; virtual ~Base() = default; };
struct IntermediateBaseA : virtual Base { int a_ = 3; };
struct IntermediateBaseB : virtual Base { int b_ = 101; };
struct MultiplyDerived : IntermediateBaseA, IntermediateBaseB { int value_ = 0; MultiplyDerived(int value) : value_(value) {}; };

TEST_CASE("Gustafsson's dilemma: multiple (virtual) base classes", "[indirect.constructors]")
{
  GIVEN("A value-constructed multiply-derived-class indirect")
  {
    int v = 7;
    indirect<MultiplyDerived> cptr(new MultiplyDerived(v));

    THEN("When copied to a indirect to an intermediate base type, data is accessible as expected")
    {
      indirect<IntermediateBaseA> cptr_IA = cptr;
      REQUIRE(cptr_IA->a_ == 3);
      REQUIRE(cptr_IA->v_ == 42);
    }

    THEN("When copied to a indirect to an intermediate base type, data is accessible as expected")
    {
      indirect<IntermediateBaseB> cptr_IB = cptr;
      REQUIRE(cptr_IB->b_ == 101);
      REQUIRE(cptr_IB->v_ == 42);
    }
  }
}

