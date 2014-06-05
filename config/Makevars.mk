# All directories mentioned here are relative to the project root
ifndef BASEDIR
  $(error BASEDIR should be defined before Makevars.mk is included)
endif
ifndef COMPONENTNAME
  $(error COMPONENTNAME should be defined before Makevars.mk is included)
endif

# Customizing/standardizing the behavior of make
SHELL:=/bin/bash

ifndef DEBUG
  OPTFLAGS += -O99
else
  OPTFLAGS += -g2
  # Default to commands if debugging
  ifndef COMMANDS
    COMMANDS := 1
  endif
endif

ifndef PROFILE
else
  OPTFLAGS := -pg -O99
  COMMON_LDFLAGS += -pg
  # Default to commands if profiling
  ifndef COMMANDS
    COMMANDS := 1
  endif
endif

ifndef COMMANDS
  MAKEFLAGS += --quiet
endif

ifndef DSHARED_DIR
  DSHARED_DIR := ~/.mfm/res
endif

ifdef FIND_DEAD_CODE
  OPTFLAGS += -Wunreachable-code
endif

# Common flags: All about errors -- let's help them help us
COMMON_CFLAGS+=-Wall -pedantic -Werror -Wundef -D DSHARED_DIR=\"$(DSHARED_DIR)\"
COMMON_CPPFLAGS+=-ansi -pedantic -Wall -Werror -D DSHARED_DIR=\"$(DSHARED_DIR)\"
COMMON_LDFLAGS+=-Wl,--fatal-warnings

# Native tool chain
NATIVE_GCC:=gcc
NATIVE_GPP:=g++
NATIVE_GCC_CFLAGS:=$(COMMON_CFLAGS)
NATIVE_GCC_CPPFLAGS:=$(COMMON_CPPFLAGS)
NATIVE_GCC_LDFLAGS:=$(COMMON_LDFLAGS)
NATIVE_GCC_DEFINES:=
NATIVE_GCC_OPTS:=$(OPTFLAGS)
NATIVE_GCC_DEBUGS:=
NATIVE_LD:=ld

# Cross tool chain
## (FUTURE EXPANSION OPTION WE NEED TO PRESERVE)
