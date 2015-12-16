#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

/** 
To print the error corresponding to the error code
Input : Error Msg
**/ 
void error(const char *msg){
	perror(msg);
	exit(0);
}

/** 
To create the get message based on the file and connectiontype
Input : File name , Connection type(Persistent or non-persistent)
**/ 
char * prepareGetMsg(char *fileName){
	char *buffer = malloc(sizeof(char)* 256);
	bzero(buffer,256);
	strcat(buffer, "GET /");
	strcat(buffer, (fileName==NULL || strlen(fileName)==0)?"":fileName);
	strcat(buffer, " HTTP/1.0\r\nConnection : ");
	strcat(buffer, "close");
	strcat(buffer, "\r\n\r\n");	
	return buffer;
}

/** 
To parse any buffer till the end of the line(/r/n)
Input : buffer containing the whole text , variable where the line will be copied to
**/ 
char *readLine(char *buffer,char *line){
	int index = 0;
	if(strlen(buffer)==0) return NULL;
	while(1){
		if(index >=1 && buffer[index-1]=='\r' && buffer[index]=='\n')break;
		line[index] = buffer[index];
		index++;
	}
	line[index-1]='\0';
	return &buffer[index+1];
}


/** 
To print the body of the http responce
Input : buffer containing the whole text
**/ 
void printFile(char *buffer){
	char line[100];
	while(1){
		buffer = readLine(buffer,line);
		if(buffer==NULL)
			break;
		else{
			if(strstr(line,"Content-Length:"))
				break;
		}	
	}	
	if(buffer!=NULL)
		printf("%s\n",buffer);
}

/** 
To send http get request and receive the http response text
Input : file name,socked Descriptor,connection type, server sockaddr
**/ 
void udpRequest(char *fileName,int sockFd,struct sockaddr_in serverAddr){
	int n;
	fd_set recvFrm;
	char * getMsg = prepareGetMsg(fileName);
	socklen_t srvlen = sizeof(serverAddr);
	char *buffer = (char*)malloc(sizeof(char)*2097152);
	char *tempBuffer = (char*)malloc(sizeof(char)*1000);
	n = sendto(sockFd,getMsg,strlen(getMsg),0,(struct sockaddr*)&serverAddr,srvlen);
	if(n<0){error("writing failed");}
	bzero(buffer,2097152);
	bzero(tempBuffer,1000);
	FD_ZERO(&recvFrm);
	FD_SET(sockFd,&recvFrm);
	struct timeval timeOut;
	timeOut.tv_sec = 5;
	timeOut.tv_usec = 0;
	//receive the first time to block till the first bytes are read before 		reaching the select timeout. This will avoid the program form elapsing 		to timeout before file is received when huge files are sent. 
	n = recvfrom(sockFd,tempBuffer,1000,0,(struct sockaddr*)&serverAddr,&srvlen);
	if(n<0){error("reading Failed");}
	strncat(buffer,tempBuffer,1000);
	// Collect all the packets and group into a single buffer. 
	while(select(sockFd+1,&recvFrm,NULL,NULL,&timeOut)){
		bzero(tempBuffer,1000);
		n = recvfrom(sockFd,tempBuffer,1000,0,(struct sockaddr*)&serverAddr,&srvlen);
		if(n<0){error("reading failed\n");}
		strncat(buffer,tempBuffer,1000);
	}
	long sizeOfBuffer = strlen(buffer);
	printFile(buffer);
	printf("Number of bytes received=%ld\n",sizeOfBuffer);
	free(tempBuffer);
	free(buffer);
}



void main(int argc,char *argv[]){

	struct sockaddr_in server_addr;
	struct hostent *server;
	int sockFd,portNum,n,connType;
	if(argc<4){
		printf("Enter host name , port number and file name ");
		exit(0);
	}
	sockFd = socket(AF_INET,SOCK_DGRAM,0);
	if(sockFd<0) { error("Error creating socket");}
	portNum = atoi(argv[2]);
	server = gethostbyname(argv[1]);
	if(server==NULL){error("No such hosts");}
	bzero((char *)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portNum);
	bcopy((char *)server->h_addr,(char*)&server_addr.sin_addr.s_addr,server->h_length);
	udpRequest(argv[3],sockFd,server_addr);
	//close(sockFd);
}


