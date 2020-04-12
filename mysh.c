/*
@author: Linh Tang & Dingjie Xi
@due date: Feb 10
@lab: Shell
*/

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

// This is the maximum number of arguments your shell should handle for one command
#define MAX_ARGS 128

/*
@para : delim - character between commands(;&|)
        cmd - a string that contains the command
@return: 0 if successfully executed 
         1 if invalid command or exit */
int executecmd(char *cmd, char delim)
{
  char *saveptr;
  char *command[MAX_ARGS];
  int i = 0;
  char delimiter[] = " \n\t";
  command[i] = strtok_r(cmd, delimiter, &saveptr);
  while (command[i] != NULL)
  {
    i++;
    command[i] = strtok_r(NULL, delimiter, &saveptr);
  }
  command[i] = NULL;
  if (command[0] == NULL)
    return 1;
  else if (strcmp(command[0], "exit") == 0)
    exit(0);
  else if (strcmp(command[0], "cd") == 0)
    chdir(command[1]);
  else
  {
    int rc = fork();
    if (rc < 0)
      fprintf(stderr, "fork failed\n");
    else if (rc == 0)
      execvp(command[0], command);
    else
    {
      int wstatus;

      // If it's the background command (command ends with "&")
      if (delim == '&')
      {
        int rc_wait = waitpid(-1, &wstatus, WNOHANG);
        if (rc_wait > 0)
          printf("Child process %d exits with status %d\n", rc_wait, WEXITSTATUS(wstatus));
      }

      // if it's not the background command (command ends with ";", "|" or nothing)
      else
      {
        int rc_wait = waitpid(-1, &wstatus, WNOHANG);
        while (rc_wait >= 0)
        {
          if (rc_wait > 0)
          {
            printf("Child process %d exits with status %d\n", rc_wait, WEXITSTATUS(wstatus));
          }
          rc_wait = waitpid(-1, &wstatus, WNOHANG);
        }
      }
    }
  }
  return 0;
}

int main(int argc, char **argv)
{
  // If there was a command line option passed in, use that file instead of stdin
  if (argc == 2)
  {
    // Try to open the file
    int new_input = open(argv[1], O_RDONLY);
    if (new_input == -1)
    {
      fprintf(stderr, "Failed to open input file %s\n", argv[1]);
      exit(1);
    }

    // Now swap this file in and use it as stdin
    if (dup2(new_input, STDIN_FILENO) == -1)
    {
      fprintf(stderr, "Failed to set new file as input\n");
      exit(2);
    }
  }

  char *line = NULL;    // Pointer that will hold the line we read in
  size_t line_size = 0; // The number of bytes available in line

  // Loop forever
  while (true)
  {
    // Print the shell prompt
    printf("$ ");

    // Get a line of stdin, storing the string pointer in line
    if (getline(&line, &line_size, stdin) == -1)
    {
      if (errno == EINVAL)
      {
        perror("Unable to read command line");
        exit(2);
      }
      else
      {
        // Must have been end of file (ctrl+D)
        printf("\nShutting down...\n");

        // Exit the infinite loop
        break;
      }
    }

    char *current_command = line;
    while (true)
    {
      // Call strpbrk to find the next occurrence of a delimiter
      char *delim_position = strpbrk(current_command, ";&|\n");

      //End of line, no more command to read
      if (delim_position == NULL)
        break;

      // There was a delimiter. First, save it.
      char delim = *delim_position;

      // Overwrite the delimiter with a null terminator so we can save the command
      *delim_position = '\0';

      // Execute command
      int check = executecmd(current_command, delim);

      // invalid command orr command = "exit"
      if (check == 1)
        break;

      // Move our current position in the string to one character past the delimiter
      current_command = delim_position + 1;
    }
  }

  // If we read in at least one line, free this space
  if (line != NULL)
  {
    free(line);
  }

  return 0;
}
