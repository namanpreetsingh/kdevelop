# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.1

# Rule file for object file kdevelop.dir/../dummy.o.

# Include any dependencies generated for this rule.
include kdevelop.dir/../dummy.o.depends.make

kdevelop.dir/../dummy.o.depends: ../dummy.cpp
	@echo "Scanning CXX dependencies of kdevelop.dir/../dummy.o"
	$(CMAKE_COMMAND) -E cmake_depends CXX kdevelop.dir/../dummy.o ../dummy.cpp

kdevelop.dir/../dummy.o: ../dummy.cpp
	@echo "Building CXX object kdevelop.dir/../dummy.o"
	c++ -o kdevelop.dir/../dummy.o -Dkdevelop_EXPORTS    -fPIC -I/usr/lib/qt/include -I/opt/kde/include -I/usr/src/kde3-svn/kdevelop/lib/interfaces -I/usr/src/kde3-svn/kdevelop/lib/util   -DQT_SHARED -DQT_NO_DEBUG -DQT_THREAD_SUPPORT -D_REENTRANT -Wnon-virtual-dtor -Wno-long-long -Wundef -ansi -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -Wcast-align -Wconversion -Wchar-subscripts -Wall -W -Wpointer-arith -Wwrite-strings -O2 -Wformat-security -Wmissing-format-attribute -fno-exceptions -fno-check-new -fno-common -c /usr/src/kde3-svn/kdevelop/lib/kdevelop/../dummy.cpp

