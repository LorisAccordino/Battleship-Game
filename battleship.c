#include "battleship.h"

extern SOCKET client;

// Definizione della variabile ships
Ship ships[NUM_OF_SHIPS] = {
    {6, 0, 4, HORIZONTAL, 0},
    {1, 1, 1, HORIZONTAL, 0},
    {3, 2, 2, HORIZONTAL, 0},
    {8, 2, 1, HORIZONTAL, 0},
    {6, 3, 3, VERTICAL, 0},
    {4, 4, 2, VERTICAL, 0},
    {8, 4, 1, HORIZONTAL, 0},
    {1, 5, 2, VERTICAL, 0},
    {3, 8, 3, HORIZONTAL, 0},
    {9, 8, 1, HORIZONTAL, 0},
};

// Formatta una nave in una stringa che la descrive
char* ship_to_string(Ship ship) {
    char* format;
    int res = asprintf(&format, "{%d,%d,%d,%d}", ship.length, ship.orientation, ship.x, ship.y);
    return res >= 0 ? format : NULL;
}

// Ottiene una nave dalla stringa formattata che la descrive
Ship string_to_ship(char* format) {
    Ship ship;
    char orientation;
    if (sscanf(format, "{%d,%d,%d,%d}", &ship.length, &ship.orientation, &ship.x, &ship.y) != 4) {
        // Se sscanf non riesce a leggere i valori correttamente, imposta valori di default
        ship.length = 0;
        ship.orientation = 0;
        ship.x = 0;
        ship.y = 0;
    }
    return ship;
}

// Funzione per serializzare un array di navi in un array di stringhe serializzate
char* serialize_ships(Ship ships[], int num_of_ships) {
    char* serialized_ships = NULL;
    size_t total_length = 0;
    char delimiter = ';';

    for (int i = 0; i < num_of_ships; i++) {
        char* ship_str = ship_to_string(ships[i]);
        if (ship_str != NULL) {
            size_t ship_str_len = strlen(ship_str);
            serialized_ships = realloc(serialized_ships, total_length + ship_str_len + 2); // +1 per il delimitatore, +1 per '\0'
            if (serialized_ships == NULL) {
                // Gestione errore allocazione memoria
                return NULL;
            }
            strcpy(serialized_ships + total_length, ship_str);
            total_length += ship_str_len;
            serialized_ships[total_length] = delimiter; // Aggiungi delimitatore
            total_length++;
            serialized_ships[total_length] = '\0'; // Aggiungi terminatore di stringa
            free(ship_str);
        }
    }

    return serialized_ships;
}

// Funzione per deserializzare un array serializzato in un array di navi
Ship* deserialize_ships(const char* str, int num_of_ships) {
    // Alloca memoria per l'array di navi
    Ship* ships = (Ship*)malloc(num_of_ships * sizeof(Ship));
    if (!ships) {
        return NULL;
    }

    // Deserializza ogni nave
    const char* start = str;
    for (int i = 0; i < num_of_ships; i++) {
        // Trova l'inizio e la fine della stringa di una singola nave
        start = strchr(start, '{');
        const char* end = strchr(start, '}');
        if (!start || !end) {
            free(ships);
            return NULL;
        }

        // Copia la sottostringa della nave
        int length = end - start + 1;
        char* ship_str = (char*)malloc(length + 1);
        if (!ship_str) {
            free(ships);
            return NULL;
        }
        strncpy(ship_str, start, length);
        ship_str[length] = '\0';

        // Deserializza la singola nave
        ships[i] = string_to_ship(ship_str);
        free(ship_str);

        // Avanza il puntatore
        start = end + 1;
    }

    return ships;
}

// Funzione per inizializzare/resettare il tabellone
void reset_board(char board[BOARD_SIZE][BOARD_SIZE]) {
    reset_char_array(board, 0, BOARD_SIZE, BOARD_SIZE);
}

