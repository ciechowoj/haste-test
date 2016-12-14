

CXX = g++
CXXFLAGS = -march=native -g -O2 -Wall -std=c++14

INCLUDE_DIRS = -Iinclude

CXXFLAGS := $(CXXFLAGS) $(INCLUDE_DIRS)

SOURCE_FILES = $(wildcard *.cpp)
SOURCE_FILES := $(filter-out test.cpp, $(SOURCE_FILES))
HEADER_FILES = $(wildcard include/haste/*)
OBJECT_FILES = $(SOURCE_FILES:%.cpp=build/%.o)
OBJECT_FILES_UT = $(SOURCE_FILES:%.cpp=build/%.ut.o)

DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/$*.Td
DEPENDENCY_POST = mv -f build/$*.Td build/$*.d

.PHONY: all test clean distclean

all: build/libhaste.a

test: build/test.bin
	@./build/test.bin

clean:
	rm -rf build

distclean:
	rm -rf build

build:
	mkdir build

build/%.d: ;

build/%.o : %.cpp build/%.d | build
	$(CXX) -c $(DEPENDENCY_FLAGS) $(CXXFLAGS) $< -o $@
	$(DEPENDENCY_POST)

build/%.ut.o: %.cpp build/%.d | build
	$(CXX) -c $(DEPENDENCY_FLAGS) $(CXXFLAGS) $< -o $@
	$(DEPENDENCY_POST)

build/libhaste.a: $(OBJECT_FILES) Makefile
	ar rcs build/libhaste.a $(OBJECT_FILES)

build/test.bin: $(OBJECT_FILES_UT) build/test.o Makefile
	$(CXX) $(OBJECT_FILES_UT) build/test.o -o build/test.bin

-include $(OBJECT_FILES:build/%.o=build/%.d)






