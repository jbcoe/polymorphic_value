# A polymorphic value-type for C++

ISO/IEC JTC1 SC22 WG21 Programming Language C++

D0201R2

Working Group: Library Evolution

Date: 2016-11-12

_Jonathan Coe \<jbcoe@me.com\>_

_Sean Parent \<sparent@adobe.com\>_

## Change history

Changes in P0201R2

* Change name to `polymorphic_value`.

* Remove `operator <<`.

* Add construction and assignment from values.

* Use `std::default_delete`.

* Rename `std::default_copier` to `std::default_copy`.

* Add notes on empty state and pointer constructor.

Changes in P0201R1

* Change name to `indirect`.

* Remove `static_cast`, `dynamic_cast` and `const_cast` as `polymorphic_value` is
  modelled on a value not a pointer.

* Add `const` accessors which return `const` references/pointers.

* Remove pointer-accessor `get`.

* Remove specialization of `propagate_const`.

* Amended authorship and acknowledgements.

* Added support for custom copiers and custom deleters.

* Removed hash and comparison operators.

## TL;DR

Add a class template, `polymorphic_value<T>`, to the standard library to support
free-store-allocated objects with value-like semantics.

## Introduction

The class template, `polymorphic_value`, confers value-like semantics on a free-store
allocated object.  An `polymorphic_value<T>` may hold a an object of a class publicly
derived from T, and copying the polymorphic_value<T> will copy the object of the derived
type.

## Motivation: Composite objects

Use of components in the design of object-oriented class hierarchies can aid
modular design as components can be potentially re-used as building-blocks for
other composite classes.

We can write a simple composite object formed from two components as follows:

    // Simple composite
    class CompositeObject_1 {
      Component1 c1_;
      Component2 c2_;

     public:
      CompositeObject_1(const Component1& c1,
                        const Component2& c2) :
                        c1_(c1), c2_(c2) {}

      void foo() { c1_.foo(); }
      void bar() { c2_.bar(); }
    };

The composite object can be made more flexible by storing pointers to objects
allowing it to take derived components in its constructor.  (We store pointers
to the components rather than references so that we can take ownership of
them).

    // Non-copyable composite with polymorphic components (BAD)
    class CompositeObject_2 {
      IComponent1* c1_;
      IComponent2* c2_;

     public:
      CompositeObject_2(const IComponent1* c1,
                        const IComponent2* c2) :
                        c1_(c1), c2_(c2) {}

      void foo() { c1_->foo(); }
      void bar() { c2_->bar(); }

      CompositeObject_2(const CompositeObject_2&) = delete;
      CompositeObject_2& operator=(const CompositeObject_2&) = delete;

      CompositeObject_2(CompositeObject_2&& o) : c1_(o.c1_), c2_(o.c2_) {
        o.c1_ = nullptr;
        o.c2_ = nullptr;
      }

      CompositeObject_2& operator=(CompositeObject_2&& o) {
        delete c1_;
        delete c2_;
        c1_ = o.c1_;
        c2_ = o.c2_;
        o.c1_ = nullptr;
        o.c2_ = nullptr;
      }

      ~CompositeObject_2()
      {
        delete c1_;
        delete c2_;
      }
    };

`CompositeObject_2`'s constructor API is unclear without knowing that the class
takes ownership of the objects. We are forced to explicitly suppress the
compiler-generated copy constructor and copy assignment operator to avoid
double-deletion of the components `c1_` and `c2_`. We also need to write a move
constructor and move assignment operator.

Using `unique_ptr` makes ownership clear and saves us writing or deleting
compiler generated methods:

    // Non-copyable composite with polymorphic components
    class CompositeObject_3 {
      std::unique_ptr<IComponent1> c1_;
      std::unique_ptr<IComponent2> c2_;

     public:
      CompositeObject_3(std::unique_ptr<IComponent1> c1,
                        std::unique_ptr<IComponent2> c2) :
                        c1_(std::move(c1)), c2_(std::move(c2)) {}

      void foo() { c1_->foo(); }
      void bar() { c2_->bar(); }
    };

The design of `CompositeObject_3` is good unless we want to copy the object.

