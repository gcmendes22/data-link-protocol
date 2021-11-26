#include "helpers.h"
#include <stdio.h>


int alarmEnabled = 1;
int alarmCount = 0;

void startAlarm() {
    (void) signal(SIGALRM, alarmHandler);
}

void alarmHandler(int signal) {
    alarmEnabled = 1;
    printf("Attempt %d\n", alarmCount);
}


void sendUATrama(int fd) {
    char ua[5] = { F, A_RX, C_UA, BCC_UA, F };
    int bytesUA = write(fd, ua, 5);
    printf("%d bytes written\n", bytesUA);
}

int sendSETTrama(int fd) {
    char set[5] = { F, A_TX, C_SET, BCC_SET, F };
    char flag;
    enum State state = START;
    startAlarm();
    do {
        write(fd, set, 5);

        if(alarmEnabled) {
            alarm(3);
            alarmEnabled = 0;
        }

        while(state != DONE && alarmEnabled == 0) {
            read(fd, &flag, 1);
            stateMachineSETMessage(&state, flag);
        }

        if(state == DONE) return 1;

        alarmCount++;
    } while (alarmCount < 4);

    return -1;
}

void getSETTrama(int fd) {
    enum State state = START;
    char flag;
    int currentState = 0;
    while(state != DONE) {
        read(fd, &flag, 1);
        stateMachineUAMessage(&state, flag);
        currentState++;
    }
}


void sendRRtrama(char controlo,int fd){

    char C= RR_C + (controlo << 4);
    char buffer[5] = { F, ACK_A , C , ACK_A^C , F };
    int bytes_RR = write(fd, buffer, 5);
    printf("bytes enviados %d\n",bytes_RR);

}

void sendREJtrama(char controlo,int fd){

    char C= REJ_C + (controlo << 4);
    char buffer[5] = { F, ACK_A , C , ACK_A^C , F };
    int bytes_REJ = write(fd, buffer, 5);
    printf("bytes enviados %d\n",bytes_REJ);
    
}


void stateMachineSETMessage(enum State* state, char flag) {
    switch(*state) {
        case START: 
            if(flag == F) *state = FLAG_RCV;
            break;
        
        case FLAG_RCV: 
            if(flag == F) *state = FLAG_RCV;
            else if(flag == A_RX) *state = A_RCV;
            else *state = START;
            break;
        
        case A_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == C_UA) *state = C_RCV;
            else *state = START;
            break;
        
        case C_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == (BCC_UA)) *state = BCC_OK;
            else *state = START;            
            break;
        
        case BCC_OK:
            if(flag == F) *state = DONE;
            break;

        case DONE:
            break;
    }
}

void stateMachineUAMessage(enum State* state, char flag) {
    switch(*state) {
        case START: 
            if(flag == F) *state = FLAG_RCV;
            break;
        
        case FLAG_RCV: 
            if(flag == F) *state = FLAG_RCV;
            else if(flag == A_TX) *state = A_RCV; // provavelmente é SET_A
            else *state = START;
            break;
        
        case A_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == C_SET) *state = C_RCV;
            else *state = START;
            break;
        
        case C_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == (BCC_SET)) *state = BCC_OK;
            else *state = START;            
            break;
        
        case BCC_OK:
            if(flag == F) *state = DONE;
            break;

        case DONE:
            break;
    }
}
