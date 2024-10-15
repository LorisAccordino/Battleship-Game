#include "battleship.h"

SOCKET client;
SOCKET server;
int host_state = -1;

int turn = 0;

void start_game() {
    Point print_xy = {0, PREP_MENU_XY.y + 3};
    if (!are_you_sure(print_xy, "Sei pronto ad iniziare?", "Sì, iniziamo!", "No, aspetta!")) return;

    print_single_line_text(print_xy, "In attesa che l'avversario sia pronto..."); // Info per l'utente

    // Invia il messaggio per notificare lo stato ready
    char *ready_status = READY_STATUS;
    send_message(client, ready_status, strlen(ready_status));

    // Ricevi l'acknowledge
    int res = 0;
    char recvbuf[BUFLEN];
    while (res <= 0) {
        res = receive_message(client, recvbuf, BUFLEN);
    }

    // Verifica che sia andato tutto a buon fine
    if (strcmp(recvbuf, ready_status) != 0) {
        print_xy.y++;
        print_single_line_text(print_xy, "Errore critico durante la connessione!");
        print_xy.y++;
        any_key_to_continue(print_xy, "- Premi qualsiasi tasto per continuare -");
        exit(-1);
    }

    clear_row_range(PREP_MENU_XY.y, print_xy.y); // Ripulisci lo schermo

    // Stabilisci a chi tocca il primo turno casualmente
    char buf[1];
    if (host_state) {
        turn = generate_random(0, 1);
        buf[0] = 'A' + turn;
        send_message(client, buf, 1);
    }
    else {
        res = 0;
        while (res <= 0) {
            res = receive_message(client, buf, 1);
        }
        turn = buf[0] - 'A';
        turn = !turn;
    }


    // Invia il contenuto del tabellone locale all'avversario (terminale remoto)

    char* serialized_ships = serialize_ships(ships, NUM_OF_SHIPS); // Serializza le navi per inviarle in rete come stream
    send_message(client, serialized_ships, strlen(serialized_ships)); // Invia il tabellone locale

    res = 0;
    while (res <= 0) {
        res = receive_message(client, recvbuf, BUFLEN); // Ricevi il tabellone avversario
    }
    strncpy(serialized_ships, recvbuf, strlen(serialized_ships)); // Copia dal buffer alla matrice del tabellone
    opponent_ships = deserialize_ships(serialized_ships, NUM_OF_SHIPS);

    place_ships(opponent_board, SHIP_MARK, opponent_ships, NUM_OF_SHIPS); // Piazza le navi avversarie nel tabellone avversario


    // Stampa i tabelloni
    print_single_line_text((Point){BOARD_XY.x, BOARD_XY.y - 2}, "TABELLONE LOCALE:");
    print_board(BOARD_XY, local_board);
    print_single_line_text((Point){OPPONENT_BOARD_XY.x, OPPONENT_BOARD_XY.y - 2}, "TABELLONE AVVERSARIO:");
    print_board(OPPONENT_BOARD_XY, opponent_dummy_board);


    // Entra nel loop principale del gioco
    int win = main_loop();

    print_xy.y = PREP_MENU_XY.y + 1;
    const char* prompt = win ? "Hai vinto!" : "Hai perso!";
    info(print_xy, prompt, "Ok");

    // Resetta e stampa nuovamente i tabelloni
    prepare_board();
}

