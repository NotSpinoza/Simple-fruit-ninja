#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>
#include <math.h>

#define MAX_OBJECTS 15
#define MAX_PARTICLES 60

// Game States
typedef enum { MENU, PLAYING, GAME_OVER } GameState;
typedef enum { FRUIT, BOMB } ObjType;
typedef enum { FRUIT_APPLE, FRUIT_BANANA, FRUIT_ORANGE, FRUIT_MELON, FRUIT_GRAPE, FRUIT_STRAWBERRY } FruitType;
typedef enum { STATE_ACTIVE, STATE_SPLIT, STATE_SPLIT_BANANA, STATE_ROBOT, STATE_EXPLODING, STATE_DEAD } ObjState;

// Color Pairs
#define CP_DEFAULT     1
#define CP_HEART       2
#define CP_BOMB        3
#define CP_EXPLOSION   4
#define CP_APPLE       5
#define CP_BANANA      6
#define CP_ORANGE      7
#define CP_MELON       8
#define CP_GRAPE       9
#define CP_STRAWBERRY  10

typedef struct {
    double x, y, vx, vy;
    char ch;
    int color_pair, lifetime;
} Particle;

typedef struct {
    ObjType type;
    FruitType ftype;
    ObjState state;
    double x, y, vx, vy;
    int width, height, color_pair, frame, timer;        
} GameObject;

GameObject objects[MAX_OBJECTS];
Particle particles[MAX_PARTICLES];
int score = 0;
int hearts = 3;
int difficulty = 1; 
int frame_count = 0;
int flash_hearts_timer = 0;
int damage_cooldown = 0; 
GameState game_state = MENU;
int cursor_y = 15; 

// High-Density Unicode Block Sprites (4 rows x 10 cols)
const char* fruit_sprites[6][4] = {
    { "  ▄████▄  ", " ██▓▓▓▓██ ", " ██▓▓▓▓██ ", "  ▀████▀  " }, // Apple
    { "    ▄██▄  ", "  ▄███▀   ", " ▄███▀    ", " ██▀      " }, // Banana
    { "  ░████░  ", " ██▒▒▒▒██ ", " ██▒▒▒▒██ ", "  ░████░  " }, // Orange
    { " ▄██████▄ ", "██▒▒▒▒▒▒██", "██▒▒▒▒▒▒██", " ▀██████▀ " }, // Watermelon
    { " ▄▄    ▄▄ ", "██████████", " ████████ ", "   ▀██▀   " }, // Grape
    { " ▀██████▀ ", "  ██████  ", "   ████   ", "    ▀▀    " }  // Strawberry
};

// Banana Split-Half Render Matrices
const char* banana_left[4]  = { "    ▄█    ", "  ▄███    ", " ▄███     ", " ██▀      " };
const char* banana_right[4] = { "      █▄  ", "      █▀  ", "     ▀    ", "          " };

const char* robot_frames[3][4] = {
    { "   ██   ", " ▄████▄ ", "  ████  ", "  █  █  " }, 
    { "   ██   ", " ▀████▀ ", "  ████  ", "  ████  " }, 
    { "   ██   ", " ▄████▄ ", "  ████  ", " /    \\ " }  
};

int get_fruit_color(FruitType ft) {
    switch(ft) {
        case FRUIT_APPLE: return CP_APPLE;
        case FRUIT_BANANA: return CP_BANANA;
        case FRUIT_ORANGE: return CP_ORANGE;
        case FRUIT_MELON: return CP_MELON;
        case FRUIT_GRAPE: return CP_GRAPE;
        case FRUIT_STRAWBERRY: return CP_STRAWBERRY;
    }
    return CP_DEFAULT;
}

void spawn_particles(double x, double y, int color_pair) {
    char p_chars[] = {'*', '+', 'o', 'x', '~'};
    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime <= 0) {
            particles[i].x = x; particles[i].y = y;
            double angle = ((double)rand() / RAND_MAX) * 2.0 * M_PI;
            double speed = 1.0 + ((double)rand() / RAND_MAX) * 2.0;
            particles[i].vx = cos(angle) * speed;
            particles[i].vy = sin(angle) * speed * 0.4; 
            particles[i].ch = p_chars[rand() % 5];
            particles[i].color_pair = color_pair;
            particles[i].lifetime = 6 + rand() % 4;
            if (++spawned >= 16) break; 
        }
    }
}

