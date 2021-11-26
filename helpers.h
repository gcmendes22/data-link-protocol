#ifndef HELPERS_H
#define HELPERS_H

#include <signal.h>

#include <unistd.h>

// Open connection tramas (SET and UA)
#define F 0x7E
#define SET_A 0x05
#define SET_C 0x03
#define SET_BCC SET_A^SET_C
#define UA_A 0x02
#define UA_C 0x07
#define UA_BCC UA_A^UA_C

// Acknowledge conection tramas(RR and REJ)
#define ACK_A 0x02
#define RR_C 0x01
#define REJ_C 0x05

enum State { START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DONE };

void startAlarm();

void alarmHandler(int signal);

void sendUATrama(int fd);

int sendSETTrama(int fd);

void getSETTrama(int fd);

void sendREJtrama(char controlo,int fd);

void sendRRtrama(char controlo,int fd);

void stateMachineSETMessage(enum State* state, char flag);

void stateMachineUAMessage(enum State* state, char flag);

#endif // !HELPERS_H