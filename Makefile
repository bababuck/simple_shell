NAME := parser

all: ${NAME}.exe

${NAME}.exe: y.tab.c y.tab.h lex.yy.c command.c
	gcc -o ${NAME}.exe y.tab.c lex.yy.c command.c -ll -ly -mmacosx-version-min=14.0

lex.yy.c: ${NAME}.l
	lex ${NAME}.l

y.tab.c y.tab.h: ${NAME}.y
	yacc -d ${NAME}.y

clean:
	rm lex.yy.c ${NAME}.exe y.tab.c y.tab.h
