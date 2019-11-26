#!/usr/bin/env python
# -*- coding: utf-8 -*-
from conans import ConanFile, CMake
import codecov
import sys
import copy


class ClearProgramArgs:
    def __init__(self):
        self.sys_args = copy.deepcopy(sys.argv)

    def __enter__(self):
        sys.argv = [sys.argv[0]]
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        sys.argv = self.sys_args
        return True

class PolymorphicValueConan(ConanFile):
    name = "polymorphic_value"
    version = "1.3.0"
    license = "MIT"
    url = "https://github.com/jbcoe/polymorphic_value"
    description = "A polymorphic value type proposed for inclusion to the C++ 20 Standard Library"
    topics = ("conan", "polymorphic value", "header-only", "std", "experimental", "wg21")
    exports_sources = "*.txt", "*.h", "*.natvis", "*.cpp", "*.cmake", "*.cmake.in", "LICENSE.txt"
    generators = "cmake"
    options = {'build_testing': [True, False],
               "enable_code_coverage": [True, False]}
    default_options = {'build_testing': False,
                       'enable_code_coverage': False}
    _cmake = None
    @property
    def cmake(self):
        if self._cmake is None:
            self._cmake = CMake(self)
            self._cmake.definitions.update({
                "BUILD_TESTING": self.options.build_testing or self.options.enable_code_coverage,
                "ENABLE_CODE_COVERAGE": self.options.enable_code_coverage
            })
            self._cmake.configure()
        return self._cmake

    def build(self):
        self.cmake.build()
        if self._cmake.definitions["BUILD_TESTING"]:
            self.cmake.test()
            if self._cmake.definitions["ENABLE_CODE_COVERAGE"]:
                with ClearProgramArgs() as _:
                    codecov.main()

    def package(self):
        self.cmake.install()
