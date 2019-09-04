#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdbool.h>

void *getProcesses(void *param);
int PIDfd[2];
int needPIDfd[2];
int allPIDs[100] = {0};

int main(int argc, char *argv[]){
  int listenfd, connfd;
  socklen_t addrlen;
  struct sockaddr_in myaddr;
  char buffer[2048] = {0};
  char history[2048][10] = {"","","","","","","","","",""};
  pthread_t PIDthread;
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
  /*pipe so that the parent can send the current jobs to child*/
  pipe(PIDfd);
  pipe(needPIDfd);
  /*create thread that will write current processes to pipe upon a request
    from a child process*/
  pthread_create(&PIDthread,NULL,getProcesses,NULL);
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
    int child_pid = fork();
    if(child_pid==0){
      close(PIDfd[1]);
      while(1){
	/*read the user input from the socket*/
	if(read(connfd,buffer,2048)<0){
	  perror("read fail");
	  exit(0);
	}
	/*calls on function to reap all zombie processes*/
	write(needPIDfd[1],"zombie",sizeof("zombie"));
	/*if user types "quit" then exit this process*/
	if(strcmp(buffer,"quit\n")==0){
	  write(needPIDfd[1],"quit",sizeof("quit"));
	  close(listenfd);
	  close(connfd);
	  exit(0);
	}
	/*decrypt the message that was received by the server*/
	int i;
	for(i=0; i<2048; i++){
	  if(buffer[i]=='\n'){
	    break;
	  }
	  buffer[i] = buffer[i]-8;
	}
	/*get rid of new line from stdin so that the command is read right*/
	if(buffer[strlen(buffer)-1]=='\n'){
	  buffer[strlen(buffer)-1] = '\0';
	}
	/*translate !N command into saved command*/
	if(buffer[0]=='!'){
	  switch(buffer[1]){
            case '!':
              strcpy(buffer,history[0]);
              break;
            case '1':
              if(buffer[2]=='0'){
                strcpy(buffer,history[9]);
              }
              else{
                strcpy(buffer,history[0]);
              }
              break;
            case '2':
              strcpy(buffer,history[1]);
              break;
            case '3':
              strcpy(buffer,history[2]);
              break;
            case '4':
              strcpy(buffer,history[3]);
              break;
            case '5':
              strcpy(buffer,history[4]);
              break;
            case '6':
              strcpy(buffer,history[5]);
              break;
            case '7':
              strcpy(buffer,history[6]);
              break;
            case '8':
              strcpy(buffer,history[7]);
              break;
            case '9':
              strcpy(buffer,history[8]);
              break;
            default:
              strcpy(buffer,history[0]);
              break;
	  }
	}
	/*update history by moving each input back 1 slot and adding
	  the newst command to the front*/
	int j;
	for(j=8; j>=0; j--){
	  strcpy(history[j+1],history[j]);
	}
	strcpy(history[0],buffer);
	/*begin parsing command*/
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
	  /*This block first checks if the input is equal to jobs and executes
	    jobs if it is, the if the input is history then it should execute 
	    history, else it should execute execvp executes the command from 
	    server*/
	  if(strcmp(allArgs[0],"jobs")==0){
	    int needToPrint[100] = {0};
	    write(needPIDfd[1],"jobs",sizeof("jobs"));
	    read(PIDfd[0],needToPrint,sizeof(needToPrint)+1);
	    int p;
	    for(p=0; p<100; p++){
	      if(needToPrint[p]>0){
		printf("%d\n",needToPrint[p]);
	      }
	    }
	  }
	  else if(strcmp(allArgs[0],"History")==0||strcmp(allArgs[0],"history")==0){
	    int k;
	    for(k=0; k<10; k++){
	      if(strcmp(history[k],"")!=0){
		printf("%d: %s\n",k+1,history[k]);
	      }
	    }
	  }
	  else{
	    if(execvp(allArgs[0],allArgs)<0){
	      perror("execvp failed");
	      exit(0);
	    }
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
    else{
      /*if this is the parent thenkeep track new child process pid for jobs*/
      int m;
      for(m=0; m<100; m++){
	if(allPIDs[m]<=0){
	  allPIDs[m]=child_pid;
	  break;
	}
      }
    }
    close(connfd);
  }
  return 0;
}

void *getProcesses(void *param){
  /*This thread will continually check if a child process is requesting jobs 
    and if it is then it will send back the list of pids, it will also reap 
    reap the zombies if a child process signals it to*/
  while(1){ 
    char buffer[2048] = "";
    read(needPIDfd[0],buffer,sizeof(buffer));
    if(strcmp(buffer,"zombie")==0){
      /*iterate through eachchild process, check if it is a zombie by accessing
       its stat file, then waiting for it to reap the zombie*/
      int z;
      for(z=0; z<100; z++){
	if(allPIDs[z]>0){
	  bool isZombie = false;
	  char buf[32];
	  snprintf(buf, sizeof(buf), "/proc/%d/stat",allPIDs[z]);
	  FILE *statFile = fopen(buf, "r");
	  int rpid=0;
	  char buf2[32];
	  char rstat = 0;
	  fscanf(statFile, "%d %30s %c", &rpid, buf2, &rstat);
	  isZombie = (rstat=='Z');
	  if(isZombie){
	    waitpid(allPIDs[z],NULL,0);
	    allPIDs[z]=0;
	  }
	}
      }
    }
    if(strcmp(buffer,"jobs")==0){
      write(PIDfd[1],allPIDs,sizeof(allPIDs));
    }
  }
  pthread_exit(NULL);
}
