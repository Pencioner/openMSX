# $Id$
#
# Configuration for Darwin.

# Does platform support symlinks?
USE_SYMLINK:=true

# Default compiler.
OPENMSX_CXX?=g++

# Compile for the selected CPU.
ifeq ($(OPENMSX_TARGET_CPU),x86)
TARGET_FLAGS+=-arch i386
else
TARGET_FLAGS+=-arch $(OPENMSX_TARGET_CPU)
endif

# File name extension of executables.
EXEEXT:=

# Bind when executable is loaded, rather then when symbols are accessed.
# I don't know why, but the linker suggests this.
LINK_FLAGS+=-bind_at_load

# These libraries are part of the base system, therefore we do not need to
# link them statically for building a redistributable binary.
SYSTEM_LIBS:=ZLIB TCL XML

ifeq ($(filter 3RD_%,$(LINK_MODE)),)
# Compile against local libs. We assume the binary is intended to be run on
# this Mac only.
SDK_PATH:=
else
# Compile against 3rdparty libs. We assume the binary is intended to be run
# on different Mac OS X versions, so we compile against the SDKs for the
# system-wide libraries.

# Select the SDK for the OS X version we want to be compatible with.
ifeq ($(OPENMSX_TARGET_CPU),ppc)
SDK_PATH:=$(firstword $(sort $(wildcard /Developer/SDKs/MacOSX10.3.?.sdk)))
OPENMSX_CXX:=g++-3.3
OSX_VER:=10.3
OSX_MIN_REQ:=1030
else
SDK_PATH:=/Developer/SDKs/MacOSX10.4u.sdk
OSX_VER:=10.4
OSX_MIN_REQ:=1040
endif

COMPILE_ENV+=NEXT_ROOT=$(SDK_PATH) MACOSX_DEPLOYMENT_TARGET=$(OSX_VER)
LINK_ENV+=NEXT_ROOT=$(SDK_PATH) MACOSX_DEPLOYMENT_TARGET=$(OSX_VER)
TARGET_FLAGS+=-D__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=$(OSX_MIN_REQ)
endif


# Probe Overrides
# ===============

DIR_IF_EXISTS=$(shell test -d $(1) && echo $(1))

# DarwinPorts library and header paths.
DARWINPORTS_CFLAGS:=$(addprefix -I,$(call DIR_IF_EXISTS,/opt/local/include))
DARWINPORTS_LDFLAGS:=$(addprefix -L,$(call DIR_IF_EXISTS,/opt/local/lib))

# Fink library and header paths.
FINK_CFLAGS:=$(addprefix -I,$(call DIR_IF_EXISTS,/sw/include))
FINK_LDFLAGS:=$(addprefix -L,$(call DIR_IF_EXISTS,/sw/lib))

MMAP_PREHEADER:=<sys/types.h>
SYS_MMAN_PREHEADER:=<sys/types.h>
SYS_SOCKET_PREHEADER:=<sys/types.h>
# TODO:
# GL_HEADER:=<OpenGL/gl.h> iso GL_CFLAGS is cleaner,
# but we have to modify the build before we can use it.
GL_CFLAGS:=-I$(SDK_PATH)/System/Library/Frameworks/OpenGL.framework/Headers
GL_LDFLAGS:=-framework OpenGL -lGL \
	-L$(SDK_PATH)/System/Library/Frameworks/OpenGL.framework/Libraries

GLEW_CFLAGS_SYS_DYN+=$(DARWINPORTS_CFLAGS) $(FINK_CFLAGS)
GLEW_LDFLAGS_SYS_DYN+=$(DARWINPORTS_LDFLAGS) $(FINK_LDFLAGS)

SDL_LDFLAGS_3RD_STA:=`$(3RDPARTY_INSTALL_DIR)/bin/sdl-config --static-libs 2>> $(LOG)`

ifneq ($(SDK_PATH),)
# Use tclConfig.sh from /usr and patch the output to point to the SDK dir
# instead of /usr. Ideally we would use tclConfig.sh from the SDK, but the SDK
# doesn't contain that file.
TCL_CFLAGS_SYS_DYN:=`build/tcl-search.sh --cflags 2>> $(LOG) | sed -e 's%-I/%-I$(SDK_PATH)/%'`
TCL_LDFLAGS_SYS_DYN:=`build/tcl-search.sh --ldflags 2>> $(LOG) | sed -e 's%-L/%-L$(SDK_PATH)/%'`

# Use libxml2 from /usr and patch the output of xml2-config to point to
# the SDK dir instead of /usr. Ideally we would use xml2-config from the SDK,
# but the SDK doesn't contain that file.
XML_CFLAGS_SYS_DYN:=`/usr/bin/xml2-config --cflags 2>> $(LOG) | sed -e 's%-I/%-I$(SDK_PATH)/%'`
XML_LDFLAGS_SYS_DYN:=`/usr/bin/xml2-config --libs 2>> $(LOG) | sed -e 's%-L/%-L$(SDK_PATH)/%'`
XML_RESULT_SYS_DYN:=`/usr/bin/xml2-config --version`
endif
