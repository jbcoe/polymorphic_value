#!/usr/bin/env python
import sys
import os
import platform
import subprocess


def check_for_executable(exe_name, args=['--version']):
    try:
        cmd = [exe_name]
        cmd.extend(args)
        subprocess.check_output(cmd)
        return True
    except Exception:
        return False


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--clean',
        help='remove build directory before build',
        action='store_true')
    parser.add_argument(
        '-t', '--tests', help='run tests', action='store_true')
    parser.add_argument(
        '-v', '--verbose', help='verbose', action='store_true')
    parser.add_argument(
        '-o', '--output',
        help='output dir (relative to source dir)',
        default='build')
    parser.add_argument(
        '-c', '--config',
        help='config (Debug or Release)',
        default='Debug')

    if platform.system() == "Windows":
        parser.add_argument(
            '--win32',
            help='Build 32-bit libraries',
            action='store_true')

    args = parser.parse_args()
    args.platform = platform.system()

    src_dir = os.path.realpath(os.path.dirname(os.path.dirname(__file__)))

    if args.clean:
        subprocess.check_call('rm -rf {}'.format(args.output).split())

    cmake_invocation = ['cmake', '.', '-B{}'.format(args.output)]
    if args.platform == 'Windows':
        if args.win32:
            cmake_invocation.extend(['-G', 'Visual Studio 14 2015'])
        else:
            cmake_invocation.extend(['-G', 'Visual Studio 14 2015 Win64'])
    else:
        if check_for_executable('ninja'):
            cmake_invocation.extend(['-GNinja'])
        cmake_invocation.extend(['-DCMAKE_BUILD_TYPE={}'.format(args.config)])
        cmake_invocation.append('-DCMAKE_BUILD_TYPE={}'.format(args.config))

    if args.verbose:
        cmake_invocation.append('-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON')

    subprocess.check_call(cmake_invocation, cwd=src_dir)
    subprocess.check_call(
        'cmake --build ./{}'.format(args.output).split(), cwd=src_dir)

    if args.tests:
        rc = subprocess.call(
            'ctest . --output-on-failure -C {}'.format(args.config).split(),
            cwd=os.path.join(src_dir, args.output))
        if rc != 0:
            sys.exit(1)


if __name__ == '__main__':
    main()
