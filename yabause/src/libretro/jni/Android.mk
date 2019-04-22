LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

HAVE_MUSASHI = 1
ARCH_IS_LINUX = 1
FORCE_GLEW = 0
HAVE_GLES = 1

include $(CORE_DIR)/Makefile.common

COMMON_FLAGS := -funroll-loops -fomit-frame-pointer -ffast-math \
	-D__LIBRETRO__ -DHAVE_OPENGLES -DHAVE_OPENGLES3 -DHAVE_GLSYM_PRIVATE \
	-DNO_CLI -DDYNAREC_KRONOS=1 -DHAVE_BUILTIN_BSWAP16=1 -DHAVE_BUILTIN_BSWAP32=1 -DHAVE_C99_VARIADIC_MACROS=1 -DHAVE_MUSASHI=1 \
	-DHAVE_FLOORF=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_STDINT_H=1 -DHAVE_SYS_TIME_H=1 -DIMPROVED_SAVESTATES \
	-DSPRITE_CACHE=1 -DHAVE_LIBGL -D_OGLES3_ -DCELL_ASYNC=1 -DRGB_ASYNC=1 -DVDP1_TEXTURE_ASYNC=1 -DHAVE_THREADS=1 -DANDROID -DARCH_IS_LINUX=1 -DHAVE_GLES

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COMMON_FLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE       := retro
LOCAL_SRC_FILES    := $(SOURCES_CXX) $(SOURCES_C) $(SOURCES_S)
LOCAL_C_INCLUDES   := $(INCLUDE_DIRS)
LOCAL_CFLAGS       := -std=gnu99 $(COMMON_FLAGS)
LOCAL_CPPFLAGS     := -std=gnu++11 $(COMMON_FLAGS)
LOCAL_LDFLAGS      := -Wl,-version-script=$(CORE_DIR)/link.T
LOCAL_LDLIBS       := -lGLESv3
LOCAL_CPP_FEATURES := exceptions rtti
LOCAL_ARM_MODE     := arm

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_ARM_NEON := true
endif

include $(BUILD_SHARED_LIBRARY)
