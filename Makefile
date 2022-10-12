# Makefile targets:
#
# all/install   build and install the NIF
# clean         clean build products and intermediates
#
# Variables to override:
#
# MIX_APP_PATH  path to the build directory
#
# CC            C compiler
# CXX           C++ compiler
# CROSSCOMPILE	crosscompiler prefix, if any
# CXXFLAGS	compiler flags for compiling all C++ files
# LDFLAGS	linker flags for linking all binaries

CXXFLAGS :=
LDFLAGS ?= $(shell pkg-config --cflags --libs libusb-1.0)
SRC = $(wildcard src/*cpp) 
HEADERS = $(wildcard src/*.h) 
OBJ = $(SRC:src/%.cpp=$(BUILD)/%.o) $(BUILD)/J2534.o

BUILD := _build
PREFIX := out
BIN   := ecudump

all: install


install: $(PREFIX) $(BUILD) $(BIN)

$(OBJ): $(HEADERS) Makefile

$(BUILD)/J2534.o: J2534/J2534.cpp
	$(CXX) -c -IJ2534 $(CXXFLAGS) -o $(BUILD)/J2534.o J2534/J2534.cpp

$(BUILD)/%.o: src/%.cpp
	@echo " CXX $(notdir $@)"
	$(CXX) -c -Ilib/j2534/j2534 -IJ2534 $(CXXFLAGS) -o $@ $<

$(BIN): $(OBJ)
	@echo " LD $(notdir $@)"
	$(CXX) -o $@ $(LDFLAGS) -Llib/j2534/j2534/ $^

$(PREFIX) $(BUILD):
	mkdir -p $@

clean:
	$(RM) $(BIN) $(OBJ)

.PHONY: all clean install

# Don't echo commands unless the caller exports "V=1"
${V}.SILENT:

