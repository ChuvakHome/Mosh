CC=gcc
CFLAGS=-std=c11 -I/usr/local/include -c -Wall
LDFLAGS=lib/*.a
SOURCES=main.c kv_list.c string_list.c interpreter.c utils.c builtins/cd.c
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=my_shell

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
