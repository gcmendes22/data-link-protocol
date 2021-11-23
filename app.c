#include "linklayer.h"
#include <stdio.h>

int main(int argc, char **argv){
    char* serialPort = argv[1];
	struct linkLayer ll;
    sprintf(ll.serialPort, "%s", serialPort);
    ll.role = TRANSMITTER;
    ll.baudRate = B9600;
    ll.numTries = MAX_RETRANSMISSIONS_DEFAULT;
	ll.timeOut = TIMEOUT_DEFAULT;
	
	 if(llopen(ll) == -1) {
            fprintf(stderr, "Could not initialize link layer connection\n");
            exit(1);
        }

	
    printf("connection opened\n");
	fflush(stdout);
    fflush(stderr);
}
