#define CATCH_CONFIG_MAIN

#include "indirect.h"
#include <catch.hpp>

struct BaseType
{
  virtual int data() const = 0;
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

  int data() const override
  {
    return data_;
  }

  void set_data(int i) override
  {
    data_ = i;
  }

  static size_t object_count;
};

struct MoveableDerivedType : BaseType
{
  int data_ = 0;

  enum
  {
    moved_from = -1
  };

  MoveableDerivedType()
  {
    ++object_count;
  }

  MoveableDerivedType(const MoveableDerivedType& d)
  {
    data_ = d.data_;
    ++object_count;
  }
  
  MoveableDerivedType(MoveableDerivedType&& d)
  {
    data_ = d.data_;
    d.data_ = MoveableDerivedType::moved_from;
    ++object_count;
  }

  MoveableDerivedType(int v) : data_(v)
  {
    ++object_count;
  }

  ~MoveableDerivedType()
  {
    --object_count;
  }

  int data() const override
  {
    return data_;
  }

  void set_data(int i) override
  {
    data_ = i;
  }

  static size_t object_count;
};

size_t DerivedType::object_count = 0;

TEST_CASE("Pointer constructed object", "[indirect.constructors]")
{
  GIVEN("A pointer-constructed indirect")
  {
    int v = 7;
    indirect<BaseType> p(new DerivedType(v));

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
  BaseCloneSelf(const BaseCloneSelf&) = delete;
  virtual std::unique_ptr<BaseCloneSelf> clone() const = 0;
};

struct DerivedCloneSelf : BaseCloneSelf
{
  static size_t object_count;
  std::unique_ptr<BaseCloneSelf> clone() const
  {
    return std::make_unique<DerivedCloneSelf>();
  }
  DerivedCloneSelf()
  {
    ++object_count;
  }
  ~DerivedCloneSelf()
  {
    --object_count;
  }
};

size_t DerivedCloneSelf::object_count = 0;

TEST_CASE("indirect destructor", "[indirect.destructor]")
{
  GIVEN("No derived objects")
  {
    REQUIRE(DerivedType::object_count == 0);

    THEN("Object count is increased on construction and decreased on "
         "destruction")
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

TEST_CASE("indirect copy constructor", "[indirect.constructors]")
{
  GIVEN("A indirect copied from a pointer-constructed indirect")
  {
    REQUIRE(DerivedType::object_count == 0);

    int v = 7;
    indirect<BaseType> op(new DerivedType(v));
    indirect<BaseType> p(op);

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

/*
TEST_CASE("indirect move constructor", "[indirect.constructors]")
{
  GIVEN("A indirect move constructed from a pointer-constructed indirect")
  {
    REQUIRE(DerivedType::object_count == 0);

    int v = 7;
    indirect<BaseType> op(new DerivedType(v));
    BaseType* resource = &op.value();
    indirect<BaseType> p(std::move(op));

    THEN("resource ownership is transferred")
    {
      REQUIRE(&p.value() == resource);
    }
  }
}

TEST_CASE("indirect move constructor","[indirect.constructors]")
{
  GIVEN("A indirect move-constructed from a default-constructed indirect")
  {
    indirect<BaseType> original_cptr;
    indirect<BaseType> cptr(std::move(original_cptr));

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

  GIVEN("A indirect move-constructed from a default-constructed indirect")
  {
    int v = 7;
    indirect<BaseType> original_cptr(new DerivedType(v));
    auto original_pointer = original_cptr.value();
    CHECK(DerivedType::object_count == 1);

    indirect<BaseType> cptr(std::move(original_cptr));
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

TEST_CASE("indirect assignment","[indirect.assignment]")
{
  GIVEN("A default-constructed indirect assigned-to a default-constructed indirect")
  {
    indirect<BaseType> cptr1;
    const indirect<BaseType> cptr2;
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

  GIVEN("A default-constructed indirect assigned to a pointer-constructed indirect")
  {
    int v1 = 7;

    indirect<BaseType> cptr1(new DerivedType(v1));
    const indirect<BaseType> cptr2;
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

  GIVEN("A pointer-constructed indirect assigned to a default-constructed indirect")
  {
    int v1 = 7;

    indirect<BaseType> cptr1;
    const indirect<BaseType> cptr2(new DerivedType(v1));
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

    THEN("The assigned-from object pointer and the assigned-to object pointer
are distinct")
    {
      REQUIRE(cptr1.value() != cptr2.value());
    }

  }

  GIVEN("A pointer-constructed indirect assigned to a pointer-constructed indirect")
  {
    int v1 = 7;
    int v2 = 87;

    indirect<BaseType> cptr1(new DerivedType(v1));
    const indirect<BaseType> cptr2(new DerivedType(v2));
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

    THEN("The assigned-from object pointer and the assigned-to object pointer
are distinct")
    {
      REQUIRE(cptr1.value() != cptr2.value());
    }
  }

  GIVEN("A pointer-constructed indirect assigned to itself")
  {
    int v1 = 7;

    indirect<BaseType> cptr1(new DerivedType(v1));
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

TEST_CASE("indirect move-assignment","[indirect.assignment]")
{
  GIVEN("A default-constructed indirect move-assigned-to a default-constructed
indirect")
  {
    indirect<BaseType> cptr1;
    indirect<BaseType> cptr2;
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

  GIVEN("A default-constructed indirect move-assigned to a pointer-constructed
indirect")
  {
    int v1 = 7;

    indirect<BaseType> cptr1(new DerivedType(v1));
    indirect<BaseType> cptr2;
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

  GIVEN("A pointer-constructed indirect move-assigned to a default-constructed
indirect")
  {
    int v1 = 7;

    indirect<BaseType> cptr1;
    indirect<BaseType> cptr2(new DerivedType(v1));
    const auto p = cptr2.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 1);

    THEN("The move-assigned-from object is null")
    {
      REQUIRE(cptr2.empty());
    }

    THEN("The move-assigned-to object pointer is the move-assigned-from
pointer")
    {
      REQUIRE(cptr1.value() == p);
    }
  }

  GIVEN("A pointer-constructed indirect move-assigned to a pointer-constructed
indirect")
  {
    int v1 = 7;
    int v2 = 87;

    indirect<BaseType> cptr1(new DerivedType(v1));
    indirect<BaseType> cptr2(new DerivedType(v2));
    const auto p = cptr2.value();

    REQUIRE(DerivedType::object_count == 2);

    cptr1 = std::move(cptr2);

    REQUIRE(DerivedType::object_count == 1);

    THEN("The move-assigned-from object is null")
    {
      REQUIRE(cptr2.empty());
    }

    THEN("The move-assigned-to object pointer is the move-assigned-from
pointer")
    {
      REQUIRE(cptr1.value() == p);
    }
  }

  GIVEN("A pointer-constructed indirect move-assigned to itself")
  {
    int v = 7;

    indirect<BaseType> cptr(new DerivedType(v));
    const auto p = cptr.value();

    REQUIRE(DerivedType::object_count == 1);

    cptr = std::move(cptr);

    THEN("The indirect is unaffected")
    {
      REQUIRE(DerivedType::object_count == 1);
      REQUIRE(cptr.value() == p);
    }
  }
}
*/

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
        REQUIRE(bptr->data() == v);
      }
    }

    WHEN("A indirect<BaseType> is assigned")
    {
      indirect<BaseType> bptr(new DerivedType(0));
      bptr = cptr;

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->data() == v);
      }
    }

    WHEN("A indirect<BaseType> is move-constructed")
    {
      indirect<BaseType> bptr(std::move(cptr));

      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->data() == v);
      }
    }

    WHEN("A indirect<BaseType> is move-assigned")
    {
      indirect<BaseType> bptr(new DerivedType(0));
      bptr = std::move(cptr);
      
      THEN("Operator-> calls the pointee method")
      {
        REQUIRE(bptr->data() == v);
      }
    }
  }
}

