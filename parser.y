/* Yacc grammar file.

Define the grammar for a command.
*/

%{
#include<stdio.h>
#include <stdlib.h>
#include "command.h"

int yylex(void);
int yyerror(char* s);
 
command_t *curr_cmd;
sub_command_t *curr_sub_cmd;
bool continue_execution;
/*
 * Command to be written to prompt user.
 *
 * Eventually want to print the working directory
 */
 void prompt() {
   char cwd[1024];
   getcwd(cwd, 1024);
   printf("%s $ ", cwd);
 }
 
%}

%token NEWLINE FORWARD BACKWARD FORWARDFORWARD FORWARDAMPERSAND AMPERSAND PIPE WORD UNRECOGNIZED FORWARDFORWARDAMPERSAND AMPERSANDAMPERSAND TWOFORWARD TWOFORWARDFORWARD WORD_NOWC

%union {
  char *str;              /* Ptr to constant string (strings are malloc'd) */
};

%type <str> WORD;
%type <str> WORD_NOWC;
%%

goal: command ;

cmd:
    WORD {
      curr_sub_cmd = sub_command_factory();
      command_insert(curr_cmd, curr_sub_cmd);
      sub_command_insert(curr_sub_cmd, $1);
    }

cmd_and_args:
    cmd_and_args WORD {
      sub_command_insert(curr_sub_cmd, $2);
    }
    | cmd_and_args WORD_NOWC {
      sub_command_insert(curr_sub_cmd, $2);
    }
    | cmd ;

pipe_list:
    pipe_list PIPE cmd_and_args
    | cmd_and_args
    ;

io_modifier:
    FORWARDFORWARD WORD {
      curr_cmd->out_file = $2;
      curr_cmd->append_out = true;
    }
    | FORWARD WORD {
      curr_cmd->out_file = $2;
      curr_cmd->append_out = false;
    }
    | TWOFORWARD WORD {
      curr_cmd->err_file = $2;
      curr_cmd->append_err = false;
    }
    | TWOFORWARDFORWARD WORD{
      curr_cmd->err_file = $2;
      curr_cmd->append_err = true;
    }
    | FORWARDFORWARDAMPERSAND WORD {
      curr_cmd->err_file = $2;
      curr_cmd->out_file = $2;
      curr_cmd->append_out = true;
      curr_cmd->append_err = true;
    }
    | FORWARDAMPERSAND WORD {
      curr_cmd->err_file = $2;
      curr_cmd->out_file = $2;
      curr_cmd->append_out = false;
      curr_cmd->append_err = false;
    }
    | BACKWARD WORD {
      curr_cmd->in_file = $2;
    }
    ;

io_modifier_list:
    io_modifier io_modifier_list
    | ;
 
background:
    AMPERSAND {
      curr_cmd->background = true;
    }
    | ;

command_line:
    pipe_list io_modifier_list background {
      if (continue_execution) {
	continue_execution = execute(curr_cmd);
      }
      free(curr_cmd);
      curr_cmd = command_factory();
    } ;

command_list:
    command_line
    | command_list AMPERSANDAMPERSAND command_line
    | ;

command:
    command command_list NEWLINE {
      delete_command(curr_cmd);
      curr_cmd = command_factory();
      prompt();
    }
    | ;
%%

int main()
{
  setbuf(stdout, NULL);
  curr_cmd = command_factory();
  continue_execution = true;
  prompt();
  while (1) {
    yyparse();
  }
  delete_command(curr_cmd);
  return 0;
}
