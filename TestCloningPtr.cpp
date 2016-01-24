#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include "cloning_ptr.h"

struct BaseType
{
  virtual int value() = 0; // intentionally non-const
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

  int value() override { return value_; }

  void set_value(int i) override { value_ = i; }

  static size_t object_count;
};

size_t DerivedType::object_count = 0;

TEST_CASE("Default constructor","[cloning_ptr.constructors]")
{
  GIVEN("A default constructed cloning_ptr to BaseType")
  {
    cloning_ptr<BaseType> cptr;

    THEN("get returns nullptr")
    {
      REQUIRE(cptr.get() == nullptr);
    }

    THEN("operator-> returns nullptr")
    {
      REQUIRE(cptr.operator->() == nullptr);
    }

    THEN("operator bool returns false")
    {
      REQUIRE((bool)cptr == false);
    }
  }

  GIVEN("A default constructed const cloning_ptr to BaseType")
  {
    const cloning_ptr<BaseType> ccptr;

    THEN("get returns nullptr")
    {
      REQUIRE(ccptr.get() == nullptr);
    }

    THEN("operator-> returns nullptr")
    {
      REQUIRE(ccptr.operator->() == nullptr);
    }

    THEN("operator bool returns false")
    {
      REQUIRE((bool)ccptr == false);
    }
  }
}

