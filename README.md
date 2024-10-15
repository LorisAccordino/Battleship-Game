# Battleship Game (C - TCP/IP Networking)

This repository contains an implementation of the classic *Battleship* game for two players, designed to be played over a LAN connection using TCP/IP. The game is text-based and rendered in ASCII, providing a simple and engaging multiplayer experience.

The game utilizes **TextGameKit**, a custom C library, which handles various aspects of the game, including networking, input/output, rendering, and other utility functions.

## Features
- Multiplayer gameplay over TCP/IP.
- Text-based interface using ASCII characters.
- Simple game mechanics: players place ships and guess coordinates.
- **TextGameKit:** library used for handling networking, input/output, rendering, and other essential functions.
- Written in C, designed primarily for educational purposes.

## Setup

1. **Clone the repository:**
   ```bash
   git clone https://github.com/LorisAccordino/Battleship-Game.git
   cd Battleship-Game
2. **Build the project:** Use your preferred C compiler or IDE (e.g., CodeBlocks) to compile the code.
If using CodeBlocks, open the `.cbp` project file and compile the project.

3. **Run the game:** The game consists of a server and client component. To start playing, follow these steps: 
   - **Server:** Run the server code to listen for incoming connections.
   - **Client:** Run the client code to connect to the server and start the game.
Both programs need to be running on separate machines within the same LAN.


## Dependencies
- **TextGameKit:** A custom C library that simplifies text-based game development. 
It manages networking (TCP/IP), input/output handling, text rendering, 
and provides other useful functions and algorithms for the game.

You can find the [TextGameKit repository here](https://github.com/LorisAccordino/TextGameKit).

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.