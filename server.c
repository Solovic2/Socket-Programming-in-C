#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <regex.h>
/* Struct */

// struct to save filename and id and body of file
struct files  
{  
	int id;
    char filename[500];  
    char body[1024];  
}; 
// struct data to save the acceptSocket number and the request message from client socket
typedef struct data {
    int acceptSocket;
    char client_request [4086];
	struct files file;
	
} requestData;

// Semaphore variables
sem_t x, y; // one for read and write
pthread_t tid;
pthread_t writerthreads[100];
pthread_t readerthreads[100];

int countReaders = 0;		// counter for readers
int count_file = 0;			// counter for files in server

struct files server_files[10];

void* readRequestWithFileFromServer(void* request)
{
    // Lock the semaphore
    sem_wait(&x);
    countReaders++;
 
	// if there is one reading file , wait on  the second semaphore to not upload any thing
    if (countReaders == 1)
        sem_wait(&y);
 
    // Unlock the semaphore 
    sem_post(&x);

	// Read The Existens File
	struct data *requestData = (struct data*) request; // Get The Request Message Struct

	// send the response to client socket
	send(requestData->acceptSocket,requestData->client_request,sizeof(requestData->client_request),0);

    sleep(5);
 
    // Lock the semaphore
    sem_wait(&x);
    countReaders--;
 
    if (countReaders == 0) {
        sem_post(&y);
    }
	
    // Lock the semaphore
    sem_post(&x);
 
    pthread_exit(NULL);
}
// write file to server (directory)
void writeFileToServer(char *filename,char *body){
	FILE *file = fopen(filename,"w");
	fputs(body, file);
	fclose(file);
}

// Function to upload file .
void* uploadFileToServer(void* request)
{

	// Lock the semaphore
	sem_wait(&y);
	struct data *requestData = (struct data*) request; // Get The Request Message Struct
	if(strcmp(requestData->client_request,"Storage Complete")!=0){	// if size of files storage is not complete 
		char body[1024];
		// send file_name
		send(requestData->acceptSocket,server_files[requestData->file.id].filename, sizeof(server_files[requestData->file.id].filename),0);
		bzero(body,1024);
		read(requestData->acceptSocket,&body, sizeof(body)); // recieve the body

		// write in file
		writeFileToServer(server_files[requestData->file.id].filename,body);
		strcpy(server_files[requestData->file.id].body,body);
		printf("\nFile Uploaded Successfully !\n");

	}else{
		char storage[50]="Storage is Completed!";
		printf("\nStorage is Completed !\n");
		return 0;
	}
	
	// Unlock the semaphore
	sem_post(&y);
	// exit
	pthread_exit(NULL);
}
// function to check if file exists of not in files structure
int checkIfExistFile(struct files myfiles[10],char *filename){
	if(strcmp(filename,"") == 0) return -1;
	for(int i=0; i<10 ;i++){
		if(strcmp(myfiles[i].filename,filename) == 0){
			return i;
		}
	}
	return -1;
}
int main(int arg,char* argv[])
{
	// Check Argument is two only
	if(arg > 2 || arg < 2){
		printf(" You Should Add exact Two Arguments only\n");
		return 0;
	}
	// port number
	int port  = strtol(argv[1],NULL,10);
	// semaphores
	sem_init(&x, 0, 1);
	sem_init(&y, 0, 1);
	// initialize the socket
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serverAddr;
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);


	
	// Bind socket to address and port number.
	if (bind(serverSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) != 0) {
        printf("socket bind failed...\n");
    }
	// Listen on the socket
	if (listen(serverSocket, 40) == 0) printf("Listening now ....\n");
	else printf("Error In Listening\n");

	// initialize pthread and semaphores
	pthread_t tid[60];
	int i = 0;

	// accept variable and serverStorage 
	int acceptSocket;
	// To store the address 
	struct sockaddr_storage storage;
	socklen_t addr_size;
	
	char recieved_request[4096];
	while(1){

		addr_size = sizeof(storage);
		// Accept connection to get the first connection in the queue 
		acceptSocket = accept(serverSocket, (struct sockaddr*) &storage, &addr_size);
		if (acceptSocket < 0) {
			printf("server accept failed...\n");
    	}
		/****  Recieve Message From Client   **/
		// check type first
		char type_request[5];
		bzero(type_request,5);	// erase the data 
		read(acceptSocket,&type_request, sizeof(type_request)); // recieve the type of message
		// recieve data (filename if get and filename and body if post)
		char filename[100];
		
		printf("\nServer Reciving... \n");
		bzero(recieved_request,4096);
		if(strcmp(type_request,"GET") == 0){
			bzero(filename,100);
			read(acceptSocket,&filename, sizeof(filename)); // recieve the filename
			int index = checkIfExistFile(server_files, filename);
			if(index != -1 ){
				// read from file exist
				// put the file in server_files again (to avoid if any changes in file)
				strcpy(recieved_request,server_files[index].body);
				requestData data;
				data.acceptSocket = acceptSocket;
				strcpy(data.client_request , "HTTP/1.1 200 OK\r\n");
				strcat(data.client_request , recieved_request);
				if (pthread_create(&readerthreads[i++], NULL,readRequestWithFileFromServer, &data)!= 0) printf("Error to create Read File thread\n");
			}else{
				// Not FOUND
				requestData data;
				data.acceptSocket = acceptSocket;
				strcpy(data.client_request , "HTTP/1.1 404 Not Found\r\n");
				if (pthread_create(&readerthreads[i++], NULL,readRequestWithFileFromServer, &data)!= 0) printf("Error to create Read File thread\n");
			}
		}else if(strcmp(type_request,"POST") == 0){
			if(count_file > 10){	// check size of storage files
				requestData data;
				data.acceptSocket = acceptSocket;
				strcpy(data.client_request , "Storage Complete");
				if (pthread_create(&writerthreads[i++], NULL,uploadFileToServer, &data)!= 0) printf("Error to create write thread\n");
			}else{
				requestData data;
				data.acceptSocket = acceptSocket;
				strcpy(data.client_request , "Done");
				bzero(filename,100);
				read(acceptSocket,&filename, sizeof(filename)); 	// recieve the filename
				int index = checkIfExistFile(server_files, filename);	// check if file exist or not
				if(index != -1 ){	// if exist
					strcpy(server_files[index].filename,filename);
					server_files[index].id = index;
					data.file = server_files[index];
				}else{	// if not exist
					strcpy(server_files[count_file].filename,filename);
					server_files[count_file].id = count_file;
					data.file = server_files[count_file];
					count_file++;
				}
				if (pthread_create(&writerthreads[i++], NULL,uploadFileToServer, &data)!= 0) printf("Error to create write thread\n");
			}
		}else{
			printf("\nInvalid Input\n");
		}
		// join threads
		if (i >= 40) {
			// reset i to zero
			i = 0;
			while (i < 40) {
				// join threads
				pthread_join(writerthreads[i++],NULL);
				pthread_join(readerthreads[i++],NULL);
				
			}
			i = 0;
		}
	}

	return 0;
}