void init_color_pairs() {
    start_color();
    use_default_colors();
    init_pair(CP_DEFAULT, COLOR_WHITE, -1);
    init_pair(CP_HEART, COLOR_RED, -1);
    init_pair(CP_BOMB, COLOR_RED, COLOR_BLACK);
    init_pair(CP_EXPLOSION, COLOR_YELLOW, COLOR_RED);
    init_pair(CP_APPLE, 196, -1);      
    init_pair(CP_BANANA, 226, -1);     
    init_pair(CP_ORANGE, 214, -1);     
    init_pair(CP_MELON, 46, -1);       
    init_pair(CP_GRAPE, 129, -1);      
    init_pair(CP_STRAWBERRY, 201, -1); 
}

void spawn_object() {
    int max_h, max_w;
    getmaxyx(stdscr, max_h, max_w);
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].state == STATE_DEAD) {
            objects[i].type = (rand() % 5 == 0) ? BOMB : FRUIT;
            objects[i].ftype = (FruitType)(rand() % 6);
            objects[i].state = STATE_ACTIVE;
            objects[i].x = 12 + rand() % (max_w - 24);
            objects[i].y = max_h - 1;
            objects[i].vx = ((double)rand() / RAND_MAX) * 1.4 - 0.7;
            objects[i].vy = -((double)rand() / RAND_MAX) * 0.3 - 1.0 - (difficulty * 0.12);
            objects[i].width = 10; objects[i].height = 4; 
            objects[i].frame = 0; objects[i].timer = 0;
            objects[i].color_pair = (objects[i].type == BOMB) ? CP_BOMB : get_fruit_color(objects[i].ftype);
            break;
        }
    }
}

void trigger_slice(int slice_row) {
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].state == STATE_ACTIVE) {
            if (slice_row >= (int)objects[i].y && slice_row < (int)objects[i].y + objects[i].height) {
                if (objects[i].type == FRUIT) {
                    score += 5;
                    if (objects[i].ftype == FRUIT_BANANA) {
                        objects[i].state = STATE_SPLIT_BANANA;
                    } else {
                        objects[i].state = STATE_SPLIT;
                    }
                    objects[i].timer = 6; 
                    spawn_particles(objects[i].x + 5, objects[i].y + 2, objects[i].color_pair);
                } else {
                    if (damage_cooldown <= 0) {
                        score = (score >= 10) ? score - 10 : 0;
                        hearts--; flash_hearts_timer = 8; damage_cooldown = 15; 
                        objects[i].state = STATE_EXPLODING; objects[i].timer = 10; 
                        spawn_particles(objects[i].x + 5, objects[i].y + 2, CP_EXPLOSION);
                    }
                }
            }
        }
    }
}

void update_game() {
    int max_h, max_w;
    getmaxyx(stdscr, max_h, max_w);
    frame_count++;
    if (flash_hearts_timer > 0) flash_hearts_timer--;
    if (damage_cooldown > 0) damage_cooldown--;
    
    int spawn_rate = 35 - (difficulty * 6);
    if (spawn_rate < 10) spawn_rate = 10;
    if (frame_count % spawn_rate == 0) spawn_object();
    if (frame_count % 400 == 0 && difficulty < 4) difficulty++;
    
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].state == STATE_DEAD) continue;
        if (objects[i].state == STATE_ACTIVE) {
            objects[i].x += objects[i].vx; objects[i].y += objects[i].vy; objects[i].vy += 0.032; 
            if (objects[i].y > max_h || objects[i].x < -12 || objects[i].x > max_w + 12) {
                if (objects[i].type == FRUIT) { hearts--; flash_hearts_timer = 6; }
                objects[i].state = STATE_DEAD; 
            }
        } 
        else if (objects[i].state == STATE_SPLIT || objects[i].state == STATE_SPLIT_BANANA) {
            if (--objects[i].timer <= 0) objects[i].state = STATE_ROBOT;
        } 
        else if (objects[i].state == STATE_ROBOT) {
            objects[i].x += (objects[i].x < max_w / 2) ? -0.8 : 0.8;
            if (frame_count % 4 == 0) objects[i].frame = (objects[i].frame + 1) % 3;
            if (objects[i].x < -10 || objects[i].x > max_w + 5) objects[i].state = STATE_DEAD;
        } 
        else if (objects[i].state == STATE_EXPLODING) {
            if (--objects[i].timer <= 0) objects[i].state = STATE_DEAD;
        }
    }
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime > 0) {
            particles[i].x += particles[i].vx; particles[i].y += particles[i].vy; particles[i].lifetime--;
        }
    }
    if (hearts <= 0) game_state = GAME_OVER;
}

