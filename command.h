/* Classes to hold the data associated with a single command.

A command can be broken up into small subcommands.
*/

#include <stdio.h>
#include<stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

/**
 * Lowest level of a command, a single program call and it's arguments.
 *
 * e.g. ls -a -l
 */
typedef struct
{
  uint8_t arg_count;
  char **args;
} sub_command_t;

/**
 * Create a sub_command deynamically.
 */
sub_command_t* sub_command_factory();

/**
 * Insert an argument for an existing sub_command.
 *
 * Sub-commands are built dynamically, adding arguments one at a time.
 */
void sub_command_insert(sub_command_t* sub_cmd, char* argument);

/**
 * Look for any wildcards and properly expand if found.
 *
 * Insert the matches into the sub-command as arguments.
 */
void insert_expand_wildcards(sub_command_t *sub_cmd, char *arg);

/**
 * Delete a sub-command, including free the associated memory.
 *
 * Assumes all the memory for the arguments was dynamically allocated.
 */
void delete_sub_command(sub_command_t* sub_cmd);

/**
 * Outer level of a command, can contain multiple sub-commnds piped together.
 *
 * Also contains associated IO modifiers
 *
 * e.g. ls | head -n 5 >> tmp
 */
typedef struct
{
  uint8_t sub_cmd_count;
  sub_command_t **sub_cmds;
  char *out_file, *in_file, *err_file;
  bool background, append_out, append_err;
} command_t;

/**
 * Create a command dynamically.
 */
command_t* command_factory();

/**
 * Delete a command and associated sub-commands.
 */
void delete_command(command_t *cmd);

/**
 * Add a sub-command to the current command.
 *
 * It will recieve it's input data from the previous command.
 */
void command_insert(command_t* cmd, sub_command_t *sub_cmd);

/**
 * Execute a command.
 *
 * Set up the proper input and output files along with pipes.
 */
bool execute(command_t *cmd);
