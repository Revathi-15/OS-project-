#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <process.h> // For _beginthread

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080 // Replace 8080 with your desired port number
#define MAX_PLAYERS 3 // Replace 10 with the maximum number of simultaneous connections
#define MAX_QUESTION_LENGTH 256
#define MAX_ANSWER_LENGTH 128

// Player structure
typedef enum {
    PRACTICE,
    COMPETE
} GameMode;

typedef enum {
    EASY,
    MEDIUM,
    HARD
} Difficulty;

typedef enum {
    PHYSICS,
    CHEMISTRY,
    BIOLOGY,
    GENERAL_KNOWLEDGE,
    MATHS
} Domain;

typedef struct {
    char question[MAX_QUESTION_LENGTH];
    char answer[MAX_ANSWER_LENGTH];
    int time_limit;  // in seconds
    Difficulty difficulty;
    Domain domain;
} Question;

typedef struct {
    int socket;
    int id;
    char name[50];
    GameMode mode;
    Domain domain;
    Difficulty difficulty;
    int score;
} Player;

Player players[MAX_PLAYERS];
int num_players = 0;
Question current_questions[MAX_PLAYERS];

// Windows critical section for player access
CRITICAL_SECTION players_critical_section;

void handle_client_connection(void* client_socket);
void send_menu_to_client(int client_socket);
void send_domain_menu(int client_socket);
void send_difficulty_menu(int client_socket);
void start_game(Player* player);
void load_questions(Domain domain, Difficulty difficulty, Question* questions);
void cleanup_client(Player* player);

// Handle client connection, receives choices and starts the game
void handle_client_connection(void* socket_ptr) {
    SOCKET client_socket = (SOCKET)(intptr_t)socket_ptr;
    Player new_player;

    // Send initial menu to client
    send_menu_to_client(client_socket);

    // Receive player's choice (game mode)
    char mode_choice[10];
    int bytes_received = recv(client_socket, mode_choice, sizeof(mode_choice), 0);
    if (bytes_received <= 0) {
        cleanup_client(&new_player);
        return;
    }
    
    // Select mode based on choice
    if (strcmp(mode_choice, "1") == 0) {
        new_player.mode = PRACTICE;
    } else if (strcmp(mode_choice, "2") == 0) {
        new_player.mode = COMPETE;
    } else {
        send(client_socket, "Invalid choice, closing connection.\n", 35, 0);
        cleanup_client(&new_player);
        return;
    }

    // Send domain menu
    send_domain_menu(client_socket);

    // Receive domain choice
    char domain_choice[10];
    bytes_received = recv(client_socket, domain_choice, sizeof(domain_choice), 0);
    if (bytes_received <= 0) {
        cleanup_client(&new_player);
        return;
    }
    new_player.domain = atoi(domain_choice);

    // Send difficulty menu
    send_difficulty_menu(client_socket);

    // Receive difficulty choice
    char difficulty_choice[10];
    bytes_received = recv(client_socket, difficulty_choice, sizeof(difficulty_choice), 0);
    if (bytes_received <= 0) {
        cleanup_client(&new_player);
        return;
    }
    new_player.difficulty = atoi(difficulty_choice);

    // Set up player details
    EnterCriticalSection(&players_critical_section);
    new_player.socket = client_socket;
    new_player.id = num_players;
    new_player.score = 0;
    players[num_players++] = new_player;
    LeaveCriticalSection(&players_critical_section);

    // Start the game for this player
    start_game(&new_player);

    // Cleanup client
    cleanup_client(&new_player);
}

void send_menu_to_client(int client_socket) {
    const char* menu = 
        "Welcome to the Quiz Game!\n"
        "Choose your mode:\n"
        "1. Practice Mode\n"
        "2. Compete Mode\n"
        "Enter your choice (1/2): ";
    send(client_socket, menu, strlen(menu), 0);
}

void send_domain_menu(int client_socket) {
    const char* menu = 
        "Choose your domain:\n"
        "0. Physics\n"
        "1. Chemistry\n"
        "2. Biology\n"
        "3. General Knowledge\n"
        "4. Maths\n"
        "Enter your choice (0-4): ";
    send(client_socket, menu, strlen(menu), 0);
}

void send_difficulty_menu(int client_socket) {
    const char* menu = 
        "Choose difficulty:\n"
        "0. Easy\n"
        "1. Medium\n"
        "2. Hard\n"
        "Enter your choice (0-2): ";
    send(client_socket, menu, strlen(menu), 0);
}

#include "cJSON.h" // Include cJSON header for JSON parsing

