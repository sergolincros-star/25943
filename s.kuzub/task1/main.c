#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ulimit.h>
#include <unistd.h>

extern char **environ;

struct Option {
  int type;
  char *argument;
};

int main(int argc, char *argv[]) {
  /* Вычисляем максимальное возможное число опций с учетом группировки */
  int max_opts = 1;
  for (int k = 1; k < argc; ++k) {
    max_opts += strlen(argv[k]);
  }

  struct Option *array = malloc(max_opts * sizeof(struct Option));
  if (array == NULL) {
    perror("Ошибка выделения памяти");
    return 1;
  }

  int opt;
  int opt_count = 0;

  /* Считываем все опции слева направо */
  while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
    array[opt_count].type = opt;
    array[opt_count].argument = optarg;
    opt_count++;
  }

  /* Обработка опций справа налево */
  for (int i = opt_count - 1; i >= 0; --i) {
    opt = array[i].type;
    char *arg = array[i].argument;

    switch (opt) {
    case 'i': {
      printf("[-i] UID: %ld, EUID: %ld, GID: %ld, EGID: %ld\n", (long)getuid(),
             (long)geteuid(), (long)getgid(), (long)getegid());
      break;
    }

    case 's': {
      if (setpgid(0, 0) == -1) {
        perror("[-s] Ошибка в setpgid");
      } else {
        printf("[-s] Процесс стал лидером группы. PGID: %ld\n",
               (long)getpgrp());
      }
      break;
    }

    case 'p': {
      printf("[-p] PID: %ld, PPID: %ld, PGID: %ld\n", (long)getpid(),
             (long)getppid(), (long)getpgrp());
      break;
    }

    case 'u': {
      /*
       * Системный вызов ulimit(UL_GETFSIZE, 0) возвращает лимит
       * максимального размера файла в блоках по 512 байт.
       */
      long lim = ulimit(UL_GETFSIZE, 0);
      if (lim == -1) {
        perror("[-u] Ошибка в ulimit(UL_GETFSIZE)");
      } else {
        printf("[-u] Значение ulimit: %ld блоков (по 512 байт)\n", lim);
      }
      break;
    }

    case 'U': {
      /*
       * Обычный пользователь может только уменьшать ulimit.
       * Увеличивать значение разрешено только root.
       */
      long new_limit = atol(arg);
      if (ulimit(UL_SETFSIZE, new_limit) == -1) {
        perror("[-U] Ошибка при установке ulimit");
      } else {
        printf("[-U] Новое значение ulimit установлено: %ld\n", new_limit);
      }
      break;
    }

    case 'c': {
      struct rlimit rl;
      if (getrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("[-c] Ошибка в getrlimit(RLIMIT_CORE)");
      } else {
        if (rl.rlim_cur == RLIM_INFINITY) {
          printf("[-c] Максимальный размер core-файла: unlimited\n");
        } else {
          printf("[-c] Максимальный размер core-файла: %llu байт\n",
                 (unsigned long long)rl.rlim_cur);
        }
      }
      break;
    }

    case 'C': {
      struct rlimit rl;
      if (getrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("[-C] Ошибка в getrlimit(RLIMIT_CORE)");
      } else {
        rl.rlim_cur = (rlim_t)strtoull(arg, NULL, 10);
        if (setrlimit(RLIMIT_CORE, &rl) == -1) {
          perror("[-C] Ошибка в setrlimit(RLIMIT_CORE)");
        } else {
          printf("[-C] Новый лимит core-файла установлен: %llu байт\n",
                 (unsigned long long)rl.rlim_cur);
        }
      }
      break;
    }

    case 'd': {
      char cwd[PATH_MAX];
      if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("[-d] Текущая директория: %s\n", cwd);
      } else {
        perror("[-d] Ошибка в getcwd");
      }
      break;
    }

    case 'v': {
      printf("[-v] Переменные окружения:\n");
      for (char **env = environ; *env != NULL; ++env) {
        printf("     %s\n", *env);
      }
      break;
    }

    case 'V': {
      /*
       * putenv помещает указатель на строку прямо в окружение,
       * не копируя ее. Т.к. optarg указывает на argv, строка
       * живет на протяжении всего времени работы процесса.
       */
      if (putenv(arg) != 0) {
        perror("[-V] Ошибка в putenv");
      } else {
        printf("[-V] Переменная окружения обновлена: %s\n", arg);
      }
      break;
    }

    case '?':
    default:
      fprintf(stderr, "Встречена некорректная опция или пропущен аргумент.\n");
      break;
    }
  }

  free(array);
  return 0;
}