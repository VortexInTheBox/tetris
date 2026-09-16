#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <wchar.h>
#include <locale.h>

#define ARENA_SIZE_X 10
#define ARENA_SIZE_Y 20
#define N 4
#define GRAVITY_TIME 250 //millisecondi

typedef enum {
    CMD_NONE = 0,
    CMD_MOV_LEFT,
    CMD_MOV_RIGHT,
    CMD_SOFT_DROP,
    CMD_HARD_DROP,
    CMD_STORE,
    CMD_ROT_RIGHT,
    CMD_ROT_LEFT
} command;

const int tetrominos [7][N][N] = {
    /*
    I:
    ####
    */
    {
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    },
    /*
    T:
    ###
     #
    */
    {
        {0, 0, 0, 0},
        {1, 1, 1, 0},
        {0, 1, 0, 0},
        {0, 0, 0, 0}
    },
    /*
    L:
    #
    #
    ##
    */
    {
        {0, 0, 0, 0},
        {1, 1, 1, 0},
        {1, 0, 0, 0},
        {0, 0, 0, 0}
    },
    /*
    J:
     #
     #
    ##
    */
    {
        {0, 0, 0, 0},
        {1, 1, 1, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 0}
    },
    /*
    S:
     ##
    ##
    */
    {
        {0, 0, 0, 0},
        {0, 1, 1, 0},
        {1, 1, 0, 0},
        {0, 0, 0, 0}
    },
    /*
    Z:
    ##
     ##
    */
    {
        {0, 0, 0, 0},
        {1, 1, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 0, 0}
    },
    /*
    O:
    ##
    ##
    */
    {
        {0, 0, 0, 0},
        {0, 1, 1, 0},
        {0, 1, 1, 0},
        {0, 0, 0, 0}
    }
};



typedef struct {
    int x;
    int y;
    int piece[N][N];
    int index;
} tetromino;

typedef struct {
    int arena[ARENA_SIZE_Y][ARENA_SIZE_X];
    tetromino current_tetromino;
    int stored_index;
    int gravity_timer;
    int gameover;
} game_state;

void set_arena(int arena[ARENA_SIZE_Y][ARENA_SIZE_X]){
    for (int i = 0; i < ARENA_SIZE_Y; i++){
        for (int j = 0; j < ARENA_SIZE_X; j++){
            arena[i][j] = 0;
        }
    }
}

void print_arena(int arena[ARENA_SIZE_Y][ARENA_SIZE_X], tetromino t){
    erase();

    chtype block = ACS_CKBOARD;

    for (int x = 0; x < (2 * ARENA_SIZE_X) + 2; x++)
        mvaddch(0, x, block);

    for (int y = 0; y < ARENA_SIZE_Y; y++){

        mvaddch(y + 1, 0, block);

        for (int x = 0; x < ARENA_SIZE_X; x++){
            int cell = arena[y][x];

            int py = y - t.y;
            int px = x - t.x;

            if (py >= 0 && py < N &&
                px >= 0 && px < N &&
                t.piece[py][px]) {
                cell = t.index + 1;
            }

            //mvaddch(y + 1, x + 1, cell ? ACS_CKBOARD : ' ');
            mvaddch(y + 1, (x * 2) + 1, cell ? (ACS_CKBOARD | COLOR_PAIR(cell)) : ' ' );
            mvaddch(y + 1, (x * 2) + 2, cell ? (ACS_CKBOARD | COLOR_PAIR(cell)) : ' ' );
        }
        mvaddch(y + 1, (2 * ARENA_SIZE_X) + 1, block);
    }

    for (int x = 0; x < (2 * ARENA_SIZE_X) + 2; x++)
        mvaddch(ARENA_SIZE_Y + 1, x, block);

    refresh();
}

void update_arena(int arena[ARENA_SIZE_Y][ARENA_SIZE_X], tetromino t) {
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            if (t.piece[y][x]) {

                int arena_x = t.x + x;
                int arena_y = t.y + y;

                arena[arena_y][arena_x] = t.index + 1;
            }
        }
    }
}

void setup_tetromino(tetromino *t, int idx, int x, int y) {
    memcpy(t->piece, tetrominos[idx], sizeof(t->piece));
    t->index = idx;
    t->x = x;
    t->y = y;
}

int is_valid_position(int arena[ARENA_SIZE_Y][ARENA_SIZE_X], tetromino t) {
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            if (t.piece[y][x]) {

                int arena_x = t.x + x;
                int arena_y = t.y + y;

                if (arena_x < 0 || arena_x >= ARENA_SIZE_X ||
                    arena_y < 0 || arena_y >= ARENA_SIZE_Y)
                    return 0;

                if (arena[arena_y][arena_x])
                    return 0;
            }
        }
    }
    return 1;
}

