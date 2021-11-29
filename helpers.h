#ifndef HELPERS_H
#define HELPERS_H

#include <signal.h>

#include <unistd.h>

// tramas
#define F 0x7E

#define A_TX 0x05
#define A_RX 0x02

#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B

#define BCC_SET A_TX^C_SET
#define BCC_UA A_RX^C_UA
#define BCC_DISC A_TX^C_DISC

// Acknowledge conection tramas(RR and REJ)
#define ACK_A 0x02
#define C_RR 0x01
#define C_REJ 0x05

enum State { START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DONE };

void startAlarm();

void alarmHandler(int signal);

void sendUATrama(int fd);

int sendSETTrama(int fd);

void getSETTrama(int fd);

void sendREJtrama(char controlo,int fd);

void sendRRtrama(char controlo,int fd);

int sendDISCTramaTransmitter(int fd);

int sendDISCTramaReceiver(int fd);

int getDISCTramaTransmitter(int fd);

int getDISCTramaReceiver(int fd);

char* createFrameI(char* buf, int bufSize);

void stateMachineSETMessage(enum State* state, char flag);

void stateMachineUAMessage(enum State* state, char flag);

void stateMachineDISCMessageTransmitter(enum State* state, char flag);

void stateMachineDISCMessageReceiver(enum State* state, char flag);

#endif // !HELPERS_H
