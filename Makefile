
CXX = g++-7
CXXFLAGS = -march=native -g -O0 -Wall -Wextra -ansi -pedantic -std=c++17

INCLUDE_DIRS = -Iinclude -Ibackward-cpp

CXXFLAGS := $(CXXFLAGS) $(INCLUDE_DIRS)

SOURCE_FILES = $(wildcard *.cpp)
SOURCE_FILES := $(filter-out test.cpp, $(SOURCE_FILES))
HEADER_FILES = $(wildcard include/haste/*)
OBJECT_FILES = $(SOURCE_FILES:%.cpp=build/%.o)

DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/$*.Td
DEPENDENCY_POST = mv -f build/$*.Td build/$*.d

.PHONY: all test clean distclean

all: build/libhaste-test.a

test: build/test.bin
	"build/test.bin"

clean:
	rm -rf build

distclean:
	rm -rf build

build:
	mkdir build

build/%.d: ;

build/%.o: %.cpp build/%.d | build
	$(CXX) -c $(DEPENDENCY_FLAGS) $(CXXFLAGS) $< -o $@
	$(DEPENDENCY_POST)

build/%.ut.o: %.cpp build/%.d | build
	$(CXX) -c $(DEPENDENCY_FLAGS) $(CXXFLAGS) $< -o $@
	$(DEPENDENCY_POST)

build/libhaste-test.a: $(OBJECT_FILES) Makefile
	ar rcs build/libhaste-test.a $(OBJECT_FILES)

build/test.bin: build/libhaste-test.a build/test.o Makefile
	$(CXX) -g build/test.o -Lbuild -lhaste-test -ldw -o build/test.bin

-include $(OBJECT_FILES:build/%.o=build/%.d)
