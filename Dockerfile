# Defines an image that is used for Travis CI.
# Getting Travis CI to install correct dependencies was too high a burden.
# Docker allows local testing of Linux CI and easy dependency management.
FROM ubuntu:17.10

RUN apt-get -y update && \
    apt-get install -y \
        python-software-properties \
        software-properties-common

# Software dependencies - sorted alphabetically
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get -y update && \
    apt-get install -y \
        clang \
        cmake \
        g++ \
        libboost-all-dev \
        ninja-build

# Cleanup
RUN apt-get autoremove -y && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# User and environment setup
RUN useradd travis && \
    mkdir -p /home/travis && \
    chown travis /home/travis

ENV HOME=/home/travis 

COPY . /home/travis
WORKDIR /home/travis
