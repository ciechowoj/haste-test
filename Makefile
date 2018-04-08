
CXX = g++-7
CXXFLAGS = -march=native -g -O0 -Wall -Wextra -ansi -pedantic -std=c++17

INCLUDE_DIRS = -Iinclude -Ibackward-cpp

CXXFLAGS := $(CXXFLAGS) $(INCLUDE_DIRS)

SOURCE_FILES = $(wildcard *.cpp)
SOURCE_FILES := $(filter-out test.cpp jvalue.cpp, $(SOURCE_FILES))
HEADER_FILES = $(wildcard include/haste/*)
OBJECT_FILES = $(SOURCE_FILES:%.cpp=build/%.o)
OBJECT_FILES_UT = $(SOURCE_FILES:%.cpp=build/%.ut.o)

DEPENDENCY_FLAGS = -MT $@ -MMD -MP -MF build/$*.Td
DEPENDENCY_POST = mv -f build/$*.Td build/$*.d

.PHONY: all test clean distclean

all: build/libhaste.a

test: build/test.bin
	@cd "./unit-tests"; "./../build/test.bin"

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

build/libhaste.a: $(OBJECT_FILES) Makefile
	ar rcs build/libhaste.a $(OBJECT_FILES)

build/test.bin: $(OBJECT_FILES_UT) build/test.o Makefile
	$(CXX) -g $(OBJECT_FILES_UT) build/test.o -ldw -lpng -o build/test.bin

-include $(OBJECT_FILES:build/%.o=build/%.d)






