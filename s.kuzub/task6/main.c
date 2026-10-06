#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_LINES 1000 // Максимальное количество строк в файле для таблицы

// Глобальные переменные, так как обработчик сигнала должен иметь к ним доступ
int global_fd = -1;

// Структура для хранения информации об одной строке
struct LineInfo {
  off_t offset; // Смещение от начала файла
  size_t length; // Точная длина строки (включая символ '\n')
};

// Функция-обработчик сигнала таймера (SIGALRM)
void handle_alarm(int sig) {
  (void)sig; // Избавляемся от предупреждения компилятора о неиспользуемой
             // переменной

  printf(
      "\n[Таймаут!] Время на ввод истекло (5 секунд). Вывод всего файла:\n\n");

  if (global_fd >= 0) {
    // Сбрасываем указатель файла на самое начало
    lseek(global_fd, 0L, SEEK_SET);

    char buffer[1024];
    ssize_t bytes_read;

    // Поблочно читаем весь файл и выводим на экран через системный вызов
    // write(2)
    while ((bytes_read = read(global_fd, buffer, sizeof(buffer))) > 0) {
      write(STDOUT_FILENO, buffer, bytes_read);
    }

    close(global_fd);
  }

  exit(0); // Завершаем работу программы
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
    return 1;
  }

  // 1. Открываем файл через системный вызов open(2)
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    perror("Ошибка открытия файла");
    return 1;
  }

  global_fd =
      fd; // Сохраняем дескриптор в глобальную переменную для обработчика

  struct LineInfo table[MAX_LINES];
  int line_count = 0;

  // Первая строка всегда начинается с самого начала файла (смещение 0)
  table[line_count].offset = 0;
  table[line_count].length = 0;

  char ch;
  ssize_t bytes_read;

  // 2. Побайтово читаем файл и строим таблицу отступов
  while ((bytes_read = read(fd, &ch, 1)) > 0) {
    table[line_count].length++; // Увеличиваем длину текущей строки

    if (ch == '\n') {
      off_t current_pos = lseek(fd, 0L, SEEK_CUR); // SEEK_CUR — это 1

      line_count++;
      if (line_count >= MAX_LINES) {
        fprintf(stderr,
                "Предупреждение: файл слишком большой, обрезано до %d строк.\n",
                MAX_LINES);
        break;
      }

      // Следующая строка начинается сразу за текущим '\n'
      table[line_count].offset = current_pos;
      table[line_count].length = 0;
    }
  }

  // Если файл не заканчивался на '\n', но данные были
  if (table[line_count].length > 0 && line_count < MAX_LINES) {
    line_count++;
  }

  // 3. Отладочный вывод таблицы
  printf("--- Таблица отступов и длин ---\n");
  printf("Строка\tОтступ\tДлина\n");
  for (int i = 0; i < line_count; i++) {
    printf("%d\t%ld\t%lu\n", i + 1, (long)table[i].offset,
           (unsigned long)table[i].length);
  }
  printf("-------------------------------\n");

  // Подключаем наш обработчик к сигналу SIGALRM
  signal(SIGALRM, handle_alarm);

  // 4. Цикл запроса номеров строк
  int target_line;
  while (1) {
    printf("\nВведите номер строки (1-%d, или 0 для выхода) [У вас есть 5 "
           "секунд]: ",
           line_count);
    fflush(stdout); // Гарантируем вывод текста на Solaris перед тем как уснуть
                    // на scanf

    // Включаем будильник на 5 секунд прямо перед ожиданием ввода
    alarm(5);

    if (scanf("%d", &target_line) != 1) {
      break;
    }

    // Пользователь успел ввести данные! Выключаем будильник
    alarm(0);

    if (target_line == 0) {
      printf("Выход из программы.\n");
      break;
    }

    if (target_line < 1 || target_line > line_count) {
      printf("Ошибка: Неверный номер строки!\n");
      continue;
    }

    int idx = target_line - 1;

    // Прыгаем на начало нужной строки с помощью lseek(2)
    lseek(fd, table[idx].offset, SEEK_SET);

    // Выделяем временный буфер под точную длину строки (+1 для '\0')
    char *line_buf = (char *)malloc(table[idx].length + 1);
    if (line_buf == NULL) {
      perror("Ошибка malloc");
      close(fd);
      return 1;
    }

    // Читаем ровно столько байт, сколько указано в таблице
    read(fd, line_buf, table[idx].length);
    line_buf[table[idx].length] = '\0'; // Превращаем в честную строку C

    printf("Строка %d: %s", target_line, line_buf);

    free(line_buf);
  }

  close(fd);
  return 0;
}
