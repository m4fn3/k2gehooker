TARGET := iphone:clang:16.5:13.0
INSTALL_TARGET_PROCESSES = LINE

ARCHS := arm64

SDK_PATH = $(THEOS)/sdks/iPhoneOS16.5.sdk/
SYSROOT = $(SDK_PATH)

THEOS_PACKAGE_SCHEME=rootless

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = k2gehooker
k2gehooker_FILES = Tweak.xm $(shell find k2gehooker -name '*.c')
k2gehooker_CFLAGS = -fobjc-arc

include $(THEOS_MAKE_PATH)/tweak.mk
