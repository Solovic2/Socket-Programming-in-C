// C program for the Client Side
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <regex.h>

/* Structures */
// struct to save type of request,filename,body
struct files  
{  
    char filename[500];  
    char body[1024]; 
	char type[5];
};
// struct to save server_ip,port,client_request,file
struct client_data{
	char server_ip[16];
	int port;
	char client_request[4096];
	struct files file;
};

struct files server_files[10];
// count files
int count_file =0;

// Function to send data to server socket and recieve respose
void* threadClient(void* args)
{
	// Get The Request Message
	struct client_data *client = (struct client_data*) args; // Get The Request Message Struct
	
	// Create socket and initialize it
	int network_socket = socket(AF_INET,SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	server_address.sin_addr.s_addr = inet_addr(client->server_ip);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(client->port);

	// Connect to socket
	int connection_status = connect(network_socket,(struct sockaddr*)&server_address,sizeof(server_address));

	// Check if no connection between two sockets
	if(connection_status < 0){
		printf("Error\n");
		return 0;
	}

	printf("Connected Successfully ! .. \n");
	// Send Type if Get or Post or invalid request
	send(network_socket, client->file.type ,sizeof(client->file.type), 0);

	if(strcmp(client->file.type,"GET")==0){		// GET REQUEST
		// send file name
		send(network_socket, client->file.filename,strlen(client->file.filename), 0);
		char response[4096];
		bzero(response,4096);
		// Recieve From Socket GET / POST REQUEST
		recv(network_socket, &response,sizeof(response), 0);
		

		printf("\n(GET)Recived %s\n" ,response );
		
	}else if(strcmp(client->file.type,"POST")==0 && strcmp(client->file.body,"NULL")!=0)  {		// POST REQUEST
		// send body and filename
		send(network_socket, client->file.filename,strlen(client->file.filename), 0);
		read(network_socket,&client->file.filename,strlen(client->file.filename));
		send(network_socket, client->file.body,strlen(client->file.body), 0);
		printf("\nUploaded!\n");
	}else{
		printf("\nINVALID REQUEST\n");
	}
	close(network_socket);
	pthread_exit(NULL);
	return 0;
}
// read from file
char *readFromFile (char *filename){
	char fileData[1024];
	FILE *file = fopen(filename,"r");
	if(file == NULL){
	return "NULL";
	}
    fgets(fileData,1024,file);
    char* data = fileData;
    return data;
}
// Parsing function
struct files parsingForGetAndPostRequest(char *recieved_request){
	regex_t regex;	// regex
	regmatch_t pmatch[4]; // to match
	size_t nmatch = 4; 
	char response[4096],regex_func[4096];
	int pattern;
	struct files myfile;
	bzero(regex_func,4096);				// erase previous regex
	// GET | POST REGEX
	strcpy(regex_func, "^(GET|POST)\\s\\/\\s(\\S+)\\.(jpg|JPG|png|PNG|gif|GIF|jpeg|JPEG|html|HTML|txt|TXT)\\s(HTTP\\/1\\.1)");
	pattern = regcomp(&regex, regex_func, REG_EXTENDED);
	pattern = regexec(&regex, recieved_request,nmatch, pmatch, 0);
	// if matches
	if(pattern == 0){
		char type[5];			 // to get type of request GET or POST
		char filename[100];	 // to store filename of request
		char file_extention[5]; //	to store extention of file 
		if(pmatch[1].rm_so != -1 && pmatch[2].rm_so != -1 && pmatch[3].rm_so != -1) {
			// Set Variables type, filename,file_extention
			bzero(type,5);bzero(filename,100);bzero(file_extention,5);
			strncpy(type, &recieved_request[pmatch[1].rm_so], pmatch[1].rm_eo-pmatch[1].rm_so);
			strncpy(filename, &recieved_request[pmatch[2].rm_so], pmatch[2].rm_eo-pmatch[2].rm_so);
			strncpy(file_extention, &recieved_request[pmatch[3].rm_so], pmatch[3].rm_eo-pmatch[3].rm_so);
			// filename name.ext
			strcat(filename, ".");
			strcat(filename, file_extention);
		}
		
		strcpy(myfile.filename,filename);
		if(strcmp(type,"GET") == 0){
			// Type is GET
			bzero(myfile.type,5);
			strcpy(myfile.type,"GET");	
		}else{
			// Type is Post 
			strcpy(myfile.type,"POST");
			// read from file
			bzero(myfile.body,1024);
			strcpy(myfile.body,readFromFile(filename));
			if(strcmp(myfile.body,"NULL") == 0)strcpy(myfile.type,"Inv");

		}
		return myfile;
	}
	strcpy(myfile.type,"Inv");	
	return myfile;
}

// Driver Code
int main(int arg , char* argv[]){	
	// check if arguments is 3
	if(arg > 3 || arg < 3){
		printf(" You Should Add exact Three Arguments only (./client)(server_ip)(port_number) \n");
		return 0;
	}
	struct client_data client;
	// pthreads
	pthread_t tid;
	pthread_t readFile[100];
	char client_request[4096];
	char file_path[1000] = "input.txt";
	int n = 0;
	int port  = strtol(argv[2],NULL,10); 	// port
	if(strcmp(argv[1],"localhost")==0 ||strcmp(argv[1],"127.0.0.1")==0 )
 		strcpy(client.server_ip,"127.0.0.1");	// if it localhost convert to 127.0.0.1
	else {
		printf("No server");
		return 0;
	}				
	client.port = port;		// Port
	/******			 Take Input From Client    ****/
	int count=0;
	int i = 0;
	FILE *file = fopen(file_path,"r");
	while(fgets(client_request,4086,file)) {
			count++;
			printf("\nFrom Client %s\n",client_request);
			// Parsing Here To Know If GET Or POST
			client.file = parsingForGetAndPostRequest(client_request);
			strcpy(client.client_request,client_request);		// Message
			bzero(client_request,4086);
			if(pthread_create(&readFile[i++], NULL,threadClient,&client)!=0) printf("Error in thread");
			sleep(10);
			if (i >= 40) {
					// reset i to zero
				i = 0;
				while (i < 40) {
					// join threads
					pthread_join(readFile[i++],NULL);
				}
				i = 0;
			}
	}
	printf("All Request %d \n",count);
	return 0;
}
