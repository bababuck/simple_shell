/* Classes to hold the data associated with a single command.

A command can be broken up into small subcommands.
*/

#include <stdio.h>
#include<stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include "command.h"

static const char * const shell_name = "buc";

sub_command_t* sub_command_factory() {
  /**
   * Allocate space for sub_command_t.
   *
   * Allocate space for 1 element in array and make NULL per execvp() documentation.
   */
  sub_command_t *sub_cmd = malloc(sizeof(sub_command_t));
  sub_cmd->args = malloc(sizeof(char *));
  sub_cmd->args[0] = NULL;
  sub_cmd->arg_count = 0;
  return sub_cmd;
}

void delete_sub_command(sub_command_t* sub_cmd)
{
  for (int i=0; i<sub_cmd->arg_count; ++i) {
    free(sub_cmd->args[i]);
  }
  free(sub_cmd->args);
  free(sub_cmd);
}

void sub_command_insert(sub_command_t* sub_cmd, char* argument)
{
  /**
   * Append new argument to args array and increase counter.
   *
   * Maintain extra NULL element in args array.
   */
  ++(sub_cmd->arg_count);
  sub_cmd->args = realloc(sub_cmd->args, (1 + sub_cmd->arg_count) * sizeof(char *));
  sub_cmd->args[sub_cmd->arg_count - 1] = argument;
  sub_cmd->args[sub_cmd->arg_count] = NULL;
}

command_t* command_factory()
{
  command_t *cmd = malloc(sizeof(command_t));
  cmd->sub_cmd_count = 0;
  cmd->sub_cmds = NULL;
  return cmd;
}

void delete_command(command_t *cmd)
{
  for (int i=0; i<cmd->sub_cmd_count; ++i) {
    delete_sub_command(cmd->sub_cmds[i]);
  }
  free(cmd->sub_cmds);
  free(cmd);
}

void command_insert(command_t* cmd, sub_command_t *sub_cmd)
{
  ++(cmd->sub_cmd_count);
  cmd->sub_cmds = realloc(cmd->sub_cmds, cmd->sub_cmd_count * sizeof(sub_command_t *));
  cmd->sub_cmds[cmd->sub_cmd_count - 1] = sub_cmd;
}


void restore_file_desc(int fd_saved, int fd_to_restore) {
  dup2(fd_saved, fd_to_restore);
  close(fd_saved);
}

int setup_output_file(int std_desc, bool append, const char * const file_name) {
  int fd;
  if (file_name) { // If user specified a file
    int flags;
    flags = O_WRONLY | O_CREAT;
    if (append) {
      flags = flags | O_APPEND;
    }
    fd = open(file_name, flags, 0777); // Open new file, fd
    if (fd < 0) {
      printf("%s: No such file or directory: %s\n", shell_name, file_name);
      return -1;
    }
  } else {
    fd = dup(std_desc); // err_out points to stderr
  }
  return fd;
}

/**
 * Fork and handle child and parent paths.
 */
bool spawn_subproc(sub_command_t *sub_cmd, command_t *cmd, int *status, bool last) {
  int ret = fork();
  if (ret == 0) { // Child
    execvp(sub_cmd->args[0], sub_cmd->args); // Should never return
    return false;
  } else if (ret < 0) { // Error in fork
    perror("fork");
    exit(1);
  } else { // parent, ret == pid
    if (last && !cmd->background) {
      waitpid(ret, status, 0);
    }
  }
  return true;
}

bool execute(command_t *cmd)
{
  if (cmd->sub_cmd_count == 0) return true; // If no sub-commands nothing to-do

  // Save copies of all the std in/out/err file descriptors.
  int tmp_in = dup(0); // New fd pointing to stdin, saving for later in case we override
  int tmp_out = dup(1); // New fd pointing to stdout, saving for later in case we override
  int tmp_err = dup(2); // New fd pointing to stderr, saving for later in case we override

  // Open all the requester in/out/err files.
  // Do this before hand so that:
  // - all fork()'s have access for error file
  // - first fork() has access to in file
  // - any invalid files will be caught before fork() calls.

  // Open specified err file
  int fd_err = setup_output_file(2, cmd->append_err, cmd->err_file);
  if (fd_err < 0) {
    close(tmp_err);
    close(tmp_in);
    close(tmp_out);
    return false;
  }
  dup2(fd_err, 2); // stderr now is fd_err, either the pipe, out_file, or stdout
  close(fd_err); // don't need 2 FileTable entries for this

  // Open specified infile
  int fd_in;
  if (cmd->in_file) { // If user specified a file to read from
    fd_in = open(cmd->in_file, O_RDONLY); // Open a new file, fd_in
    if (fd_in < 0) {
      restore_file_desc(tmp_err, 2); // restore stderr
      close(tmp_in);
      close(tmp_out);
      printf("%s: No such file or directory: %s\n", shell_name, cmd->in_file);
      return false;
    }
  } else {
    fd_in = dup(0); // fd_in points to stdin
  }

  // Open specified outfile (won't be used until last fork()
  int svd_fd_out = setup_output_file(1, cmd->append_out, cmd->out_file);
  if (svd_fd_out  < 0) {
    restore_file_desc(tmp_err, 2); // restore stderr
    close(tmp_in); // Don't restore in since haven't modified stdin yet
    close(fd_in);
    close(tmp_out);
    printf("%s: No such file or directory: %s\n", shell_name, cmd->out_file);
    return false;
  }

  // Loop through all subcommands and fork() a process off for each.
  // Create a pipe to go between each process.
  int ret;
  int fd_out;
  int status = 0;
  bool last;
  for (int i=0; i<cmd->sub_cmd_count; ++i) { // Iterate through all sub-commands
    // At beginning of each iteration, fd_in will always point to fd to read from
    // Will have to create the fd to print to each iteration

    dup2(fd_in, 0); // stdin now is fd_in, which was already set properly
    close(fd_in); // stdin now points to this file, don't need 2 entries in File Table pointing to this file object

    last = i == (cmd->sub_cmd_count - 1);
    if (last) { // Last sub command, will not be pipeing
      fd_out = svd_fd_out;
    } else { // Sub command after this, will send data to pipe
      int fd_pipe[2];
      pipe(fd_pipe); // Pipe from fd_pipe[0] to fd_pipe[1]
      fd_out = fd_pipe[1]; // fd_out is inflow of pipe
      fd_in = fd_pipe[0]; // fd_in is outflow of pipe
    }

    dup2(fd_out, 1); // stdout now is fd_out, either the pipe, out_file, or stdout
    close(fd_out); // don't need 2 FileTable entries for this

    if (!spawn_subproc(cmd->sub_cmds[i], cmd, &status, last)) {
      break;
    }
  }

  // Restore std in/out/err to point to their origional files.
  restore_file_desc(tmp_in, 0); // restore stdin
  restore_file_desc(tmp_out, 1); // restore stdout
  restore_file_desc(tmp_err, 2); // restore stderr
  return (bool) (status == 0);
}
