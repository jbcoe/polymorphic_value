#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include "deep_ptr.h"

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

TEST_CASE("Default constructor","[deep_ptr.constructors]")
{
  GIVEN("A default constructed deep_ptr to BaseType")
  {
    deep_ptr<BaseType> dptr;

    THEN("get returns nullptr")
    {
      REQUIRE(dptr.get() == nullptr);
    }

    THEN("operator-> returns nullptr")
    {
      REQUIRE(dptr.operator->() == nullptr);
    }

    THEN("operator bool returns false")
    {
      REQUIRE((bool)dptr == false);
    }
  }

  GIVEN("A default constructed const deep_ptr to BaseType")
  {
    const deep_ptr<BaseType> cdptr;

    THEN("get returns nullptr")
    {
      REQUIRE(cdptr.get() == nullptr);
    }

    THEN("operator-> returns nullptr")
    {
      REQUIRE(cdptr.operator->() == nullptr);
    }

    THEN("operator bool returns false")
    {
      REQUIRE((bool)cdptr == false);
    }
  }
}

TEST_CASE("Pointer constructor","[deep_ptr.constructors]")
{
  GIVEN("A pointer-constructed deep_ptr")
  {
    int derived_type_value = 7;
    deep_ptr<BaseType> dptr(new DerivedType(derived_type_value));

    THEN("get returns a non-null pointer")
    {
      REQUIRE(dptr.get() != nullptr);
    }

    THEN("Operator-> calls the pointee method")
    {
      REQUIRE(dptr->value() == derived_type_value);
    }

    THEN("operator bool returns true")
    {
      REQUIRE((bool)dptr == true);
    }
  }
  GIVEN("A pointer-constructed const deep_ptr")
  {
    int derived_type_value = 7;
    const deep_ptr<BaseType> cdptr(new DerivedType(derived_type_value));

    THEN("get returns a non-null pointer")
    {
      REQUIRE(cdptr.get() != nullptr);
    }

    THEN("Operator-> calls the pointee method")
    {
      REQUIRE(cdptr->value() == derived_type_value);
    }

    THEN("operator bool returns true")
    {
      REQUIRE((bool)cdptr == true);
    }
  }
}

TEST_CASE("deep_ptr destructor","[deep_ptr.destructor]")
{
  GIVEN("No derived objects")
  {
    REQUIRE(DerivedType::object_count == 0);

    THEN("Object count is increased on construction and decreased on destruction")
    {
      // begin and end scope to force destruction
      {
        deep_ptr<BaseType> tmp(new DerivedType());
        REQUIRE(DerivedType::object_count == 1);
      }
      REQUIRE(DerivedType::object_count == 0);
    }
  }
}

TEST_CASE("deep_ptr copy constructor","[deep_ptr.constructors]")
{
  GIVEN("A deep_ptr copied from a default-constructed deep_ptr")
  {
    deep_ptr<BaseType> original_dptr;
    deep_ptr<BaseType> dptr(original_dptr);

    THEN("get returns nullptr")
    {
      REQUIRE(dptr.get() == nullptr);
    }

    THEN("operator-> returns nullptr")
    {
      REQUIRE(dptr.operator->() == nullptr);
    }

    THEN("operator bool returns false")
    {
      REQUIRE((bool)dptr == false);
    }
  }

  GIVEN("A deep_ptr copied from a pointer-constructed deep_ptr")
  {
    REQUIRE(DerivedType::object_count == 0);

    int derived_type_value = 7;
    deep_ptr<BaseType> original_dptr(new DerivedType(derived_type_value));
    deep_ptr<BaseType> dptr(original_dptr);

    THEN("get returns a distinct non-null pointer")
    {
      REQUIRE(dptr.get() != nullptr);
      REQUIRE(dptr.get() != original_dptr.get());
    }

    THEN("Operator-> calls the pointee method")
    {
      REQUIRE(dptr->value() == derived_type_value);
    }

    THEN("operator bool returns true")
    {
      REQUIRE((bool)dptr == true);
    }

    THEN("object count is two")
    {
      REQUIRE(DerivedType::object_count == 2);
    }

    WHEN("Changes are made to the original deep pointer after copying")
    {
      int new_value = 99;
      original_dptr->set_value(new_value);
      REQUIRE(original_dptr->value() == new_value);
      THEN("They are not reflected in the copy (copy is distinct)")
      {
        REQUIRE(dptr->value() != new_value);
        REQUIRE(dptr->value() == derived_type_value);
      }
    }
  }
}

