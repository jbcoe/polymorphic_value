# A polymorphic value-type for C++

[![ConanCenter Package][badge.conan]][conan]
[![codecov][badge.codecov]][codecov]
[![language][badge.language]][language]
[![license][badge.license]][license]
[![issues][badge.issues]][issues]

[badge.conan]: https://repology.org/badge/version-for-repo/conancenter/polymorphic_value.svg
[badge.language]: https://img.shields.io/badge/language-C%2B%2B14-yellow.svg
[badge.codecov]: https://img.shields.io/codecov/c/github/jbcoe/polymorphic_value/master.svg?logo=codecov
[badge.license]: https://img.shields.io/badge/license-MIT-blue.svg
[badge.issues]: https://img.shields.io/github/issues/jbcoe/polymorphic_value.svg

[conan]: https://conan.io/center/polymorphic_value
[codecov]: https://codecov.io/gh/jbcoe/polymorphic_value
[language]: https://en.wikipedia.org/wiki/C%2B%2B14
[license]: https://en.wikipedia.org/wiki/MIT_License
[issues]: http://github.com/jbcoe/polymorphic_value/issues

The class template `polymorphic_value` is proposed for addition to the C++ Standard Library.

The class template, `polymorphic_value`, confers value-like semantics on a free-store
allocated object.  A `polymorphic_value<T>` may hold an object of a class publicly
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

# ISO Standardisation
`polymorphic_value` has been proposed for standardisation for C++20 in P0201: <http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0201r1.pdf>.
The draft in this repository is more up to date than the paper linked above, in particular the class has been renamed from `indirect` to `polymorphic_value`.

# Contents
- [Integration](#integration)
  - [CMake](#cmake)
    - [External](#external)
- [Building](#building)
  - [Building Manually Via CMake](#building-manually-via-cmake)
  - [Installing Via CMake](#installing-via-cmake)
- [Packaging](#packaging)
  - [Conan](#conan)
- [License](#license)

# Integration
Polymorphic value is shipped as a single header file, [`polymorphic_value.h`](https://github.com/jbcoe/polymorphic_value/blob/master/polymorphic_value.h) that can be directly included in your project or included via an official [release package](https://github.com/jbcoe/polymorphic_value/releases).

## CMake
To include in your CMake build then add a dependency upon the interface target, `polymorphic_value::polymorphic_value`.  This provides the necessary include paths and C++ features required to include `polymorphic_value` into your project.

### Extenal
To include `polymorphic_value` you will need use find package to locate the provided namespace imported targets from the generated package configuration.  The package configuration file, *polymorphic_value-config.cmake* can be included from the install location or directly out of the build tree.
```cmake
# CMakeLists.txt
find_package(polymorphic_value 1.0.0 REQUIRED)
...
add_library(foo ...)
...
target_link_libraries(foo PRIVATE polymorphic_value::polymorphic_value)
```
# Building

The project contains a helper scripts for building that can be found at **<project root>/scripts/build.py**. The project can be build with the helper script as follows:

```bash
cd <project root>
python script/build.py [--clean] [-o|--output=<build dir>] [-c|--config=<Debug|Release>] [--sanitizers] [-v|--verbose] [-t|--tests]
```

The script will by default build the project via Visual Studio on Windows. On Linux and Mac it will attempt to build via Ninja if available, then Make and will default to the system defaults for choice of compiler.

## Building Manually Via CMake

It is possible to build the project manually via CMake for a finer grained level of control regarding underlying build systems and compilers. This can be achieved as follows:
```bash
cd <project root>
mkdir build
cd build

cmake -G <generator> <configuration options> ../
cmake --build ../
ctest
```

The following configuration options are available:

| Name                | Possible Values | Description                             | Default Value |
|---------------------|-----------------|-----------------------------------------|---------------|
| `BUILD_TESTING`     | `ON`, `OFF`     | Build the test suite                    | `ON`          |
| `ENABLE_SANITIZERS` | `ON`, `OFF`     | Build the tests with sanitizers enabled | `OFF`         |
| `Catch2_ROOT`       | `<path>`        | Path to a Catch2 installation           | undefined     |

## Installing Via CMake

```bash
cd <project root>
mkdir build
cd build

cmake -G <generator> <configuration options> -DCMAKE_INSTALL_PREFIX=<install dir> ../
cmake --install ../
```

# Packaging

The Polymorphic Value library is available for integration into your own project via our favorite package manager: [Conan](https://docs.conan.io/en/latest/).

## Conan

Polymorphic Value is now available on the Conan Center Index: https://conan.io/center/polymorphic_value.  Just include the following dependency in your `conanfile.txt` or `conanfile.py` within your project, install via Conan and build using build system of choice.

```bash
polymorphic_value/1.3.0
```