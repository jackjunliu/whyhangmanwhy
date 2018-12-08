// CS176A - hangman_client.c
// Jack Liu & Kindy Tan
// Partial code taken from www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/client.c

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <ctype.h> // for isAlpha

#define BUFFER_SIZE 128

void usage() {
  printf("Usage: ./hangman_client ip port\n");
  exit(1);
}

int main(int argc, char** argv) {

  //declarations
  char* host; //ip
  int port;
  int recv_size, recv_val, word_len, num_incorrect;
  char buffer[BUFFER_SIZE];
  
  //print how to use
  if (argc < 3) {
    usage();
  }

  //parse host and port for use
  host = argv[1];
  port = atoi(argv[2]);

  //negative port
  if (port <= 0) {
    usage();
  }

  //create socket
  int sockfd;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Socket creation failed.");
    exit(EXIT_FAILURE);
  }

  //test to see if it reaches here
  printf("test");
  
  struct sockaddr_in server;
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = inet_addr(host);

  //connect the socket
  if (connect(sockfd, (struct sockaddr*) &server, sizeof(server)) < 0) {
    perror("Failed to connect to server.");
    exit(EXIT_FAILURE);
  }

  recv_size = recv(sockfd, (char *)& buffer, 1, 0);
  recv_val = buffer[0];

  //begin game
  printf("Ready to start game? (y/n): ");
  fflush(stdout);

  fgets(buffer, BUFFER_SIZE, stdin);
  if (buffer[0] != 'y') { //anything but y.., exit
    close(sockfd);
    exit(0);
  }

  //send empty message to the server to begin the game
  char signal = 0;
  send(sockfd, &signal, 1, MSG_NOSIGNAL);

  //TODO: Game variable length limiters

  //start game
  while(1) {
    printf("Letter to guess: ");
    fgets(buffer, BUFFER_SIZE, stdin);

    //if the string is one character, continue
    if (strlen(buffer) == 1 && isalpha(buffer[0])) {
      buffer[1] = tolower(buffer[0]); //turn to lowercase
      buffer[0] = 1; //string message flag

      //send message to server
      send(sockfd, buffer, 2, 0);
      //receive response
      recv(sockfd, buffer, 1, 0);
    }
  }
  
  //close sockets
  close(sockfd);
  return 0;
}
