# indirect : a free-store allocated value-type for C++

The class template `indirect` is proposed for addition to the C++ Standard Library.

<http://open-std.org/JTC1/SC22/WG21/docs/papers/2016/p0201r1.pdf>

The class template, `indirect`, confers value-like semantics on a free-store
allocated object.  An `indirect<T>` may hold a an object of a class publicly
derived from T, and copying the indirect<T> will copy the object of the derived
type.

Using `indirect` a copyable composite object with polymorphic components can be
written as:

~~~ {.cpp}
    // Copyable composite with mutable polymorphic components
    class CompositeObject {
      std::indirect<IComponent1> c1_;
      std::indirect<IComponent2> c2_;

     public:
      CompositeObject(std::indirect<IComponent1> c1,
                        std::indirect<IComponent2> c2) :
                        c1_(std::move(c1)), c2_(std::move(c2)) {}

      // `indirect` propagates constness so const methods call 
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
**Build status (on Travis-CI):** [![Build Status](https://travis-ci.org/jbcoe/inline_visitor.svg?branch=master)](https://travis-ci.org/jbcoe/indirect)

