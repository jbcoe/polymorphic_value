# A polymorphic value-type for C++

The class template `polymorphic_value` is proposed for addition to the C++ Standard Library.

The class template, `polymorphic_value`, confers value-like semantics on a free-store
allocated object.  A `polymorphic_value<T>` may hold a an object of a class publicly
derived from T, and copying the polymorphic_value<T> will copy the object of the derived
type.

Using `polymorphic_value` a copyable composite object with polymorphic components can be
written as:

~~~ {.cpp}
// Copyable composite with mutable polymorphic components
class CompositeObject {
  std::polymorphic_value<IComponent1> c1_;
  std::polymorphic_value<IComponent2> c2_;

 public:
  CompositeObject(std::polymorphic_value<IComponent1> c1,
                  std::polymorphic_value<IComponent2> c2) :
                    c1_(std::move(c1)), c2_(std::move(c2)) {}

  // `polymorphic_value` propagates constness so const methods call 
  // corresponding const methods of components
  void foo() const { c1_->foo(); }
  void bar() const { c2_->bar(); }

  void foo() { c1_->foo(); }
  void bar() { c2_->bar(); }
};
~~~

## Submodules
Tests use the 'catch' test framework: <https://github.com/philsquared/Catch.git>

To get the submodule run:

```
git submodule update --init
```

## Building
The build uses cmake driven by a simple Python script. To build and run tests, run the following from the console:

```
./scripts/build.py --tests
```

## Continuous integration
**Build status (on Travis-CI):** [![Build Status](https://travis-ci.org/jbcoe/indirect.svg?branch=master)](https://travis-ci.org/jbcoe/indirect)

## ISO Standardisation
`polymorphic_value` has been proposed for standardisation for C++20 in P0201: <http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0201r1.pdf>.
The draft in this repository is more up to date than the paper linked above, in particular the class has been renamed from `indirect` to `polymorphic_value`.
