/* Lexer file for the shell.

Use to parse commands.
*/

extern YYSTYPE yylval;
%{ 
#include <string.h> 
#include "y.tab.h"

%}

%%
"\n" {
  return(NEWLINE);
}

[ \t] {
  /* ignore */
}

"2>>" {
  return(TWOFORWARDFORWARD);
}

">>&" {
  return(FORWARDFORWARDAMPERSAND);
}

"2>" {
  return(TWOFORWARD);
}

">&" {
  return(FORWARDAMPERSAND);
}

">>" {
  return(FORWARDFORWARD);
}

"<" {
  return(BACKWARD);
}

"&&" {
  return(AMPERSANDAMPERSAND);
}

"&" {
  return(AMPERSAND);
}

">" {
   return(FORWARD);
}

"|" {
  return(PIPE);
}

[A-Za-z0-9\-\/\.]+ {
  yylval.str=strdup(yytext);
  return(WORD);
}

. {
  return(UNRECOGNIZED);
}
