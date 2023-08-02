#! /bin/sh
#
# build.sh
# Copyright (C) 2023 rzavalet <rzavalet@noemail.com>
#
# Distributed under terms of the MIT license.
#


set -xe

clang  -ggdb `pkg-config --cflags raylib` -o simulator simulator.c `pkg-config --libs raylib` -lm

