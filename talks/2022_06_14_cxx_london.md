---
marp: true
theme: default
paginate: true
footer: 'Vocabulary types for composite class design: https://github.com/jbcoe/polymorphic_value/pull/105'
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

An objects member data becomes const-qualified when the object is accessed through a const-access-path:

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

TODO

---

# class-instance members

TODO

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

TODO

---

# Polymorphism

TODO

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

TODO

---

# Generated special-member functions for pointers

TODO

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

# `std::experimental::propagate_const<T>`

TODO

---

# `polymorphic_value<T>` members

TODO

---

# `indirect_value<T>` members

TODO

---

# Implementing `polymorphic_value<T>`

TODO

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