We can avoid having to define our own copy constructor by using shared
pointers.  As `shared-ptr`'s copy constructor is shallow, we need to modify the
component pointers to be pointers-to `const` to avoid introducing shared mutable
state [S.Parent].

    // Copyable composite with immutable polymorphic components class
    class CompositeObject_4 {
      std::shared_ptr<const IComponent1> c1_;
      std::shared_ptr<const IComponent2> c2_;

     public:
      CompositeObject_4(std::shared_ptr<const IComponent1> c1,
                        std::shared_ptr<const IComponent2> c2) :
                        c1_(std::move(c1)), c2_(std::move(c2)) {}

      void foo() { c1_->foo(); }
      void bar() { c2_->bar(); }
    };

`CompositeObject_4` has polymorphism and compiler-generated destructor, copy,
move and assignment operators. As long as the components are not mutated, this
design is good. If non-const methods of components are used then this won't
compile.

Using `polymorphic_value` a copyable composite object with polymorphic components can be
written as:

    // Copyable composite with mutable polymorphic components
    class CompositeObject_5 {
      std::polymorphic_value<IComponent1> c1_;
      std::polymorphic_value<IComponent2> c2_;

     public:
      CompositeObject_5(std::polymorphic_value<IComponent1> c1,
                        std::polymorphic_value<IComponent2> c2) :
                        c1_(std::move(c1)), c2_(std::move(c2)) {}

      void foo() { c1_->foo(); }
      void bar() { c2_->bar(); }
    };

`CompositeObject_5` has a (correct) compiler-generated destructor, copy, move,
and assignment operators. In addition to enabling compiler-generation of
functions, `polymorphic_value` performs deep copies of `c1_` and `c2_` without the class
author needing to provide a special 'clone' method.

## Deep copies

To allow correct copying of polymorphic objects, `polymorphic_value` uses the
copy constructor of the derived-type pointee when copying a base type
`polymorphic_value`.  Similarly, to allow correct destruction of polymorphic
component objects, `polymorphic_value` uses the destructor of the derived-type
pointee in the destructor of a base type `polymorphic_value`.

The requirements of deep-copying can be illustrated by some simple test code:
```
// GIVEN base and derived classes.
class Base { virtual void foo() const = 0; };
class Derived : public Base { void foo() const override {} };

// WHEN an polymorphic_value to base is formed from a derived pointer
polymorphic_value<Base> poly(new Derived());
// AND the polymorphic_value to base is copied.
auto poly_copy = poly;

// THEN the copy points to a distinct object
assert(&*poly != &*poly_copy);
// AND the copy points to a derived type.
assert(dynamic_cast<Derived*>(*&poly_copy);
```

Note that while deep-destruction of a derived class object from a base class
pointer can be performed with a virtual destructor, the same is not true for
deep-copying. C++ has no concept of a virtual copy constructor and we are not
proposing its addition.  The class template `shared_ptr` already implements
deep-destruction without needing virtual destructors. deep-destruction and
deep-copying can be implemented using type-erasure [Impl].

## Pointer constructor
`polymorphic_value` can be constructed from a pointer and optionally a copier
and/or deleter. The `polymorphic_value` constructed in this manner takes
ownership of the pointer. This constructor is potentially dangerous as a
mismatch in the dynamic and static type of the pointer will result in
incorrectly synthesized copiers and deleters potentiall resulting in slicing
when copying and incomplete deletion during destruction. 

```
class Base { /* methods and members */ };
class Derived : public Base { /* methods and members */ };

Derived d = new Derived();
Base* p = d; // static type and dynamic type differ
polymorphic_value<Base> poly(p);

// This copy will have been made using Base's copy constructor.
polymorphic_value<Base> poly_copy = poly;

// Destruction of poly and poly_copy uses Base's destructor.
```

While this is potentially error prone, we have elected to trust users with the
tools they are given. `shared_ptr` and `unique_ptr` have similar constructors
and issues.  There are more constructors for `polymorphic_value` of a less
expert-friendly nature that do not present such dangers including a factory
method `make_polymorphic_value`. 