void load_questions(Domain domain, Difficulty difficulty, Question* questions) {
    const char* domain_files[] = {
        "physics_questions.json",    // PHYSICS
        "chemistry_questions.json",  // CHEMISTRY
        "biology_questions.json",    // BIOLOGY
        "general_knowledge_questions.json", // GENERAL_KNOWLEDGE
        "maths_questions.json"       // MATHS
    };

    // Open the correct JSON file for the selected domain
    FILE* file = fopen(domain_files[domain], "r");
    if (file == NULL) {
        perror("Error opening question file");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* json_data = (char*)malloc(file_size + 1);
    fread(json_data, 1, file_size, file);
    json_data[file_size] = '\0'; // Null terminate the string
    fclose(file);

    cJSON* root = cJSON_Parse(json_data); // Parse the JSON string into a cJSON structure
    free(json_data); // Free the raw JSON data buffer

    if (root == NULL) {
        fprintf(stderr, "Error parsing JSON data\n");
        return;
    }

    cJSON* questions_array = cJSON_GetObjectItemCaseSensitive(root, "questions");
    if (cJSON_IsArray(questions_array)) {
        int found_questions = 0;
        cJSON* question_item = NULL;
        cJSON_ArrayForEach(question_item, questions_array) {
            // Get the question details
            cJSON* question_text = cJSON_GetObjectItemCaseSensitive(question_item, "question");
            cJSON* answer_text = cJSON_GetObjectItemCaseSensitive(question_item, "answer");
            cJSON* question_difficulty = cJSON_GetObjectItemCaseSensitive(question_item, "difficulty");
            cJSON* time_limit = cJSON_GetObjectItemCaseSensitive(question_item, "time_limit");

            if (cJSON_IsString(question_text) && cJSON_IsString(answer_text) &&
                cJSON_IsNumber(question_difficulty) && cJSON_IsNumber(time_limit)) {
                // Only add question if difficulty matches
                if (difficulty == question_difficulty->valueint) {
                    strncpy(questions[found_questions].question, question_text->valuestring, MAX_QUESTION_LENGTH - 1);
                    strncpy(questions[found_questions].answer, answer_text->valuestring, MAX_ANSWER_LENGTH - 1);
                    questions[found_questions].difficulty = difficulty;
                    questions[found_questions].time_limit = time_limit->valueint;

                    found_questions++;
                    if (found_questions >= 10) break; // Limit to 10 questions
                }
            }
        }
        if (found_questions < 10) {
            fprintf(stderr, "Not enough questions found for domain %d and difficulty %d\n", domain, difficulty);
            exit(1);
        }
    }

    cJSON_Delete(root); // Clean up the cJSON structure
}



void start_game(Player* player) {
    Question questions[10];
    load_questions(player->domain, player->difficulty, questions); // Load questions based on difficulty

    char game_info[200];
    snprintf(game_info, sizeof(game_info), 
        "Starting %s Mode\n"
        "Domain: %d, Difficulty: %d\n"
        "10 questions incoming...\n", 
        player->mode == PRACTICE ? "Practice" : "Competition",
        player->domain, player->difficulty);
    send(player->socket, game_info, strlen(game_info), 0);

    // Loop through and ask the player 10 questions
    for (int i = 0; i < 10; i++) {
        // Send the current question to the player
        send(player->socket, questions[i].question, strlen(questions[i].question), 0);

        // Receive the player's answer
        char player_answer[MAX_ANSWER_LENGTH];
        int bytes_received = recv(player->socket, player_answer, sizeof(player_answer), 0);
        if (bytes_received > 0) {
            player_answer[bytes_received] = '\0';  // Null terminate answer string

            // Check if the player's answer is correct
            if (strcmp(player_answer, questions[i].answer) == 0) {
                player->score += 3;  // Correct answer
                send(player->socket, "Correct!\n", 9, 0);
            } else {
                player->score -= 2;  // Incorrect answer
                send(player->socket, "Incorrect!\n", 11, 0);
            }
        }

        // Send current score after each question
        char score_msg[100];
        snprintf(score_msg, sizeof(score_msg), "Current Score: %d\n", player->score);
        send(player->socket, score_msg, strlen(score_msg), 0);
    }

    // Send final score after all questions
    char final_score[100];
    snprintf(final_score, sizeof(final_score), "Final Score: %d\n", player->score);
    send(player->socket, final_score, strlen(final_score), 0);
}

// Cleanup client connection
void cleanup_client(Player* player) {
    // Close player socket and clear player info
    if (player->socket != INVALID_SOCKET) {
        closesocket(player->socket);
    }

    // Remove player from the list (if needed)
    EnterCriticalSection(&players_critical_section);
    for (int i = player->id; i < num_players - 1; i++) {
        players[i] = players[i + 1];
    }
    num_players--;
    LeaveCriticalSection(&players_critical_section);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, MAX_PLAYERS) == SOCKET_ERROR) {
        fprintf(stderr, "Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Initialize critical section
    InitializeCriticalSection(&players_critical_section);

    printf("Server started on port %d\n", PORT);
    while (1) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            fprintf(stderr, "Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        // Start a new thread to handle the client connection
        _beginthread((void(*)(void*))handle_client_connection, 0, (void*)(intptr_t)client_socket);
    }

    // Cleanup
    DeleteCriticalSection(&players_critical_section);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
