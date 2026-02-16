#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h> // Terminal I/O control - Configuring the terminal
#include <fcntl.h>   // 0_NONBlOCK - lets you change how the file for descriptor stdoin behaves
#include <sys/time.h> // for get time of day

static struct termios orig_termios;
static const int paddle_spin[5] = {-2, -1, 0, 1, 2};


#define PADDLE_LEFT        0
#define PADDLE_RIGHT       1
#define NO_PADDLE         -1
#define FRAME_TIME_US 16667
#define BALL_MOVE_INTERVAL 5

typedef struct {
  int dx, dy;
} Vec2;

static const Vec2 ball_dirs[5] = {
  {1, -2},
  {2, -1},
  {3, 0},
  {2, 1},
  {1, 2}
};

typedef struct {
  int x, y;
  int vx, vy;
  int last_paddle_hit;
  int ball_move_counter;
} Ball;

typedef struct {
  int x, y;
  int height;
  int move_counter;
} Paddle;

typedef struct {
  int width;
  int height;
  Ball ball;
  Paddle left_paddle;
  Paddle right_paddle;
  int score_left;
  int score_right;
  int running;
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

void init_game(Game* game) { // INITIALIZE GAME
  game->width = 60;
  game->height = 30;

  // Centers ball, moving right+down
  game->ball.x = game->width / 2;
  game->ball.y = game->height / 2;
  game->ball.vx = 1;
  game->ball.vy = 1;
  game->ball.last_paddle_hit = NO_PADDLE;
  game->ball.ball_move_counter = 0;

  // Paddles centered and on each edges
  game->left_paddle.x = 1;
  game->left_paddle.y = game->height / 2 - 2;
  game->left_paddle.height = 5;

  game->right_paddle.x = game->width - 2;
  game->right_paddle.y = game->height / 2 - 2;
  game->right_paddle.height = 5;

  game->score_left = 0;
  game->score_right = 0;
  game->running = true;  // true = 1   false = 0
}

void render(const Game* game) { // DRAW GAME
  printf("\033[2J\033[H"); // Clear screen + home

  // Print score
  printf("Score: %d   -   %d\n", game->score_left, game->score_right);

  // Draw field borders (simple loop)
  for (int row = 0; row < game->height; row++) {
    printf("|"); // left border

    for (int col = 0; col < game->width; col++) {
      // check if this position has ball or paddle
      if (row == game->ball.y && col == game->ball.x) {
        printf("O"); // Ball
      } else if (col == game->left_paddle.x &&
                 row >= game->left_paddle.y &&
                 row < game->left_paddle.y + game->left_paddle.height) {
          printf("|"); // Left paddle
      } else if (col == game->right_paddle.x &&
                 row >= game->right_paddle.y &&
                 row < game->right_paddle.y + game->right_paddle.height) {
          printf("|"); // Right paddle
      } else {
        printf(" "); // Empty space
      }
    }
    printf("|\n"); // right border + new line
  }

  printf("\nW/S = left paddle, I/K = right paddle (later)\n");
}

void handle_input(Game* game) {
  unsigned char ch;
  ssize_t n = read(STDIN_FILENO, &ch, 1);

  if (ch == 'q') {
    game->running = 0;
  } else if (ch == 'w') {
    game->left_paddle.y--;
  } else if (ch == 's') {
    game->left_paddle.y++;
  } else if (ch == 'i') {
    game->right_paddle.y--;
  } else if (ch == 'k') {
    game->right_paddle.y++;
  }

  // Clamp paddles so they stay on screen
  if (game->left_paddle.y < 0) {
    game->left_paddle.y = 0;
  }
  if (game->left_paddle.y + game->left_paddle.height > game->height) {
    game->left_paddle.y = game->height - game->left_paddle.height;
  }
  if (game->right_paddle.y < 0) {
    game->right_paddle.y = 0;
  }
  if (game->right_paddle.y + game->right_paddle.height > game->height) {
    game->right_paddle.y = game->height - game->right_paddle.height;
  }
}

void update_ball_position(Game* game) {
  // Move ball every 3rd frame
  game->ball.ball_move_counter++;
  if (game->ball.ball_move_counter >= BALL_MOVE_INTERVAL) {
    game->ball.x += game->ball.vx;
    game->ball.y += game->ball.vy;
    game->ball.ball_move_counter = 0;
  }
}

void handle_wall_collision(Game* game) {
  // Bounce off top/bottom walls
  if (game->ball.y <= 0 || game->ball.y >= game->height - 1) {
    game->ball.vy = -game->ball.vy;
  }
}

void handle_paddle_collision(Game* game) {
  // Bounce off paddles (fixed version)
  if (game->ball.x <= game->left_paddle.x + 1 &&
    game->ball.y >= game->left_paddle.y &&
    game->ball.y < game->left_paddle.y + game->left_paddle.height &&
    game->ball.last_paddle_hit != PADDLE_LEFT) {

    int hit_pos = game->ball.y - game->left_paddle.y; // hit pos = either 0,1,2,3,4
    Vec2 dir = ball_dirs[hit_pos];
    game->ball.vx = dir.dx;
    game->ball.vy = dir.dy;
    game->ball.last_paddle_hit = PADDLE_LEFT; // sets cooldown
  }
  if (game->ball.x >= game->right_paddle.x - 1 &&
    game->ball.y >= game->right_paddle.y &&
    game->ball.y < game->right_paddle.y + game->right_paddle.height &&
    game->ball.last_paddle_hit != PADDLE_RIGHT) {

    int hit_pos = game->ball.y - game->right_paddle.y; // hit pos = either 0,1,2,3,4
    Vec2 dir = ball_dirs[hit_pos];
    game->ball.vx = -dir.dx;
    game->ball.vy = dir.dy;
    game->ball.last_paddle_hit = PADDLE_RIGHT; // sets cooldown
  }
}

void handle_scoring_and_reset(Game* game) {
  // Score + reset if ball goes off sides
  if (game->ball.x < 1) {
    sleep(2);
    game->score_right++;
    game->ball.last_paddle_hit = NO_PADDLE;
    game->ball.x = game->width / 2;
    game->ball.y = game->height / 2;
    game->ball.vx = 1;
    game->ball.vy = 1;
  } else if (game->ball.x >= game->width -1) {
    sleep(2);
    game->score_left++;
    game->ball.last_paddle_hit = NO_PADDLE;
    game->ball.x = game->width / 2;
    game->ball.y = game->height / 2;
    game->ball.vx = -1;
    game->ball.vy = 1;
  }
}

void update(Game* game) {
  update_ball_position(game);
  handle_wall_collision(game);
  handle_paddle_collision(game);
  handle_scoring_and_reset(game);
}

int main(void) {
  Game game;
  init_game(&game);

  enable_raw_mode(); // Puts terminal in game mode


  int frame = 0;
  while (frame < 500 && game.running) {  // Loop
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
