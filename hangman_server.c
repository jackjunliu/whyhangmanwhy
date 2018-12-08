// CS176A - hangman_server.c
// Jack Liu & Kindy Tan
// Partial code taken from www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/server.c

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h> //used for time
#include <ctype.h>

#define BUFFER_SIZE 128

char MAX_OVERLOAD = 16;
char ZERO = 0; //since when we send messages we need to access the address of it (line 191, 272)

//usage error
void usage() {
  printf("Usage: ./hangman_server port\n");
  exit(1);
}

//used to replace the _ with characters guessed
int replace_chars(char* currentString, char* word, char c) {
  int found = 0;
  for(int i = 0; i < strlen(word); i++){
    if(word[i] == c){
      currentString[i] = c;
      found = 1; 
    }

  }

  return found; 
}

int main(int argc, char** argv) {

  //declarations
  srand(time(0));
  int port;
  FILE* file;
  char* line = NULL;
  size_t length = 0;

  char* words[15];
  int wordCount = 0;
  int clientsWord[3]; //Max 3 clients 
  char* clientsString[3]; 
  char* clientsWrongGuess[3];

  //print how to use
  if (argc < 2) {
    usage();
  }

  //parse port for use
  port = atoi(argv[1]); 

  //negative port
   if (port <= 0) {
    usage();
   }

  //allocate memory
  for(int i = 0; i < 15; ++i) {
    words[i] = malloc(9);  
  }
  
  for(int i = 0; i < 3; ++i){
    clientsString[i] = malloc(9);
    clientsWrongGuess[i] = malloc(7);
  }
  //read the file 
  file = fopen("./hangman_words.txt", "r");
  if(file == NULL){
    perror("File reading error");
    exit(1);
  }

  //reads file to put into words array
  /**char fileBuffer[7];
  while(fgets(fileBuffer, 6, file)!= NULL){
    //printf(fileBuffer);
    for(int i = 1; i < 15; i++){
       words[i] = fileBuffer[i];
    }
  }

  for (int i = 0; i < 15; i++) {
    printf(words[i]);
    }**/
  
  //reads file and inputs into array of words
  while (getline(&line, &length, file) != -1) {
    line[strcspn(line, "\n")] = 0;
    strncpy(words[wordCount++], line, strlen(line) + 1);
  }
  fclose(file);
  

  //more declarations
  int sockfd, clientSockfd[3] = {0}, client_sockfd;
  int numberClients = 0;
  int messageLength, recvSize, found, maxSd, sockAct;
  char buffer[BUFFER_SIZE];
  fd_set readfds;
  char messageSize;
  
  memset(buffer, '\0', 128); //sets memory

  //create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Socket creation failed.");
    exit(1);
  }

  struct sockaddr_in server;
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(port);
  

  //open socket errors  
  if (setsockopt(sockfd,
		 SOL_SOCKET,
		 SO_REUSEADDR,
		 &(int){ 1 }, sizeof(int)) < 0) {
    perror("Failed to set reusable socket.");
    exit(1);
  }
  if (bind(sockfd, (const struct sockaddr *)& server, sizeof(server)) < 0) {
    perror("Failed to bind.");
    exit(1);
  }

  if (listen(sockfd, 3) < 0) {
    perror("Listening error.");
    exit(1);
  }
  
  int addressLength = sizeof(server);
  
  //begin broadcast
  while(1) {
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    maxSd = sockfd;

    for(int i = 0; i < 3; i++) {
      if(clientSockfd[i] > 0) {
        FD_SET(clientSockfd[i], &readfds); 
      }
      if(clientSockfd[i] > maxSd){
        maxSd = clientSockfd[i];
      }
    }
    sockAct = select(maxSd+1, &readfds, NULL, NULL, NULL);
    if((sockAct < 0)&&(errno != EINTR)){
      printf("select error\n");
    }
    if(FD_ISSET(sockfd, &readfds)){
      client_sockfd = accept(sockfd, (struct sockaddr *)&server, (socklen_t*)&addressLength);
      if(client_sockfd < 0){
        perror("accept failure");
        exit(0); 
      }
      if(numberClients < 3){
        for(int i = 0; i < 3; i++){
        	//if no client, and accessed
        	if(clientSockfd[i] == 0){
        	  clientSockfd[i] = client_sockfd;
        	  int randomValue = rand() % wordCount;
        	  int wordLength = strlen(words[randomValue]);
        	  clientsWord[i] = randomValue;
            memset(clientsWrongGuess[i], '\0', sizeof(char) * 7); //seven wrong guesses
        	  memset(clientsString[i], '_', sizeof(char) * wordLength);
        	  memset(clientsString[i] + wordLength, '\0', sizeof(char) * (9 - wordLength));
        	  numberClients++;

        	  send(client_sockfd, &ZERO, 1, 0);

        	  break;
        	}
        }
      }
      
      //more than 3 clients wanting to get in 
      else{
        send(client_sockfd, &MAX_OVERLOAD, 1,0);
        
        send(client_sockfd, "server-overloaded", MAX_OVERLOAD, 0);
      }
    }

    for (int i = 0; i < 3; i++) {
      if (FD_ISSET(clientSockfd[i], &readfds)) {
        recvSize = recv(clientSockfd[i], buffer, 1, 0);
        if (recvSize == 0) {
        	close(clientSockfd[i]);
        	clientSockfd[i] = 0;
          numberClients--;
        } else {

          messageLength = buffer[0]; //flag

          //no message, then quit
        	if (messageLength == 0) {
        	  break;
        	}

        	//get client socket and the message
        	recv(clientSockfd[i], buffer, messageLength, 0);
        	buffer[messageLength] = '\0';
        	found = 0;

        	//decide on what to do with data
        	if (strchr(clientsString[i], buffer[0]) == NULL) {
        	  found = replace_chars(clientsString[i], words[clientsWord[i]], buffer[0]);
        	}

        	if (found == 0) {
        	  for (int j = 0; j < 7; j++){
        	    if (clientsWrongGuess[i][j] == '\0') {
        	      clientsWrongGuess[i][j] = buffer[0];
        	      break;
        	    }
        	  }
        	}

        	//States depending on user guesses
        	if(strlen(clientsWrongGuess[i])>5){
        	  messageSize = 20;

        	  send(clientSockfd[i], &messageSize, 1, 0);
        	  send(clientSockfd[i], "You lose.\n", 10, 0);
        	  send(clientSockfd[i], "Game over!", 10, 0);

        	  close(clientSockfd[i]);

        	  clientSockfd[i] = 0;
        	  numberClients--; 
        	}
        	else if(strcmp(words[clientsWord[i]], clientsString[i]) == 0){
        	  int messageSize = 13 + strlen(clientsString[i]) + 1 + 20;
        	  send(clientSockfd[i], &messageSize, 1, 0);
        	  send(clientSockfd[i], "The word was ", 13, 0);
        	  send(clientSockfd[i], clientsString[i], strlen(clientsString[i]), 0); 
        	  send(clientSockfd[i], "\n", 1,0);
        	  send(clientSockfd[i], "You win!\n", 9,0);
        	  send(clientSockfd[i], "Game over!", 10, 0);

        	  close(clientSockfd[i]);

        	  clientSockfd[i] = 0;
        	  numberClients--; 
        	}
        	else {
        	  int wordLen = strlen(clientsString[i]);
        	  int numWrongGuesses = strlen(clientsWrongGuess[i]);

        	  send(clientSockfd[i], &ZERO, 1, 0);
        	  send(clientSockfd[i], &wordLen, 1, 0);
        	  send(clientSockfd[i], &numWrongGuesses, 1, 0);
        	  send(clientSockfd[i], clientsString[i], wordLen, 0);
        	  send(clientSockfd[i], clientsWrongGuess[i], numWrongGuesses, 0); 
        	       
        	}
        }
      }
    }
  }
}
  

