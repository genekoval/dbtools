project := dbtools
summary := C++ library for managing database data

STD := c++20

library := lib$(project)
$(library).type := shared
$(library).libs := ext++ fmt netcore pg++

install := $(library)
targets := $(install)

install.directories = $(include)/$(project)

files = $(include) $(src) Makefile VERSION

include mkbuild/base.mk
