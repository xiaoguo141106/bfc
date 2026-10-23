# bfc - portable build
#
#   make           build bfc (bfc.exe on Windows)
#   make test      build and run the full test suite
#   make sanitize  build bfc-san with ASan + UBSan (Linux / macOS)
#   make test-san  build bfc-san and run the suite against it
#   make clean
#
# Override the toolchain for cross builds, e.g.
#   make CXX=x86_64-linux-gnu-g++

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

# Sanitizer build: development and CI only, never shipped.
# Needs libasan/libubsan (Linux or macOS); MinGW does not ship them.
# ASan must link dynamically, so SANLDFLAGS stays empty (no -static).
SANFLAGS ?= -std=c++17 -O1 -g -Wall -Wextra \
            -fsanitize=address,undefined -fno-omit-frame-pointer \
            -fno-sanitize-recover=all -D_GLIBCXX_ASSERTIONS
SANLDFLAGS ?=
SANBIN   := bfc-san$(if $(filter Windows_NT,$(OS)),.exe,)

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

# ---------------------------------------------------------------------------
#  Sanitizer build (ASan + UBSan). Run `make test-san` on Linux or macOS.
#  Not used for release binaries: 2-3x slower and roughly double the memory.
# ---------------------------------------------------------------------------
sanitize:
ifeq ($(OS),Windows_NT)
	@echo "sanitize needs libasan/libubsan; use Linux or macOS (or CI)."
	@exit 1
else
	@$(MAKE) $(SANBIN)
endif

$(SANBIN): bfc.cpp
	$(CXX) $(SANFLAGS) $(SANLDFLAGS) -o $(SANBIN) bfc.cpp

test-san:
ifeq ($(OS),Windows_NT)
	@echo "test-san needs libasan/libubsan; use Linux or macOS (or CI)."
	@exit 1
else
	@$(MAKE) $(SANBIN)
	BFC="$(CURDIR)/$(SANBIN)" sh tests/run_tests.sh
endif

clean:
	-$(RM) $(BIN) $(SANBIN)

.PHONY: all test clean sanitize test-san
