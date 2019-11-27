#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import re
import argparse
from cpt.packager import ConanMultiPackager

def username():
    return os.getenv("CONAN_USERNAME", "public-conan")

def login_username():
    return os.getenv("CONAN_LOGIN_USERNAME", "Twonington")

def upload():
    return os.getenv("CONAN_UPLOAD", "https://api.bintray.com/conan/twonington/public-conan")

def upload_only_when_stable():
    return os.getenv("CONAN_UPLOAD_ONLY_WHEN_STABLE", "True").lower() in ["true", "1", "yes"]

def channel():
    return os.getenv("CONAN_CHANNEL", "testing")

def stable_branch_pattern():
    return os.getenv("CONAN_STABLE_BRANCH_PATTERN", r"v\d+\.\d+\.\d+")

def version():
    version = 'latest'
    with open(os.path.join(os.path.dirname(__file__), "..", "CMakeLists.txt")) as file:
        pattern = re.compile(r'set\(POLYMOPHIC_VALUE_VERSION (\d+\.\d+\.\d+)\)')
        for line in file:
            result = pattern.search(line)
            if result:
                version = result.group(1)
    return version

def reference():
    return os.getenv("CONAN_REFERENCE", "polymorphic_value/{}".format(version()))

def str2bool(v):
    if isinstance(v, bool):
        return v
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')

if __name__ == "__main__":

    # To call this its usual for Travis or Appveyor to set environment variables in order to either provide setting you dont want to be visible in the script (i.e. passwords)
    # or to limit the compilers users when generating profiles (particularly for mac where usually only on apple-clang version is installed, even though the script will attempt
    # to generate profiles for numerous.  Compiler versions can be limited as such (with VS and GCC equivalents available).
    # CONAN_APPLE_CLANG_VERSIONS=11.0
    # Travis and Appveyor allow for private environment variables to be defines on the project configuration setting (i.e. not via config files checking into source control).
    # CONAN_PASSWORD=<get this from your user profile on bintray via edit profile -> API Key)
    # Set this to upload testing branches while in development
    # CONAN_UPLOAD_ONLY_WHEN_STABLE=0

    parser = argparse.ArgumentParser()
    parser.add_argument('--username', default=username(), help="The name for the organisation that the linear_algebra repository belongs to.")
    parser.add_argument('--login', default=login_username(), help="The name of a user within the organisation to upload the package under.")
    parser.add_argument('--upload', default=upload(), help="The remote URL to upload to.")
    parser.add_argument('--channel', default=channel(), help="The channel to upload to by default, unless the branch name matches the specified stable branch matching pattern.")
    parser.add_argument('--testing', default=False, type=str2bool, help="Build and run the unit tests as part of the build process")
    parser.add_argument('--code_coverage', default=False, type=str2bool, help="Calculate code coverage as part of running the unit tests.")
    args = parser.parse_args()

    options = {}
    if args.testing:
        options['polymorphic_value:build_testing'] = True
    if args.code_coverage:
        options['polymorphic_value:enable_code_coverage'] = True

    builder = ConanMultiPackager(pip_install=["codecov==2.0.15"],
        username = args.username,
        login_username=args.login,
        upload=args.upload,
        upload_only_when_stable=upload_only_when_stable(),
        stable_branch_pattern=stable_branch_pattern(),
        reference=reference(),
        test_folder=os.path.join(".conan", "test_package")
    )
    if options:
        builder.add(options=options)
    builder.add_common_builds()
    builder.run()