// Include necessary libraries
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "cJSON.h" // Include cJSON for JSON parsing

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "pthreadVC2.lib")

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define MAX_BUFFER 1024

// Thread for receiving server messages
void* receive_messages(void* socket_ptr) {
    SOCKET client_socket = *(SOCKET*)socket_ptr;
    char buffer[MAX_BUFFER];
    int bytes_received;

    while (1) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            printf("Connection lost or closed by server.\n");
            break;
        }

        buffer[bytes_received] = '\0';  // Null-terminate the received data
        printf("Raw JSON: %s\n", buffer);

        cJSON *json = cJSON_Parse(buffer);
        if (json == NULL) {
            printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
            continue;
        }
        else if (cJSON_GetObjectItem(json, "question")== NULL) {
            printf("Error: 'question' field not found in JSON.\n");
            cJSON_Delete(json);
            continue;
        }
        // Try to parse the received message as JSON
        cJSON *json1 = cJSON_Parse(buffer);

        // Check if the data is valid JSON
        if (json == NULL) {
            // If it's not JSON, treat it as plain text and print it
            printf("Received (not JSON): %s\n", buffer);
        } else {
            // If it's valid JSON, process the question
            cJSON *question_item = cJSON_GetObjectItem(json, "question");
            if (cJSON_IsString(question_item) && (question_item->valuestring != NULL)) {
                // Print the question
                printf("Server Question: %s\n", question_item->valuestring);
                
                // Ask for the user's answer
                printf("Your answer: ");
                char user_answer[MAX_BUFFER];
                if (fgets(user_answer, sizeof(user_answer), stdin) != NULL) {
                    // Remove the newline character if present
                    user_answer[strcspn(user_answer, "\n")] = 0;
                    
                    // Send the user's answer to the server
                    if (send(client_socket, user_answer, strlen(user_answer), 0) < 0) {
                        printf("Error sending answer to the server.\n");
                        break;
                    }
                }
            } else {
                printf("Error: 'question' field not found or is not a string.\n");
            }

            // Clean up the JSON object
            cJSON_Delete(json);
        }
    }

    return NULL;
}



// Thread for sending messages
void* send_messages(void* socket_ptr) {
    SOCKET client_socket = *(SOCKET*)socket_ptr;
    char buffer[MAX_BUFFER];

    while (1) {
        // Clear buffer
        memset(buffer, 0, sizeof(buffer));

        // Get user input
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        // Remove newline character
        buffer[strcspn(buffer, "\n")] = 0;

        // Send input to server
        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            break;
        }
    }

    // Shutdown sending part of the socket when done
    shutdown(client_socket, SD_SEND);
    closesocket(client_socket);

    return NULL;
}

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    SOCKET client_socket;
    struct sockaddr_in server_addr;
    pthread_t receive_thread, send_thread;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address/ Address not supported\n");
        closesocket(client_socket);
        WSACleanup();
        exit(1);
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Connection failed: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        exit(1);
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, PORT);

    // Create threads for sending and receiving messages
    if (pthread_create(&receive_thread, NULL, receive_messages, &client_socket) != 0) {
        fprintf(stderr, "Receive thread creation failed\n");
        closesocket(client_socket);
        WSACleanup();
        exit(1);
    }

    if (pthread_create(&send_thread, NULL, send_messages, &client_socket) != 0) {
        fprintf(stderr, "Send thread creation failed\n");
        closesocket(client_socket);
        WSACleanup();
        exit(1);
    }

    // Wait for threads to complete
    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);

    // Close socket
    closesocket(client_socket);
    WSACleanup();

    return 0;
}
