#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int alarmEnabled = 0;
int alarmCount = 0;

void alarmHandler(int signal) {
    alarmEnabled = 0;
    alarmCount++;
    printf("Alarm #%d\n", alarmCount);
}

int main(void) {
    (void) signal(SIGALRM, alarmHandler);
    while(alarmCount < 4) {
        if(alarmEnabled == 0) {
            alarm(3);
            alarmEnabled = 1;
        }
    }

    printf("Ending program\n");

    return 0;
}