# bfc - portable build
#
#   make          build bfc (bfc.exe on Windows)
#   make test     build and run the full test suite
#   make clean
#
# Override the toolchain for cross builds, e.g.
#   make CXX=x86_64-linux-gnu-g++

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

ifeq ($(OS),Windows_NT)
  BIN := bfc.exe
  RM  := del /Q
else
  BIN := bfc
  RM  := rm -f
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Darwin)
    LDFLAGS ?=
  else
    LDFLAGS ?= -static
  endif
endif

all: $(BIN)

$(BIN): bfc.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(BIN) bfc.cpp

test: $(BIN)
ifeq ($(OS),Windows_NT)
	powershell -NoProfile -File tests/run_tests.ps1
else
	sh tests/run_tests.sh
endif

clean:
	-$(RM) $(BIN)

.PHONY: all test clean