void render_game() {
    int max_h, max_w;
    getmaxyx(stdscr, max_h, max_w);
    erase();
    
    mvprintw(1, 4, "SCORE: %04d   LIVES: ", score);
    attron(COLOR_PAIR(CP_HEART));
    for (int h = 0; h < 3; h++) printw(h < hearts ? "♥ " : "  ");
    attroff(COLOR_PAIR(CP_HEART));
    printw("   BLADE ROW: %d (CLICK and hold/drag to slice!)", cursor_y);
    
    attron(COLOR_PAIR(CP_DEFAULT) | A_DIM);
    mvhline(cursor_y, 0, '=', max_w);
    attroff(COLOR_PAIR(CP_DEFAULT) | A_DIM);
    
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].state == STATE_DEAD) continue;
        int ox = (int)objects[i].x, oy = (int)objects[i].y;
        attron(COLOR_PAIR(objects[i].color_pair));
        
        if (objects[i].state == STATE_ACTIVE) {
            if (objects[i].type == FRUIT) {
                for (int r = 0; r < 4; r++) if (oy + r >= 0 && oy + r < max_h) mvaddstr(oy + r, ox, fruit_sprites[objects[i].ftype][r]);
            } else {
                if (oy >= 0 && oy < max_h)         mvaddstr(oy,     ox, "  ▄████▄  ");
                if (oy + 1 >= 0 && oy + 1 < max_h) mvaddstr(oy + 1, ox, " ▓██▀▀██▓ ");
                if (oy + 2 >= 0 && oy + 2 < max_h) mvaddstr(oy + 2, ox, " ▓██▄▄██▓ ");
                if (oy + 3 >= 0 && oy + 3 < max_h) mvaddstr(oy + 3, ox, "  ▀████▀  ");
            }
        } 
        else if (objects[i].state == STATE_SPLIT) {
            if (oy >= 0 && oy < max_h)         mvaddstr(oy,     ox, "████▀  ▀████");
            if (oy + 1 >= 0 && oy + 1 < max_h) mvaddstr(oy + 1, ox, "▓▓█▀ /  ▀█▓▓");
            if (oy + 2 >= 0 && oy + 2 < max_h) mvaddstr(oy + 2, ox, "▓▓█▄    ▄█▓▓");
            if (oy + 3 >= 0 && oy + 3 < max_h) mvaddstr(oy + 3, ox, "████▄  ▄████");
        } 
        else if (objects[i].state == STATE_SPLIT_BANANA) {
            int offset = 6 - objects[i].timer; 
            for (int r = 0; r < 4; r++) {
                if (oy + r >= 0 && oy + r < max_h) {
                    mvaddstr(oy + r, ox - offset, banana_left[r]);
                    mvaddstr(oy + r, ox + offset, banana_right[r]);
                }
            }
        }
        else if (objects[i].state == STATE_ROBOT) {
            for (int r = 0; r < 4; r++) if (oy + r >= 0 && oy + r < max_h) mvaddstr(oy + r, ox, robot_frames[objects[i].frame][r]);
        } 
        else if (objects[i].state == STATE_EXPLODING) {
            attroff(COLOR_PAIR(objects[i].color_pair)); attron(COLOR_PAIR(CP_EXPLOSION));
            if (oy >= 0 && oy < max_h)         mvaddstr(oy,     ox - 2, "  💥▒▒▒▒💥  ");
            if (oy + 1 >= 0 && oy + 1 < max_h) mvaddstr(oy + 1, ox - 2, " ▓████████▓ ");
            if (oy + 2 >= 0 && oy + 2 < max_h) mvaddstr(oy + 2, ox - 2, " ▓████████▓ ");
            if (oy + 3 >= 0 && oy + 3 < max_h) mvaddstr(oy + 3, ox - 2, "  💥▒▒▒▒💥  ");
            attroff(COLOR_PAIR(CP_EXPLOSION)); attron(COLOR_PAIR(objects[i].color_pair));
        }
        attroff(COLOR_PAIR(objects[i].color_pair));
    }
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime > 0) {
            int px = (int)particles[i].x, py = (int)particles[i].y;
            if (px >= 0 && px < max_w && py >= 0 && py < max_h) {
                attron(COLOR_PAIR(particles[i].color_pair)); mvaddch(py, px, particles[i].ch); attroff(COLOR_PAIR(particles[i].color_pair));
            }
        }
    }
    refresh();
}

