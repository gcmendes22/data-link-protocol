#include "linklayer.h"
#include "helpers.h"
#include <signal.h>

#define ERROR -1

#define BUF_SIZE 2008

volatile int STOP = FALSE;
char read_data[BUF_SIZE];
int fd;ters.numTries;
    connection.timeOut =connectionParamet
struct linkLayer connection;
char *Tramas_lidas[1000];
int read_count=0;


int llopen(linkLayer connectionParameters) { 

    connection.role = connectionParameters.role;
    connection.numTries =connectionParameers.timeOut;
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );
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
        if(sendSETTrama(fd) == TRUE){ 
            return TRUE;
        }
    }

    if(connectionParameters.role == RECEIVER) {
        getSETTrama(fd);
        sendUATrama(fd);
        return TRUE;
    }

    return ERROR;
}

int llread(char *package){
    int read_bytes=read(fd,package,MAX_PAYLOAD_SIZE);

    for(int i=0;i<read_count;i++){

        if(package[2]==Tramas_lidas[i])
        {
            memset(package, 0, read_bytes);
            return read_bytes;
        }
    }
    

}