Static analysis tools can be written to find cases where static and dynamic
types for pointers passed in to `polymorphic_value` constructors are not
provably equivalent.

## Empty state
`polymorphic_value` presents an empty state as it is desirable for it to be
cheaply constructed and then later assigned.  In addition, it may not be
possible to construct the `T` of a `polymorphic_value<T>` if it is an abstract
class (a common intended use pattern).  While permitting an empty state will
necessitate occasional checks for `null`, `polymorphic_value` is intended to
replace uses of pointers or smart pointers where such checks are also
necessary. The benefits of default constructability (use in vectors and maps)
outweight the costs of a possible empty state.

## Lack of hashing and comparisons
For a given user-defined type, `T`, there are multiple strategies to make
`polymorphic_value<T>` hashable and comparable.  Without requiring additional named
member functions on the type, `T`, or mandating that `T` has virtual functions
and RTTI, the authors do not see how `polymorphic_value` can generically support hashing
or comparisons. Incurring a cost for functionality that is not required goes
against the 'pay for what you use' philosophy of C++.

For a given user-defined type `T` the user is free to specialize
`std::hash<polymorphic_value<T>>` and implement comparison operators for `polymorphic_value<T>`.

## Custom copiers and deleters
The resource management performed by `polymorphic_value` - copying and destruction of
the managed object - can be customized by supplying a _copier_ and _deleter_.
If no copier or deleter is supplied then a default copier or deleter will be
used. 

The default deleter is already defined by the standard library and used by
`unique_ptr`.

We define the default copier in technical specifications below. 


## Custom allocators
Custom allocators are not explicitly supported by `polymorphic_value`. Since all
memory allocation and deallocation performed by `polymorphic_value` is done by copying
and deletion, any requirement for custom allocators can be handled by suitable
choices for a custom copier and custom deleter.


## Design changes from `cloned_ptr`
The design of `polymorphic_value` is based upon `cloned_ptr` after advice from LEWG. The
authors would like to make LEWG explicitly aware of the cost of these design
changes.

`polymorphic_value<T>` has value-like semantics: copies are deep and `const` is
propagated to the owned object. The first revision of this paper presented
`cloned_ptr<T>` which had mixed pointer/value semantics: copies are deep but
`const` is not propagated to the owned object. `polymorphic_value` can be built from
`cloned_ptr` and `propagate_const` but there is no way to remove `const`
propagation from polymorphic_value.

As `polymorphic_value` is a value, `dynamic_pointer_cast`, `static_pointer_cast` and
`const_pointer_cast` are not provided.  If an `polymorphic_value` is constructed with a
custom copier or deleter, then there is no way for a user to implement the cast
operations provided for `cloned_ptr`.

[Should we be standardizing vocabulary types (`optional`, `variant` and
`polymorphic_value`) or components through which vocabulary types can be trivially
composed (`propagate_const`, `cloned_ptr`)?]


## Impact on the standard
This proposal is a pure library extension. It requires additions to be made to
the standard library header `<memory>`. 


## Technical specifications

## X.X Class template `default_copy` [default.copy]

### X.X.1 Class template `default_copy` general [default.copy.general]
The class template `default_copy` serves as the default copier for the class
template `polymorphic_value`.

The template parameter `T` of `default_copy` may be an incomplete type.

### X.X.2 Class template `default_copy` synopsis [default.copier.synopsis]

```
namespace std {
template <class T> struct default_copy {
  T* operator()(const T& t) const;
};

} // namespace std
```

### X.X.3 Class template `default_copy` [default.copier]

```
T* operator()(const T& t) const;
```

* _Returns:_  `new T(t)`.
          

## X.Y Class template `polymorphic_value` [polymorphic_value]

### X.Y.1 Class template `polymorphic_value` general [polymorphic_value.general]

An _polymorphic_value_ is an object that owns another object and manages that other
object through a pointer.  

When a `polymorphic_value` is constructed from a pointer to an object of type `U`, a
copier `c`, and a deleter `d`, then a move-constructed copy of the copier and
deleter is stored in the `polymorphic_value` along with the pointer `u`.

