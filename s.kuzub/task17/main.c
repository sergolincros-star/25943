#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define LINE_MAX_LEN 40
#define KEY_CTRL(c) ((c)&037)
#define ESC 033

static struct termios saved;
static char line[LINE_MAX_LEN + 1];
static int len = 0;

static void restore_tty(void) { tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved); }

/* При ^C вернуть терминал в нормальный режим и выйти */
static void handle_signal(int sig) {
  (void)sig;
  restore_tty();
  _exit(1);
}

static void put(const char *s, int n) { write(STDOUT_FILENO, s, n); }

static void beep(void) { put("\a", 1); }

/* Стереть n последних символов строки с экрана и из буфера */
static void erase_chars(int n) {
  while (n-- > 0 && len > 0) {
    put("\b \b", 3);
    len--;
  }
}

static void new_line(void) {
  put("\n", 1);
  len = 0;
}

/* Сначала пробелы в конце строки, потом само слово */
static void erase_word(void) {
  int n = 0;
  while (len - n > 0 && line[len - n - 1] == ' ')
    n++;
  while (len - n > 0 && line[len - n - 1] != ' ')
    n++;
  erase_chars(n);
}

static void add_char(char c) {
  if (len == LINE_MAX_LEN) {
    if (c == ' ') {
      new_line();
      return;
    }
    /* Слово пересекает 40-й столбец: переносим его целиком */
    int start = len;
    while (start > 0 && line[start - 1] != ' ')
      start--;
    if (start == 0) {
      /* Слово длиннее строки -- переносить некуда, просто рвём */
      new_line();
    } else {
      char word[LINE_MAX_LEN];
      int wlen = len - start;
      for (int i = 0; i < wlen; i++)
        word[i] = line[start + i];
      erase_chars(wlen);
      new_line();
      for (int i = 0; i < wlen; i++)
        line[i] = word[i];
      len = wlen;
      put(line, len);
    }
  }
  line[len++] = c;
  put(&c, 1);
}

int main(void) {
  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "Ввод должен быть терминалом\n");
    return 1;
  }
  if (tcgetattr(STDIN_FILENO, &saved) == -1) {
    perror("Ошибка tcgetattr");
    return 1;
  }

  struct termios t = saved;
  t.c_lflag &= ~(ICANON | ECHO);
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == -1) {
    perror("Ошибка tcsetattr");
    return 1;
  }
  atexit(restore_tty);
  signal(SIGINT, handle_signal);

  char c;
  while (read(STDIN_FILENO, &c, 1) == 1) {
    // DEL (127) Backspace
    if (c == saved.c_cc[VERASE] || c == 127) {
      erase_chars(1);
    } else if (c == saved.c_cc[VKILL]) {
      erase_chars(len);
    } else if (c == KEY_CTRL('w')) {
      erase_word();
    } else if (c == KEY_CTRL('d')) {
      if (len == 0)
        break;
      beep();
    } else if (c == '\n') {
      new_line();
    } else if (c == ESC) {
      /* Стрелка приходит как три символа: ESC [ A.
         Пропускаем оставшиеся два, чтобы "[A" не напечаталось */
      char seq[2];
      read(STDIN_FILENO, &seq[0], 1);
      if (seq[0] == '[')
        read(STDIN_FILENO, &seq[1], 1);
      beep();
    } else if (isprint((unsigned char)c)) {
      add_char(c);
    } else {
      beep(); /* Tab и прочие непечатаемые */
    }
  }

  put("\n", 1);
  return 0;
}
