project := dbtools
summary := C++ library for managing database data

STD := c++20

library := lib$(project)
$(library).type := shared
$(library).libs := ext++ fmt

install := $(library)
targets := $(install)

files = $(include) $(src) Makefile VERSION

include mkbuild/base.mk
