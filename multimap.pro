TEMPLATE = subdirs

SUBDIRS = \
  build/benchmarks.pro \
  build/shared-library.pro \
  build/shared-library-dbg.pro \
  build/shared-library-jni.pro \
  build/unit-tests.pro

# To generate Makefiles, object files, and build targets in the build directory
# you need to disable Shadow build in QtCreator.