void print_board(Point print_xy, char board[BOARD_SIZE][BOARD_SIZE]) {
    int x, y;

    // Stampa l'intestazione con le etichette delle colonne (numeri)
    mvprintw(print_xy.y, print_xy.x, "R/C");
    for (y = 0; y < BOARD_SIZE; y++) {
        mvprintw(print_xy.y, print_xy.x + 4 + y * 3, "%d", y + 1); // Stampa l'etichetta della colonna (numero)
        mvprintw(print_xy.y + y + 1, print_xy.x + 1, "%c", 'A' + y); // Stampa l'etichetta della riga (lettera)

        // Stampa i valori della griglia
        for (x = 0; x < BOARD_SIZE; x++) mvprintw(print_xy.y + y + 1, print_xy.x + 4 + x * 3, "%c", board[y][x]);
    }

    // Aggiorna lo schermo
    refresh();
}

// Edit mode per navigare tra le navi, selezionarne una e modificarla
void edit_mode() {
    char* text[] = {
        " - Scorri tra le navi con le frecce (<- / ->)",
        " - Modifica la nave selezionata con [ENTER]",
        " - Termina di modificare le navi con [ESC]"
    };
    print_multi_line_text(HINTS_XY, text, 3);


    Point pos = {generate_random(0, BOARD_SIZE - 1), generate_random(0, BOARD_SIZE - 1)};

    Ship* ship;
    Point previous_pos = {BOARD_SIZE, BOARD_SIZE}; // Per garantire che sia diverso da pos all'ingresso nel while

    char key;
    while (key != ESC) {
        // Se l'elemento selezionato è cambiato
        if (previous_pos.x != pos.x || previous_pos.y != pos.y) {
            select_cell(BOARD_XY, previous_pos, 0); // Deseleziona l'elemento precedente
            select_cell(BOARD_XY, pos, 1); // Seleziona il nuovo elemento
        }

        // Aggiorna l'ultima posizione
        previous_pos = pos;

        // Prendi il tasto in input
        key = getch();

        // Aggiorna l'indice in base al tasto premuto
        switch (key) {
            case ARROW_LEFT:
                pos.x--;
                break;
            case ARROW_RIGHT:
                pos.x++;
                break;
            case ARROW_DOWN:
                pos.y++;
                break;
            case ARROW_UP:
                pos.y--;
                break;
            case ENTER:
                ship = find_ship_in_point(pos, ships);
                if (ship == NULL) continue; // Vai all'iterazione successiva se la nave selezionata è nulla

                clear_text_area(HINTS_XY, text, 3); // Ripulisci la parte di schermo con il testo residuo
                edit_ship(ship); // Modifica la nave selezionata
                select_cell(BOARD_XY, pos, 1); // Riseleziona la cella di prima
                print_multi_line_text(HINTS_XY, text, 3); // Stampa nuovamento il testo e vai all'iterazione successiva
                continue; // Vai alla iterazione successiva
            default:
                continue; // Vai alla iterazione successiva
        }

        // Limita ai confini
        pos.x = CLAMP(pos.x, 0, BOARD_SIZE - 1);
        pos.y = CLAMP(pos.y, 0, BOARD_SIZE - 1);
    }

    clear_text_area(HINTS_XY, text, 3); // Ripulisci la parte di schermo con il testo residuo
    select_cell(BOARD_XY, pos, 0); // Deseleziona la nave alla fine
}

