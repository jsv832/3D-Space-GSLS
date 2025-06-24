#!/bin/bash

# Create release build and disable OpenGL debugging
release () {
  make clean config=release_x64

  make CXXFLAGS+=" -DNDEBUG" -j8 config=release_x64
}

release

