#include "auxiliar.h"

void initAlarme() {
   (void) signal(SIGALRM, alarme_handler);
}

void alarme_handler(int signal) {

   flag = 1;
}

