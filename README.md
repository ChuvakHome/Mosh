# Mosh
Command Line Interpreter on C.

My Own Shell (MOSh) is an attempt to make interpreter like bash, zsh.

Avaliable features:

- command substitution
- arrows for manipulating (⬆ - go to prevoius command, ⬇ - go to next command, ⬅/➡ - move caret left/right)
- cd
- autocd (if you type directory path, interpreter change your **PWD**)
- autocat (if you type regular file path, interpreter print its content to stdout)
- environment variable substitution

# Dependencies

You should download and build GNU/readline library to build the interpreter successfully.

# Building

**Makefile** for building. To build project type `make` in terminal.
