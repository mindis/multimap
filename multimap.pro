TEMPLATE = subdirs

SUBDIRS = \
  multimap-library.pro \
  multimap-library-dbg.pro \
  multimap-library-jni.pro \
  multimap-library-jni-dbg.pro \
  multimap-tests.pro \
  multimap-tool.pro

# In order to generate Makefiles, object files, and build targets in the
# project's root directory you need to disable Shadow build in QtCreator.
