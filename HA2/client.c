#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

#define CHUNK_SIZE 512


//https://codeshare.io/50KQ3n

// Based on the input address return the IPv4 or IPv6
void *getInAddr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }else{
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }
}

char* new_str(size_t size){
    return calloc(size, sizeof(char));
}

char* append(char* body, char* new){
    char* all;
    size_t len;
    
    if(body == NULL){
        len = strlen(new);
    }else{
        len = strlen(body) + strlen(new);
    }
    
    if ((all = calloc(len + 1, sizeof(char))) == NULL) //+1?
        return NULL;
  
    strcat(all, body);
    strcat(all, new);
  
    return all;
}

char* copy_until(char* str, int i){
    char* new = calloc(i, sizeof(char));
    strncpy(new, str, i);
    return new;
}

char* rm_first_elem(char* str, int i){
    char* new = new_str(CHUNK_SIZE - i+1);
    for(int j = 0; j < CHUNK_SIZE - i+1; j++){
        new[j] = str[j+1];
    }
    return new;
}

char* cut_r(char* str){
    char* whole = NULL;
    for(int i = 0; i < CHUNK_SIZE; i++){
        if (str[i] == '\r'){
            char* tmp = copy_until(str, i);
            char* tmp2 = rm_first_elem(&str[i], i);
            whole = append(tmp, tmp2);
            free(str);
            strncpy(str, whole, CHUNK_SIZE);
            free(whole);
            free(tmp);
            free(tmp2);
        }
        
    }
    return str;
}


int main(int argc, char*argv[]) {
    // Check for inputs
    if(argc != 3){
        fprintf(stderr, "Usage: IP-Address Port");
        exit(1);
    }
    struct addrinfo hints, *serverInfo, *p;
    int sockfd, numbytes, status;

    // Clear the hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Get the server info
    if((status = getaddrinfo(argv[1],argv[2], &hints, &serverInfo)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    for(p = serverInfo; p != NULL; p = p->ai_next){
        if((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1){
            perror("client: socket() error");
            continue;
        }
        if(connect(sockfd,p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            perror("client: connect() error");
            continue;
        }
        break;
    }
    if( p == NULL){
        fprintf(stderr, "client: failed to connect...\n");
        return 3;
    }

    // Convert IP adress to bits
    inet_ntop(p->ai_family, getInAddr((struct sockaddr*)p->ai_addr),argv[1], sizeof(argv[1]));

    // Free the server info, we don't need it anymore.
    freeaddrinfo(serverInfo);

    char* buf = calloc(CHUNK_SIZE, sizeof(char));
    char* wholeMsg = calloc(CHUNK_SIZE, sizeof(char)); //causes one alloc left in the end

    while(1){
        // Clear the buffer
        memset(buf,0, sizeof(char)*CHUNK_SIZE);
        // Receive the data
        numbytes = recv(sockfd, buf, CHUNK_SIZE, 0);
        if(numbytes == -1){
            perror("client: recv() error");
            exit(1);
        }else if(numbytes == 0){
            break;
        }
        
        wholeMsg = append(wholeMsg, cut_r(buf));
    }
    // Print the whole message
    fprintf(stdout,"%s\n", wholeMsg);
    // Free the memory
    free(buf);
    free(wholeMsg);
    // Close the main socket
    close(sockfd);

    return 0;
}

