#!/usr/bin/env bash
set -eu

# Build the docs
(cd $(dirname $0); export BOOST_ROOT=/Users/jon/DEV/boost_1_65_1/; "$BOOST_ROOT"/b2)

