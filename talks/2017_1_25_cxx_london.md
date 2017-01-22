
# A polymorphic value-type for C++

Jonathan B. Coe

<https://github.com/jbcoe/polymorphic_value/blob/master/draft.md>

---

### Overview

The class template `polymorphic_value` is proposed for addition to the C++ Standard Library.

A `polymorphic_value<T>` may hold an object of a class publicly derived from T.

Copying a `polymorphic_value<T>` will copy the object of the derived type.

We will look at motivation for and implementation of `polymorphic_value<T>`.

---

## Component-based object design

Object orientated languages let us design objects in terms of components:

```
class Composite {
  FooComponent f_; BarComponent b_;
  
 public:
  Composite(FooComponent f, BarComponent b) : 
    f_(std::move(f)), b_(std::move(b)) {}
  
  void foo() const { f_.foo(); }
  void foo() { f_.foo(); }
    
  void bar() const { b_.bar(); }
  void bar() { b_.bar(); }
};
```

---

### Compiler-generated functions

A very simple code example gives some illuminating assembler output:

```
struct FooComponent { 
  ~FooComponent(); 
  void foo() const; 
  void foo(); 
};

struct BarComponent { 
  ~BarComponent(); 
  void bar() const; 
  void bar(); 
};

void f() { 
  auto c = Composite(FooComponent(), BarComponent());
  auto cc = c;
  c.foo();
  cc.bar();
}
```

---

### Assembler output

With gcc-7-trunk `-O3 -std=c++1z -fno-inline -fno-exceptions`

```
// A function we never wrote.
Composite::~Composite():
        push    rbx
        mov     rbx, rdi
        lea     rdi, [rdi+1]
        call    BarComponent::~BarComponent()
        mov     rdi, rbx
        pop     rbx
        jmp     FooComponent::~FooComponent()
```

---

### Assembler output (2)

```
// A call to `f` which contains ...
f():
        sub     rsp, 24
        mov     rdi, rsp
        call    FooComponent::~FooComponent()
        mov     rdi, rsp
        call    BarComponent::~BarComponent()
        mov     rdi, rsp
        call    Composite::foo()
        mov     rdi, rsp
        // ... a call to a function we never wrote.
        call    Composite::~Composite() 
        add     rsp, 24
        ret
```

