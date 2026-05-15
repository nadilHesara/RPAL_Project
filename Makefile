CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
TARGET = rpal20

SRCS = rpal20.c lexer.c ast.c parser.c standardizer.c cse.c utils.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
