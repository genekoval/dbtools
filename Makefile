project := dbtools
description := C++ library for managing database data

STD := c++20

library := lib$(project)
$(library).type := shared
$(library).libs := ext++ fmt

install := $(library)
targets := $(install)

include mkbuild/base.mk
