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
	int headerSize = 70;
	long contentSize = strlen(contents);
	long totalSize = contentSize + headerSize;
	char *buffer = (char*)malloc(sizeof(char)*totalSize);
	bzero(buffer,totalSize);
	snprintf(buffer, totalSize, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n%s\r\n", strlen(contents),contents);	
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

char * getFileName(char *getMsg){
	char *tempMsg;
	tempMsg = getMsg;
	char line[100];
	char *fileName = (char *)(sizeof(char)*50);
	while(1){
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
		fseek(fp,0L,SEEK_END);
		size = ftell(fp);
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
	if(fileName!=NULL && strlen(fileName)!=0){
		char *contents = getFileContents(fileName);
		free(fileName-1);
		char *respMsg;
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
	int sockFd,portNum;
	pthread_t serverThread;
	int th1;
	struct sockaddr_in server_addr;

	if(argc<2){
		error("No port number provided");
	}
	sockFd = socket(AF_INET,SOCK_DGRAM,0);
	if(sockFd<0){error("Error Opening Socket");}
	bzero((char *)&server_addr,sizeof(server_addr));
	portNum = atoi(argv[1]);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port  = htons(portNum);
	if(bind(sockFd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){error("failed to bind");}
	listen(sockFd,5);
	while(1){
		th1 = pthread_create(&serverThread,NULL,&handleClient,(void*)&sockFd);
		pthread_join( serverThread, NULL);
	}
	close(sockFd);
}

/** 
Handle read write for every client in a thread
Input : socket ID
**/
void *handleReadWrite(void *sockFd){
	struct sockaddr_in cli_addr;
	socklen_t clilen;
	char * buffer = (char*)malloc(sizeof(char)*256);
	int n;
	time_t currentTime; 
	int newSockFd;
	clilen = sizeof(cli_addr);
	newSockFd = *((int*)sockFd);
	if(newSockFd<0){error("Connection failed");}
	bzero(buffer,256);
	n = recvfrom(newSockFd,buffer,256,0,(struct sockaddr*)&cli_addr,&clilen);	
	if(n<0){error("Reading failed");}
	char *respMsg = responceMsg(buffer);
	// Break buffer into small chunks and transmit into UDP sockets. 
	while(strlen(respMsg)>0){
		n = sendto(newSockFd,respMsg,1000,0,(struct sockaddr*)&cli_addr,clilen);
		respMsg = respMsg + 1000;
	}
	currentTime = time(NULL);
	char *timeString = ctime(&currentTime);
	printf("Last byte transmitted at %s", timeString);
	if(n<0){error("writing failed");}
}

/** 
Handle every client in a thread
Input : socket ID
**/
void *handleClient(void *sockFd){
	char buffer[256];
	pthread_t readWriteThread;
	int th1, newSockFd;
	newSockFd = *((int*)sockFd);
	handleReadWrite((void*)&newSockFd);
}
