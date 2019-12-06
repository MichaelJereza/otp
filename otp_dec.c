#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
/*---------------------------------checkText-------------------------------------*/
// Checks text for invalid characters
// Exits 1 and first encountered invalid character to stderr
// Else return 1
int checkText(char* Text, int len){
    int p;
    // For each letter of ext - newline
    // Check validity
    for(p = 0; p < len-1; p++){
        int letter = Text[p];
        // If bad char exit
        if( ((letter < 65)||(letter > 90)) && (letter!=32) ){
            fprintf(stderr, "Invalid char: %c\n", Text[p]);
            exit(1);
        }
    }
    return 1;
}
/*------------------------------------plaintex-----------------------------------*/
// Get cipher at argv[1]
char* getCipher(char* pfile){
    FILE* fd = fopen(pfile, "r");
    if(fd == NULL){
        perror("Can't open plaintext\n");
        exit(1);
    }
    char* cipher = NULL;
    size_t cSize = 0;   
    int cLen = -1;

    cLen = getline(&cipher, &cSize, fd);

    if(cLen<0){
        fprintf(stderr, "Can't get from file %s\n", pfile);
        exit(1);
    }
    if(checkText(cipher, cLen)){
        fclose(fd);
        return cipher;
    }
    fclose(fd);
    return NULL;
}
/*-------------------------------------key---------------------------------------*/
// Retrieves key from fd kfile, assures key is long enough
char* getKey(char* kfile, int plen){
    FILE* fd = fopen(kfile, "r");
    char* key = NULL;
    size_t keySize = 0;
    int keyLen = -1;

    keyLen = getline(&key, &keySize, fd);
    
    // Check key isn't shorter than cipher
    if(keyLen<plen){
        fprintf(stderr, "Invalid keysize: %d\n", keyLen);
        exit(1);
    }
    // Check key for bad characters
    if(checkText(key, keyLen)){
        fclose(fd);
        return key;
    }
    fclose(fd);
    return NULL;
}

/*------------------------------check---daemon-----------------------------------*/
// Returns 1 if verified otp_enc_d daemon, else exit on rejection
int verifyDaemon(int socketFD){
    int chars = 0;

    // Send encryption char
    char res = 'd';
    do{
        chars = send(socketFD, &res, 1, 0);
    }while(chars<=0);
    chars = 0;

    // Wait for response
    do{
        chars = recv(socketFD, &res, 1, 0); // Read data from the socket
    }while(chars<=0);

    // If rejected
    if(res=='r'){
        exit(2);
    }
    // otp_enc connected to otp_enc_d
    // Verified
    else if(res=='a'){
        return 1;
    }
}
/*-------------------------------------connect-----------------------------------*/
// Returns the socket file descriptor of validated encryption daemon
int connectDaemon(char* port){
    int socketFD, portNumber;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
    // Connect to server
    // Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));
	portNumber = atoi(port);
	serverAddress.sin_family = AF_INET; 
	serverAddress.sin_port = htons(portNumber);
	serverHostInfo = gethostbyname("localhost");
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(2); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr_list[0], serverHostInfo->h_length);

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0){
        fprintf(stderr, "CLIENT: ERROR opening socket");
        exit(1);
    }
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){ // Connect socket to address
		fprintf(stderr, "CLIENT: ERROR connecting\n");
        exit(1);
    }

    // If correct daemon, return socket
    if(verifyDaemon(socketFD)){
        return socketFD;
    }

}

void sendString(int socketFD, char* str){
    int sent = 0;
    int len = strlen(str);
    int charsOut = 0;
    do{
        // Send Plaintext
        charsOut = send(socketFD, str, len, 0);
        sent+=charsOut;
    }while(sent!=len);

    charsOut = 0;
    char res;
    // Wait for response
    do{
        charsOut = recv(socketFD, &res, 1, 0); // Read data from the socket
    }while(charsOut<=0);

    // String received
    if(res=='!'){
        return;
    }
}

char* recvCipher(int socketFD, int length){
    int charsRead = -5;
    char buffer[10];
    char* key = malloc(sizeof(char)*length);
    memset(key, '\0', length);
    int strLength = 1; // Complete length of string + NULL term
    int received = 0;

    //printf("\nlen:%d\n",length);

    // Loop until no more characters received
    do{
        charsRead = recv(socketFD, buffer, 10, 0);
        if(charsRead>0){
            // Stop at newline
            charsRead = strcspn(buffer, "\n");
            strLength += charsRead;

            // Copy until newline
            strncat(key, buffer, charsRead * sizeof(char));
            memset(buffer, '\0', 10);

            // If encountered newline, stop looping
            if(charsRead<10){
                break;
            }
        }
    }while(strLength!=length);
    key[length-1]='\n';
    key[length]='\0';

   return key;
}
int main(int argc, char* argv[]){
    // Catch bad parameters
    if(argc < 4){
        fprintf(stderr, "Syntax Error!\nUsage: %s plaintext key port\n", argv[0]);
        return 1;
    }
    
    else{
        // Get cipher
        char* cipher = getCipher(argv[1]);
        // Get key
        char* key = getKey(argv[2], strlen(cipher));
        
        // Connect
        int socket = connectDaemon(argv[3]);
        // Send
        sendString(socket, cipher);
        sendString(socket, key);
        // Recv
        char* plaintext = recvCipher(socket, strlen(cipher));
        printf("%s",plaintext);
        free(cipher);
        free(key);
        free(plaintext);
    }
}