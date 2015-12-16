#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

void *handleClient(void *sockfd);
void *handleReadWrite(void *sockFd);
void *handleShutDown(void *sockFd);

/** 
To print the error corresponding to the error code
Input : Error Msg
**/ 
void error(const char* msg){
	perror(msg);
	exit(0);
}

/** 
To parse any buffer till the end of the line(/r/n)
Input : buffer containing the whole text , variable where the line will be copied to
**/
char *readLine(char *buffer,char *line){
	int index = 0;
	if(strlen(buffer)==0) return NULL;
	while(1){
		// search for the end of line(\r\n)
		if(index >=1 && buffer[index-1]=='\r' && buffer[index]=='\n')break;
		line[index] = buffer[index];
		index++;
	}
	// change the \r char(last char) to \0
	line[index-1]='\0';
	// return address of char after \n
	return &buffer[index+1];
}

/** 
To create http response message
Input : buffer containing contents of the file
**/

char *createResponceMsg(char *contents){
	int headerSize = 1000;
	long contentSize = strlen(contents);
	long totalSize = contentSize + headerSize;
	char *buffer = (char*)malloc(sizeof(char)*totalSize);
	bzero(buffer,totalSize);
	snprintf(buffer, totalSize,"HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s\r\n",strlen(contents),contents);
	//printf("buffer = %s",buffer);
	return buffer;
	
}

/** 
To create http file not found response message
**/
char *createFileNotFoundMsg(){
	char *buffer = (char*)malloc(sizeof(char)*2000);
	bzero(buffer,2000);
	snprintf(buffer, 2000,"HTTP/1.1 404 Not Found\r\nContent-Length: 15\r\n\r\nFile Not Found\r\n");
	return buffer;
}

/** 
To create http bad response message
**/
char *createBadRequestMsg(){
	char *buffer = (char*)malloc(sizeof(char)*2000);
	bzero(buffer,2000);
	snprintf(buffer,2000,"HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\r\n");
}

/** 
To identify the connection type
Input : http get message
**/
int getConnectionType(char *getMsg){
	char *tempMsg = getMsg;
	char line[100];
	while(1){
	bzero(line,100);
	tempMsg = readLine(tempMsg,line);
	if(tempMsg==NULL)
		break;
	else{	// iterate till connection is found in the line
		if(strstr(line,"Connection: ")!=NULL){
			strtok(line," ");
			char *closeConnection = (char *)malloc(sizeof(char)*50);
			bzero(closeConnection,50);
			strcpy(closeConnection,strtok(NULL," : "));
			if(strcmp(closeConnection,"close")==0)
				return 0;
			else
				return 1;
			free(closeConnection);
			}			
		}
	}

}

/** 
To identify the connection type
Input : http get message
**/
char * getFileName(char *getMsg){
	char *tempMsg;
	tempMsg = getMsg;
	char line[1000];
	char *fileName = (char *)(sizeof(char)*50);
	while(strlen(tempMsg)>0){
		tempMsg = readLine(tempMsg,line);
		if(tempMsg==NULL)
			break;
		else{
			if(strstr(line,"GET ")!=NULL){
				strtok(line," ");
				char *fileName = (char *)malloc(sizeof(char)*50);
				bzero(fileName,50);
				strcpy(fileName,strtok(NULL," "));
				return fileName+1;
				break;
			}
		}	
	}	
	return NULL;
}


/** 
To identify the file size
Input : http get message
**/
int getFileSize(char *fileName){
	FILE *fp;
	long size;
	fp = fopen(fileName,"rb");
	if(fp!=NULL){
		// seek till end of file
		fseek(fp,0L,SEEK_END);
		size = ftell(fp);
		fclose(fp);
	}
	return size;
}

