## For C++ Developers

Multimap is implemented in Standard C++11 and POSIX.
Its target platform is GNU/Linux. Currently, the library has only been tested
under Debian 7.8 (wheezy) and 8.1 (jessie) both for amd64 and i386.

Multimap is built from source using `qmake` as the makefile generator.
Due to compiler optimizations, the produced binary might not run on
different machines. If you want to minimize dependencies, consider to copy/paste
the source code into your project.

### Install Build Tools

```bash
# For Debian 7 based distros
sudo apt-get install qt4-make

# For Debian 8 based distros
sudo apt-get install qt5-make

# Don't know your Debian release?
# lsb_release -r

sudo apt-get install make g++ 
```

### Install Dependencies

```bash
sudo apt-get install libboost-filesystem-dev libboost-thread-dev
```

### Clone Git Repository

```bash
git clone https://bitbucket.org/mtrenkmann/multimap.git
cd multimap
git checkout release-0.2
```

### Build and Install Shared Library

```bash
cd build

# For Debian 7 based distros
qmake shared-library.pro && make

# For Debian 8 based distros (amd64)
/usr/lib/x86_64-linux-gnu/qt5/bin/qmake shared-library.pro && make

# For Debian 8 based distros (i386)
/usr/lib/i386-linux-gnu/qt5/bin/qmake shared-library.pro && make

sudo make install
```

## For Java Developers

Multimap comes with a language binding for Java based on JNI. In order to use
this library you need to compile and install an enhanced version of Multimap's
native library that includes support for this purpose.

First, follow these steps:

1. [Install Build Tools](#install-build-tools)
2. [Install Dependencies](#install-dependencies)
3. [Clone Git Repository](#clone-git-repository)

Then, if not already done, install a Java Development Kit (JDK).

```bash
sudo apt-get install openjdk-7-jdk
```

Set the `JAVA_HOME` environment variable pointing to your JDK. This is only a
temporary configuration required by the subsequent compilation step.

```bash
# For Debian 8 based distros (amd64)
export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64

# For Debian 8 based distros (i386)
export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-i386
```

Compile and install the native library with JNI support.

```bash
cd build

# For Debian 7 based distros
qmake shared-library-jni.pro && make

# For Debian 8 based distros (amd64)
/usr/lib/x86_64-linux-gnu/qt5/bin/qmake shared-library-jni.pro && make

# For Debian 8 based distros (i386)
/usr/lib/i386-linux-gnu/qt5/bin/qmake shared-library-jni.pro && make

sudo make install
```

If you don't have permission to install the library system-wide, you might set
the `java.library.path` property pointing to the directory where your native
library lives. For example, if your build is located in `/home/user/multimap/build`
you can set `-Djava.library.path=/home/user/multimap/build` as VM argument of your Java
project.

### Download Java Library

Download [multimap-0.2-beta.jar](https://bitbucket.org/mtrenkmann/multimap/downloads/multimap-0.2-beta.jar) and make it available to your project's classpath.

