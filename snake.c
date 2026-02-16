#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h> // Terminal I/O control - Configuring the terminal
#include <fcntl.h>   // 0_NONBlOCK - lets you change how the file for descriptor stdoin behaves
#include <sys/time.h> // for get time of day

static struct termios orig_termios;

#define FRAME_TIME_US 16667
#define WIDTH         8
#define HEIGHT        8
#define MAX_LENGTH    WIDTH * HEIGHT

typedef enum {
  DIR_UP,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT
} Direction;

typedef struct {
  int x, y;
} Point;

typedef struct {
  Point head;
  int length;
  Direction dir;
  Direction last_dir;
  int move_counter;
  Point body[MAX_LENGTH - 1];
} Snake;

typedef struct {
  Point location;
} Apple;

typedef struct {
  int width;
  int height;
  int score;
  int running;
  Snake snake;
  Apple apple;
} Game;




void disable_raw_mode() {
  tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void enable_raw_mode() {
  struct termios raw;
  tcgetattr(STDIN_FILENO, &orig_termios); // Get current terminal settings and save them
  raw = orig_termios; // Make a copy to modify
  raw.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and ECHO
  tcsetattr(STDIN_FILENO, TCSANOW, &raw); // Apply new settings immidiately

  // Make stdin non-blocking
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

long long now_us(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL); // fills tv.tv_sec and tv.tv_uses [web:217][web:221]
  return (long long)tv.tv_sec * 1000000 + (long long)tv.tv_usec;
}




void init_game(Game* game) {
  game->width = WIDTH;
  game->height = HEIGHT;
  game->score = 0;
  game->running = true;
  game->snake.head.x = game->width / 2;
  game->snake.head.y = game->height / 2;
  game->snake.body[0] = game->snake.head;
  game->snake.dir = DIR_RIGHT;
  game->snake.last_dir = DIR_RIGHT;
  game->snake.length = 1;
  game->snake.move_counter = 0;
  game->apple.location.x = rand() % game->width;
  game->apple.location.y = rand() % (game->height - 2) + 1;
}

void render(Game* game) {
  printf("\033[2J\033[H");

  // Prints score
  printf("Score: %d\n", game->score);

  if (game->running == false) {
    printf("\n\n GAME OVER \n");;
  }

  // Draw field borders (Simple lopp)
  for (int row = 0; row < game->height; row++) {
    printf("|");

    for (int col = 0; col < game->width; col++) {
      if (row == 0 && col >= 0 && col < game->width) {
        printf("=");
      } else if (row == game->height - 1 && col >= 0 && col < game->width) {
        printf("=");
      } else if (row == game->apple.location.y && col == game->apple.location.x) {
        printf("x");
      } else if (row == game->snake.head.y && col == game->snake.head.x) {
        printf("0");
      } else {
        int i = 1;
        int found = 0;
        while (i < game->snake.length && found == 0) {
          if (row == game->snake.body[i].y && col == game->snake.body[i].x) {
            found = 1;
          }
          i++;
        }
        if (found == 1) {
          printf("0");
        } else {
          printf(" ");
        }
      }
    }
    printf("|\n");
  }
}

void shift_snake_body_coords(Game* game) {
  int i = game->snake.length;
  game->snake.body[0] = game->snake.head;
  while (i > 0) {
    game->snake.body[i] = game->snake.body[i - 1];
    i--;
  }
}

void check_snake_body_collision(Game* game) {
  for (int i = 1; i < game->snake.length; i++) {
    if (game->snake.head.x == game->snake.body[i].x &&
        game->snake.head.y == game->snake.body[i].y) {
      game->running = false;
    }
  }
}

void update_snake(Game* game) {
  // Move Snake
  game->snake.move_counter++;
  if (game->snake.move_counter == 15) {
    game->snake.move_counter = 0;
    shift_snake_body_coords(game);
    game->snake.last_dir = game->snake.dir;

    switch (game->snake.dir) {
      case DIR_UP: game->snake.head.y--; break;
      case DIR_DOWN: game->snake.head.y++; break;
      case DIR_LEFT: game->snake.head.x--; break;
      case DIR_RIGHT: game->snake.head.x++; break;
    }
    check_snake_body_collision(game);
  }
}

void update_apple_location(Game* game) {
  while (1) {
    game->apple.location.x = rand() % game->width;
    game->apple.location.y = rand() % (game->height - 2) + 1;

    int occupied = 0;
    for (int i = 0; i < game->snake.length; i++) {
      if (game->apple.location.x == game->snake.body[i].x && game->apple.location.y == game->snake.body[i].y) {
        occupied = 1;
        break;
      }
    }
    if (!occupied) {
      break;
    }
  }
}

void handle_snake_eating_apple(Game* game) {
  if (game->snake.head.x == game->apple.location.x && game->snake.head.y == game->apple.location.y) {
    game->score = game->score + 10;
    game->snake.length++;
    update_apple_location(game);
  }
}

void handle_snake_hitting_wall(Game* game) {
  if (game->snake.head.x < 0 || game->snake.head.x >= game->width) {
    game->running = false;
  }
  if (game->snake.head.y <= 0 || game->snake.head.y >= game->height -1 ) {
    game->running = false;
  }
}

void update(Game* game) {
  update_snake(game);
  handle_snake_eating_apple(game);
  handle_snake_hitting_wall(game);
}

void handle_input(Game* game) {
  unsigned char ch;
  ssize_t n = read(STDIN_FILENO, &ch, 1);

  if (n <= 0) {
    return; // No input - let frame continue without pause
  }

  if (ch == 'q') {
    game->running = false; // "q" = QUIT - game state stops running
    return;
  }

  if (ch == '\x1b') { // ESCAPE
    unsigned char seq[2];

    if (read(STDIN_FILENO, &seq[0], 1) <= 0) return;
    if (read(STDIN_FILENO, &seq[1], 1) <= 0) return;

    if (seq[0] == '[') {
      Direction new_dir;

      switch (seq[1]) {
        case 'A': new_dir = DIR_UP;    break;
        case 'B': new_dir = DIR_DOWN;  break;
        case 'C': new_dir = DIR_RIGHT; break;
        case 'D': new_dir = DIR_LEFT;  break;
      }

      Direction last_dir = game->snake.last_dir;

      if (!((last_dir == DIR_UP    && new_dir == DIR_DOWN) ||
            (last_dir == DIR_DOWN  && new_dir == DIR_UP)   ||
            (last_dir == DIR_LEFT  && new_dir == DIR_RIGHT)||
            (last_dir == DIR_RIGHT && new_dir == DIR_LEFT))) {
        game->snake.dir = new_dir;
      }
    }
  }
}



int main(void) {
  Game game;
  init_game(&game);

  enable_raw_mode(); // Puts terminal in game mode


  int frame = 0;
  while (game.running) {  // Loop
    long long start = now_us(); // Get current time in microseconds

    handle_input(&game);
    update(&game);
    render(&game);
    frame++;

    long long end = now_us(); // Time after doing all work
    long long elapsed = end - start;
    long long remaining = FRAME_TIME_US - elapsed;
    if (remaining > 0) {
      usleep(remaining); // sleep just enough until ~60 FPS
    }
  }

  disable_raw_mode();

  return 0;
}