void rotate_left(tetromino *t){
    int tmp[N][N];

    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
            tmp[y][x] = t->piece[x][N - 1 - y];


    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
            t->piece[y][x] = tmp[y][x];
}

void rotate_right(tetromino *t){
    int tmp[N][N];

    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
            tmp[y][x] = t->piece[N - 1 - x][y];

    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
            t->piece[y][x] = tmp[y][x];
}

void clear_full_lines(int arena[ARENA_SIZE_Y][ARENA_SIZE_X]) {
    for (int y = ARENA_SIZE_Y - 1; y >= 0; y--) {

        int full = 1;

        for (int x = 0; x < ARENA_SIZE_X; x++) {
            if (!arena[y][x]) {
                full = 0;
                break;
            }
        }

        if (full) {
            for (int z = y; z > 0; z--) {
                for (int x = 0; x < ARENA_SIZE_X; x++)
                    arena[z][x] = arena[z - 1][x];
            }

            for (int x = 0; x < ARENA_SIZE_X; x++)
                arena[0][x] = 0;

            y++;
        }
    }
}

void lock_piece(game_state *g) {
    update_arena(g->arena, g->current_tetromino);
    clear_full_lines(g->arena);

    setup_tetromino(&g->current_tetromino, rand() % 7, 3, 0);
}

void hard_drop(game_state *g) {
    tetromino *t = &g->current_tetromino;

    while (1) {
        tetromino tmp = *t;
        tmp.y++;

        if (!is_valid_position(g->arena, tmp))
            break;

        t->y++;
    }

    lock_piece(g);
}

command input() {
    int key = getch();

    if (key == ERR)
        return -1;

    switch (key) {
        case 'a': return CMD_MOV_LEFT;
        case 'd': return CMD_MOV_RIGHT;
        case 's': return CMD_SOFT_DROP;
        case 'w': return CMD_HARD_DROP;
        case 'e': return CMD_ROT_RIGHT;
        case 'q': return CMD_ROT_LEFT;
        case ' ': return CMD_STORE;
        default:  return -1;
    }
}

void apply_command(game_state *g, command cmd) {
    tetromino tmp = g->current_tetromino;

    switch (cmd)
    {
    case CMD_MOV_LEFT:
        tmp.x -= 1;
        break;

    case CMD_MOV_RIGHT:
        tmp.x += 1;
        break;
        
    case CMD_SOFT_DROP:
        tmp.y += 1;
        break;

    case CMD_HARD_DROP:
        hard_drop(g);
        //clear_full_lines(g->arena);
        setup_tetromino(&tmp, rand() % 7, 2, 2);
        break;

    case CMD_ROT_LEFT:
        rotate_left(&tmp);
        break;
    
    case CMD_ROT_RIGHT:
        rotate_right(&tmp);
        break;

    case CMD_STORE:
        int current_idx = g->current_tetromino.index;

        if (g->stored_index == -1) {
            g->stored_index = current_idx;
            setup_tetromino(&tmp, rand() % 7, tmp.x, tmp.y);
        } else {
            setup_tetromino(&tmp, g->stored_index, tmp.x, tmp.y);
            g->stored_index = current_idx;
        }

        tmp.x = 2;
        tmp.y = 5;
        
        break;

    default:
        break;
    }

    if (is_valid_position(g->arena, tmp))
        g->current_tetromino = tmp;
}

void update(game_state *g) {
    g->gravity_timer--;

    if (g->gravity_timer <= 0) {
        g->gravity_timer = GRAVITY_TIME;

        tetromino tmp = g->current_tetromino;
        tmp.y++;

        if (is_valid_position(g->arena, tmp)) {
            g->current_tetromino = tmp;
        }
        else {
            lock_piece(g);
        }
    }
}

int main() {
    setlocale(LC_ALL, "");

    initscr();
    start_color();
    use_default_colors();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    bkgd(COLOR_PAIR(0));
    
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(4, COLOR_GREEN, COLOR_BLACK);
    init_pair(5, COLOR_RED, COLOR_BLACK);
    init_pair(6, COLOR_BLUE, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);

    game_state game;

    set_arena(game.arena);

    getchar();
    setup_tetromino(&game.current_tetromino, rand() % 7, 2, 2);

    game.gameover = 0;
    game.gravity_timer = GRAVITY_TIME;

    while (!game.gameover) {
        command cmd = input();

        apply_command(&game, cmd);

        update(&game);

        print_arena(game.arena, game.current_tetromino);

        usleep(GRAVITY_TIME*10);
    }

    endwin();
    
    return 0;
}
