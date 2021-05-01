################################################################################
# Copyright 2020 - NVIDIA Corporation
# SPDX-License-Identifier: MIT
################################################################################
CXX=g++

APP:= deepstream-pose-app

TARGET_DEVICE = $(shell gcc -dumpmachine | cut -f1 -d -)

NVDS_VERSION:=5.1

LIB_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/lib/
APP_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/bin/

ifeq ($(TARGET_DEVICE),aarch64)
  CFLAGS:= -DPLATFORM_TEGRA
endif

#SRCS:= /opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/sources/apps/apps-common/src/deepstream_perf.c src/deepstream_pose_app.cpp \
	src/facialpose_postprocess.cpp src/bodypose_postprocess.cpp src/msgconv.cpp
SRCS:= $(wildcard src/*.cpp) $(wildcard src/*.c) /opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/sources/apps/apps-common/src/deepstream_perf.c 

INCS:= $(wildcard src/*.h) 

PKGS:= gstreamer-1.0 gstreamer-video-1.0 x11 json-glib-1.0 opencv

OBJS:= $(patsubst %.c,%.o, $(patsubst %.cpp,%.o, $(SRCS)))

CFLAGS+= -g -I./src -I/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/sources/apps/apps-common/includes -I/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/sources/includes -I/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/sources/apps/sample_apps/deepstream-app/ -I/usr/local/cuda-$(CUDA_VER)/include -DDS_VERSION_MINOR=0 -DDS_VERSION_MAJOR=5

LIBS+= -L$(LIB_INSTALL_DIR) -lnvdsgst_meta -lnvds_meta -lnvds_utils -lm \
        -lpthread -ldl -Wl,-rpath,$(LIB_INSTALL_DIR)

CFLAGS+= $(shell pkg-config --cflags $(PKGS))

LIBS+= $(shell pkg-config --libs $(PKGS))

all: $(APP)

%.o: %.c $(INCS) Makefile
	$(CC) -c -o $@ $(CFLAGS) $<

%.o: %.cpp $(INCS) Makefile
	$(CXX) -c -o $@ $(CFLAGS) $<

$(APP): $(OBJS) Makefile
	$(CXX) -o $(APP) $(OBJS) $(LIBS)

install: $(APP)
	cp -rv $(APP) $(APP_INSTALL_DIR)

clean:
	rm -rf $(OBJS) $(APP)


