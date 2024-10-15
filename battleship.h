#ifndef BATTLESHIP_H_INCLUDED
#define BATTLESHIP_H_INCLUDED

#include "common.h"
#include "ui.h"
#include "math2D.h"
#include "networking.h"


// Definizioni varie

#define READY_STATUS "ready"
#define HIT_DONE_STATUS "hit_done"

#define BOARD_SIZE 10 // Dimensione tabellone
#define NUM_OF_SHIPS 10 // Quante navi avere nel tabellone

#define SHIP_MARK 'H'
#define EMPTY_MARK '~'
#define HIT_MARK 'X'
#define MISSED_MARK '-'

#define ASCII_ART_XY (Point){0, 0}
#define START_MENU_XY (Point){0, 7}
#define BOARD_XY (Point){1, 10}
#define OPPONENT_BOARD_XY (Point){38, 10}
#define PREP_MENU_XY (Point){BOARD_XY.x, BOARD_XY.y + 12}
#define HINTS_XY (Point){38, BOARD_XY.y + BOARD_SIZE / 2 - 1}
#define USER_INPUT_XY (Point){0, 11}



// Enumerazione per l'orientamento delle navi
typedef enum {
    HORIZONTAL = 0,
    VERTICAL = 1,
} Orientation;

typedef enum {
    MISSED,
    HIT,
    HIT_N_SUNK,
} HitType;

// Struttura per rappresentare una nave
typedef struct {
    int x;          // A prescindere dall'orientamento il punto (x, y)
    int y;          // sarà il punto più "in altro a sinistra" della nave
    int length;
    Orientation orientation;
    int hits;
} Ship;

// Array contentente le navi già piazzate, in modo da fornire un "template" iniziale
// e semplificare di molto la codifica in alcune fasi ed evitare diversi errori
Ship ships[NUM_OF_SHIPS]; // Dichiarazione della variabile ships

Ship* opponent_ships; // Navi dell'avversario

// Rappresentazione del tabellone di gioco
char local_board[BOARD_SIZE][BOARD_SIZE]; // Giocatore locale
char opponent_board[BOARD_SIZE][BOARD_SIZE]; // Giocatore remoto
char opponent_dummy_board[BOARD_SIZE][BOARD_SIZE]; // Giocatore remoto (visualizzato in locale, senza le navi)


// Dichiarazioni di funzioni

char* ship_to_string(Ship ship);
Ship string_to_ship(char* format);
char* serialize_ships(Ship ships[], int num_of_ships);
Ship* deserialize_ships(const char* str, int num_of_ships);

void reset_board(char board[BOARD_SIZE][BOARD_SIZE]);
void print_board(Point print_xy, char board[BOARD_SIZE][BOARD_SIZE]);
void edit_mode();
void edit_ship(Ship* ship);
void hit_mode(char dummy_board[BOARD_SIZE][BOARD_SIZE], Point* hit_point);
HitType hit_board(Point hit_point, char dummy_board[BOARD_SIZE][BOARD_SIZE], Ship ships[NUM_OF_SHIPS]);
void select_ship(Point print_xy, Ship ship, int highlight);
int is_valid_place(char board[BOARD_SIZE][BOARD_SIZE], char empty_mark, Ship previous_ship, Ship ship);
Ship* find_ship_in_point(Point point, Ship ships[NUM_OF_SHIPS]);
int ship_contains_point(Ship ship, Point p1);
void place_ship(char board[BOARD_SIZE][BOARD_SIZE], char ship_mark, Ship ship);
void place_ships(char board[BOARD_SIZE][BOARD_SIZE], char ship_mark, Ship ship[], int num_of_items);

#endif // BATTLESHIP_H_INCLUDED
