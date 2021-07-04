//Berkay Durur 170418026 Sistem Programlama dersi dönem projesi C tabanlı multithread Web Server.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
//#include <kuyruk.h> //Kuyruk ve sözlük daha sonra dahil edilecek.
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include "sozluk.h"
#define CLIENT_NUM 1
#define RCV_BUF_SIZE 5000
//Fonksiyonlar

//Değişkenler

const char *indexHTMLpackage;

const char *HTTP_404_CONTENT = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1>The requested resource could not be found but may be available again in the future.<div style=\"color: #eeeeee; font-size: 8pt;\">Actually, it probably won't ever be available unless this is showing up because of a bug in your program. :(</div></html>";
const char *HTTP_501_CONTENT = "<html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1>The server either does not recognise the request method, or it lacks the ability to fulfill the request.</body></html>";

const char *HTTP_200_STRING = "OK";
const char *HTTP_404_STRING = "Not Found";
const char *HTTP_501_STRING = "Not Implemented";

long imageLength;

char * http_baslik_istegi (char * istek); //HTTP başlığının istek satırını işler.
void * clientConn(void * clientSocket);
char * readFile(char *filePath);
void sigHandler(int sigNo);

// Socket'i oluşturduğumuz ve uygun portu açtığımız ve thread'leri başlattığımız kısım.
int main (int argc, char **argv){
		
		int portNo = atoi(argv[1]);
		int *socketHolder , clientNum = 0, threadCount = 0, clientSocket[100];
		int socketFd = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in serverAddr;
		int addrLength = sizeof(serverAddr);
		pthread_t acceptThread[100];
		
		if(socketFd < 0){
			perror("Socket acilirken bir sorun olustu");
			exit(EXIT_FAILURE);
		}
		
		bzero((char *) &serverAddr, sizeof(serverAddr));
		
		if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &(int){1}, sizeof(int))){
    			perror("Set Socket Option'da bir sorun olustu");
    			exit(EXIT_FAILURE);
    		}

    		serverAddr.sin_family = AF_INET;
    		serverAddr.sin_port = htons(portNo);
    		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    		
    		if((bind(socketFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr))) < 0){
    			perror("Bind sirasinda bir sorun oluştu.");
    			exit(EXIT_FAILURE);
    		}
    		
    		while(threadCount < 10){
    		
    		if(signal(SIGQUIT, sigHandler) == SIG_ERR){
    			printf("Başarıyla Çıkış Yapıldı");
    		}
    		
    		if(listen(socketFd, 10) < 0){
    			perror("Server dinleme moduna giremedi.");
    			exit(EXIT_FAILURE);
    		}
    				
		clientSocket[threadCount] = accept(socketFd, (struct sockaddr *) &serverAddr, (socklen_t*)&addrLength);	
		
		if(clientSocket[threadCount] < 0){
			perror("Accept hatasi");
			exit(EXIT_FAILURE);
		}
	
		
		if(pthread_create(&acceptThread[threadCount], NULL, clientConn, &clientSocket[threadCount]) < 0){
			perror("Thread baslatilamadi.");
			exit(EXIT_FAILURE);
		}
		
		pthread_join(acceptThread[threadCount], NULL);
		threadCount++;
    		}
		
    		close(socketFd);
    		return 0;
 }
 
//Sigquit yakalandığında çıkış yapılır.
void sigHandler(int sigNo){
	
	exit(0);

}

