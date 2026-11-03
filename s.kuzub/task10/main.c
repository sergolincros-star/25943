#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Использование: %s <команда> [аргументы...]\n", argv[0]);
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("Ошибка fork");
    return 1;
  }

  if (pid == 0) {
    /* argv[1] -- имя команды, argv[argc] == NULL, как требует execvp */
    execvp(argv[1], &argv[1]);
    perror("Ошибка exec");
    _exit(127);
  }

  int status;
  while (waitpid(pid, &status, 0) == -1) {
    if (errno != EINTR) {
      perror("Ошибка waitpid");
      return 1;
    }
  }

  if (WIFEXITED(status)) {
    printf("Процесс %ld завершился с кодом %d\n", (long)pid,
           WEXITSTATUS(status));
  } else if (WIFSIGNALED(status)) {
    printf("Процесс %ld убит сигналом %d\n", (long)pid, WTERMSIG(status));
  } else {
    printf("Процесс %ld завершился, статус 0x%x\n", (long)pid,
           (unsigned)status);
  }
  return 0;
}
