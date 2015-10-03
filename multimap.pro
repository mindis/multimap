TEMPLATE = subdirs

SUBDIRS = \
  build/multimap-benchmarks.pro \
  build/multimap-benchmarks-diskio.pro \
  build/multimap-benchmarks-leveldb.pro \
  build/multimap-library.pro \
  build/multimap-library-dbg.pro \
  build/multimap-library-jni.pro \
  build/multimap-tests.pro \
  build/multimap-tool.pro

# To generate Makefiles, object files, and build targets in the build directory
# you need to disable Shadow build in QtCreator.
