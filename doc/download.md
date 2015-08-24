## For C++ Developers

Multimap is implemented in Standard C++11 and makes use of POSIX routines.
Its target platform is GNU/Linux. Currently, the library has only been tested
under Debian 7.8 (wheezy).

Multimap is built from source using `qmake` as the makefile generator.
Due to compiler optimizations, the produced binary might not run on
different machines. If you want to minimize dependencies, consider to copy/paste
the source code into your project.

### Install Dependencies

```bash
# For Debian 7 based distros
sudo apt-get install libboost-filesystem-dev libboost-thread-dev qt4-qmake

# For Debian 8 based distros
sudo apt-get install libboost-filesystem-dev libboost-thread-dev qt5-qmake
```

### Clone Git Repository

```bash
git clone https://bitbucket.org/mtrenkmann/multimap.git
cd multimap
git checkout release-0.1
```

### Build and Install Shared Library
```bash
cd targets/shared-library

# For Debian 7 based distros
qmake && make

# For Debian 8 based distros
/usr/lib/x86_64-linux-gnu/qt5/bin/qmake && make

sudo make install
```

## For Java Developers

Multimap comes with a language binding for Java based on JNI. In order to make
use of the provided Java library, you need to compile and install a dedicated
version of Multimap's native library.

First, if not already done, install a Java Development Kit (JDK) **in addition** to [Install Dependencies](#install-dependencies).

```bash
sudo apt-get install openjdk-7-jdk
```

Set the `JAVA_HOME` environment variable pointing to your JDK. This is only a
temporary configuration required by the subsequent compilation step.

```bash
export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
```

Get the code via [Clone Git Repository](#clone-git-repository).

Compile and install the native library with JNI support.

```bash
cd targets/shared-library-jni

# For Debian 7 based distros
qmake && make

# For Debian 8 based distros
/usr/lib/x86_64-linux-gnu/qt5/bin/qmake && make

sudo make install
```

If you don't have permission to install the library system-wide, you might also
set `-Djava.library.path=/path/to/lib` as VM argument of your Java project.

Download: [multimap-0.1-alpha.jar](https://bitbucket.org/mtrenkmann/multimap/downloads/multimap-0.1-alpha.jar)
