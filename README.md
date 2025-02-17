# TCP-Based Real-Time Quiz Game

## 📖 Overview

The **TCP-Based Real-Time Quiz Game** is a multiplayer quiz platform built on a TCP socket-based client-server architecture. This project enables multiple players to connect to a centralized server, answer quiz questions in real time, and compete for the highest score.

## 🚀 Features

- **Real-Time Multiplayer Gameplay**: Multiple clients can join the quiz session simultaneously.
- **Server-Client Communication**: The server handles quiz logic, distributes questions, and tracks scores.
- **Dynamic Questions**: The quiz questions are dynamically delivered to all connected clients.
- **Instant Feedback**: Players get immediate results after submitting their answers.
- **Leaderboard**: Scores are updated and displayed dynamically to show the ranking of players.

## 🛠️ Technologies Used

- **Programming Language**: C
- **Networking Protocol**: TCP/IP
- **Standard Libraries**:
  - `<stdio.h>` for input/output operations
  - `<string.h>` for string manipulation
  - `<pthread.h>` for handling multiple clients concurrently
  - `<winsock2.h>` for socket programming and `<ws2tcpip.h>` for for TCP/IP protocols

## 📂 Project Structure

```
.
├── img                    # Contains the screenshot of the steps to execute
    ├── image-1.png
    ├── image-2.png 
    ├── image-3.png
    ├── image-4.png
    ├── image.png
├── quiz_server.c          # Server-side code for managing the quiz
├── cJSON.h                # Contains the header files
├── quiz_client.c          # Client-side code for participating in the quiz
├── physics_question.json  # File containing quiz questions and answers
├── README.md              # Documentation
├── Steps_to_execute.md    # Steps for running the application
├── TCP-Multiplayer-Game-System.pptx # PPT for this application
```

## 🔧 Setup and Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/Revathi-15/OS-project-
   cd OS-project-
   ```

2. **Compile the Code**:
   Use a C compiler to build the server and client programs:
   - Compile the server:
     ```bash
     gcc quiz_server.c -o server
     ```
   - Compile the client:
     ```bash
     gcc quiz_client.c -o client
     ```

3. **Run the Server**:
   Start the server before any clients connect:
   ```bash
   ./server
   ```

4. **Run the Client**:
   Connect to the server by running the client program on any machine:
   ```bash
   ./client
   ```

## 🕹️ How to Play

1. **Start the Server**:
   The server initializes the quiz and waits for clients to connect.

2. **Connect Clients**:
   Clients can join the game by running the client executable and entering the server's IP address and port.

3. **Answer Questions**:
   - Players receive questions in real time.
   - Submit answers via the client interface.
   - Receive immediate feedback on correctness.

4. **View Leaderboard**:
   Scores are updated dynamically, and the server announces the final leaderboard after the quiz ends.

## 🛡️ Error Handling

- The server handles common errors such as:
  - Invalid inputs from clients
  - Disconnection of clients during the game
- The client retries the connection if the server is temporarily unavailable.


## 🤝 Contributing

Contributions are welcome! Feel free to fork the repository, make changes, and submit a pull request.

---

Enjoy the challenge of real-time quizzing with this TCP-based multiplayer game!
