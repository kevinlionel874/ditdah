# Makefile — C decoder tests for DitDah
#
# Usage:
#   make test        build and run all vector tests (exits non-zero on failure)
#   make clean       remove build artifacts

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Werror -I. -I../../spec

SRCS    = test_decoder.c morse_decoder.c
HDRS    = morse_decoder.h ../../spec/morse_vectors.h

ifeq ($(OS),Windows_NT)
  # On Windows the runner is built in TEMP so no .exe lands in the repo, and cmd has no rm.
  # If Smart App Control blocks the first run of a new build (error 4551), run make test again.
  TARGET := $(subst \,/,$(TEMP))/ditdah_test_decoder.exe
  RUN    := "$(TARGET)"
  CLEAN  := del /q /f "$(subst /,\,$(TARGET))" 2>nul
else
  TARGET := test_decoder
  RUN    := ./$(TARGET)
  CLEAN  := rm -f $(TARGET)
endif

.PHONY: test clean

test: $(TARGET)
	$(RUN)

$(TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o "$@" $(SRCS)

clean:
	-$(CLEAN)