TEST_CASE("make_indirect return type can be converted to base-type",
"[indirect.make_indirect]")
{
  GIVEN("A indirect<BaseType> constructed from make_indirect<DerivedType>")
  {
    int v = 7;
    indirect<BaseType> cptr = make_indirect<DerivedType>(v);

    THEN("Operator-> calls the pointee method")
    {
      REQUIRE(cptr->data() == v);
    }
  }
}

struct Base { int v_ = 42; virtual ~Base() = default; };
struct IntermediateBaseA : virtual Base { int a_ = 3; };
struct IntermediateBaseB : virtual Base { int b_ = 101; };
struct MultiplyDerived : IntermediateBaseA, IntermediateBaseB { int data_ = 0;
  MultiplyDerived(int data) : data_(data){};
};

TEST_CASE("Gustafsson's dilemma: multiple (virtual) base classes",
          "[indirect.constructors]")
{
  GIVEN("A data-constructed multiply-derived-class indirect")
  {
    int v = 7;
    indirect<MultiplyDerived> cptr(new MultiplyDerived(v));

    WHEN("When copied to a indirect to an intermediate base type, data is accessible as expected")
    {
      indirect<IntermediateBaseA> cptr_IA = cptr;
      REQUIRE(cptr_IA->a_ == 3);
      REQUIRE(cptr_IA->v_ == 42);
    }

    WHEN("When copied to a indirect to an intermediate base type, data is accessible as expected")
    {
      indirect<IntermediateBaseB> cptr_IB = cptr;
      REQUIRE(cptr_IB->b_ == 101);
      REQUIRE(cptr_IB->v_ == 42);
    }
  }
}
