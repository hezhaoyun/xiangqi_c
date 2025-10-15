
# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -std=c11
LDFLAGS =

# Project name
TARGET = xiangqi

# Source files
SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:.c=.o) $(SRCDIR)/tt.o

# Include directories
INCLUDEDIR = src
CFLAGS += -I$(INCLUDEDIR)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(SRCDIR)/*.o $(TARGET)