int main_loop() {
    Point print_xy = {2, PREP_MENU_XY.y};

    int res;
    char recvbuf[BUFLEN];

    HitType hit = -1;
    int sunk_opponent_ships = 0;
    int sunk_local_ships = 0;
    const char* hit_text;

    while(1) {
        // Informa l'utente del turno cambiato
        clear_row(print_xy.y);
        print_single_line_text(print_xy, "Turno: %s", turn ? "Tocca a te!" : "Tocca all'avversario...");

        // Assegna in base al turno
        Point board_xy = turn ? OPPONENT_BOARD_XY : BOARD_XY;
        char** board = turn ? opponent_dummy_board : local_board;
        Ship* turn_ships = turn ? opponent_ships : ships;
        int* sunk_ships = turn ? &sunk_opponent_ships : &sunk_local_ships;

        // Genera un punto casuale
        Point hit_point = {
            generate_random(0, BOARD_SIZE - 1),
            generate_random(0, BOARD_SIZE - 1)
        };

        do {
            if (turn) { // Colpisci l'avversario
                hit_mode(opponent_dummy_board, &hit_point);
                char* msg = point_to_string(hit_point);
                send_message(client, msg, strlen(msg)); // Invia il colpo all'avversario
            }
            else { // Aggiorna il colpo ricevuto in locale
                // Aggiorna in tempo reale la mira del colpo che si sposta
                res = 0;
                Point previous_point = {BOARD_SIZE, BOARD_SIZE};
                while (strcmp(recvbuf, HIT_DONE_STATUS) != 0) { // Aspetta che il colpo sia stato effettuato
                    res = receive_message(client, recvbuf, BUFLEN);
                    hit_point = string_to_point(recvbuf);

                    select_cell(board_xy, previous_point, 0);
                    select_cell(board_xy, hit_point, 1);
                    //refresh();

                    // Aggiorna il punto
                    previous_point = hit_point;
                }

                // Aggiorna il colpo effettuato
                res = 0;
                while (res <= 0) {
                    res = receive_message(client, recvbuf, BUFLEN); // Ricevi il colpo avversario
                }
                hit_point = string_to_point(recvbuf);
            }

            // Ottieni il risultato del colpo e aggiorna il tabellone
            hit = hit_board(hit_point, board, turn_ships);
            print_board(board_xy, board);

            // Incrementa il numero di navi affondate
            if (hit == HIT_N_SUNK) (*sunk_ships)++;

            // Gioco finito
            if (sunk_local_ships >= NUM_OF_SHIPS || sunk_opponent_ships >= NUM_OF_SHIPS) {
                clear_row_range(print_xy.y, print_xy.y + 1);
                return sunk_opponent_ships >= NUM_OF_SHIPS;
            }

            // Informa l'utente dell'ultimo colpo
            hit_text = hit == MISSED ? "Mancato!" : (hit == HIT_N_SUNK ? "Colpito e AFFONDATO!" : (hit == HIT ? "Colpito!" : "-"));
            clear_row(print_xy.y + 1);
            print_single_line_text((Point){print_xy.x, print_xy.y + 1}, "Ultimo tiro: %d%c, %s", hit_point.x + 1, 'A' + hit_point.y, hit_text);
        } while (hit != MISSED); // Permani, se inizia a colpire l'avversario, fino a quando non manca un colpo

        // Inverti il turno
        if (hit == MISSED) turn = !turn;
    }
}

void prepare_board() {
    // Ripulisci la zona dello schermo che sarà occupata dal tabellone
    clear_row_range(START_MENU_XY.y, PREP_MENU_XY.y);

    reset_boards();

    // Stampa il tabellone
    print_single_line_text((Point){BOARD_XY.x, BOARD_XY.y - 2}, "TABELLONE LOCALE:");
    print_board(BOARD_XY, local_board);
}

void preparation() {
    prepare_board(); // "Prepara" il tabellone

    // Menu di gioco
    MenuItem game_menu_items[] = {
        {"Posiziona le tue navi", edit_mode},
        {"Inizia la partita", start_game},
    };

    choice_menu(PREP_MENU_XY, game_menu_items, ARRAY_SIZE(game_menu_items), 1, 1);
}