// Funzione per modificare (spostare, ruotare) una nave del tabellone
void edit_ship(Ship* ship) {
    // Stmpa l'help a lato
    char* text[] = {
        " - Sposta la nave con i tasti freccia",
        " - Ruota la nave con [R]",
        " - Esci dalla edit mode con [ESC]",
    };
    print_multi_line_text(HINTS_XY, text, 3);

    Ship new_ship;

    char key;
    while (key != ESC) {
        // Rimuovi la nave, per poi successivamente validarne il posizionamento
        select_ship(BOARD_XY, *ship, 0);

        // Se la nave è in una posizione valida, allora spostala
        if (is_valid_place(local_board, EMPTY_MARK, *ship, new_ship)) {
            place_ship(local_board, EMPTY_MARK, *ship); // Rimuovi la nave precedente (usando gli EMPTY_MARK invece di SHIP_MARK)
            place_ship(local_board, SHIP_MARK, new_ship); // Posiziona la nuova nave aggiornata
            print_board(BOARD_XY, local_board); // Aggiorna la visualizzazione del tabellone
            *ship = new_ship; // Aggiorna la nave
        }

        // Aggiorna la visualizzazione della nave
        select_ship(BOARD_XY, *ship, 1);

        // Crea una copia temporanea della nave
        new_ship = *ship;

        // Prendi il tasto in input
        key = getch();

        // Modifica la copia temporanea della nave
        switch (key) {
            case ARROW_LEFT:
                new_ship.x--;
                break;
            case ARROW_RIGHT:
                new_ship.x++;
                break;
            case ARROW_DOWN:
                new_ship.y++;
                break;
            case ARROW_UP:
                new_ship.y--;
                break;
            case 'r':
            case 'R':
                new_ship.orientation = new_ship.orientation == HORIZONTAL ? VERTICAL : HORIZONTAL; // Inverti l'orientamento
                break;
            default:
                break;
        }
    }

    clear_text_area(HINTS_XY, text, 3); // Ripulisci la parte di schermo con il testo residuo
    select_ship(BOARD_XY, *ship, 0); // Deseleziona la nave
}

// Modalità in cui si colpisce la nave avversaria
void hit_mode(char dummy_board[BOARD_SIZE][BOARD_SIZE], Point* hit_point) {
    // Stmpa l'help a lato
    char* text[] = {
        " - Mira il colpo con i tasti freccia",
        " - Colpisci con [ENTER]",
    };
    //print_multi_line_text(HINTS_XY, text, 2);

    Point previous_point = {BOARD_SIZE, BOARD_SIZE};
    Point hit_p = *hit_point;
    int valid = 1;

    char key;
    while (1) {
        // Limita il punto ai confini
        hit_p.x = CLAMP(hit_p.x, 0, BOARD_SIZE - 1);
        hit_p.y = CLAMP(hit_p.y, 0, BOARD_SIZE - 1);

        // Se il punto selezionato è cambiato
        if (previous_point.x != hit_p.x || previous_point.y != hit_p.y) {
            select_cell(OPPONENT_BOARD_XY, previous_point, 0); // Deseleziona la cella precedente
            select_cell(OPPONENT_BOARD_XY, hit_p, 1); // Seleziona la nuova cella

            char* msg = point_to_string(hit_p);
            send_message(client, msg, strlen(msg)); // Invia il colpo all'avversario
        }

        // Aggiorna il punto precedente
        previous_point = hit_p;

        // Prendi il tasto in input
        key = getch();

        // Se è già stato colpito, allora non è valido
        valid = dummy_board[hit_p.y][hit_p.x] != MISSED_MARK && dummy_board[hit_p.y][hit_p.x] != HIT_MARK;

        // Modifica la copia temporanea della nave
        switch (key) {
            case ARROW_LEFT:
                hit_p.x--;
                break;
            case ARROW_RIGHT:
                hit_p.x++;
                break;
            case ARROW_DOWN:
                hit_p.y++;
                break;
            case ARROW_UP:
                hit_p.y--;
                break;
            case ENTER:
                if (!valid) continue; // Se non è valido, vai alla iterazione successiva

                //clear_text_area(HINTS_XY, text, 3); // Ripulisci la parte di schermo con il testo residuo
                select_cell(OPPONENT_BOARD_XY, hit_p, 0); // Deseleziona la cella selezionata

                char* msg = HIT_DONE_STATUS;
                send_message(client, msg, strlen(msg)); // Invia il colpo all'avversario

                *hit_point = hit_p;
                return;
            default:
                continue; // Vai alla iterazione successiva
        }
    }
}

// Funzione per colpire l'avversario, aggiornando il tabellone "dummy" (tabellone locale o quello avversario senza mostrare le navi)
HitType hit_board(Point hit_point, char dummy_board[BOARD_SIZE][BOARD_SIZE], Ship ships[NUM_OF_SHIPS]) {
    int i;

    int hit = 0, sunk = 0;
    for(i = 0; i < NUM_OF_SHIPS; i++) {
        if (ship_contains_point(ships[i], hit_point)) {
            ships[i].hits++;
            hit = 1;
            if (ships[i].hits == ships[i].length) sunk = 1;
            break;
        }
    }

    if (hit) {
        dummy_board[hit_point.y][hit_point.x] = HIT_MARK;
    }
    else {
        dummy_board[hit_point.y][hit_point.x] = MISSED_MARK;
    }

    return !hit ? MISSED : (sunk ? HIT_N_SUNK : HIT);
}

