#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024 // Достаточный размер для самой длинной строки

// Структура узла односвязного списка
struct Node {
  char *str; // Указатель на динамически выделенную строку
  struct Node *next; // Указатель на следующий узел
};

int main(void) {
  struct Node *head = NULL; // Голова списка
  struct Node *tail = NULL; // Хвост списка (для быстрой вставки в конец)
  char buffer[BUFFER_SIZE];

  printf("Вводите строки (для завершения введите '.' в начале строки):\n");

  while (1) {
    // Чтение строки с клавиатуры
    if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
      break;
    }

    // Проверка условия выхода: первый символ — точка
    if (buffer[0] == '.') {
      break;
    }

    // Вычисляем длину строки (+1 для нуль-терминатора '\0')
    size_t len = strlen(buffer);

    // Выделяем память под узел списка
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    if (new_node == NULL) {
      fprintf(stderr, "Ошибка выделения памяти под узел!\n");
      return 1;
    }

    // Выделяем память под саму строку
    new_node->str = (char *)malloc((len + 1) * sizeof(char));
    if (new_node->str == NULL) {
      fprintf(stderr, "Ошибка выделения памяти под строку!\n");
      free(new_node);
      return 1;
    }

    // Копируем строку из буфера в выделенную память
    strcpy(new_node->str, buffer);
    new_node->next = NULL;

    // Вставляем узел в конец списка
    if (head == NULL) {
      head = new_node;
      tail = new_node;
    } else {
      tail->next = new_node;
      tail = new_node;
    }
  }

  // Вывод всех строк из списка на экран
  printf("\nВведенные строки из списка:\n");
  struct Node *current = head;
  while (current != NULL) {
    printf("%s", current->str);
    current = current->next;
  }

  // Освобождение динамической памяти
  current = head;
  while (current != NULL) {
    struct Node *temp = current;
    current = current->next;
    free(temp->str); // Сначала удаляем строку внутри узла
    free(temp); // Затем сам узел
  }

  return 0;
}
