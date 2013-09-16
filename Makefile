#    pixi-tools: a set of software to interface with the Raspberry Pi
#    and PiXi-200 hardware
#    Copyright (C) 2013 Simon Cantrill
#
#    pixi-tools is free software; you can redistribute it and/or
#    modify it under the terms of the GNU Lesser General Public
#    License as published by the Free Software Foundation; either
#    version 2.1 of the License, or (at your option) any later version.
#
#    This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Lesser General Public License for more details.
#
#    You should have received a copy of the GNU Lesser General Public
#    License along with this library; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

PYTHON        = python$(PYTHON_VERSION)
PYTHON_CONFIG = $(PYTHON)-config

get_python_version = $(shell $(PYTHON) -c 'from sys import version_info as ver; print("%d.%d" % (ver.major, ver.minor))')

BUILD_MODE = release
ifeq ($(BUILD_MODE),debug)
    DEBUG = 1
else
    DEBUG = 0
endif

basebuilddir = build
# If user does not specify PYTHON_VERSION, then the python version suffix is empty:
builddir := $(basebuilddir)/$(BUILD_MODE)$(PYTHON_VERSION)
topdir := $(CURDIR)

.SUFFIXES:
.PHONY: $(builddir)

# Create, if necessary, a Makefile in the build directory, and invoke make from there.
$(builddir):
	+@test -d $@ || mkdir -p $@
	+@test -e $@/Makefile || ( \
		echo 'DEBUG          = $(DEBUG)' ; \
		echo 'topdir         = $(topdir)' ; \
		echo include '$$(topdir)'/Makefile.build ; \
		echo ; \
		echo 'PYTHON         = $(PYTHON)' ; \
		echo 'PYTHON_CONFIG  = $(PYTHON_CONFIG)' ; \
		echo 'PYTHON_CFLAGS  = -fPIC $(shell $(PYTHON_CONFIG) --cflags)' ; \
		echo 'PYTHON_VERSION = $(get_python_version)' ; \
		) > $@/Makefile
	+@$(MAKE) -C $@ $(MAKECMDGOALS)

Makefile : ;

clean: clean-source
	rm -rf $(builddir)

clean-all-builds: clean-source
	rm -rf $(basebuilddir)

clean-source:
	find -name \*~ | xargs --no-run-if-empty rm

multi-check:
	$(MAKE) check BUILD_MODE=debug
	PYTHON_VERSION=3 $(MAKE) check BUILD_MODE=debug
	$(MAKE) check
	PYTHON_VERSION=3 $(MAKE) check

multi-check-python:
	$(MAKE) check-python BUILD_MODE=debug
	PYTHON_VERSION=3 $(MAKE) check-python BUILD_MODE=debug
	$(MAKE) check-python
	PYTHON_VERSION=3 $(MAKE) check-python

% :: $(builddir) ; :