[See Matt Godbolt's awesome compiler explorer - https://godbolt.org.]

---

### Working with the compiler

The compiler has written code for us.

The destructor for `Composite` needs to call the destructors for `BarComponent` and `FooComponent`.

The compiler knows that members need destroying, and has written the code for us.

Code written by me can contain bugs.

Code written by people other than me can sometimes contain bugs.

Code written by the compiler _does_ _the_ _right_ _thing_...

... if the compiler can see what you're trying to do.

---

### Generated functions

The compiler can generate the following special member functions:

* A default constructor.
* A copy constructor.
* A copy assignment operator.
* A move constructor.
* A move assignment operator.
* A destructor.

There are rules about when these might get suppressed (which I won't go into in any depth). 

Prudent design of a class will ensure that the compiler only generates the right special member functions and generates them _correctly_.

---

## Polymorphism

It would be more flexible to build composite objects out of components where each component could be a member of a set of types.

* A `Zoo` could contain a list of `Animals` of different types.

* A `code_checker` could contain different `checking_tool`s.

* A `Game` could contain different `Entities`.

* A `Portfolio` could contain different kind of `FinancialInstrument`.

If a class contains objects of undecided type, it still needs to reserve storage for something.

---

### Hopes for a polymorphic Composite class

We want code like this to work:

```
std::vector<Composite> composites;

FooComponent0 f0;
BarComponent0 b0;
composites.push_back(Composite(f0, b0)); // compile error

FooComponent1 f1;
BarComponent1 b1;
composites.push_back(Composite(f1, b1)); // compile error

for (auto& c : composites) {
  c.foo();
  c.bar();
}
```

Sadly it won't compile. 

`Composite` has members of type `FooComponent` and `BarComponent` and takes objects with those types as constructor arguments.

---

### Compile-time polymorphism

```
template <class FooComponent_t, class BarComponent_t>
class GenericComposite {
  FooComponent f_; BarComponent b_;
  
 public:
  GenericComposite(FooComponent_t f, BarComponent_t b) : 
    f_(std::move(f)), b_(std::move(b)) {}
  
  void foo() const { f_.foo(); }
  void foo() { f_.foo(); }
    
  void bar() const { b_.bar(); }
  void bar() { b_.bar(); }
};
```

The downside here is that the generic nature of this class can leak into other functions/classes. 

If we want to treat all `Composite`s in the same way then `Composite`s with different components need to be the same type.

---

### Closed-set polymorphism

When the number of different types is known at compile time, we can represent polymorphism with a variant:

```
using vehicle = std::variant<plane, train, automobile>;
```

`std::variant` will be part of C++17.

A variant keeps enough storage for its largest type and can use this storage to represent any of the types.

[ It's not identical to `std::variant` but I've found `eggs::variant` intuitive, fast and fit-for-purpose <https://eggs-cpp.github.io/variant/>. ]

---

### Closed-set polymorphism implementation

```
class ClosedSetComposite {
  using FooComponent = variant<Foo1, Foo2>;
  using BarComponent = variant<Bar1, Bar2, Bar3>;
  FooComponent f_; BarComponent b_;
  
 public:
  ClosedSetComposite(FooComponent f, BarComponent b) : 
    f_(std::move(f)), b_(std::move(b)) {}
  
  void foo() const { std::visit([](const auto& x){x.foo();}, f_); }
  void foo() { std::visit([](auto& x){x.foo();}, f_); }
    
  void bar() const { std::visit([](const auto& x){x.bar();}, b_); }
  void bar() { std::visit([](auto& x){x.bar();}, b_); }
};
```

This will work nicely. The compiler generates all 5 special member functions for us.

---

### Open-set polymorphism

When the number of different types is not known at compile time or is intentionally open for extension, we can represent polymorphism with inheritance:

```
struct Animal {
  virtual ~Animal() = default;
  virtual const char* make_noise() const =0;
};

struct Lion : Animal {
  virtual const char* make_noise() const override;
};
```

To store an polymorphic object using inheritance we typically use a pointer to the base type. 

The pointer will give us access to storage which can be allocated later.

Polymorphism through inheritance can localize code changes as type-specific behaviour can be handled by each type.

---

### Open-set polymorphism implementation

```
class OpenSetComposite {
  FooComponent* f_; BarComponent* b_;

 public:
  OpenSetComposite(FooComponent* f, BarComponent* b) : 
    f_(f), b_(b) {}

  void foo() const { f_->foo(); }
  void foo() { f_->foo(); }

  void bar() const { b_->bar(); }
  void bar() { b_->bar(); }
};
```

This has several significant problems.

It compiles, the compiler generates all 5 special member functions, but they won't behave as we want them to.

---

### Problems with our pointer-based open-set polymorphic composite

The compiler understands what pointers do.

* They point to things.
* They don't own things.

This is at odds with what we want.

The compiler will generate:
* A copy constructor.
* A copy assignment operator.
* A move constructor.
* A move assignment operator.
* A destructor.

None of them will do what we want as the compiler does not consider pointers to own resources.

We have to write those functions ourselves, or encourage the compiler to help us.

---

## Smart pointers

Smart pointers have different semantics to raw pointers and can be used to express intent to the compiler.

* Ownership can be transferred.

* Resources can be freed on destruction.

`C++98` had `std::auto_ptr`. With the introduction of move semantics, we have improved smart pointers in C++11.

[C++11 deprecated `std::auto_ptr`. C++17 will remove it.]

---

### `unique_ptr<T>`

```
class OpenSetComposite {
  std::unique_ptr<FooComponent> f_; std::unique_ptr<BarComponent> b_;

 public:
  OpenSetComposite(std::unique_ptr<FooComponent> f, 
                   std::unique_ptr<BarComponent> b) : 
    f_(std::move(f)), b_(std::move(b)) {}

  void foo() const { f_->foo(); }
  void foo() { f_->foo(); }

  void bar() const { b_->bar(); }
  void bar() { b_->bar(); }
};
```

This is an improvement, we now have a correct compiler-generated destructor, move constructor and move assignement operator.

---

### Issues with `unique_ptr<T>`: non-copyable

```
OpenSetComposite c(/* omitted */);
auto cc = c; // won't compile
```

I mentioned earlier that under some conditions the compiler will not generate special member functions.

If a member variable cannot be neither copy-constructed nor assigned then the compiler will not generate a copy constructor or assignement operator.

`std::unique_ptr` does not provide a copy-constructor or assignement operator, this supresses compiler generation of those functions for any class with `std::unique_ptr` members.

---

### Issues with `unique_ptr<T>`: const-propagation

```
struct SimpleFooComponent : FooComponent {
  void foo() const override { std::cout << "SimpleFooComponent::foo() const\n"; }
  void foo() override { std::cout << "SimpleFooComponent::foo()\n"; }
}

OpenSetComposite c(std::make_unique<SimpleFooComponent>(), /* omitted */ );
c.foo(); // prints "SimpleFooComponent::foo()"

const OpenSetComposite& cc = c;
cc.foo(); // prints "SimpleFooComponent::foo()"
```

The `const`-ness of the composite is _not_ propagated to the component. 

This is how pointers, including smart pointers, work.

This _might_ be fine but in my experience it's not future-proof and is a bug-in-waiting.

---

### `propagate_const<T>`

C++ Library Fundamentals Technical Specification 2 offers `propagate_const<T>`.

<http://en.cppreference.com/w/cpp/experimental/propagate_const>


```
template <class T>
using member_ptr = std::experimental::propagate_const<std::unique_ptr<T>>;

class OpenSetComposite {
  member_ptr<FooComponent> f_; 
  member_ptr<BarComponent> b_;

 public:
  OpenSetComposite(std::unique_ptr<FooComponent> f, 
                   std::unique_ptr<BarComponent> b) : 
    f_(std::move(f)), b_(std::move(b)) {}

  void foo() const { f_->foo(); }
  void foo() { f_->foo(); }

  void bar() const { b_->bar(); }
  void bar() { b_->bar(); }
};
```

`const`-ness of the composite object is now propagated to its components.

Our `OpenSetComposite` is still non-copyable though.

---

### `shared_ptr<const T>`

`shared_ptr<T>` as a member variable of a copyable object will introduce shared mutable state. We don't want that.

`shared_ptr<const T>` might work.

```
class OpenSetComposite {
  std::shared_ptr<const FooComponent> f_; 
  std::shared_ptr<const BarComponent> b_;

 public:
  OpenSetComposite(std::shared_ptr<const FooComponent> f, 
                   std::shared_ptr<const BarComponent> b) : 
    f_(std::move(f)), b_(std::move(b)) {}

  void foo() const { f_->foo(); }

  void bar() const { b_->bar(); }
};
```

Compiler-generated destructor, copy-constructor, move-constructor, copy-assignment and move-assignment operators all work as intended.

We've lost mutability.

---

### `polymorphic_value<T>`

```
class OpenSetComposite {
  polymorphic_value<FooComponent> f_; 
  polymorphic_value<BarComponent> b_;

 public:
  OpenSetComposite(polymorphic_value<FooComponent> f, 
                   polymorphic_value<BarComponent> b) : 
    f_(std::move(f)), b_(std::move(b)) {}

  void foo() const { f_->foo(); }
  void foo() { f_->foo(); }

  void bar() const { b_->bar(); }
  void bar() { b_->bar(); }
};
```

The compiler-generated destructor, copy-constructor, move-constructor, copy-assignment and move-assignment operators all work as intended.

We've added polymorphism to our original component-based design.

---

## Design and implementation of `polymorphic_value<T>`

---

### `polymorphic_value`: constructors

```
template <class T> class polymorphic_value { 

  polymorphic_value() noexcept;
  
  template <class U, // restrictions apply
            class C=default_copy<U>, 
            class D=default_delete<U>> 
  explicit polymorphic_value(U* p, C c=C{}, D d=D{});

  polymorphic_value(const polymorphic_value& p);
  
  template <class U> // restrictions apply
  polymorphic_value(const polymorphic_value<U>& p);
  
  template <class U> // restrictions apply
  polymorphic_value(const U& u);

  polymorphic_value(polymorphic_value&& p) noexcept;
  
  template <class U> // restrictions apply
  polymorphic_value(polymorphic_value<U>&& p);
  
  template <class U> // restrictions apply
  polymorphic_value(U&& u);

```

---

### `polymorphic_value`: assignment

```
  polymorphic_value &operator=(const polymorphic_value& p);
  
  template <class U> // restrictions apply
  polymorphic_value& operator=(const polymorphic_value<U>& p);
  
  template <class U> // restrictions apply
  polymorphic_value& operator=(const U& u);

  polymorphic_value &operator=(polymorphic_value &&p) noexcept;
  
  template <class U> // restrictions apply
  polymorphic_value& operator=(polymorphic_value<U>&& p);
  
  template <class U> // restrictions apply
  polymorphic_value& operator=(U&& u);
```

---

### `polymorphic_value`: modifiers and observers
```
  void swap(polymorphic_value<T>& p) noexcept;

  T& operator*();
  T* operator->();
  const T& operator*() const;
  const T* operator->() const;
  explicit operator bool() const noexcept;
};
```

---

### `polymorphic_value`: creation
```
template <class T, class ...Ts> 
polymorphic_value<T> make_polymorphic_value(Ts&& ...ts); 
```

---

## Implementing `polymorphic_value`

```
template <class T> class polymorphic_value {
   std::unique_ptr<control_block<T>> cb_;
   T* ptr_ = nullptr;
   
 public:
   polymorphic_value() = default;
   
   template<class U, 
            class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
   polymorphic_value(const U& u) : 
     cb_(std::make_unique<direct_control_block<T,U>>(u)) 
   {
     ptr_ = cb_->ptr();
   }
  
   polymorphic_value(const polymorphic_value& p) : 
     cb_(p.cb_->clone()) 
   {
     ptr_ = cb_->ptr();
   }
   
   // Some methods omitted for brevity.
  
   T* operator->() { assert(*this); return ptr_; }
   
   const T* operator->() const { assert(*this); return ptr_; }
   
   explicit operator bool() const noexcept { return bool(cb_); }
};
```

The hard-work is done by the control block.

---

### Implementing the control block

```
template <class T>
struct control_block
{
  virtual ~control_block() = default;
  virtual T* ptr() = 0;
  virtual std::unique_ptr<control_block> clone() const = 0;
};

template <class T, class U>
class direct_control_block : public control_block<T>
{
  U u_;
 public:
  direct_control_block(const U& u) : u_(u) {}
  
  virtual T* ptr() override { return &u_; }
  
  virtual std::unique_ptr<control_block<T>> clone() const override {
    return std::make_unique<direct_control_block>(*this);
  }
};
```

The control block instance knows how to copy and delete the object it owns.

---

### Copying from another polymorphic value

We can support constructors of the form:

```
template <class U> // restrictions apply
polymorphic_value(const polymorphic_value<U>& p);
```

with a special control block

```
class delegating_control_block : public control_block<T> {
  std::unique_ptr<control_block<U>> delegate_;

public:
  explicit delegating_control_block(std::unique_ptr<control_block<U>> b) : 
    delegate_(std::move(b)) {
  }

  std::unique_ptr<control_block<T>> clone() const override {
    return std::make_unique<delegating_control_block>(delegate_->clone());
  }

  T* ptr() override {
    return delegate_->ptr();
  }
};
```

---

### Copying from another polymorphic value (2)

```
template <class U, 
          class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
polymorphic_value(const polymorphic_value<U>& p) {
  polymorphic_value<U> tmp(p);
  ptr_ = tmp.ptr_;
  cb_ = std::make_unique<delegating_control_block<T, U>>(std::move(tmp.cb_));
}
```

---

### Construction from a pointer with custom copier and deleter

We can support constructors of the form:

```
  template <class U, 
            class C = default_copy<U>, 
            class D = default_delete<U>> // restrictions apply
  explicit polymorphic_value(U* u, C copier = C{}, D deleter = D{})
```

with a special control block

```
class pointer_control_block : public control_block<T> {
  std::unique_ptr<U, D> p_;
  C c_;

public:
  explicit pointer_control_block(U* u, C c = C{}, D d = D{})
      : c_(std::move(c)), p_(u, std::move(d)) {
  }

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

### Construction from a pointer with custom copier and deleter (2)

```
template <class U, 
          class C = default_copy<U>, 
          class D = default_delete<U>,
          class V = std::enable_if_t<std::is_convertible<U*, T*>::value>>
explicit polymorphic_value(U* u, C copier = C{}, D deleter = D{})
{
  if (!u) {
    return;
  }

  assert(typeid(*u) == typeid(U)); // Here be dragons!

  cb_ = std::make_unique<pointer_control_block<T, U, C, D>>(
      u, std::move(copier), std::move(deleter));
  ptr_ = u;
}
```

---

## Component objects

| Polymorphism | Copyable | Mutable | Recommend |
| :---: | :---: | :---: | :---: |
| none |  |  | `T` |
| compile-time |  |  | `template <class T>` |
| closed-set |  |  | `std::variant<Ts...>` |
| open-set | No | No | `std::unique_ptr<const T>` |
| open-set | No | Yes | `propagate_const<std::unique_ptr<T>>` |
| open-set | Yes | No | `std::shared_ptr<const T>` |
| open-set | Yes | Yes | `polymorphic_value<T>` |

---

## Conclusion

`polymorphic_value` confers value semantics on a (potentially) free-store allocated object. This is what we need for open-set polymorphism.

Implementation of `polymorphic_value` is relatively simple and does not rely on exotic language features or compiler support.

`polymorphic_value` is progressing through the various groups in the C++ standards committee. 

It may be ready in time for C++20.

In the meantime, anyone can still use it.

---

## Feedback

The reference implementation is available on GitHub: <https://github.com/jbcoe/polymorphic_value>

It is MIT licensed with the intention of making it widely useable. 

[Please let me know if an MIT license is problematic for you.]

---

