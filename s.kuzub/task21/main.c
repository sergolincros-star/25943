#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static volatile sig_atomic_t beeps = 0;
static volatile sig_atomic_t quit = 0;

static void on_int(int sig) {
  (void)sig;
  write(STDOUT_FILENO, "\a", 1); /* 1 -- длина строки "\a" */
  beeps++;
}

static void on_quit(int sig) {
  (void)sig;
  quit = 1;
}

static void set_handler(int sig, void (*handler)(int)) {
  struct sigaction sa;
  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(sig, &sa, NULL);
}

int main(void) {
  set_handler(SIGINT, on_int);
  set_handler(SIGQUIT, on_quit);

  printf("Ctrl-C -- сигнал, Ctrl-\\ -- выход\n");

  /* pause() спит, пока не придёт какой-нибудь сигнал */
  while (!quit)
    pause();

  printf("\nСигнал прозвучал %d раз\n", (int)beeps);
  return 0;
}