TEST_CASE("Pointer constructor","[cloning_ptr.constructors]")
{
  GIVEN("A pointer-constructed cloning_ptr")
  {
    int v = 7;
    cloning_ptr<BaseType> cptr(new DerivedType(v));

    THEN("get returns a non-null pointer")
    {
      REQUIRE(cptr.get() != nullptr);
    }

    THEN("Operator-> calls the pointee method")
    {
      REQUIRE(cptr->value() == v);
    }

    THEN("operator bool returns true")
    {
      REQUIRE((bool)cptr == true);
    }
  }
  GIVEN("A pointer-constructed const cloning_ptr")
  {
    int v = 7;
    const cloning_ptr<BaseType> ccptr(new DerivedType(v));

    THEN("get returns a non-null pointer")
    {
      REQUIRE(ccptr.get() != nullptr);
    }

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

TEST_CASE("cloning_ptr destructor","[cloning_ptr.destructor]")
{
  GIVEN("No derived objects")
  {
    REQUIRE(DerivedType::object_count == 0);

    THEN("Object count is increased on construction and decreased on destruction")
    {
      // begin and end scope to force destruction
      {
        cloning_ptr<BaseType> tmp(new DerivedType());
        REQUIRE(DerivedType::object_count == 1);
      }
      REQUIRE(DerivedType::object_count == 0);
    }
  }
}

TEST_CASE("cloning_ptr copy constructor","[cloning_ptr.constructors]")
{
  GIVEN("A cloning_ptr copied from a default-constructed cloning_ptr")
  {
    cloning_ptr<BaseType> original_cptr;
    cloning_ptr<BaseType> cptr(original_cptr);

    THEN("get returns nullptr")
    {
      REQUIRE(cptr.get() == nullptr);
    }

    THEN("operator-> returns nullptr")
    {
      REQUIRE(cptr.operator->() == nullptr);
    }

    THEN("operator bool returns false")
    {
      REQUIRE((bool)cptr == false);
    }
  }

  GIVEN("A cloning_ptr copied from a pointer-constructed cloning_ptr")
  {
    REQUIRE(DerivedType::object_count == 0);

    int v = 7;
    cloning_ptr<BaseType> original_cptr(new DerivedType(v));
    cloning_ptr<BaseType> cptr(original_cptr);

    THEN("get returns a distinct non-null pointer")
    {
      REQUIRE(cptr.get() != nullptr);
      REQUIRE(cptr.get() != original_cptr.get());
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

TEST_CASE("cloning_ptr move constructor","[cloning_ptr.constructors]")
{
  GIVEN("A cloning_ptr move-constructed from a default-constructed cloning_ptr")
  {
    cloning_ptr<BaseType> original_cptr;
    cloning_ptr<BaseType> cptr(std::move(original_cptr));

    THEN("The original pointer is null")
    {
      REQUIRE(original_cptr.get()==nullptr);
      REQUIRE(original_cptr.operator->()==nullptr);
      REQUIRE(!(bool)original_cptr);
    }

    THEN("The move-constructed pointer is null")
    {
      REQUIRE(cptr.get()==nullptr);
      REQUIRE(cptr.operator->()==nullptr);
      REQUIRE(!(bool)cptr);
    }
  }

  GIVEN("A cloning_ptr move-constructed from a default-constructed cloning_ptr")
  {
    int v = 7;
    cloning_ptr<BaseType> original_cptr(new DerivedType(v));
    auto original_pointer = original_cptr.get();
    CHECK(DerivedType::object_count == 1);

    cloning_ptr<BaseType> cptr(std::move(original_cptr));
    CHECK(DerivedType::object_count == 1);

    THEN("The original pointer is null")
    {
      REQUIRE(original_cptr.get()==nullptr);
      REQUIRE(original_cptr.operator->()==nullptr);
      REQUIRE(!(bool)original_cptr);
    }

    THEN("The move-constructed pointer is the original pointer")
    {
      REQUIRE(cptr.get()==original_pointer);
      REQUIRE(cptr.operator->()==original_pointer);
      REQUIRE((bool)cptr);
    }

    THEN("The move-constructed pointer value is the constructed value")
    {
      REQUIRE(cptr->value() == v);
    }
  }
}

TEST_CASE("cloning_ptr assignment","[cloning_ptr.assignment]")
{
  GIVEN("A default-constructed cloning_ptr assigned-to a default-constructed cloning_ptr")
  {
    cloning_ptr<BaseType> cptr1;
    const cloning_ptr<BaseType> cptr2;
    const auto p = cptr2.get();

    REQUIRE(DerivedType::object_count == 0);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 0);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(cptr2.get() == p);
    }

    THEN("The assigned-to object is null")
    {
      REQUIRE(cptr1.get() == nullptr);
    }
  }

  GIVEN("A default-constructed cloning_ptr assigned to a pointer-constructed cloning_ptr")
  {
    int v1 = 7;

    cloning_ptr<BaseType> cptr1(new DerivedType(v1));
    const cloning_ptr<BaseType> cptr2;
    const auto p = cptr2.get();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 0);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(cptr2.get() == p);
    }

    THEN("The assigned-to object is null")
    {
      REQUIRE(cptr1.get() == nullptr);
    }
  }

  GIVEN("A pointer-constructed cloning_ptr assigned to a default-constructed cloning_ptr")
  {
    int v1 = 7;

    cloning_ptr<BaseType> cptr1;
    const cloning_ptr<BaseType> cptr2(new DerivedType(v1));
    const auto p = cptr2.get();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 2);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(cptr2.get() == p);
    }

    THEN("The assigned-to object is non-null")
    {
      REQUIRE(cptr1.get() != nullptr);
    }

    THEN("The assigned-from object 'value' is the assigned-to object value")
    {
      REQUIRE(cptr1->value() == cptr2->value());
    }

    THEN("The assigned-from object pointer and the assigned-to object pointer are distinct")
    {
      REQUIRE(cptr1.get() != cptr2.get());
    }

  }

  GIVEN("A pointer-constructed cloning_ptr assigned to a pointer-constructed cloning_ptr")
  {
    int v1 = 7;
    int v2 = 87;

    cloning_ptr<BaseType> cptr1(new DerivedType(v1));
    const cloning_ptr<BaseType> cptr2(new DerivedType(v2));
    const auto p = cptr2.get();

    REQUIRE(DerivedType::object_count == 2);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 2);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(cptr2.get() == p);
    }

    THEN("The assigned-to object is non-null")
    {
      REQUIRE(cptr1.get() != nullptr);
    }

    THEN("The assigned-from object 'value' is the assigned-to object value")
    {
      REQUIRE(cptr1->value() == cptr2->value());
    }

    THEN("The assigned-from object pointer and the assigned-to object pointer are distinct")
    {
      REQUIRE(cptr1.get() != cptr2.get());
    }
  }

  GIVEN("A pointer-constructed cloning_ptr assigned to itself")
  {
    int v1 = 7;

    cloning_ptr<BaseType> cptr1(new DerivedType(v1));
    const auto p = cptr1.get();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr1;

    REQUIRE(DerivedType::object_count == 1);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(cptr1.get() == p);
    }
  }
}

