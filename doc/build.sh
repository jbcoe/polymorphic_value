#!/usr/bin/env bash
set -u
set -e
BOOST_ROOT=/Users/jon/DEV/boost_1_65_1/ ../../boost_1_65_1/b2 && 
  cp -R html/ ~/Google\ Drive/boost/boost-polymorphic-value
