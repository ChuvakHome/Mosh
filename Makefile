CC=gcc
CFLAGS=-std=c11 -I/usr/local/include -c -Wall
LDFLAGS=
SLIBRARIES=lib/*.a
SOURCES=main.c kv_list.c string_list.c interpreter/interpreter.c interpreter/interpreter_utils.c utils.c builtins/cd.c
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=my_shell

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(SLIBRARIES) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)
