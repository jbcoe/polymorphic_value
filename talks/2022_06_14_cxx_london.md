---
marp: true
theme: default
paginate: true
#footer: 'Vocabulary types for composite class design: https://github.com/jbcoe/polymorphic_value/pull/105'
---

# Vocabulary types for composite class design

Jonathan B. Coe & Antony Peacock

<https://github.com/jbcoe/indirect_value>

<https://github.com/jbcoe/polymorphic_value>

---

* We recommend the use of two new class templates, `indirect_value<T>` and `polymorphic_value<T>` to make the design of composite classes simple and correct.

* We'll look into some of the challenges of composite class design and see which problems are unsolved by current vocabulary types.

* We'll run through our reference implementations of the two proposed new types.

---

# Encapsulation with classes (and structs)

Classes (and structs) let us group together logically associated data and functions that operate on that data:

```~cpp
class Circle {
    std::pair<double, double> position_;
    double radius_;
    std::string colour_;

public:
    std::string_view colour() const; 
    double area() const;
};
```

---

# Encapsulation with classes (and structs) II

User-defined types can be used for member data:

```~cpp
struct Point {
    double x, y;
};

class Circle {
    Point position_;
    double radius_;
    std::string colour_;
};
```

---

# Special member functions

We can define special member functions to create, copy, move or destroy instances of our class:

```~cpp
class Circle {
    Circle(std::string_view colour, double radius, Point position);
    
    Circle(const Circle&);
    Circle& operator=(const Circle&);
    
    Circle(Circle&&);
    Circle& operator=(Circle&&);

    ~Circle();
};
```

---

# Compiler generated functions

* Under certain conditions, the compiler can generate special member functions for us.

* Members of the class will be copied/moved/deleted in turn by compiler-generated functions.

* We can use the amazing Compiler Explorer to see an example.

---

# Compiler generated functions II

```~cpp
class A {
  public:
    A();  
    A(const A&);
    ~A();
};

class B {
    A a_;
  public:
    int foo();
};

int do_something(B b) {
    B b2(b);
    return b2.foo();
}
```

---

# Compiler generated functions III

X86 assembly generated from https://godbolt.org/

```~assembly
do_something(B):
        push    rbx
        mov     rsi, rdi
        sub     rsp, 16
        lea     rdi, [rsp+15]
        call    A::A(A const&) [complete object constructor]
        lea     rdi, [rsp+15]
        call    B::foo()
        lea     rdi, [rsp+15]
        mov     ebx, eax
        call    A::~A() [complete object destructor]
        add     rsp, 16
        mov     eax, ebx
        pop     rbx
        ret
```

---

# Compiler generated functions IV

* We can specify special member functions for classes we define.

* The compiler will (sometimes) generate special member functions for us.

* Generated special member functions are member-by-member calls to the appropriate 
  special member function of each member object. 

---

# Compiler generated functions with pointer members

When a class contains pointer members, the compiler generated special
member functions will copy/move/delete the pointer but not the pointee:

```~cpp
struct A {
    A(...);
    B* b;
};

A a(...);
A a2(a);
assert(a.b == a2.b);

```

This might require us to write our own versions of the special member functions, something we'd sorely like to avoid having to do.

---

# `const` in C++

Member functions in C++ can be const-qualified:

```~cpp
struct A {
    void foo() const;
    void foo(); 
    void bar();
};
```
 When an object is accessed through a const-access-path then only const-qualified member functions can be called:

```
const A a;
a.bar();
```
```~shell
error: passing 'const A' as 'this' argument discards qualifiers
```

---

# `const` in C++ II

We get a const-access-path by directly accessing a const-qualified object or accessing an object through a reference or pointer to a const-qualified object.

Pointers can be const-qualifed and can point to const-qualified objects

```~cpp
const A*;        // pointer to a const-qualified A.
const A* const;  // const pointer to a const-qualified A.
const A* const;  // const pointer to a non-const-qualified A.
A*;              // pointer to a non-const-qualified A.
```

Note that references are always `const` - they cannot be made to refer to a different object although they can refer to a const-qualified or non-const-qualified object.


---

# `const` propagation

An object's member data becomes const-qualified when the object is accessed through a const-access-path:

```~cpp
class A {
  public:
    void foo(); // non-const
};

class B {
    A a_;
  public:
    void bar() const { a_.foo(); }
};
```

```~shell
error: passing 'const A' as 'this' argument discards qualifiers
```

---

# `const` propagation and reference types

Pointer or reference member data becomes const-qualified when accessed through a const-access-path, but the const-ness does not propagate through to the pointee.

Since references cannot be modified, this const-qualification makes no difference to references. 

Pointers can't be made to point at different objects when accessed through a const-access-path but the object they point to can be accessed in a non-const manner.

const-propagation must be borne in mind when designing composite classes for const-correctness.