When copies of a `polymorphic_value` are required due to copy construction or copy
assignment, copies are made using `c(*u)`.

an `polymorphic_value` object is empty if it does not own a pointer.  

The template parameter `T` of `polymorphic_value` may be an incomplete type.


### X.Y.2 Class template `polymorphic_value` synopsis [polymorphic_value.synopsis]

```
namespace std {
template <class T> class polymorphic_value {
 public:
  typedef T element_type;

  // Constructors
  polymorphic_value() noexcept; // see below
  template <class U, class C=default_copy<U>, class D=default_delete<U>> 
    explicit polymorphic_value(U* p, C c=C{}, D d=D{}); // see below
  
  polymorphic_value(const polymorphic_value& p);
  template <class U> polymorphic_value(const polymorphic_value<U>& p); // see below
  template <class U> polymorphic_value(const U& u); // see below
  
  polymorphic_value(polymorphic_value&& p) noexcept;
  template <class U> polymorphic_value(polymorphic_value<U>&& p); // see below
  template <class U> polymorphic_value(U&& u); // see below

  // Destructor
  ~polymorphic_value();

  // Assignment
  polymorphic_value &operator=(const polymorphic_value& p);
  template <class U> polymorphic_value &operator=(const polymorphic_value<U>& p); // see below
  template <class U> polymorphic_value &operator=(const U& u); // see below
  
  polymorphic_value &operator=(polymorphic_value &&p) noexcept;
  template <class U> polymorphic_value &operator=(polymorphic_value<U>&& p); // see below
  template <class U> polymorphic_value &operator=(U&& u); // see below

  // Modifiers
  void swap(polymorphic_value<T>& p) noexcept;

  // Observers
  T& operator*();
  T* operator->();
  const T& operator*() const;
  const T* operator->() const;
  explicit operator bool() const noexcept;
};

// polymorphic_value creation
template <class T, class ...Ts> polymorphic_value<T>
  make_polymorphic_value(Ts&& ...ts); // see below

// polymorphic_value specialized algorithms
template<class T>
  void swap(polymorphic_value<T>& p, polymorphic_value<T>& u) noexcept;

} // end namespace std
```


### X.Y.3 Class template `polymorphic_value` constructors [polymorphic_value.ctor]

```
polymorphic_value() noexcept;
```

* _Effects:_  Constructs an empty `polymorphic_value`.

* _Postconditions:_  `bool(*this) == false`


```
template <class U, class C=default_copy<U>, class D=default_delete<U>> 
  explicit polymorphic_value(U* p, C c=C{}, D d=D{});
```

* _Effects_: Creates a `polymorphic_value` object that _owns_ the pointer `p` and has
  a move-constructed copy of both `c` and `d`.

* _Preconditions:_  `p` is non-null. `c` and `d` are copy constructible. The
  expression `c(*p)` shall return an object of type `U*`. The expression `d(p)`
  shall be well formed, shall have well defined behavior, and shall not throw
  exceptions.  Either `U` and `T` must be the same type, or the dynamic and
  static type of `U` must be the same.

* _Postconditions:_  `operator->() == p`

* _Throws:_ `bad_alloc`, or an implementation-defined exception when a resource
  other than memory could not be obtained.

* _Exception safety:_ If an exception is thrown, `d(p)` is called.

* _Requires:_ `U` is copy-constructible. 

* _Remarks:_ This constructor shall not participate in overload
  resolution unless `U` is derived from `T`.


```
polymorphic_value(const polymorphic_value &p);
template <class U> polymorphic_value(const polymorphic_value<U> &p);
```

* _Remarks:_ The second constructor shall not participate in overload
  resolution unless `U` is derived from `T`.

* _Effects:_ Creates a `polymorphic_value` object that owns a copy of the object
  managed by `p`.

* _Postconditions:_  `bool(*this) == bool(p)`.

```
template <class U> polymorphic_value(const U& u);
```

* _Remarks_: This constructor shall not participate in overload
  resolution unless `U*` is convertible to `T*`.

* _Effects:_ Copy-constructs a `polymorphic_value` instance from `u`.


```
polymorphic_value(polymorphic_value &&p) noexcept;
template <class U> polymorphic_value(polymorphic_value<U> &&p);
```