void start_session(int is_host) {
    char* ip_address = NULL;
    int port;

    // Riabilita il WinSock solo se è fallito qualcosa
    init_winsock();

    Point print_xy = USER_INPUT_XY;

    // Abilita il cursore e l'eco
    curs_set(1);
    echo();

    if (is_host) {
        int err = 0;
        do {
            port = ask_int(print_xy, "Inserisci la porta su cui vuoi ospitare la partita", &err);
            if (err == ERR || port < PORT_MIN || port > PORT_MAX) info(print_xy, "Inserire un numero intero valido e compreso tra 1024 e 65536", "Ok");
        } while (err == ERR || port < PORT_MIN || port > PORT_MAX);

        // Inizializza il server
        server = init_server(port);

        // Connessione fallita
        if (server == NULL) {
            info(print_xy, "Errore durante la connessione!", "Ok");
            return;
        }

        // Stampa a schermo le opportune informazioni
        print_single_line_text(print_xy, "La tua partita e' stata creata all'indirizzo %s, sulla porta: %d", get_local_IP_address(), port);
        print_xy.y++;
        print_single_line_text(print_xy, "In attesa dell'avversario...");
        print_xy.y += 2;

        // Disabilita il cursore e l'echo
        curs_set(0);
        noecho();

        // Attendi la connessione del client
        client = handle_client(server, &ip_address);
        if (ip_address == NULL) {
            info(print_xy, "Errore durante la connessione!", "Ok");
            return;
        }
    }
    else {
        ip_address = ask_string(print_xy, "Inserisci l'indirizzo IP dell'amico a cui vuoi unirti", 15);
        port = ask_int(print_xy, "Inserisci la porta a cui vuoi connetterti", NULL);

        // Disabilita il cusore e l'echo
        curs_set(0);
        noecho();

        // Connetti il client al server
        int res = connect_client(ip_address, port, &client);
        if (res != 0) {
            info(print_xy, "Errore durante la connessione!", "Ok");
            return;
        }
    }

    // Informa l'utente della connessione riuscita
    print_single_line_text(print_xy, "Connesso con successo con %s, sulla porta %d\n", ip_address, port);
    print_xy.y++;
    info(print_xy, "Tutto pronto! Procediamo?", "Ok");

    // Ripulisci tutto
    clear_row_range(USER_INPUT_XY.y, print_xy.y);

    // Preparazione delle navi per la partita
    preparation();
}

// Inizia una sessione di gioco, come host
void start_host() {
    host_state = 1;
    set_title("Battleship - Host");
    start_session(1);
}

// Inizia una sessione di gioco, come guest
void start_guest() {
    host_state = 0;
    set_title("Battleship - Guest");
    start_session(0);
}

// Esci dal gioco, chiedendo conferma
void exit_game() {
    if (are_you_sure(USER_INPUT_XY, "Stai per uscire dal gioco. Sei sicuro?", "Sì", "No")) {
        exit(0);
    }
}

// Resetta i vari tabelloni
void reset_boards() {
    reset_char_array(local_board, EMPTY_MARK, BOARD_SIZE, BOARD_SIZE);
    reset_char_array(opponent_board, EMPTY_MARK, BOARD_SIZE, BOARD_SIZE);
    reset_char_array(opponent_dummy_board, EMPTY_MARK, BOARD_SIZE, BOARD_SIZE);

    // Posiziona le navi (prese dal "template") nel tabellone locale
    place_ships(local_board, SHIP_MARK, ships, 10);
}

// Inizializzazioni varie del gioco
void init_game() {
    // Init vari
    init_ncurses(); // Inizializza ncurses
    set_window(85, 28, "Battleship"); // Imposta dimensione e titolo della finestra
    init_random(); // Inizializza il seed del generatore random

    prevent_window_resize(get_window_handle());

    // Inizializza il socket a priori
    int res = init_winsock();
    if (res) {
        cleanup_ncurses();
        printf("Avvio fallito! Problemi con il socket...");
        return -1;
    }
}



int main() {
    // Inizializza il gioco
    init_game();

    // Stampa l'ASCII art a schermo
    char *ascii_art = read_ascii_art_from_file("ascii_art.txt"); // Scritta in ASCII art
    int num_lines;
    char** text = split_string(ascii_art, &num_lines); // Dividi la scritta in linee
    print_multi_line_text(ASCII_ART_XY, text, num_lines);

    // Stampa il menu principale
    MenuItem main_menu_items[] = {
        {"Invita un amico", start_host},
        //{"Invita un amico", preparation},
        {"Unisciti ad un amico", start_guest},
        //{"Unisciti ad un amico", preparation},
        {"Esci", exit_game}
    };
    choice_menu(START_MENU_XY, main_menu_items, ARRAY_SIZE(main_menu_items), 0, 1);

    // Pulisci e termina ncurses
    cleanup_ncurses();

    return 0;
}
