#!/usr/bin/env python
# -*- coding: utf-8 -*-
from conans import ConanFile, CMake
from conans.tools import load
import re, os

class PolymorphicValueConan(ConanFile):
    name = "polymorphic_value"
    license = "MIT"
    url = "https://github.com/jbcoe/polymorphic_value"
    description = "A polymorphic value type proposed for inclusion to the C++ 20 Standard Library"
    topics = ("conan", "polymorphic value", "header-only", "std", "experimental", "wg21")
    exports_sources = "*.txt", "*.h", "*.natvis", "*.cpp", "*.cmake", "*.cmake.in", "LICENSE.txt"
    generators = "cmake"

    def set_version(self):
        content = load(os.path.join(os.path.dirname(__file__), "CMakeLists.txt"))
        version = re.search(r"set\(POLYMOPHIC_VALUE_VERSION (.*)\)", content).group(1)
        self.version = version.strip()

    _cmake = None
    @property
    def cmake(self):git
        if self._cmake is None:
            self._cmake = CMake(self)
            self._cmake.definitions.update({
                "BUILD_TESTING": False
            })
            self._cmake.configure()
        return self._cmake

    def build(self):
        self.cmake.build()
        if self._cmake.definitions["BUILD_TESTING"]:
            self.cmake.test()

    def package(self):
        self.cmake.install()