TEST_CASE("derived deep_ptr copy constructor", "[deep_ptr.constructors]")
{
  GIVEN("A deep_ptr to a BaseType copy-constructed from a deep_ptr to a derived type")
  {
    int derived_type_value = 7;
    CHECK(DerivedType::object_count == 0);

    deep_ptr<DerivedType> derived(new DerivedType(derived_type_value));
    CHECK(DerivedType::object_count == 1);

    deep_ptr<BaseType> base(derived);
    
    THEN("object count is 2")
    {
      REQUIRE(DerivedType::object_count == 2);
    }
  }
}

TEST_CASE("deep_ptr move constructor","[deep_ptr.constructors]")
{
  GIVEN("A deep_ptr move-constructed from a default-constructed deep_ptr")
  {
    deep_ptr<BaseType> original_dptr;
    deep_ptr<BaseType> dptr(std::move(original_dptr));

    THEN("The original pointer is null")
    {
      REQUIRE(original_dptr.get()==nullptr);
      REQUIRE(original_dptr.operator->()==nullptr);
      REQUIRE(!(bool)original_dptr);
    }

    THEN("The move-constructed pointer is null")
    {
      REQUIRE(dptr.get()==nullptr);
      REQUIRE(dptr.operator->()==nullptr);
      REQUIRE(!(bool)dptr);
    }
  }
  
  GIVEN("A deep_ptr move-constructed from a default-constructed deep_ptr")
  {
    int derived_type_value = 7;
    deep_ptr<BaseType> original_dptr(new DerivedType(derived_type_value));
    auto original_pointer = original_dptr.get();
    CHECK(DerivedType::object_count == 1);

    deep_ptr<BaseType> dptr(std::move(original_dptr));
    CHECK(DerivedType::object_count == 1);

    THEN("The original pointer is null")
    {
      REQUIRE(original_dptr.get()==nullptr);
      REQUIRE(original_dptr.operator->()==nullptr);
      REQUIRE(!(bool)original_dptr);
    }

    THEN("The move-constructed pointer is the original pointer")
    {
      REQUIRE(dptr.get()==original_pointer);
      REQUIRE(dptr.operator->()==original_pointer);
      REQUIRE((bool)dptr);
    }

    THEN("The move-constructed pointer value is the constructed value")
    {
      REQUIRE(dptr->value() == derived_type_value);
    }
  }
}

TEST_CASE("deep_ptr assignment","[deep_ptr.assignment]")
{
  GIVEN("A default-constructed deep_ptr assigned to a default-constructed deep_ptr")
  {
    deep_ptr<BaseType> dptr1;
    deep_ptr<BaseType> dptr2;

    REQUIRE(DerivedType::object_count == 0);
    
    dptr1 = dptr2;

    REQUIRE(DerivedType::object_count == 0);
  }
  
  GIVEN("A default-constructed deep_ptr assigned to a pointer-constructed deep_ptr")
  {
    int v1 = 7;
    
    deep_ptr<BaseType> dptr1(new DerivedType(v1));
    deep_ptr<BaseType> dptr2;
  
    REQUIRE(DerivedType::object_count == 1);

    dptr1 = dptr2;

    REQUIRE(DerivedType::object_count == 0);
  }

  GIVEN("A pointer-constructed deep_ptr assigned to a default-constructed deep_ptr")
  {
    int v1 = 7;
    
    deep_ptr<BaseType> dptr1;
    deep_ptr<BaseType> dptr2(new DerivedType(v1));

    REQUIRE(DerivedType::object_count == 1);
    
    dptr1 = dptr2;
    
    REQUIRE(DerivedType::object_count == 2);
  }
  
  GIVEN("A pointer-constructed deep_ptr assigned to a pointer-constructed deep_ptr")
  {
    int v1 = 7;
    int v2 = 87;

    deep_ptr<BaseType> dptr1(new DerivedType(v1));
    deep_ptr<BaseType> dptr2(new DerivedType(v2));

    REQUIRE(DerivedType::object_count == 2);
    
    dptr1 = dptr2;
    
    REQUIRE(DerivedType::object_count == 2);
  }
}

TEST_CASE("deep_ptr move assignment","[deep_ptr.assignment]")
{
  //REQUIRE("tests are written" == 0);
}
