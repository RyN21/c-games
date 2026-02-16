#include <stdio.h>
#include <unistd.h>

int main(void) {
  int frame = 0;

  while (frame < 100) {
    printf("Frame: %d\r", frame);
    fflush(stdout);
    usleep(1000000);
    frame++;
  }

  printf("\nDone!\n");
  return 0;
}
