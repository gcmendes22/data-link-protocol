#include <stdio.h>
#include "linklayer.h"

typedef struct linklayer {
  char serialPortTrasmitter[50];
  char serialPortReceiver[50];
  int role; // defines the role of the program: 0 == Transmitter, 1 == Receiver
  int baudRate;
  int numTries;
  int timeOut;
} linkLayer_t;

void printIncorrectUsageMessage(char** argv);

/* ./main <trasmitterPort> <receiverPort> <role> <baudRate> <numTries> <timeOut> */
int main(int argc, char **argv) {
  if (argc < 6) {
    printIncorrectUsageMessage(argv);
    exit(1);
  }

  linkLayer_t config = {

  };
  
  return 0;
}

void printIncorrectUsageMessage(char** argv) {
    printf("Incorrect program usage\n"
      "Usage: %s <trasmitterPort> <receiverPort> <role> <baudRate> <numTries> <timeOut>\n"
      "Example: %s /dev/ttyS1\n",
      argv[0], argv[0]);
}