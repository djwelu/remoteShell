#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){
  int listenfd, connfd;
  socklen_t addrlen;
  struct sockaddr_in myaddr;
  char buffer[2048] = {0};
  /*create a new socket for the server*/
  listenfd = socket(AF_INET,SOCK_STREAM,0);
  if(listenfd<0){
    perror("error opening socket");
    exit(1);
  }
  /*initialize the sockaddr struct for the server, set the address family
    and the port that the server will listen on*/
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = INADDR_ANY;
  myaddr.sin_port = htons(4028);
  if(bind(listenfd, (struct sockaddr *)&myaddr, sizeof(struct sockaddr))<0){
    perror("bind error");
    exit(1);
  }
  /*listen for clients*/
  if(listen(listenfd,5)<0){
    perror("listening error");
    exit(1);
  }
  /*loop infinitely so that the server coninually recieves new clients*/
  for(;;){
    /*accept the next connection request*/
    addrlen = sizeof(myaddr);
    connfd = accept(listenfd,(struct sockaddr *)&myaddr,&addrlen);
    if(connfd<0){
      perror("accept error");
      exit(1);
    }
    /*create new process so that there can be multiple clients*/
    if(fork()==0){
      while(1){
	/*read the user input from the socket*/
	if(read(connfd,buffer,2048)<0){
	  perror("read fail");
	  exit(0);
	}
	/*if user types "quit" then exit this process*/
	if(strcmp(buffer,"quit\n")==0){
	  close(listenfd);
	  close(connfd);
	  exit(0);
	}
	/*get rid of new line from stdin so that the command is read right*/
	if(buffer[strlen(buffer)-1]=='\n'){
	  buffer[strlen(buffer)-1] = '\0';
	}
	char **allArgs = NULL;
	char *singleArg = strtok(buffer," ");
	int space = 0;
	/*reallocate for each new parsed word from strtok*/
	while(singleArg){
	  allArgs = realloc(allArgs,sizeof(char *) * ++space);
	  allArgs[space-1] = singleArg;
	  singleArg = strtok(NULL," ");
	}
	/*add extra argument so that it is null for execvp*/
	allArgs = realloc(allArgs, sizeof(char*) * (space+1));
	allArgs[space] = 0;
	/*make a new process for execvp to be executed*/
	if(fork()==0){
	  /*close stdout and stderr so that the server prints nothing*/
	  close(1);
	  close(2);
	  /*duplicate stdout and stderr to socket*/
	  dup2(connfd,STDOUT_FILENO);
	  dup2(connfd,STDERR_FILENO);
	  /*execvp executes the command from server*/
	  if(execvp(allArgs[0],allArgs)<0){
	    perror("execvp failed");
	    exit(0);
	  }
	  exit(0);
	}
	/*wait so that execvp finishes before continuing*/
	wait(NULL);
	/*signal that the stdout is done writing*/
	write(connfd,"\0",1);
	free(allArgs);
	memset(buffer,0,sizeof(buffer));
      }
    }
    close(connfd);
  }
  return 0;
}