TEST_CASE("cloning_ptr move-assignment","[cloning_ptr.assignment]")
{
  GIVEN("A default-constructed cloning_ptr move-assigned-to a default-constructed cloning_ptr")
  {
    cloning_ptr<BaseType> cptr1;
    cloning_ptr<BaseType> cptr2;
    const auto p = cptr2.get();

    REQUIRE(DerivedType::object_count == 0);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 0);

    THEN("The move-assigned-from object is null")
    {
      REQUIRE(cptr1.get() == nullptr);
    }

    THEN("The move-assigned-to object is null")
    {
      REQUIRE(cptr1.get() == nullptr);
    }
  }

  GIVEN("A default-constructed cloning_ptr move-assigned to a pointer-constructed cloning_ptr")
  {
    int v1 = 7;

    cloning_ptr<BaseType> cptr1(new DerivedType(v1));
    cloning_ptr<BaseType> cptr2;
    const auto p = cptr2.get();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 0);

    THEN("The move-assigned-from object is null")
    {
      REQUIRE(cptr1.get() == nullptr);
    }

    THEN("The move-assigned-to object is null")
    {
      REQUIRE(cptr1.get() == nullptr);
    }
  }

  GIVEN("A pointer-constructed cloning_ptr move-assigned to a default-constructed cloning_ptr")
  {
    int v1 = 7;

    cloning_ptr<BaseType> cptr1;
    cloning_ptr<BaseType> cptr2(new DerivedType(v1));
    const auto p = cptr2.get();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 1);

    THEN("The move-assigned-from object is null")
    {
      REQUIRE(cptr2.get() == nullptr);
    }

    THEN("The move-assigned-to object pointer is the move-assigned-from pointer")
    {
      REQUIRE(cptr1.get() == p);
    }
  }

  GIVEN("A pointer-constructed cloning_ptr move-assigned to a pointer-constructed cloning_ptr")
  {
    int v1 = 7;
    int v2 = 87;

    cloning_ptr<BaseType> cptr1(new DerivedType(v1));
    cloning_ptr<BaseType> cptr2(new DerivedType(v2));
    const auto p = cptr2.get();

    REQUIRE(DerivedType::object_count == 2);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 1);

    THEN("The move-assigned-from object is null")
    {
      REQUIRE(cptr2.get() == nullptr);
    }

    THEN("The move-assigned-to object pointer is the move-assigned-from pointer")
    {
      REQUIRE(cptr1.get() == p);
    }
  }

  GIVEN("A pointer-constructed cloning_ptr move-assigned to itself")
  {
    int v = 7;

    cloning_ptr<BaseType> cptr(new DerivedType(v));
    const auto p = cptr.get();

    REQUIRE(DerivedType::object_count == 1);

    cptr = std::move(cptr);

    THEN("The cloning_ptr is unaffected")
    {
      REQUIRE(DerivedType::object_count == 1);
      REQUIRE(cptr.get() == p);
    }
  }
}

