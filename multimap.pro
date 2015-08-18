TEMPLATE = subdirs

SUBDIRS = \
  targets/shared-library \
  targets/shared-library-dbg \
  targets/shared-library-jni \
  targets/unit-tests

# To generate Makefiles, object files, and build targets in the respective
# targets/* sub-directories, you need to disable Shadow build in QtCreator.
