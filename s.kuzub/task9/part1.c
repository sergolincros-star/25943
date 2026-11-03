#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

/* Часть 1: родитель не ждёт потомка, поэтому его вывод
   перемешивается с выводом cat(1). */
int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Использование: %s <длинный_файл>\n", argv[0]);
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("Ошибка fork");
    return 1;
  }

  if (pid == 0) {
    execlp("cat", "cat", argv[1], (char *)NULL);
    perror("Ошибка exec cat");
    _exit(127);
  }

  printf("Родитель: создан потомок с PID %ld\n", (long)pid);
  printf("Родитель: эта строка может оказаться посреди вывода cat\n");
  return 0;
}
