CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -g
LDFLAGS = -lm

SRCDIR = src
INCDIR = include

SRCS = $(SRCDIR)/mapl.c $(SRCDIR)/lexer.c $(SRCDIR)/parser.c $(SRCDIR)/interpreter.c $(SRCDIR)/main.c
OBJS = $(SRCS:.c=.o)
TARGET = mapl

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/mapl.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	del /Q $(SRCDIR)\\*.o 2>nul || rmdir /S /Q $(SRCDIR)
	del /Q $(TARGET).exe 2>nul || exit 0

run: $(TARGET)
	$(TARGET)
