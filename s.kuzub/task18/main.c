#include <dirent.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

static void base_name(const char *path, char *out, size_t size) {
  strncpy(out, path, size - 1);
  out[size - 1] = '\0';

  size_t n = strlen(out);
  while (n > 1 && out[n - 1] == '/')
    out[--n] = '\0';

  char *slash = strrchr(out, '/');
  if (slash != NULL && slash[1] != '\0')
    memmove(out, slash + 1, strlen(slash + 1) + 1);
}

static void print_file(const char *path) {
  struct stat st;
  if (lstat(path, &st) == -1) {
    perror(path);
    return;
  }

  char type;
  if (S_ISDIR(st.st_mode))
    type = 'd';
  else if (S_ISREG(st.st_mode))
    type = '-';
  else
    type = '?';

  char perms[10]; /* 9 символов rwxrwxrwx + '\0' */
  perms[0] = (st.st_mode & S_IRUSR) ? 'r' : '-';
  perms[1] = (st.st_mode & S_IWUSR) ? 'w' : '-';
  perms[2] = (st.st_mode & S_IXUSR) ? 'x' : '-';
  perms[3] = (st.st_mode & S_IRGRP) ? 'r' : '-';
  perms[4] = (st.st_mode & S_IWGRP) ? 'w' : '-';
  perms[5] = (st.st_mode & S_IXGRP) ? 'x' : '-';
  perms[6] = (st.st_mode & S_IROTH) ? 'r' : '-';
  perms[7] = (st.st_mode & S_IWOTH) ? 'w' : '-';
  perms[8] = (st.st_mode & S_IXOTH) ? 'x' : '-';
  perms[9] = '\0';

  /* Если пользователя/группы нет в системе, печатаем число */
  char owner[32], group[32];
  struct passwd *pw = getpwuid(st.st_uid);
  struct group *gr = getgrgid(st.st_gid);
  if (pw != NULL)
    snprintf(owner, sizeof(owner), "%s", pw->pw_name);
  else
    snprintf(owner, sizeof(owner), "%ld", (long)st.st_uid);
  if (gr != NULL)
    snprintf(group, sizeof(group), "%s", gr->gr_name);
  else
    snprintf(group, sizeof(group), "%ld", (long)st.st_gid);

  /* Размер только у обычного файла, иначе поле пустое */
  char size[32] = "";
  if (S_ISREG(st.st_mode))
    snprintf(size, sizeof(size), "%lld", (long long)st.st_size);

  /* Месяц, день, часы:минуты*/
  char date[32];
  strftime(date, sizeof(date), "%b %e %H:%M", localtime(&st.st_mtime));

  /* PATH_MAX -- максимальная длина пути в системе (на Solaris 1024) */
  char name[PATH_MAX];
  base_name(path, name, sizeof(name));

  /* Числа в формате -- ширина колонок, '-' прижимает к левому краю */
  printf("%c%s %3lu %-10s %-10s %10s %s %s\n", type, perms,
         (unsigned long)st.st_nlink, owner, group, size, date, name);
}

/* Каталог: строка про каждый его элемент, включая скрытые, . и .. */
static void print_dir(const char *dir) {
  DIR *d = opendir(dir);
  if (d == NULL) {
    perror(dir);
    return;
  }

  struct dirent *e;
  while ((e = readdir(d)) != NULL) {
    /* lstat нужен путь, а readdir даёт только имя: склеиваем
       "каталог/имя". Если каталог уже кончается на '/', второй не ставим */
    char full[PATH_MAX];
    size_t n = strlen(dir);
    const char *sep = (n > 0 && dir[n - 1] == '/') ? "" : "/";
    if (snprintf(full, sizeof(full), "%s%s%s", dir, sep, e->d_name) >=
        (int)sizeof(full)) {
      fprintf(stderr, "%s/%s: слишком длинный путь\n", dir, e->d_name);
      continue;
    }
    print_file(full);
  }
  closedir(d);
}

/* Как ls -la: каталог -- его содержимое, остальное -- одна строка */
static void list(const char *path) {
  struct stat st;
  if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode))
    print_dir(path);
  else
    print_file(path);
}

int main(int argc, char *argv[]) {
  /* Без аргументов ls -la показывает текущий каталог */
  if (argc < 2) {
    list(".");
    return 0;
  }
  for (int i = 1; i < argc; i++) {
    /* Несколько аргументов: как ls, подписываем, чей это список */
    if (argc > 2)
      printf("%s%s:\n", i > 1 ? "\n" : "", argv[i]);
    list(argv[i]);
  }
  return 0;
}