// Funzione per selezionare una cella
void select_cell(Point print_xy, Point cell, int highlight) {
    // Sposta il cursore nella cella da selezionare
    Point point = {print_xy.x + 4 + cell.x * 3, print_xy.y + cell.y + 1};
    highlight_point(point, highlight);
}

// Funzione per selezionare/deselezionare una nave dal tabellone
void select_ship(Point print_xy, Ship ship, int highlight) {
    int x, y;

    print_xy.x += 4;
    print_xy.y += 1;

    Point start_point = (Point){print_xy.x + ship.x * 3, print_xy.y + ship.y};
    Point end_point = ship.orientation == HORIZONTAL ? (Point){ship.length * 3 - 3, 0} : (Point){0, ship.length - 1};
    end_point.x += start_point.x;
    end_point.y += start_point.y;

    highlight_rect_area(start_point, end_point, highlight);
}

// Funzione per validare una nave in una posizione del tabellone
int is_valid_place(char board[BOARD_SIZE][BOARD_SIZE], char empty_mark, Ship previous_ship, Ship ship) {
    Point end_point = ship.orientation == HORIZONTAL ? (Point){ship.length, 1} : (Point){1, ship.length};

    // Controlla se la nave si trova all'interno del tabellone
    if (ship.x < 0 || ship.x > BOARD_SIZE - end_point.x ||
        ship.y < 0 || ship.y > BOARD_SIZE - end_point.y) return 0;

    // Controlla se la nave si sovrappone ad altre navi
    int x, y;
    for (x = ship.x - 1; x <= ship.x + end_point.x; x++) {
        for (y = ship.y - 1; y <= ship.y + end_point.y; y++) {
            // Salta il controllo se le coordinate sono fuori dai confini
            if (x != CLAMP(x, 0, BOARD_SIZE - 1) || y != CLAMP(y, 0, BOARD_SIZE - 1)) continue;

            // Controlla se la cella è occupata o meno
            if (board[y][x] != empty_mark) {
                // Se non è occupata dalla nave precedente
                if (!ship_contains_point(previous_ship, (Point){x, y})) return 0;
            }
        }
    }

    return 1; // La posizione è valida
}

// Funzione che trova una nave, se c'è, che corrisponde a quel punto
Ship* find_ship_in_point(Point point, Ship ships[NUM_OF_SHIPS]) {
    int i;

    for (i = 0; i < NUM_OF_SHIPS; i++) {
        if (ship_contains_point(ships[i], point)) {
            return &ships[i];
        }
    }
    return NULL;
}

// Funzione per controllare se un punto è contenuto nei "bounds" della nave
int ship_contains_point(Ship ship, Point point) {
    if (ship.orientation == HORIZONTAL) {
        // Controlla se p1 è all'interno della nave orizzontale
        return (point.y == ship.y && point.x >= ship.x && point.x < ship.x + ship.length);
    } else {
        // Controlla se p1 è all'interno della nave verticale
        return (point.x == ship.x && point.y >= ship.y && point.y < ship.y + ship.length);
    }
}

// Funzione per posizionare una nave in una determinata posizione
void place_ship(char board[BOARD_SIZE][BOARD_SIZE], char ship_mark, Ship ship) {
    int x, y;

    Point end_point = ship.orientation == HORIZONTAL ? (Point){ship.length, 1} : (Point){1, ship.length};
    for (x = 0; x < end_point.x; x++) {
        for (y = 0; y < end_point.y; y++) {
            board[ship.y + y][ship.x + x] = ship_mark;
        }
    }
}

// Funzione per posizionare più navi nel tabellone
void place_ships(char board[BOARD_SIZE][BOARD_SIZE], char ship_mark, Ship ship[], int num_of_items) {
    int i;

    for (i = 0; i < num_of_items; i++) {
        place_ship(board, SHIP_MARK, ship[i]);
    }
}
