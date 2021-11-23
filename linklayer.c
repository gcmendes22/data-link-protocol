#include "linklayer.h"
#include "helpers.h"
#include <signal.h>

#define ERROR -1

#define BUF_SIZE 2008

volatile int STOP = FALSE;


int llopen(linkLayer connectionParameters) {    
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );
    if (fd < 0) {
        perror("Error: cannot open the serial port.");
        return ERROR;
    }

    struct termios oldtio;
    struct termios newtio;

    if (tcgetattr(fd, &oldtio) == -1) {
        perror("tcgetattr");
        return ERROR;
    }

    bzero(&newtio, sizeof(newtio));

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 5;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        return ERROR;
    }
    printf(connectionParameters.role == TRANSMITTER ? "Openning as transmitter\n" : "Openning as receiver\n");
    printf("New termios structure set\n");

    if(connectionParameters.role == NOT_DEFINED) {
        perror("Error: role was not defined");
        return ERROR;
    }

    if(connectionParameters.role == TRANSMITTER) {
        if(sendSETTrama(fd)) {
            printf("The transmitter just sent the SET trama.\n");
            return TRUE;
        }
    }

    if(connectionParameters.role == RECEIVER) {
        getSETTrama(fd);
        sendUATrama(fd);
        printf("The receiver just receive the SET trama and sent the UA trama as confirmation.\n");
        return TRUE;
    }

    return ERROR;
}