* _Remarks_: The second constructor shall not participate in overload
  resolution unless `U*` is convertible to `T*`.

* _Effects:_ Move-constructs a `polymorphic_value` instance from `p`.

* _Postconditions:_  `*this` shall contain the old value of `p`. `p` shall be
  empty.

```
template <class U> polymorphic_value(U&& u);
```

* _Remarks_: This constructor shall not participate in overload
  resolution unless `U*` is convertible to `T*`.

* _Effects:_ Move-constructs a `polymorphic_value` instance from `u`.

### X.Y.4 Class template `polymorphic_value` destructor [polymorphic_value.dtor]

```
~polymorphic_value();
```

* _Effects:_ `d(u)` is called.

### X.Y.5 Class template `polymorphic_value` assignment [polymorphic_value.assignment]

```
polymorphic_value &operator=(const polymorphic_value &p);
template <class U> polymorphic_value &operator=(const polymorphic_value<U>& p);
```

* _Remarks_: The second function shall not participate in overload resolution
  unless `U*` is convertible to `T*`.

* _Effects:_ `*this` shall own a copy of the resource managed by `p`.

* _Returns:_ `*this`.

* _Postconditions:_  `bool(*this) == bool(p)`.


```
polymorphic_value &operator=(polymorphic_value&& p) noexcept;
template <class U> polymorphic_value &operator=(polymorphic_value<U> &&p);
```

* _Remarks:_ The second constructor shall not participate in overload
  resolution unless `U` is derived from `T`.

* _Effects_: Ownership of the resource managed by `p` shall be transferred to `this`.

* _Returns:_ `*this`.

* _Postconditions:_  `*this` shall contain the old value of `p`. `p` shall be empty.


### X.Y.6 Class template `polymorphic_value` modifiers [polymorphic_value.modifiers]

```
void swap(polymorphic_value<T>& p) noexcept;
```

* _Effects:_ Exchanges the contents of `p` and `*this`.


### X.Y.7 Class template `polymorphic_value` observers [polymorphic_value.observers]

```const T& operator*() const;```

* _Requires:_ `bool(*this)`.

* _Returns:_ A reference to the owned object.


```const T* operator->() const;```

* _Requires:_ `bool(*this)`.

* _Returns:_ A pointer to the owned object.


```T& operator*();```

* _Requires:_ `bool(*this)`.

* _Returns:_ A reference to the owned object.


```T* operator->();```

* _Requires:_ `bool(*this)`.

* _Returns:_ A pointer to the owned object.


```explicit operator bool() const noexcept;```

* _Returns:_ `false` if the `polymorphic_value` is empty, otherwise `true`.

### X.Y.8 Class template `polymorphic_value` creation [polymorphic_value.creation]

```
template <class T, class ...Ts> polymorphic_value<T>
  make_polymorphic_value(Ts&& ...ts);
```

* _Returns:_ `polymorphic_value<T>(new T(std::forward<Ts>(ts)...)`;


### X.Y.9 Class template `polymorphic_value` specialized algorithms [polymorphic_value.spec]

```
template <typename T>
void swap(polymorphic_value<T>& p, polymorphic_value<T>& u) noexcept;
```

* _Effects:_ Equivalent to `p.swap(u)`.


## Acknowledgements
The authors would like to thank Maciej Bogus, Matthew Calbrese, Germán Diago,
Louis Dionne, Bengt Gustafsson, David Krauss, Nevin Liber, Nathan Meyers, Roger
Orr, Sean Parent, Patrice Roy, Tony van Eerd and Ville Voutilainen for useful
discussion.


## References

[N3339] "A Preliminary Proposal for a Deep-Copying Smart Pointer",  
W.E.Brown, 2012  
```<http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3339.pdf>```

[S.Parent] "C++ Seasoning", Sean Parent, 2013  
```<https://github.com/sean-parent/sean-parent.github.io/wiki/Papers-and-Presentations>```

[Impl] Reference implementation: `polymorphic_value`, J.B.Coe  
```<https://github.com/jbcoe/polymorphic_value>```

