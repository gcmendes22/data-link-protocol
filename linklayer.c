#include "linklayer.h"
#include <signal.h>

// Open connection tramas (SET and UA)
#define F 0x7E
#define SET_A 0x05
#define SET_C 0x03
#define SET_BCC SET_A^SET_C
#define UA_A 0x02
#define UA_C 0x07
#define UA_BCC UA_A^UA_C

#define ERROR -1

#define BUF_SIZE 2008

volatile int STOP = FALSE;

int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal) {
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

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
    newtio.c_cc[VTIME] = 3;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        return ERROR;
    }

    printf("New termios structure set\n");

    if(connectionParameters.role == NOT_DEFINED) {
        perror("Error: role was not defined");
        return ERROR;
    }

    if(connectionParameters.role == TRANSMITTER) {
        char set[5] = { F, SET_A, SET_C, SET_BCC, F };
        char buf[BUF_SIZE + 1];
        (void) signal(SIGALRM, alarmHandler);
        while(alarmCount < connectionParameters.numTries) {
            if(alarmEnabled==FALSE){
                int bytesSET = write(fd, set, 5);
                printf("%d bytes written\n", bytesSET);
                alarm(3);  // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;
            }
            
            if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
                perror("tcsetattr");
                return ERROR;
            }
            int bytes = read(fd, buf, BUF_SIZE);
            for(int i = 0; i < 5; i++) printf("%x ", buf[i]);
            putchar('\n');
            if(buf[2] == UA_C){
                return TRUE;
            }
        }
        close(fd);
        return ERROR;
    }

    if(connectionParameters.role == RECEIVER) {
        char buf[BUF_SIZE + 1];
        while (STOP == FALSE) {
            int bytes = read(fd, buf, BUF_SIZE);

            if (bytes > 0){
                if(buf[2] == SET_C) {
                    char ua[5] = { F, UA_A, UA_C, UA_BCC, F };
                    int bytesUA = write(fd, ua, 5);
                    printf("%d bytes written\n", bytesUA);
                    return TRUE;
                }
            }
        }
    }

    return TRUE;
}