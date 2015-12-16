#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>

int numberOfRead=0;
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
char * prepareGetMsg(char *fileName,int connType){
	char *buffer = malloc(sizeof(char)* 256);
	bzero(buffer,256);
	strcat(buffer, "GET /");
	strcat(buffer, (fileName==NULL || strlen(fileName)==0)?"":fileName);
	strcat(buffer, " HTTP/1.0\r\nConnection: ");
	// append string based on the type of connection
	strcat(buffer, connType==0?"close":"keep-alive");
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
Input : file name,socked Descriptor,connection type
**/ 
void httpRequest(char *fileName,int sockFd,int connType){
	int n;
	fd_set readFd;
	if(strlen(fileName)==0)return;
	char * getMsg = prepareGetMsg(fileName,connType);
	char *buffer = (char*)malloc(sizeof(char)*2097152);
	char *tempBuff = (char*)malloc(sizeof(char)*1000);
	n = write(sockFd,getMsg,256);
	free(getMsg);
	if(n<0){error("writing failed");}
	bzero(buffer,2097152);
	bzero(tempBuff,1000);
	FD_ZERO(&readFd);
	FD_SET(sockFd,&readFd);
	struct timeval timeOut;
	timeOut.tv_sec = 2;
	timeOut.tv_usec = 0;
	// read outside loop to block the code until first read is made
	n = read(sockFd,tempBuff,1000);
	if(n<0){error("readig failed");}
	strncat(buffer,tempBuff,1000);
	// select statement to time out if no content to read for 2sec
	while(select(sockFd+1,&readFd,NULL,NULL,&timeOut)){
		n = read(sockFd,tempBuff,1000);
		if(n<0){error("reading failed");}
		strncat(buffer,tempBuff,1000);
		bzero(tempBuff,1000);
	}
	numberOfRead++;
	n = write(sockFd,"OK",3);
	if(n<0){error("writing failed");}
	printFile(buffer);
	free(tempBuff);
	free(buffer);
}


/** 
To send http get request and receive the http response text for persistence conncection
Input : file name,socked Descriptor,connection type
**/ 
void httpPersistentRequest(char *fileName,int sockFd,int connType){
	FILE *fp = fopen(fileName,"r");
	char *line = (char*)malloc(sizeof(char)*100);
	bzero(line,100);
	if(fp!=NULL){
		while((line = fgets(line,20,fp))!=NULL){
			line[strlen(line)-1]='\0';
			printf("######## File : %s  ##########\n",line);
			if(strlen(line)!=0)
				httpRequest(line,sockFd,connType);
			printf("#####################\n");
		}
	}else{
		error(fileName);
	}
	free(line);
	
	fclose(fp);
}


void main(int argc,char *argv[]){

	struct sockaddr_in server_addr;
	struct hostent *server;
	int sockFd,portNum,n,connType;
	struct timeval start,end;
	gettimeofday(&start,NULL);
	if(argc<4){
		{printf("Enter host name , port number, Connection type and file name ");exit(0);}
	}
	// check for persistence and non persistence connection
	if(strcmp(argv[3],"p")==0){
		connType = 1;
	}else if(strcmp(argv[3],"np")==0){
		connType = 0;
	}else {
		error("Invalid Connection type"); 
	}

	// create a socket
	sockFd = socket(AF_INET,SOCK_STREAM,0);
	if(sockFd<0) { error("Error creating socket");}
	portNum = atoi(argv[2]);
	server = gethostbyname(argv[1]);
	if(server==NULL)
		{error("No such hosts");}
	bzero((char *)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portNum);
	bcopy((char *)server->h_addr,(char*)&server_addr.sin_addr.s_addr,server->h_length);
	
	if(connect(sockFd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
		{error("Error Connecting");}
	if(connType == 1){
		httpPersistentRequest(argv[4],sockFd,connType);
	}else{	
		httpRequest(argv[4],sockFd,connType);
	}
	gettimeofday(&end,NULL);
	// wait for server to close connection
	char check[2];
	while(read(sockFd,check,2)>0){}
	close(sockFd);
	long timeTaken = ((end.tv_sec*1000000 + end.tv_usec)-(start.tv_sec*1000000+start.tv_usec))/1000;
	printf("Time taken for transfer = %ld",timeTaken - (numberOfRead*2000));
}


