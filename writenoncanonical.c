// Non-canonical input processing

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B9600
#define _POSIX_SOURCE 1 // POSIX compliant source
#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

#define F 0x7E
#define A 0x05
#define C 0x03
#define BCC A^C


volatile int STOP = FALSE;

int alarmEnabled = FALSE; 
int alarmCount = 0; // n tentativas

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}


int main(int argc, char** argv)
{
    // Program usage: Uses either COM1 or COM2
    if ((argc < 2) ||
        ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
         (strcmp("/dev/ttyS1", argv[1]) != 0)))
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0], argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) {
        perror(argv[1]);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1) {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    bzero(&newtio, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred  to
    // by  fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Create string to send
    char buf[BUF_SIZE];
    char set[5] = { F, A, C, BCC, F };
    //printf("Escreva uma string: ");
    //scanf("%s", buf);

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // buf[25] = '\n';
    buf[strlen(buf)] = '\0';

    (void) signal(SIGALRM, alarmHandler);

    while (alarmCount < 4) {
        int bytes = write(fd, set, strlen(set));
        printf("%d bytes written\n", bytes);
        for(int i = 0; i < 5; i++) printf("%x ", set[i]);
        putchar('\n');
        if (alarmEnabled == FALSE) {
            alarm(3);  // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }
        char bufUA[5];
        int UAbytes = read(fd, bufUA, 5);
        char UA[5] = { F, 0x02, 0x07, 0x02^0x07, F};
        int count = 0;
        if(UAbytes == 5) {
            int x;
            for(x = 0; x < UAbytes; x++) return 1;
                if(bufUA[x] == UA[x]) count++;
        }

    }


    // The for() cycle and the following instructions should be changed in order to
    // follow specifications of the protocol indicated in the Lab guide.

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