void render_menu() {
    int max_h, max_w; getmaxyx(stdscr, max_h, max_w); erase(); box(stdscr, 0, 0);
    mvprintw(max_h / 2 - 3, max_w / 2 - 14, "=== NCURSES FRUIT NINJA ===");
    mvprintw(max_h / 2 - 1, max_w / 2 - 25, "Hover mouse to move line. Left-Click to slice!");
    mvprintw(max_h / 2 + 1, max_w / 2 - 15, "Press SPACEBAR to start playing");
    mvprintw(max_h / 2 + 3, max_w / 2 - 9, "Press 'q' to Quit");
    refresh();
}

void render_gameover() {
    int max_h, max_w; getmaxyx(stdscr, max_h, max_w); erase(); box(stdscr, 0, 0);
    attron(COLOR_PAIR(CP_HEART) | A_BOLD); mvprintw(max_h / 2 - 2, max_w / 2 - 7, "☠ GAME OVER ☠"); attroff(COLOR_PAIR(CP_HEART) | A_BOLD);
    mvprintw(max_h / 2, max_w / 2 - 10, "Final Score: %d", score);
    mvprintw(max_h / 2 + 2, max_w / 2 - 13, "Press SPACEBAR to retry");
    refresh();
}

int main() {
    srand(time(NULL));
    initscr(); cbreak(); noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    
    // Force absolute tracking (tracks hover coordinates and clicks)
    printf("\033[?1003h"); 
    fflush(stdout);
    
    init_color_pairs();
    
    for (int i = 0; i < MAX_OBJECTS; i++) objects[i].state = STATE_DEAD;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].lifetime = 0;
    
    int max_h, max_w; getmaxyx(stdscr, max_h, max_w);
    cursor_y = max_h / 2; 
    
    while (1) {
        getmaxyx(stdscr, max_h, max_w);
        int ch;
        
        while ((ch = getch()) != ERR) {
            if (ch == 'q' || ch == 'Q') {
                printf("\033[?1003l"); fflush(stdout);
                endwin(); return 0;
            }
            
            if (ch == '\033') {
                int n1 = getch();
                int n2 = getch();
                if (n1 == '[' && n2 == 'M') {
                    int b = getch() - 32; // Extract the button event mask
                    getch();              // Skip X coordinate tracking
                    int mouse_y = getch() - 32 - 1; // Extract Y coordinate tracking
                    
                    // Always update the line's position instantly on hover
                    if (mouse_y > 2 && mouse_y < max_h - 1) {
                        cursor_y = mouse_y;
                    }
                    
                    // Only trigger slices if Left Click is active (b == 0 or b == 32)
                    if (game_state == PLAYING && (b == 0 || b == 32)) {
                        trigger_slice(cursor_y); 
                    }
                }
            }
            
            if (ch == ' ' || ch == '\n') {
                if (game_state == MENU) {
                    score = 0; hearts = 3; frame_count = 0; flash_hearts_timer = 0; damage_cooldown = 0;
                    for (int i = 0; i < MAX_OBJECTS; i++) objects[i].state = STATE_DEAD;
                    game_state = PLAYING;
                } else if (game_state == GAME_OVER) {
                    game_state = MENU;
                }
            }
        }
        
        if (game_state == PLAYING) { update_game(); render_game(); } 
        else if (game_state == MENU) { render_menu(); } 
        else if (game_state == GAME_OVER) { render_gameover(); }
        
        usleep(20000); // 50 FPS for smoother tracking response
    }
    
    printf("\033[?1003l"); fflush(stdout);
    endwin(); return 0;
}
