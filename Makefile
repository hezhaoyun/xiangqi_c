# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -std=c11
LDFLAGS =

# Project name
TARGET = xiangqi

# Directories
SRCDIR = src
BINDIR = bin
INCLUDEDIR = src

# Executable path
TARGET_EXEC = $(BINDIR)/$(TARGET)

# Source files and object files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BINDIR)/%.o,$(SOURCES))

# Add include directory to CFLAGS
CFLAGS += -I$(INCLUDEDIR)

.PHONY: all clean

all: $(TARGET_EXEC)

# Rule to link the executable
$(TARGET_EXEC): $(OBJECTS)
	@echo "Linking..."
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to compile C source files into object files
$(BINDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling $<..."
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "Cleaning up..."
	rm -rf $(BINDIR)