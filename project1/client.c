#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[]){
  int sockfd;
  struct sockaddr_in server;
  char input1[2048] = "";
  char buffer[1] = "";
  int quit = 1;
  /*creation of the socket file descriptor*/
  sockfd = socket(AF_INET,SOCK_STREAM,0);
  if(sockfd<0){
    perror("socket error");
    exit(0);
  }
  /*Initializing the sockaddr struct for the server and setting the 
    address family, ip address, and ports. Since it is local host for now
    the IP address is just 127.0.0.1 and 4028 is my assigned port
  */
  server.sin_family = AF_INET;
  inet_pton(AF_INET,"127.0.0.1",&server.sin_addr);
  server.sin_port = htons(4028);
  /*Setting up a connection to the server*/
  if(connect(sockfd,(struct sockaddr *)&server,sizeof(server))<0){
    perror("connection error");
    exit(0);
  }
  /*Once connected, this while loop will loop until "quit" is entered,
    so that it will continually ask for commands*/
  while(quit){
    /*get stdin from user*/
    read(STDIN_FILENO, input1, 2048);
    /*exit loop when quit is entered*/
    if(strcmp(input1,"quit\n")==0){
      quit = 0;
    }
    /*write command to the socket to be sent to server*/
    write(sockfd,input1,strlen(input1));
    memset(input1,0,sizeof(input1));
    /*read the redirected stdout and stderr from server*/
    while(read(sockfd,buffer,1)>=0){
      printf("%s",buffer);
      if(strcmp(buffer,"\0")==0){
	break;
      }
    }
    printf("\n");
  }
  return 0;
}
