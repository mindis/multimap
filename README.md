# Multimap

<!-- Official website: <http://multimap.io> -->

## Installation

### Shared Library

    $ qmake multimap-library.pro
    $ make
    $ make install

Builds the shared library `libmultimap.so` and installs it under `/usr/local/lib`. Header files are copied to `/usr/local/include/multimap`. The call to `make install` requires superuser privileges; use `sudo` if available.

### Command Line Tool

    $ qmake multimap-tool.pro
    $ make
    $ make install

Builds the command line tool `multimap` and installs it under `/usr/local/bin`. The call to `make install` requires superuser privileges; use `sudo` if available.
