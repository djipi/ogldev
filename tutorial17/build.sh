#!/bin/bash

CC=g++
CPPFLAGS=`pkg-config --cflags glew ImageMagick++ freetype2 libglfw`
CPPFLAGS="$CPPFLAGS -I../Include -I../Common/FreetypeGL"
LDFLAGS=`pkg-config --libs glew ImageMagick++ freetype2 libglfw`
LDFLAGS="$LDFLAGS -lglut ../Lib/libAntTweakBar.a -lX11"

$CC tutorial17.cpp lighting_technique.cpp ../Common/ogldev_util.cpp ../Common/pipeline.cpp ../Common/math_3d.cpp ../Common/camera.cpp ../Common/ogldev_atb.cpp ../Common/glut_backend.cpp ../Common/ogldev_texture.cpp ../Common/ogldev_basic_lighting.cpp ../Common/technique.cpp ../Common/ogldev_app.cpp $CPPFLAGS $LDFLAGS -o tutorial17
