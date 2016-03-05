#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include "poly.h"

struct BaseType
{
  virtual int data() = 0; // intentionally non-const
  virtual void set_data(int) = 0;
  virtual ~BaseType() = default;
};

struct DerivedType : BaseType
{
  int data_ = 0;

  DerivedType()
  {
    ++object_count;
  }

  DerivedType(const DerivedType& d)
  {
    data_ = d.data_;
    ++object_count;
  }

  DerivedType(int v) : data_(v)
  {
    ++object_count;
  }

  ~DerivedType()
  {
    --object_count;
  }

  int data() override { return data_; }

  void set_data(int i) override { data_ = i; }

  static size_t object_count;
};

size_t DerivedType::object_count = 0;

TEST_CASE("Default constructed object is empty","[poly.constructors]")
{
  GIVEN("A default constructed poly<BaseType>")
  {
    poly<BaseType> p;

    THEN("poly is empty")
    {
      REQUIRE(p.empty());
    }
    THEN("poly is false")
    {
      REQUIRE(!(bool)p);
    }
  }
}

TEST_CASE("Pointer constructed object","[poly.constructors]")
{
  GIVEN("A pointer-constructed poly")
  {
    int v = 7;
    poly<BaseType> p(new DerivedType(v));
    
    THEN("poly is non-empty")
    {
      REQUIRE(!p.empty());
    }
    THEN("poly is true")
    {
      REQUIRE((bool)p);
    }
    THEN("Operator-> calls the pointee method")
    {
      REQUIRE(p->data() == v);
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

TEST_CASE("Pointer constructor with custom copier avoids slicing",
          "[poly.constructors]") {
  GIVEN("A poly constructed with a custom copier") {
    auto p = std::unique_ptr<BaseCloneSelf>(new DerivedCloneSelf);
    REQUIRE(DerivedCloneSelf::object_count == 1);
    auto c = poly<BaseCloneSelf>(p.release(), invoke_clone_member{});

    WHEN("A copy is made") {
      auto c2 = c;
      THEN("The copied poly manages a distinc resource") {
        CHECK(DerivedCloneSelf::object_count == 2);
        REQUIRE(c2);
        REQUIRE(&c2.value() != &c.value());
      }
    }
    CHECK(DerivedCloneSelf::object_count == 1);
  }
}

TEST_CASE("poly destructor","[poly.destructor]")
{
  GIVEN("No derived objects")
  {
    REQUIRE(DerivedType::object_count == 0);

    THEN("Object count is increased on construction and decreased on destruction")
    {
      // begin and end scope to force destruction
      {
        poly<BaseType> tmp(new DerivedType());
        REQUIRE(DerivedType::object_count == 1);
      }
      REQUIRE(DerivedType::object_count == 0);
    }
  }
}

TEST_CASE("poly copy constructor","[poly.constructors]")
{
  GIVEN("A poly copied from a default-constructed poly")
  {
    poly<BaseType> op;
    poly<BaseType> p(op);

    THEN("poly is empty")
    {
      REQUIRE(p.empty());
    }
  }

  GIVEN("A poly copied from a pointer-constructed poly")
  {
    REQUIRE(DerivedType::object_count == 0);

    int v = 7;
    poly<BaseType> op(new DerivedType(v));
    poly<BaseType> p(op);

    THEN("copy is non-empty")
    {
      REQUIRE(p.empty() == false);
    }
    
    THEN("values are distinct objects")
    {
      REQUIRE(&op.value() != &p.value());
    }

    THEN("managed object has been copied")
    {
      REQUIRE(p->data() == v);
    }

    THEN("object count is two")
    {
      REQUIRE(DerivedType::object_count == 2);
    }
  }
}

TEST_CASE("poly move constructor","[poly.constructors]")
{
  GIVEN("A poly move constructed from a default-constructed poly")
  {
    poly<BaseType> op;
    poly<BaseType> p(std::move(op));

    THEN("move-constructed poly is empty")
    {
      REQUIRE(p.empty());
    }
  }

  GIVEN("A poly move constructed from a pointer-constructed poly")
  {
    REQUIRE(DerivedType::object_count == 0);

    int v = 7;
    poly<BaseType> op(new DerivedType(v));
    BaseType* resource = &op.value();
    poly<BaseType> p(std::move(op));

    THEN("resource ownership is transferred")
    {
      REQUIRE(&p.value() == resource);
      REQUIRE(op.empty());
    }
  }
}
/*
TEST_CASE("poly move constructor","[poly.constructors]")
{
  GIVEN("A poly move-constructed from a default-constructed poly")
  {
    poly<BaseType> original_cptr;
    poly<BaseType> cptr(std::move(original_cptr));

    THEN("The original pointer is null")
    {
      REQUIRE(original_cptr.value()==nullptr);
      REQUIRE(original_cptr.operator->()==nullptr);
      REQUIRE(!original_cptr->empty());
    }

    THEN("The move-constructed pointer is null")
    {
      REQUIRE(cptr.value()==nullptr);
      REQUIRE(cptr.operator->()==nullptr);
      REQUIRE(!cptr->empty());
    }
  }

  GIVEN("A poly move-constructed from a default-constructed poly")
  {
    int v = 7;
    poly<BaseType> original_cptr(new DerivedType(v));
    auto original_pointer = original_cptr.value();
    CHECK(DerivedType::object_count == 1);

    poly<BaseType> cptr(std::move(original_cptr));
    CHECK(DerivedType::object_count == 1);

    THEN("The original pointer is null")
    {
      REQUIRE(original_cptr.value()==nullptr);
      REQUIRE(original_cptr.operator->()==nullptr);
      REQUIRE(!original_cptr->empty());
    }

    THEN("The move-constructed pointer is the original pointer")
    {
      REQUIRE(cptr.value()==original_pointer);
      REQUIRE(cptr.operator->()==original_pointer);
      REQUIRE(cptr->empty());
    }

    THEN("The move-constructed pointer data is the constructed data")
    {
      REQUIRE(cptr->data() == v);
    }
  }
}

TEST_CASE("poly assignment","[poly.assignment]")
{
  GIVEN("A default-constructed poly assigned-to a default-constructed poly")
  {
    poly<BaseType> cptr1;
    const poly<BaseType> cptr2;
    const auto p = cptr2.value();

    REQUIRE(DerivedType::object_count == 0);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 0);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(cptr2.value() == p);
    }

    THEN("The assigned-to object is null")
    {
      REQUIRE(cptr1.empty());
    }
  }

  GIVEN("A default-constructed poly assigned to a pointer-constructed poly")
  {
    int v1 = 7;

    poly<BaseType> cptr1(new DerivedType(v1));
    const poly<BaseType> cptr2;
    const auto p = cptr2.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 0);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(cptr2.value() == p);
    }

    THEN("The assigned-to object is null")
    {
      REQUIRE(cptr1.empty());
    }
  }

  GIVEN("A pointer-constructed poly assigned to a default-constructed poly")
  {
    int v1 = 7;

    poly<BaseType> cptr1;
    const poly<BaseType> cptr2(new DerivedType(v1));
    const auto p = cptr2.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 2);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(cptr2.value() == p);
    }

    THEN("The assigned-to object is non-null")
    {
      REQUIRE(cptr1.value() != nullptr);
    }

    THEN("The assigned-from object 'data' is the assigned-to object data")
    {
      REQUIRE(cptr1->data() == cptr2->data());
    }

    THEN("The assigned-from object pointer and the assigned-to object pointer are distinct")
    {
      REQUIRE(cptr1.value() != cptr2.value());
    }

  }

  GIVEN("A pointer-constructed poly assigned to a pointer-constructed poly")
  {
    int v1 = 7;
    int v2 = 87;

    poly<BaseType> cptr1(new DerivedType(v1));
    const poly<BaseType> cptr2(new DerivedType(v2));
    const auto p = cptr2.value();

    REQUIRE(DerivedType::object_count == 2);

    cptr1 = cptr2;

    REQUIRE(DerivedType::object_count == 2);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(cptr2.value() == p);
    }

    THEN("The assigned-to object is non-null")
    {
      REQUIRE(cptr1.value() != nullptr);
    }

    THEN("The assigned-from object 'data' is the assigned-to object data")
    {
      REQUIRE(cptr1->data() == cptr2->data());
    }

    THEN("The assigned-from object pointer and the assigned-to object pointer are distinct")
    {
      REQUIRE(cptr1.value() != cptr2.value());
    }
  }

  GIVEN("A pointer-constructed poly assigned to itself")
  {
    int v1 = 7;

    poly<BaseType> cptr1(new DerivedType(v1));
    const auto p = cptr1.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = cptr1;

    REQUIRE(DerivedType::object_count == 1);

    THEN("The assigned-from object is unchanged")
    {
      REQUIRE(cptr1.value() == p);
    }
  }
}

TEST_CASE("poly move-assignment","[poly.assignment]")
{
  GIVEN("A default-constructed poly move-assigned-to a default-constructed poly")
  {
    poly<BaseType> cptr1;
    poly<BaseType> cptr2;
    const auto p = cptr2.value();

    REQUIRE(DerivedType::object_count == 0);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 0);

    THEN("The move-assigned-from object is null")
    {
      REQUIRE(cptr1.empty());
    }

    THEN("The move-assigned-to object is null")
    {
      REQUIRE(cptr1.empty());
    }
  }

  GIVEN("A default-constructed poly move-assigned to a pointer-constructed poly")
  {
    int v1 = 7;

    poly<BaseType> cptr1(new DerivedType(v1));
    poly<BaseType> cptr2;
    const auto p = cptr2.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 0);

    THEN("The move-assigned-from object is null")
    {
      REQUIRE(cptr1.empty());
    }

    THEN("The move-assigned-to object is null")
    {
      REQUIRE(cptr1.empty());
    }
  }

  GIVEN("A pointer-constructed poly move-assigned to a default-constructed poly")
  {
    int v1 = 7;

    poly<BaseType> cptr1;
    poly<BaseType> cptr2(new DerivedType(v1));
    const auto p = cptr2.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 1);

    THEN("The move-assigned-from object is null")
    {
      REQUIRE(cptr2.empty());
    }

    THEN("The move-assigned-to object pointer is the move-assigned-from pointer")
    {
      REQUIRE(cptr1.value() == p);
    }
  }

  GIVEN("A pointer-constructed poly move-assigned to a pointer-constructed poly")
  {
    int v1 = 7;
    int v2 = 87;

    poly<BaseType> cptr1(new DerivedType(v1));
    poly<BaseType> cptr2(new DerivedType(v2));
    const auto p = cptr2.value();

    REQUIRE(DerivedType::object_count == 2);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 1);

    THEN("The move-assigned-from object is null")
    {
      REQUIRE(cptr2.empty());
    }

    THEN("The move-assigned-to object pointer is the move-assigned-from pointer")
    {
      REQUIRE(cptr1.value() == p);
    }
  }

  GIVEN("A pointer-constructed poly move-assigned to itself")
  {
    int v = 7;

    poly<BaseType> cptr(new DerivedType(v));
    const auto p = cptr.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr = std::move(cptr);

    THEN("The poly is unaffected")
    {
      REQUIRE(DerivedType::object_count == 1);
      REQUIRE(cptr.value() == p);
    }
  }
}

TEST_CASE("Derived types", "[poly.derived_types]")
{
  GIVEN("A poly<BaseType> constructed from make_poly<DerivedType>")
  {
    int v = 7;
    auto cptr = make_poly<DerivedType>(v);

    WHEN("A poly<BaseType> is copy-constructed")
    {
      poly<BaseType> bptr(cptr);

      THEN("get returns a non-null pointer")
      {
        REQUIRE(bptr.value() != nullptr);
      }

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->data() == v);
      }

      THEN("operator bool returns true")
      {
        REQUIRE(bptr->empty() == true);
      }
    }

    WHEN("A poly<BaseType> is assigned")
    {
      poly<BaseType> bptr;
      bptr = cptr;

      THEN("get returns a non-null pointer")
      {
        REQUIRE(bptr.value() != nullptr);
      }

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->data() == v);
      }

      THEN("operator bool returns true")
      {
        REQUIRE(bptr->empty() == true);
      }
    }

    WHEN("A poly<BaseType> is move-constructed")
    {
      poly<BaseType> bptr(std::move(cptr));

      THEN("get returns a non-null pointer")
      {
        REQUIRE(bptr.value() != nullptr);
      }

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->data() == v);
      }

      THEN("operator bool returns true")
      {
        REQUIRE(bptr->empty() == true);
      }
    }

    WHEN("A poly<BaseType> is move-assigned")
    {
      poly<BaseType> bptr;
      bptr = std::move(cptr);

      THEN("get returns a non-null pointer")
      {
        REQUIRE(bptr.value() != nullptr);
      }

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->data() == v);
      }

      THEN("operator bool returns true")
      {
        REQUIRE(bptr->empty() == true);
      }
    }
  }
}

TEST_CASE("make_poly return type can be converted to base-type", "[poly.make_poly]")
{
  GIVEN("A poly<BaseType> constructed from make_poly<DerivedType>")
  {
    int v = 7;
    poly<BaseType> cptr = make_poly<DerivedType>(v);

    THEN("get returns a non-null pointer")
    {
      REQUIRE(cptr.value() != nullptr);
    }

    THEN("Operator-> calls the pointee method")
    {
      REQUIRE(cptr->data() == v);
    }

    THEN("operator bool returns true")
    {
      REQUIRE(cptr->empty() == true);
    }
  }
}

TEST_CASE("reset","[poly.reset]")
{
  GIVEN("An empty poly")
  {
    poly<DerivedType> cptr;

    WHEN("reset to null")
    {
      cptr.reset();

      THEN("The poly remains empty")
      {
        REQUIRE(!cptr);
        REQUIRE(cptr.value()==nullptr);
      }
    }

    WHEN("reset to non-null")
    {
      int v = 7;
      cptr.reset(new DerivedType(v));

      CHECK(DerivedType::object_count == 1);

      THEN("The poly is non-empty and owns the pointer")
      {
        REQUIRE(cptr);
        REQUIRE(cptr.value()!=nullptr);
        REQUIRE(cptr->data() == v);
      }
    }
  }
  CHECK(DerivedType::object_count == 0);

  GIVEN("A non-empty poly")
  {
    int v1 = 7;
    poly<DerivedType> cptr(new DerivedType(v1));
    CHECK(DerivedType::object_count == 1);

    WHEN("reset to null")
    {
      cptr.reset();
      CHECK(DerivedType::object_count == 0);

      THEN("The poly is empty")
      {
        REQUIRE(!cptr);
        REQUIRE(cptr.value()==nullptr);
      }
    }

    WHEN("reset to non-null")
    {
      int v2 = 7;
      cptr.reset(new DerivedType(v2));
      CHECK(DerivedType::object_count == 1);

      THEN("The poly is non-empty and owns the pointer")
      {
        REQUIRE(cptr);
        REQUIRE(cptr.value()!=nullptr);
        REQUIRE(cptr->data() == v2);
      }
    }
  }
}

struct AlternativeBaseType {
  virtual ~AlternativeBaseType() = default;
  virtual int alternative_data() = 0;
};

class AlternativeDerivedType : public BaseType, public AlternativeBaseType {
  int data_;
public:
  AlternativeDerivedType(int data) : data_(data) {}

  int data() override { return data_; }
  void set_data(int v) override { data_ = v; }
  int alternative_data() override { return data_; }
};

TEST_CASE("cast operations", "[poly.casts]")
{
  GIVEN("A pointer-constructed poly<BaseType>")
  {
    int v = 7;
    poly<BaseType> cptr(new DerivedType(v));
    REQUIRE(DerivedType::object_count == 1);

    WHEN("static_pointer_cast to the derived type is called")
    {
      auto st_cptr = std::static_pointer_cast<DerivedType>(cptr);

      THEN("The static-cast pointer is non-null")
      {
        REQUIRE(st_cptr);
      }
      THEN("The static-cast pointer has the required data")
      {
        REQUIRE(st_cptr->data() == v);
      }
      THEN("The static-cast pointer is distinct from the original pointer")
      {
        REQUIRE(st_cptr.value() != cptr.value());
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
      THEN("The dynamic-cast pointer has the required data")
      {
        REQUIRE(dyn_cptr->data() == v);
      }
      THEN("The dynamic-cast pointer is distinct from the original pointer")
      {
        REQUIRE(dyn_cptr.value() != cptr.value());
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
  GIVEN("A pointer-constructed poly<const DerivedType>")
  {
    int v = 7;
    poly<const DerivedType> ccptr(new DerivedType(v));
    REQUIRE(DerivedType::object_count == 1);

    WHEN("static_pointer_cast to the derived type is called")
    {
      auto cptr = std::const_pointer_cast<DerivedType>(ccptr);

      THEN("The static-cast pointer is non-null")
      {
        REQUIRE(cptr);
      }
      THEN("The static-cast pointer has the required data")
      {
        REQUIRE(cptr->data() == v);
      }
      THEN("The static-cast pointer is distinct from the original pointer")
      {
        REQUIRE(cptr.value() != ccptr.value());
      }
      THEN("Object count is increased")
      {
        REQUIRE(DerivedType::object_count == 2);
      }
    }
  }
  GIVEN("An AlternativeDerivedType-pointer-constructed poly<BaseType>")
  {
    int v = 7;
    poly<BaseType> cptr(new AlternativeDerivedType(v));

    WHEN("dynamic_pointer_cast to AlternativeBaseType is called")
    {
      auto dyn_cptr = std::dynamic_pointer_cast<AlternativeBaseType>(cptr);

      THEN("The dynamic-cast pointer is non-null")
      {
        REQUIRE(dyn_cptr);
      }
      THEN("The dynamic-cast pointer has the required data")
      {
        REQUIRE(dyn_cptr->alternative_data() == v);
      }
      THEN("The dynamic-cast pointer is distinct from the original pointer")
      {
        REQUIRE(dyn_cptr.value() != dynamic_cast<AlternativeBaseType*>(cptr.value()));
      }
    }
  }
}

struct Base { int v_ = 42; virtual ~Base() = default; };
struct IntermediateBaseA : virtual Base { int a_ = 3; };
struct IntermediateBaseB : virtual Base { int b_ = 101; };
struct MultiplyDerived : IntermediateBaseA, IntermediateBaseB { int data_ = 0; MultiplyDerived(int data) : data_(data) {}; };

TEST_CASE("Gustafsson's dilemma: multiple (virtual) base classes", "[poly.constructors]")
{
  GIVEN("A data-constructed multiply-derived-class poly")
  {
    int v = 7;
    poly<MultiplyDerived> cptr(new MultiplyDerived(v));

    THEN("When copied to a poly to an intermediate base type, data is accessible as expected")
    {
      poly<IntermediateBaseA> cptr_IA = cptr;
      REQUIRE(cptr_IA->a_ == 3);
      REQUIRE(cptr_IA->v_ == 42);
    }

    THEN("When copied to a poly to an intermediate base type, data is accessible as expected")
    {
      poly<IntermediateBaseB> cptr_IB = cptr;
      REQUIRE(cptr_IB->b_ == 101);
      REQUIRE(cptr_IB->v_ == 42);
    }
  }
}
*/
