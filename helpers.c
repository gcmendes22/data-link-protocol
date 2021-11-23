#include "helpers.h"
#include <stdio.h>


int alarmEnabled = 0;
int alarmCount = 0;

void startAlarm() {
    (void) signal(SIGALRM, alarmHandler);
}

void alarmHandler(int signal) {
    alarmEnabled = 1;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}


void sendUATrama(int fd) {
    char ua[5] = { F, UA_A, UA_C, UA_BCC, F };
    int bytesUA = write(fd, ua, 5);
    printf("%d bytes written\n", bytesUA);
}

int sendSETTrama(int fd) {
    char set[5] = { F, SET_A, SET_C, SET_BCC, F };
    char flag;
    enum State state = START;
    startAlarm();
    while(alarmCount < 4) {
        int bytesSET = write(fd, set, 5);
        printf("%d bytes written\n", bytesSET);

        if(alarmEnabled == 0) {
            alarm(3);
            alarmEnabled = 1;
        }

        while(state != DONE && alarmEnabled == 0) {
            read(fd, &flag, 1);
            receptionSETMessage(&state, flag);
        }

        if(state == DONE) return 0;

        alarmCount++;
    }

    return 1;
}

void getSETTrama(int fd) {
    enum State state = START;
    char flag;
    int currentState = 0;
    while(state != DONE) {
        read(fd, &flag, 1);
        receptionUAMessage(&state, flag);
        currentState++;
    }
}

void receptionSETMessage(enum State* state, char flag) {
    switch(*state) {
        case START: 
            if(flag == F) *state = FLAG_RCV;
            break;
        
        case FLAG_RCV: 
            if(flag == F) *state = FLAG_RCV;
            else if(flag == UA_A) *state = A_RCV;
            else *state = START;
            break;
        
        case A_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == UA_C) *state = C_RCV;
            else *state = START;
            break;
        
        case C_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == (UA_BCC)) *state = BCC_OK;
            else *state = START;            
            break;
        
        case BCC_OK:
            if(flag == F) *state = DONE;
            break;

        case DONE:
            break;
    }
}

void receptionUAMessage(enum State* state, char flag) {
    switch(*state) {
        case START: 
            if(flag == F) *state = FLAG_RCV;
            break;
        
        case FLAG_RCV: 
            if(flag == F) *state = FLAG_RCV;
            else if(flag == SET_A) *state = A_RCV; // provavelmente é SET_A
            else *state = START;
            break;
        
        case A_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == SET_C) *state = C_RCV;
            else *state = START;
            break;
        
        case C_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == (SET_BCC)) *state = BCC_OK;
            else *state = START;            
            break;
        
        case BCC_OK:
            if(flag == F) *state = DONE;
            break;

        case DONE:
            break;
    }
}