TEST_CASE("Derived types", "[cloning_ptr.derived_types]")
{
  GIVEN("A cloning_ptr<BaseType> constructed from make_cloning_ptr<DerivedType>")
  {
    int v = 7;
    auto cptr = make_cloning_ptr<DerivedType>(v);

    WHEN("A cloning_ptr<BaseType> is copy-constructed")
    {
      cloning_ptr<BaseType> bptr(cptr);

      THEN("get returns a non-null pointer")
      {
        REQUIRE(bptr.get() != nullptr);
      }

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->value() == v);
      }

      THEN("operator bool returns true")
      {
        REQUIRE((bool)bptr == true);
      }
    }

    WHEN("A cloning_ptr<BaseType> is assigned")
    {
      cloning_ptr<BaseType> bptr;
      bptr = cptr;

      THEN("get returns a non-null pointer")
      {
        REQUIRE(bptr.get() != nullptr);
      }

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->value() == v);
      }

      THEN("operator bool returns true")
      {
        REQUIRE((bool)bptr == true);
      }
    }

    WHEN("A cloning_ptr<BaseType> is move-constructed")
    {
      cloning_ptr<BaseType> bptr(std::move(cptr));

      THEN("get returns a non-null pointer")
      {
        REQUIRE(bptr.get() != nullptr);
      }

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->value() == v);
      }

      THEN("operator bool returns true")
      {
        REQUIRE((bool)bptr == true);
      }
    }

    WHEN("A cloning_ptr<BaseType> is move-assigned")
    {
      cloning_ptr<BaseType> bptr;
      bptr = std::move(cptr);

      THEN("get returns a non-null pointer")
      {
        REQUIRE(bptr.get() != nullptr);
      }

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

TEST_CASE("make_cloning_ptr return type can be converted to base-type", "[cloning_ptr.make_cloning_ptr]")
{
  GIVEN("A cloning_ptr<BaseType> constructed from make_cloning_ptr<DerivedType>")
  {
    int v = 7;
    cloning_ptr<BaseType> cptr = make_cloning_ptr<DerivedType>(v);

    THEN("get returns a non-null pointer")
    {
      REQUIRE(cptr.get() != nullptr);
    }

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

TEST_CASE("release","[cloning_ptr.release]")
{
  GIVEN("An empty cloning_ptr")
  {
    cloning_ptr<DerivedType> cptr;

    WHEN("release is called")
    {
      auto p = cptr.release();

      THEN("The cloning_ptr remains empty and the returned pointer is null")
      {
        REQUIRE(!cptr);
        REQUIRE(cptr.get()==nullptr);
        REQUIRE(p==nullptr);
      }
    }
  }

  GIVEN("A non-empty cloning_ptr")
  {
    int v = 7;
    cloning_ptr<DerivedType> cptr(new DerivedType(v));
    CHECK(DerivedType::object_count == 1);

    const auto op =cptr.get();


    WHEN("release is called")
    {
      auto p = cptr.release();
      std::unique_ptr<DerivedType> cleanup(p);
      CHECK(DerivedType::object_count == 1);

      THEN("The cloning_ptr is empty and the returned pointer is the previous pointer value")
      {
        REQUIRE(!cptr);
        REQUIRE(cptr.get()==nullptr);
        REQUIRE(p==op);
      }
    }
    CHECK(DerivedType::object_count == 0);
  }
}

TEST_CASE("reset","[cloning_ptr.reset]")
{
  GIVEN("An empty cloning_ptr")
  {
    cloning_ptr<DerivedType> cptr;

    WHEN("reset to null")
    {
      cptr.reset();

      THEN("The cloning_ptr remains empty")
      {
        REQUIRE(!cptr);
        REQUIRE(cptr.get()==nullptr);
      }
    }

    WHEN("reset to non-null")
    {
      int v = 7;
      cptr.reset(new DerivedType(v));

      CHECK(DerivedType::object_count == 1);

      THEN("The cloning_ptr is non-empty and owns the pointer")
      {
        REQUIRE(cptr);
        REQUIRE(cptr.get()!=nullptr);
        REQUIRE(cptr->value() == v);
      }
    }
  }
  CHECK(DerivedType::object_count == 0);

  GIVEN("A non-empty cloning_ptr")
  {
    int v1 = 7;
    cloning_ptr<DerivedType> cptr(new DerivedType(v1));
    CHECK(DerivedType::object_count == 1);

    WHEN("reset to null")
    {
      cptr.reset();
      CHECK(DerivedType::object_count == 0);

      THEN("The cloning_ptr is empty")
      {
        REQUIRE(!cptr);
        REQUIRE(cptr.get()==nullptr);
      }
    }

    WHEN("reset to non-null")
    {
      int v2 = 7;
      cptr.reset(new DerivedType(v2));
      CHECK(DerivedType::object_count == 1);

      THEN("The cloning_ptr is non-empty and owns the pointer")
      {
        REQUIRE(cptr);
        REQUIRE(cptr.get()!=nullptr);
        REQUIRE(cptr->value() == v2);
      }
    }
  }
}

struct AlternativeBaseType {
  virtual ~AlternativeBaseType() = default;
  virtual int alternative_value() = 0;
};

class AlternativeDerivedType : public BaseType, public AlternativeBaseType {
  int value_;
public:
  AlternativeDerivedType(int value) : value_(value) {}

  int value() override { return value_; }
  void set_value(int v) override { value_ = v; }
  int alternative_value() override { return value_; }
};

TEST_CASE("cast operations", "[cloning_ptr.casts]")
{
  GIVEN("A pointer-constructed cloning_ptr<BaseType>")
  {
    int v = 7;
    cloning_ptr<BaseType> cptr(new DerivedType(v));
    REQUIRE(DerivedType::object_count == 1);

    WHEN("static_pointer_cast to the derived type is called")
    {
      auto st_cptr = std::static_pointer_cast<DerivedType>(cptr);

      THEN("The static-cast pointer is non-null")
      {
        REQUIRE(st_cptr);
      }
      THEN("The static-cast pointer has the required value")
      {
        REQUIRE(st_cptr->value() == v);
      }
      THEN("The static-cast pointer is distinct from the original pointer")
      {
        REQUIRE(st_cptr.get() != cptr.get());
      }
      THEN("Object count is increased")
      {
        REQUIRE(DerivedType::object_count == 2);
      }
    }
    WHEN("dynamic_pointer_cast to the derived type is called")
    {
      auto dyn_cptr = std::dynamic_pointer_cast<DerivedType>(cptr);

      THEN("The dynamic-cast pointer is non-null")
      {
        REQUIRE(dyn_cptr);
      }
      THEN("The dynamic-cast pointer has the required value")
      {
        REQUIRE(dyn_cptr->value() == v);
      }
      THEN("The dynamic-cast pointer is distinct from the original pointer")
      {
        REQUIRE(dyn_cptr.get() != cptr.get());
      }
      THEN("Object count is increased")
      {
        REQUIRE(DerivedType::object_count == 2);
      }
    }
    WHEN("dynamic_pointer_cast to the wrong derived type is called")
    {
      auto dyn_cptr = std::dynamic_pointer_cast<AlternativeDerivedType>(cptr);

      THEN("The dynamic-cast pointer is null")
      {
        REQUIRE(!dyn_cptr);
      }
      THEN("Object count is unchanged")
      {
        REQUIRE(DerivedType::object_count == 1);
      }
    }
  }
  GIVEN("A pointer-constructed cloning_ptr<const DerivedType>")
  {
    int v = 7;
    cloning_ptr<const DerivedType> ccptr(new DerivedType(v));
    REQUIRE(DerivedType::object_count == 1);

    WHEN("static_pointer_cast to the derived type is called")
    {
      auto cptr = std::const_pointer_cast<DerivedType>(ccptr);

      THEN("The static-cast pointer is non-null")
      {
        REQUIRE(cptr);
      }
      THEN("The static-cast pointer has the required value")
      {
        REQUIRE(cptr->value() == v);
      }
      THEN("The static-cast pointer is distinct from the original pointer")
      {
        REQUIRE(cptr.get() != ccptr.get());
      }
      THEN("Object count is increased")
      {
        REQUIRE(DerivedType::object_count == 2);
      }
    }
  }
  GIVEN("An AlternativeDerivedType-pointer-constructed cloning_ptr<BaseType>")
  {
    int v = 7;
    cloning_ptr<BaseType> cptr(new AlternativeDerivedType(v));

    WHEN("dynamic_pointer_cast to AlternativeBaseType is called")
    {
      auto dyn_cptr = std::dynamic_pointer_cast<AlternativeBaseType>(cptr);

      THEN("The dynamic-cast pointer is non-null")
      {
        REQUIRE(dyn_cptr);
      }
      THEN("The dynamic-cast pointer has the required value")
      {
        REQUIRE(dyn_cptr->alternative_value() == v);
      }
      THEN("The dynamic-cast pointer is distinct from the original pointer")
      {
        REQUIRE(dyn_cptr.get() != dynamic_cast<AlternativeBaseType*>(cptr.get()));
      }
    }
  }
}

struct Base { int v_ = 42; virtual ~Base() = default; };
struct IntermediateBaseA : virtual Base { int a_ = 3; };
struct IntermediateBaseB : virtual Base { int b_ = 101; };
struct MultiplyDerived : IntermediateBaseA, IntermediateBaseB { int value_ = 0; MultiplyDerived(int value) : value_(value) {}; };

TEST_CASE("Gustafsson's dilemma: multiple (virtual) base classes", "[cloning_ptr.constructors]")
{
  GIVEN("A value-constructed multiply-derived-class cloning_ptr")
  {
    int v = 7;
    cloning_ptr<MultiplyDerived> cptr(new MultiplyDerived(v));

    THEN("When copied to a cloning_ptr to an intermediate base type, data is accessible as expected")
    {
      cloning_ptr<IntermediateBaseA> cptr_IA = cptr;
      REQUIRE(cptr_IA->a_ == 3);
      REQUIRE(cptr_IA->v_ == 42);
    }

    THEN("When copied to a cloning_ptr to an intermediate base type, data is accessible as expected")
    {
      cloning_ptr<IntermediateBaseB> cptr_IB = cptr;
      REQUIRE(cptr_IB->b_ == 101);
      REQUIRE(cptr_IB->v_ == 42);
    }
  }
}