//Siteye bağlanıldığında her bir bağlantı bir thread ile bu fonksiyonu çalıştırır. Bu fonksiyon client'ten gelen request'i yakalar bunu parse eder ve buna göre
//yeni bir response oluşturup gönderir.
void * clientConn(void * clientSocket){
 
 	int *connFd = (int *)clientSocket;
 	int recvIndex, fileCheck, cnt = 0, fileExtFlag = 0, statusCode, memLength;;
 	char recvBuffer[2048], sendBuffer[2048], *tokenLine, *httpLines[50], *httpResult, *contentType, *fileExtension, *fileBuffer, filePath[50] = "web/", httpPackage[1024], contentTypeHeader[48], contentLengthHeader[48], connectionHeader[48], imageBuffer[20800];
 	long numbytes;
 	FILE *fd;
 	recvIndex = (recv (*connFd, recvBuffer, sizeof(recvBuffer), 0));
 	if(recvIndex < 0){
 		perror("Veri receive edilemedi");
 		exit(EXIT_FAILURE);
 	}
 	
 	tokenLine = strtok(recvBuffer, "\r\n");
 	httpLines[cnt] = tokenLine;
 	
 	while(tokenLine != NULL){

 		tokenLine = strtok(NULL, "\r\n");
 		cnt++;
 		httpLines[cnt] = tokenLine;
 	}
	 
	httpResult = http_baslik_istegi(httpLines[0]);
	
	if(httpResult == NULL){
	
		statusCode = 501;
		contentType = "text/html";
		
		sprintf(httpPackage, "HTTP/1.1 %d %s\r\n", statusCode, HTTP_501_STRING);
		
		sprintf(contentTypeHeader, "Content-Type: %s\r\n", contentType);
		
		sprintf(contentLengthHeader, "Contenth-Length: %ld\r\n", strlen(HTTP_501_CONTENT)); 
		
		strcpy(connectionHeader, "Connection: close\r\n");
		 
		strcat(httpPackage, contentTypeHeader);
		
		strcat(httpPackage, contentLengthHeader);
	
		strcat(httpPackage, connectionHeader);
	
		strcat(httpPackage,"\r\n");
		
		strcat(httpPackage,HTTP_501_CONTENT);
		
		send(*connFd, httpPackage, strlen(httpPackage), 0);
		
	}
	else if(httpResult == "404"){
	
		statusCode = 404;
		
		contentType = "text/html";
		
		sprintf(httpPackage, "HTTP/1.1 %d %s\r\n", statusCode, HTTP_404_STRING);
		
		sprintf(contentTypeHeader, "Content-Type: %s\r\n", contentType);
		
		sprintf(contentLengthHeader, "Contenth-Length: %ld\r\n", strlen(HTTP_404_CONTENT)); 
		
		strcpy(connectionHeader, "Connection: close\r\n");
		 
		strcat(httpPackage, contentTypeHeader);
		
		strcat(httpPackage, contentLengthHeader);
	
		strcat(httpPackage, connectionHeader);
	
		strcat(httpPackage,"\r\n");
		
		strcat(httpPackage,HTTP_404_CONTENT);
		
		send(*connFd, httpPackage, strlen(httpPackage), 0);
	}
	else{
		
		statusCode = 200;
		strcat(filePath, httpResult);
		fileBuffer = readFile(filePath);
		
		sprintf(httpPackage, "HTTP/1.1 %d %s\r\n", statusCode, HTTP_200_STRING);
		
		if((strstr(httpResult, ".html")) != NULL){
			contentType = "text/html";
			
			sprintf(contentTypeHeader, "Content-Type: %s\r\n", contentType);
		
			sprintf(contentLengthHeader, "Contenth-Length: %ld\r\n", strlen(fileBuffer)); 
			
			strcpy(connectionHeader, "Connection: Keep-Alive\r\n");
			 
			strcat(httpPackage, contentTypeHeader);
			
			strcat(httpPackage, contentLengthHeader);
		
			strcat(httpPackage, connectionHeader);
		
			strcat(httpPackage,"\r\n");
			
			strcat(httpPackage,fileBuffer);
			
			printf("%s",fileBuffer);		
			send(*connFd, httpPackage, strlen(httpPackage), 0);		
		}	
		else if((strstr(httpResult, ".css")) != NULL ){
			contentType = "text/css";
			
			sprintf(contentTypeHeader, "Content-Type: %s\r\n", contentType);
		
			sprintf(contentLengthHeader, "Contenth-Length: %ld\r\n", strlen(fileBuffer)); 
			
			strcpy(connectionHeader, "Connection: Keep-Alive\r\n");
			 
			strcat(httpPackage, contentTypeHeader);
			
			strcat(httpPackage, contentLengthHeader);
		
			strcat(httpPackage, connectionHeader);
		
			strcat(httpPackage,"\r\n");
			
			strcat(httpPackage,fileBuffer);
			
			printf("%s",fileBuffer);		
			send(*connFd, httpPackage, strlen(httpPackage), 0);		
		}
		else if((strstr(httpResult, ".jpg")) != NULL){
			contentType = "image/jpeg";
			
			memLength = snprintf(imageBuffer, sizeof(imageBuffer),
			"HTTP/1.1 %d %s\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", 
			statusCode, HTTP_200_STRING, contentType,imageLength);
			
			memcpy(imageBuffer + memLength, fileBuffer, imageLength);
			send(*connFd, imageBuffer, memLength + imageLength, 0);
			
			
			
		}
		else if((strstr(httpResult, ".png")) != NULL){
			contentType = "image/png";
			
			memLength = snprintf(imageBuffer, sizeof(imageBuffer),
			"HTTP/1.1 %d %s\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", 
			statusCode, HTTP_200_STRING, contentType,imageLength);
			
			memcpy(imageBuffer + memLength, fileBuffer, imageLength);
			send(*connFd, imageBuffer, memLength + imageLength, 0);
			
			
		}
		else{
			contentType = "text/plain";
			
			sprintf(contentTypeHeader, "Content-Type: %s\r\n", contentType);
		
			sprintf(contentLengthHeader, "Contenth-Length: %ld\r\n", strlen(fileBuffer)); 
			
			strcpy(connectionHeader, "Connection: Keep-Alive\r\n");
			 
			strcat(httpPackage, contentTypeHeader);
			
			strcat(httpPackage, contentLengthHeader);
		
			strcat(httpPackage, connectionHeader);
		
			strcat(httpPackage,"\r\n");
			
			strcat(httpPackage,fileBuffer);
			
			printf("%ld",sizeof(fileBuffer));		
			send(*connFd, httpPackage, strlen(httpPackage), 0);		
		}
		
		
		
		
	}
	
	//close(clientSocket);
 }
 