---

# class-instance members

Class-instance members are often a good option for member data.

```~cpp
class A {
  B b_;
  C c_;
};
```

Class-instance members ensure const-correctness.

Compiler generated special member functions will be correct.

---

# Repeated member data

We might have cause to store a variable number of objects as part of our class

```~cpp
class Audience {
    Person speaker_;
    std::vector<Person> audience_;
}
```

The standard library has us covered. Container-like class-templates like `vector`, `map`, `unordered_map`, `set`, `unordered_set` support a variable number
of objects and have well-defined special member functions that will copy/move/
delete the contents of the container.

---

# Incomplete types

If the definition of a member is not available when a class is defined then we'll need to store the member as an incomplete type.

This can come about it node-like structures:

```~cpp
class Node {
    int data_;
    Node next_; // won't compile as `Node` is not yet defined.
}
```
---

# The Pointer To Implementation Pattern

The PIMPL pattern can be used to reduce compile times and keep ABI stable.
We store an incomplete type which defines the implementation detail of our class.

This can come about it node-like structures:

```~cpp
class A {
  public:
    int foo();
    double bar();
  private:
    Impl implementation_; // won't compile as `Impl` is not yet defined.
}
```

Where `A` is defined in a header file we want to define `Impl` in the associated `cc` source file.

---

# Polymorphism

We might require a member of our composite class to be one of a number of types.

* A Zoo could contain a list of Animals of different types.

* A code_checker could contain different checking_tools.

* A Game could contain different GameEntities.

* A Portfolio could contain different kinds of FinancialInstrument.

Our class will need to reserve storage for our polymorphic data member.

--- 

# Closed-set polymorphism

Closed-set polymorphism gives users of a class a choices for member data from a known set of types. We can use sum-types like `optional` and `variant` to
represent this.

```~cpp
class Taco {
    std::optional<Avocado> avocado_;
    std::variant<Chicken, Pork, Beef> meat_;
    std::variant<Chipotle, GhostPepper, Hot> sauce_;
};
```

Storing a closed-set polymorphic member directly is possible as `variant` and `optional` reserve enough memory for the largest possible type.

--- 

# Open-set polymorphism

Open-set polymorphism allows users of a class represent member data with types
that were not known when the class was defined.

```
struct SimulationObject {
    virtual ~SimulationObject();
    virtual void update() = 0;
};

class Simulation {
    ???? simulation_objects_;
};
```

Storing open-set polymorphic objects is challenging. We've no idea how much memory
any of the objects might take so direct storage of the data is not possible.

---

# pointer (and reference) members

We can support polymorphism and incomplete types by storing a pointer as a member.

The pointer can be an incomplete type:

```~cpp
class A {
    class B* b_;
    class A* next_;
}
```

or the base type in a class heirarchy:

```
struct Shape { 
    virtual void Draw() const = 0;
};

class Picture {
    Shape* shape_;
}
```

---

We can store multiple pointers to objects in our class in standard library collections:

```~cpp
struct Animal {
    const char* MakeNoise() const = 0;
};

class Zoo {
    std::vector<Animal*> animals_;
};

class SafeZoo {
    std::map<std::string, std::vector<Animal*>> animals_;
};
```

---

# Issues with pointer members

* Compiler generated special members functions handle only the pointer, not the pointee.

* `const` will not propagate to pointees

* If we want to model ownership in our composite then we'll have to do work:

    * Manually maintain special member functions.
    * Check const-qualified member functions for const correctness.

---

# Improving on pointer members

C++'s handling of pointers is not wrong, but in the examples above, we've failed to communicate what we mean to the compiler.

There are instances where pointer members perfectly model what we want to express but such instances are not composites:

```~cpp
class Worker {
    std::string name_;
    Manager* manager_;
};
```

Let's see if we can do better.

---

# `std::unique_ptr<T>` members

TODO

---

# `std::shared_ptr<T>` members

TODO

---

# `std::shared_ptr<const T>` members

TODO

---

# `polymorphic_value<T>`

TODO

---

# `indirect_value<T>`

TODO

---

# `polymorphic_value<T>` members

TODO

---

# `indirect_value<T>` members

TODO

---

# Implementing `polymorphic_value<T>`

```cpp
template <class T> class polymorphic_value {
  std::unique_ptr<control_block<T>> cb_;
  T* ptr_ = nullptr;

 public:
  polymorphic_value() = default;

  polymorphic_value(const polymorphic_value& p) : cb_(p.cb_->clone()) {
    ptr_ = cb_->ptr();
  }

  T* operator->() { return ptr_; }
  const T* operator->() const { return ptr_; }

  T& operator*() { return *ptr_; }
  const T& operator*() const { return *ptr_; }
};
```

All of the real work is done by the control block.

---

## Implementing the control block

Control blocks will inherit from a base-class:

