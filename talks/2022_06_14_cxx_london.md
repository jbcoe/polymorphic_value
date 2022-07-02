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

    ...
public:
    std::string_view colour() const; 
    double area() const;
    ...
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
    ...
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
 When an object is accessed through a const-access-path then only `const` qualified member functions can be called:

```
    const A a;
    a.bar();

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

---

# `const` propagation and reference types

---

# Reference and value types

---

# The need for incomplete types

---

# The need for polymorphism

--- 

# Closed and open-set polymorphism

---

# class-instance members

---

# template-type-instance members

---

# `std::variant<Ts...>` members

---

# pointer (and reference) members

---

# `std::unique_ptr<T>` members

---

# `std::shared_ptr<T>` members

---

# `std::experimental::propagate_const<T>`

---

# `indirect_value<T>` members

---

# `polymorphic_value<T>` members

---

# Implementing `indirect_value<T>`

---

# Implementing `polymorphic_value<T>`

---

# Using `indirect_value<T>` in your code

---

# Using `polymorphic_value<T>` in your code

---

# Standardisation efforts

---

# Acknowledgements