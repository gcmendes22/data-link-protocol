#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linklayer.h"

int main(int argc, char **argv){
    char* serialPort = argv[1];
    int role = atoi(argv[2]);
	struct linkLayer ll;
    sprintf(ll.serialPort, "%s", serialPort);
    ll.role = role;
    ll.baudRate = B9600;
    ll.numTries = MAX_RETRANSMISSIONS_DEFAULT;
	ll.timeOut = TIMEOUT_DEFAULT;
	
    if(llopen(ll) < 0) {
        fprintf(stderr, "Could not initialize link layer connection\n");
        return -1;
    }
	
    printf("Connection opened successfuly\n");
	fflush(stdout);
    fflush(stderr);


    if(llclose(TRUE) < 0) {
        fprintf(stderr, "Could not close the link layer connection\n");
        return -1;
    }
    printf("Connection closed successfuly\n");
    return 0;
}