```cpp
template <class T>
struct control_block
{
  virtual ~control_block() = default;
  virtual T* ptr() = 0;
  virtual std::unique_ptr<control_block> clone() const = 0;
};
```

---


## Construction from a value
We can support constructors of the form:

```cpp
template<class U> // restrictions apply
polymorphic_value(const U& u);
```

---

with a suitable control block:

```
template <class T, class U>
class direct_control_block : public control_block<T> {
  U u_;
 public:
  direct_control_block(const U& u) : u_(u) {}

  T* ptr() override { return &u_; }

  std::unique_ptr<control_block<T>> clone() const override {
    return std::make_unique<direct_control_block>(*this);
  }
};
```

The control block knows how to copy and delete the object it owns.

---

```cpp
template<class U,
         class = std::enable_if_t<
           std::is_convertible<U*, T*>::value>>
polymorphic_value(const U& u) :
  cb_(std::make_unique<direct_control_block<T,U>>(u))
{
  ptr_ = cb_->ptr();
}
```

---

## Copying from another polymorphic value

We can support constructors of the form:

```cpp
template <class U> // restrictions apply
polymorphic_value(const polymorphic_value<U>& p);
```

---

with a suitable control block:

```cpp
template <class T, class U>
class delegating_control_block : public control_block<T> {
  std::unique_ptr<control_block<U>> delegate_;

public:
  explicit delegating_control_block(
    std::unique_ptr<control_block<U>> b) :
      delegate_(std::move(b)) {}

  std::unique_ptr<control_block<T>> clone() const override {
    return std::make_unique<delegating_control_block>(
      delegate_->clone());
  }

  T* ptr() override {
    return delegate_->ptr();
  }
};
```

---

```cpp
template <class U,
          class = std::enable_if_t<
            std::is_convertible<U*, T*>::value>>
polymorphic_value(const polymorphic_value<U>& p)
{
  polymorphic_value<U> tmp(p);

  ptr_ = tmp.ptr_;
  cb_ = std::make_unique<delegating_control_block<T, U>>(
    std::move(tmp.cb_));
}
```

---

## Construction from a pointer

We can support constructors of the form:

```cpp
template <class U,
          class C = default_copy<U>,
          class D = default_delete<U>> // restrictions apply
explicit polymorphic_value(U* u,
                           C copier = C{},
                           D deleter = D{});
```

---

with a suitable control block:

```cpp
template <class U,
          class C = default_copy<U>,
          class D = default_delete<U>>
class pointer_control_block : public control_block<T> {
  std::unique_ptr<U, D> p_;
  C c_;

public:
  explicit pointer_control_block(U* u, C c = C{}, D d = D{})
      : c_(std::move(c)), p_(u, std::move(d)) {}

  std::unique_ptr<control_block<T>> clone() const override {
    assert(p_);
    return std::make_unique<pointer_control_block>(
      c_(*p_), c_, p_.get_deleter());
  }

  T* ptr() override {
    return p_.get();
  }
};
```

---

```cpp
template <class U,
          class C = default_copy<U>,
          class D = default_delete<U>,
          class = std::enable_if_t<
            std::is_convertible<U*, T*>::value>>
explicit polymorphic_value(U* u,
                           C copier = C{},
                           D deleter = D{})
{
  if (!u) return;

  assert(typeid(*u) == typeid(U)); // Here be dragons!

  cb_ = std::make_unique<pointer_control_block<T, U, C, D>>(
      u, std::move(copier), std::move(deleter));
  ptr_ = u;
}
```

---

# Implementing `indirect_value<T>`

TODO

---

# Using `polymorphic_value<T>` in your code

`polymorphic_value` is a single-file-, header-only-library and can be included in your C++ project by using the header file from our reference implementation

https://github.com/jbcoe/polymorphic_value

```
jbcoe/polymorphic_value is licensed under the

MIT License

A short and simple permissive license with conditions only 
requiring preservation of copyright and license notices.
Licensed works, modifications, and larger works may be 
distributed under different terms and without source code.
```
---

# Using `indirect_value<T>` in your code

`indirect_value` is a single-file-, header-only-library and can be included in your C++ project by using the header file from our reference implementation

https://github.com/jbcoe/indirect_value

```
jbcoe/indirect_value is licensed under the

MIT License

A short and simple permissive license with conditions only 
requiring preservation of copyright and license notices.
Licensed works, modifications, and larger works may be 
distributed under different terms and without source code.
```
---

# Standardisation efforts

There are two papers in flight to add `polymorphic_value` and `indirect_value` 
to a future version of the C++ standard.

* Polymorphic Value https://wg21.link/p0201r5

* Indirect Value https://wg21.link/p1950r0

Now that travel restrictions are lifted, we hope to resume our work on standardisation.

---

# Acknowledgements

TODO