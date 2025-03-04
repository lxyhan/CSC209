#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLINE 256
#define MAX_PASSWORD 10

#define SUCCESS "Password verified\n"
#define INVALID "Invalid password\n"
#define NO_USER "No such user\n"

int main(void)
{
  char user_id[MAXLINE];
  char password[MAXLINE];

  /* The user will type in a user name on one line followed by a password
     on the next.
     DO NOT add any prompts.  The only output of this program will be one
   of the messages defined above.
     Please read the comments in validate carefully
   */

  if (fgets(user_id, MAXLINE, stdin) == NULL)
  {
    perror("fgets");
    exit(1);
  }
  if (fgets(password, MAXLINE, stdin) == NULL)
  {
    perror("fgets");
    exit(1);
  }

  user_id[strcspn(user_id, "\n")] = '\0';
  password[strcspn(password, "\n")] = '\0';

  if (strlen(user_id) > MAX_PASSWORD || strlen(password) > MAX_PASSWORD)
  {
    exit(1);
  }

  int fd[2];
  if (pipe(fd) == -1)
  {
    perror("pipe");
    exit(1);
  }

  pid_t pid = fork();
  if (pid == -1)
  {
    perror("fork");
    exit(1);
  }

  if (pid == 0)
  {
    close(fd[1]);
    if (dup2(fd[0], STDIN_FILENO) == -1)
    {
      perror("dup2");
      exit(1);
    }

    close(fd[0]);
    execl("./validate", "validate", NULL);

    perror("execl");
    exit(1);
  }
  else
  {
    close(fd[0]);

    if (write(fd[1], user_id, strlen(user_id)) == -1)
    {
      perror("write");
      exit(1);
    }

    if (write(fd[1], password, strlen(password)) == -1)
    {
      perror("write");
      exit(1);
    }

    close(fd[1]);

    int status;
    wait(&status);

    if (!WIFEXITED(status))
    {
      exit(1);
    }

    int exit_status = WEXITSTATUS(status);

    if (exit_status == 0)
    {
      printf(SUCCESS);
    }
    else if (exit_status == 1)
    {
      exit(0);
    }
    else if (exit_status == 2)
    {
      printf(INVALID);
    }
    else if (exit_status == 3)
    {
      printf(NO_USER);
    }
  }

  return 0;
}
