#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

from conans import ConanFile, CMake, tools

class TestPackageConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    build_requires = "Catch2/2.10.2@catchorg/stable"

    _cmake = None
    @property
    def cmake(self):
        if self._cmake is None:
            self._cmake = CMake(self)
            self._cmake.configure()
        return self._cmake

    def build(self):
        self.cmake.build()

    def test(self):
        if tools.cross_building(self.settings):
            return
        self.cmake.test()