/** 
To extract contents of the file
Input : file name
**/
char *getFileContents(char *fileName)	
{
	FILE *fp;
	long size = getFileSize(fileName);
	long length = size>1000?size:1000;
	char *line = (char*)malloc(sizeof(char)*1000);
	char *buffer = (char*)malloc(sizeof(char)*length);
	bzero(buffer,length);
	bzero(line,1000);	
	fp = fopen(fileName,"r");	
	if(fp!=NULL){
		int num = 0;
		while((line = fgets(line,1000,fp))!=NULL){
			strncat(buffer,line,strlen(line));
			num++;
			bzero(line,1000);
		}
		fclose(fp);
	}else
		{return NULL;}
	free(line);
	return buffer;
}


/** 
To prepare for the responce message
Input : http get msg
**/
char *responceMsg(char *getMsg){
	char *fileName = getFileName(getMsg);
	// check the file name
	if(fileName!=NULL && strlen(fileName)!=0){
		char *contents = getFileContents(fileName);
		free(fileName-1);
		char *respMsg;
		//check if the file exits or not
		if(contents!=NULL){
			respMsg =  createResponceMsg(contents);	
		}else{
			respMsg = createFileNotFoundMsg();
		}
		return respMsg;
	}else{
		char *respMsg  = createBadRequestMsg();
		return respMsg;
	}
	return NULL;
}


void main(int argc,char *argv[]){
	int sockFd,newSockFd,portNum;
	pthread_t serverThread,threadShutdown;
	int th1,thS;
	struct sockaddr_in server_addr,cli_addr;
	socklen_t clilen;
	if(argc<2){
		error("No port number provided");
	}

	sockFd = socket(AF_INET,SOCK_STREAM,0);
	if(sockFd<0){error("Error Opening Socket");}

	bzero((char *)&server_addr,sizeof(server_addr));
	portNum = atoi(argv[1]);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port  = htons(portNum);

	if(bind(sockFd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
		{error("failed to bind");}
	listen(sockFd,5);
	clilen = sizeof(cli_addr);
	
	// thread to handle shutdown
	printf("Type \"shutdown\" to quit.....\n");
	thS = pthread_create(&threadShutdown,NULL,&handleShutDown,(void*)&sockFd);

	// thread to handle accepted connections
	while(1){
		newSockFd = accept(sockFd,(struct sockaddr *)&cli_addr,&clilen);
		if(newSockFd<0){error("Connection not Established\n");}
		th1 = pthread_create(&serverThread,NULL,&handleClient,(void*)&newSockFd);
	}
}


/** 
Handle shudown
Input : socket ID
**/
void *handleShutDown(void *sockFd){
	int newSockFd = *((int *)sockFd);
	char *buffer = (char * )malloc(sizeof(char)*9);
	while(1){
		// get input from stdin
		fgets(buffer,9,stdin);
		if(strncmp(buffer,"shutdown",9)==0){
			close(newSockFd);
			exit(0);
		}
	}
}

/** 
Handle read write for every client in a thread
Input : socket ID
**/
void *handleReadWrite(void *sockFd){
	char *buffer = (char*)malloc(sizeof(char)*1500);
	int n;
	fd_set read_fd;
	int newSockFd,connectionType;
	newSockFd = *((int*)sockFd);
	if(newSockFd<0){error("Connection failed");}
	bzero(buffer,1500);
	FD_ZERO(&read_fd);
	FD_SET(newSockFd,&read_fd);
	struct timeval timeout;
	// time out value is 10 seconds. 
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	do{
		n = read(newSockFd,buffer,1500);
		connectionType = getConnectionType(buffer);
		if(n<0){error("Reading failed");}
		char *respMsg  = responceMsg(buffer);
		if(respMsg!=NULL){
			n = write(newSockFd,respMsg,strlen(respMsg));	
			if(n<0){error("writing failed");}
			char status[3];
			n = read(newSockFd,status,3);
		}
		free(respMsg);
		bzero(buffer,1500);
	}while(connectionType && select(newSockFd+1,&read_fd,NULL,NULL,&timeout)!=0); // select statement to read or time out
	free(buffer);
	close(newSockFd);
}

/** 
Handle every client in a thread
Input : socket ID
**/

void *handleClient(void *sockFd){
	pthread_t readWriteThread;
	int th1, newSockFd;
	newSockFd = *((int*)sockFd);
	handleReadWrite((void*)&newSockFd);
}
