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
    printf("UA was sent\n");
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

int sendDISCTramaTransmitter(int fd) {
    char disc[5] = { F, A_TX, C_DISC, BCC_DISC, F };
    char flag;
    enum State state = START;
    startAlarm();
    do {
        write(fd, disc, 5);

        if(alarmEnabled) {
            alarm(3);
            alarmEnabled = 0; 
        }

        while(state != DONE && alarmEnabled == 0) {
            read(fd, &flag, 1);
            stateMachineDISCMessageTransmitter(&state, flag);
        }

        if(state == DONE) {
            printf("DISC was sent by transmitter\n");
            return 1;
        }
        alarmCount++;
    } while (alarmCount < 4);

    return -1;
}

int sendDISCTramaReceiver(int fd) {
    
    char disc[5] = { F, A_RX, C_DISC, BCC_DISC, F };
    char flag;
    enum State state = START;
    startAlarm();
    do {
        write(fd, disc, 5);

        if(alarmEnabled) {
            alarm(3);
            alarmEnabled = 0; 
        }

        while(state != DONE && alarmEnabled == 0) {
            read(fd, &flag, 1);
            stateMachineDISCMessageReceiver(&state, flag);
        }

        if(state == DONE) {
            printf("DISC was sent by receiver\n");
            return 1;
        }
        alarmCount++;
    } while (alarmCount < 4);

    return -1;
}


int getDISCTramaTransmitter(int fd) {

    enum State state = START;
    char flag;
    int currentState = 0;
    while(state != DONE) {
        read(fd, &flag, 1);
        stateMachineDISCMessageReceiver(&state, flag);
        currentState++;
    }
    
    if(state == DONE) {
        printf("Transmitter received DISC\n");
        return 1;
    }
    return -1;
}


int getDISCTramaReceiver(int fd) {

    enum State state = START;
    char flag;
    int currentState = 0;
    while(state != DONE) {
        read(fd, &flag, 1);
        stateMachineDISCMessageTransmitter(&state, flag);
        currentState++;
    }
    if(state == DONE){
        printf("Receiver received DISC\n");
        return 1;
    }
    return -1;
}

void sendRRtrama(char controlo,int fd){

    char C= C_RR + (controlo << 4);
    char buffer[5] = { F, ACK_A , C , ACK_A^C , F };
    int bytes_RR = write(fd, buffer, 5);
    printf("bytes enviados %d\n",bytes_RR);

}

void sendREJtrama(char controlo,int fd){

    char C = C_REJ + (controlo << 4);
    char buffer[5] = { F, ACK_A , C , ACK_A^C , F };
    int bytes_REJ = write(fd, buffer, 5);
    printf("bytes enviados %d\n",bytes_REJ);
    
}

char* createFrameI(char* buf, int bufSize) {
   
    
    return NULL;
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
            else if(flag == A_TX) *state = A_RCV;
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

void stateMachineDISCMessageTransmitter(enum State* state, char flag) {
    switch(*state) {
        case START: 
            if(flag == F) *state = FLAG_RCV;
            printf("START %x\n", flag);
            break;
        
        case FLAG_RCV: 
            if(flag == F) *state = FLAG_RCV;
            else if(flag == A_TX) *state = A_RCV;
            else *state = START;
            printf("FLAG_RCV %x\n", flag);
            break;
        
        case A_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == C_DISC) *state = C_RCV;
            else *state = START;
            printf("A_RCV %x\n", flag);
            break;
        
        case C_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == (BCC_DISC)) *state = BCC_OK;
            else *state = START;    
            printf("C_RCV %x\n", flag);        
            break;
        
        case BCC_OK:
            if(flag == F) *state = DONE;
            printf("BCC_OK %x\n", flag);
            break;

        case DONE:
            printf("DONE %x\n", flag);
            break;
    }
}

void stateMachineDISCMessageReceiver(enum State* state, char flag) {
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
            else if(flag == C_DISC) *state = C_RCV;
            else *state = START;
            break;
        
        case C_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == (BCC_DISC)) *state = BCC_OK;
            else *state = START;            
            break;
        
        case BCC_OK:
            if(flag == F) *state = DONE;
            break;

        case DONE:
            break;
    }
}