# README

Simple shell capable of parsing basic command line prompts.

Loosely follow ideas from [https://www.cs.purdue.edu/homes/grr/SystemsProgrammingBook/Book/Chapter5-WritingYourOwnShell.pdf].

## Support

This shell supports:
- [x] Calling other commands (e.g ls -a)
- [x] Chaining commands with pipes (e.g ls -a | grep "this")
- [x] Wildcard matching (e.g rm *.txt)
- [x] In/out file modification (e.g. >> outfile.txt)
- [x] Builtin commands (e.g. cd ..)
- [x] Running processes in background (e.g. ./a.exe &)

## Sample output

```
personal@Ryans-MBP shell % ./parser.exe
/Users/personal/Documents/shell $ ls -a
.		.git		Makefile	command.c	lex.yy.c	parser.l	y.tab.c
..		.gitignore	README.md	command.h	parser.exe	parser.y	y.tab.h
/Users/personal/Documents/shell $ ls -a | grep comm >> matches.txt
/Users/personal/Documents/shell $ cat matches.txt
command.c
command.h
/Users/personal/Documents/shell $ touch a.txt
/Users/personal/Documents/shell $ ls
Makefile	a.txt		command.h	matches.txt	parser.l	y.tab.c
README.md	command.c	lex.yy.c	parser.exe	parser.y	y.tab.h
/Users/personal/Documents/shell $ rm *.txt
/Users/personal/Documents/shell $ ls
Makefile	README.md	command.c	command.h	lex.yy.c	parser.exe	parser.l	parser.y	y.tab.c		y.tab.h
/Users/personal/Documents/shell $ git add README.md
/Users/personal/Documents/shell $ git commit -m "Update README"
[main 535b113] Update README
 1 file changed, 23 insertions(+), 1 deletion(-)
```

## Design

The system relies on yacc/lex for parsing the user input. This was likely overkill but was more fun to be able to define a language than to simply parse argument by argument.

Each line input is stored as a series of `command_t` objects, made up of one or more `sub_command_t` objects. Once a command has been parsed, it is passed to `execute()` which handles calls to `fork()` and modifying the file descriptors for stdout, stdin, and stderr as needed, including calls to `pipe()`.