//Dosya okuma işlemlerini byte düzeyinde yapan fonksiyon. (Resimleri göndermek için byte düzeyinde çalışmak gerekiyor.)
char * readFile(char *filePath){ 
	
	FILE *fd;
	char *fileBuffer;
	
	fd = fopen(filePath, "rb");
	if(fd == NULL){
		perror("Dosya acilamadi");
		return NULL;
	}
	else{
		fseek( fd, 0L, SEEK_END);
		imageLength = ftell(fd);
		fseek(fd, 0L, SEEK_SET);
		fileBuffer = (char*)calloc(imageLength,sizeof(char));
		
		if(fileBuffer == NULL){
			perror("Dosya bos");
			
		}
		
		fread(fileBuffer, sizeof(char), imageLength, fd);
		fclose(fd);
		return fileBuffer;
			
	}
	

}

 
//Gelen başlığa göre dosya adını ve request türünü belirleyen fonksiyon.
char * http_baslik_istegi (char * istek){
	

	char *fileNameStr = istek, filePath[50] = "web/";
	FILE *fd;
	if((strncmp(istek, "GET", 3)) == 0){
		
		if(istek[5] == ' '){
		 				
		      	if(fd = fopen("web/index.html", "r")){
		      		fclose(fd);
		      		return "index.html";
		      	}
		      	
		      	else{
		      		return "404";
		  
		      	}		
		}
		
		else{
			
			for(int i=5; i < (strlen(istek)-8); i++){
				fileNameStr[i-5] = istek[i];
			} 
			fileNameStr = strtok(fileNameStr, " ");
			strcat(filePath, fileNameStr);
			
			if(fd = fopen(filePath,"r")){
				fclose(fd);
				
				return fileNameStr;
			}
			else{
				return "404";
			}
			
		}
	} 
	else{
		
		return NULL;		
		
	}													
}

