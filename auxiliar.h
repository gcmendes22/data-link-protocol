#ifndef ALARME_H
#define ALARME_H

#include <signal.h>

extern int count, flag;

enum State {
  START, FLAG_RCV, A_RCV, C_RCV, C_RCV_RR, C_RCV_REJ, BCC_OK, DATA_RCV, FLAG_RCV2, END
};

void initAlarme();
void alarme_handler(int signal);

#endif
