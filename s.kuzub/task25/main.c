#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  const char *text = "Hello, World! This Is Mixed Case Text.\n"
                     "Pipe Test: abc DEF ghi JKL\n";

  /* fd[0] -- конец для чтения, fd[1] -- конец для записи */
  int fd[2];
  if (pipe(fd) == -1) {
    perror("Ошибка pipe");
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("Ошибка fork");
    return 1;
  }

  if (pid == 0) {
    /* Потомок читает */
    close(fd[1]);

    char buf[256];
    ssize_t n;
    while ((n = read(fd[0], buf, sizeof(buf))) > 0) {
      for (ssize_t i = 0; i < n; i++)
        buf[i] = (char)toupper((unsigned char)buf[i]);
      write(STDOUT_FILENO, buf, (size_t)n);
    }
    close(fd[0]);
    _exit(0);
  }

  /* Родитель пишет*/
  close(fd[0]);

  write(fd[1], text, strlen(text));
  close(fd[1]);

  waitpid(pid, NULL, 0);
  return 0;
}
