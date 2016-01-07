#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include "deep_ptr.h"

struct BaseType
{
  virtual int value() const = 0;
  virtual ~BaseType() = default;
};

struct DerivedType : BaseType
{
  int value_ = 0;
  DerivedType() = default;
  DerivedType(int v) : value_(v) {}
  int value() const override { return value_; }
};

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
}
