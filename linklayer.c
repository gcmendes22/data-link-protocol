#include "linklayer.h"
#include <signal.h>
#include <unistd.h>

#define ERROR -1

#define BUF_SIZE 2008

// tramas
#define F 0x7E

#define A_TX 0x05
#define A_RX 0x02

#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B
#define C_RR_R0 0x01
#define C_RR_R1 0x21
#define C_REJ_R0 0x05
#define C_REJ_R1 0x25
#define C_REJ 0x05
#define C_I_S0 0x00 // C frame of I trama when SendNumber = 0
#define C_I_S1 0x02 // C frame of I trama when SendNumber = 1

#define BCC_SET A_TX^C_SET
#define BCC_UA A_RX^C_UA
#define BCC_DISC A_TX^C_DISC
#define BCC_I_S0 A_TX^C_I_S0 // BCC frame of I trama when SendNumber = 0
#define BCC_I_S1 A_TX^C_I_S1 // BCC frame of I trama when SendNumber = 1

#define ACK_A 0x02

#define ESCAPE_CHAR 0x7D


volatile int STOP = FALSE;
char read_data[BUF_SIZE];
int fd;
struct linkLayer connection;
char Tramas_lidas[1000];
int read_count=0;

int alarmEnabled = 1;
int alarmCount = 0;
int sendNumber = 0;

enum State { START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DONE };

// Start the alarm
void startAlarm();

// Activate the alarm
void alarmHandler(int signal);

// Generate supervision and unnumbered tramas
int generateSUTrama(char* dest, char frameA, char frameC);

// Generate information tramas
int generateITrama(char* tramaI, char* buffer, int bufSize);

// Generate RR and REJ based on Ns
int generateRRandREJTramas(char* tramaRR, char* tramaREJ, int sendNumber);

int sendUATrama(int fd);

int sendSETTrama(int fd);

void getSETTrama(int fd);

void sendREJtrama(char controlo,int fd);

void sendRRtrama(char controlo,int fd);

int sendDISCTrama(int fd);

int getDISCTrama(int fd);

// Generate BCC2 frame
char generateBCC2Frame(char* buffer, int bufSize);

void stateMachineSETMessage(enum State* state, char flag);

void stateMachineUAMessage(enum State* state, char flag);

void stateMachineDISCMessageTransmitter(enum State* state, char flag);

void stateMachineDISCMessageReceiver(enum State* state, char flag);

void printArrayHex(char* array, int length);

/***********************************************************************************/
/*                                  IMPLEMENTATION                                 */
/***********************************************************************************/

int llopen(linkLayer connectionParameters) {
    sprintf(connection.serialPort, "%s", connectionParameters.serialPort);
    connection.role = connectionParameters.role;
    connection.numTries = connectionParameters.numTries;
    connection.timeOut = connectionParameters.timeOut;

    fd = open(connection.serialPort, O_RDWR | O_NOCTTY );
    if (fd < 0) {
        perror("Error: cannot open the serial port.");
        return ERROR;
    }

    struct termios oldtio;
    struct termios newtio;

    if (tcgetattr(fd, &oldtio) == -1) {
        perror("tcgetattr");
        return ERROR;
    }

    bzero(&newtio, sizeof(newtio));

    newtio.c_cflag = connection.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 5;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        return ERROR;
    }
    printf(connection.role == TRANSMITTER ? "Openning as transmitter\n" : "Openning as receiver\n");
    printf("New termios structure set\n");

    if(connection.role == NOT_DEFINED) {
        perror("Error: role was not defined.\n");
        return ERROR;
    }

    if(connection.role == TRANSMITTER) {
        if(sendSETTrama(fd) == TRUE){ 
            return TRUE;
        }
    }

    if(connectionParameters.role == RECEIVER) {
        getSETTrama(fd);
        sendUATrama(fd);
        return TRUE;
    }

    return ERROR;
}

int llread(char *package){


    int frame_pos=0,package_pos=0,controlo,stuffing_pos;
    char buffer[BUF_SIZE];
    int bytes_read = read(fd,buffer,MAX_PAYLOAD_SIZE);

    if(bytes_read <= 0) return -1;

    if(buffer[0] != F) return -1; //Vê se o primeiro elemento é a flag pretendida

    while(buffer[frame_pos] == F) frame_pos++;  //avança até terminarem as flags no header(até o adress)

    if(buffer[frame_pos] != 0x05) return -1; //verifica se o adress é o pretendido

    frame_pos++;            // avança para o controlo do header
    controlo=frame_pos;         

    for(int i=0;i<read_count;i++){      //verifica se a trama é repetida

        if(buffer[controlo]==Tramas_lidas[i])       //se for envia ACK sem alterar o pacote
        {
            sendRRtrama(buffer[controlo],fd);
            return 0;
        }
    }

    read_count++;
    realloc(Tramas_lidas,sizeof(char)*read_count);
    Tramas_lidas[read_count-1] = buffer[controlo];   //grava o numero de sequência;

    frame_pos=frame_pos+2;      //avança para a data do pacote
    
    
    while(buffer[frame_pos+1] != F){    //neste ciclo preenche-se o pacote com a data pretendida

        if(frame_pos ==  bytes_read){   //se a data esta corrompida pede para o pacote ser enviado de novo;
            
            memset(package,0,package_pos+1);
            sendREJtrama(buffer[controlo],fd);
            bytes_read=0;
            while(bytes_read<=0) bytes_read=read(fd,buffer,MAX_PAYLOAD_SIZE); //fica constantemente a ler até obter um resultado de volta
            package_pos=0;
            frame_pos=controlo+2;
        }

        package[package_pos]=buffer[frame_pos];
        frame_pos++;
        package_pos++;
    }

    for(int i=0;i<package_pos;i++){   // repara todos os possiveis bytes alterados
        if(package[i]==0x7d){
            i++;

            if(package[i]==0x5e){  // se o byte for 0x7e
                package[i-1]=F;
                stuffing_pos=i;
                i++;

                while(i<package_pos){ // se o byte for 0x7d
                 
                    package[i-1]=package[i];
                    i++;

                }
                package_pos--;
                i=stuffing_pos;
            }

            else if(package[i]==0x5d){
                stuffing_pos=i;
                i++;

                while(i<package_pos){ // coloca as posiçôes todas um passo para a diretia
                 
                    package[i-1]=package[i];
                    i++;

                }

                package_pos--;
                i=stuffing_pos;
            }
        }

    }

    sendRRtrama(buffer[controlo],fd); //manda o ACK
    return package_pos;
}

int llwrite(char* buf, int bufSize) {
    char tramaI[1024], tramaRR[5], tramaREJ[5], buffer[2008];
    int response, tries = 0, state = 0, done = 0;;
    int tramaILength = generateITrama(tramaI, buf, bufSize);

    generateRRandREJTramas(tramaRR, tramaREJ, sendNumber);

    while(!done) {
        switch(state) {
            case 0:
                response = write(fd, tramaI, tramaILength);
                state = 1;
                break;
            case 1:
                response = read(fd, buffer, 1);
                if(response == -1) {
                    if(tries < connection.numTries) {
                        tries++;
                        state = 0;
                    } else {
                        return ERROR;
                    }
                } else state = 2;
                break;
            case 2:
                if (memcmp(tramaRR, buffer, 5) == 0) {
                    sendNumber ^= 1;
                    done = 1;
                } else if (memcmp(tramaREJ, buffer, 5) == 0) {
                    tries = 0;
                    state = 0;
                } else {
                    tries++;
                    state = 0;
                }
                break;
            default: break;
        }
    }
    
    return response;
}

int llclose(int showStatistics) {
    int role = connection.role;
    char disc[5], ua[5], buffer[2008];
    int state = 0, done = 0, tries = 0;

    if(role == NOT_DEFINED) {
        perror("Error: role was not defined.\n");
        return ERROR;
    }

    generateSUTrama(disc, A_TX, C_DISC);
    generateSUTrama(ua, A_TX, C_UA);

    if (role == TRANSMITTER) {
        while(!done) {
            switch(state) {
                case 0: 
                    if(write(fd, disc, 5) < 0) {
                        printf("ERROR: Cannot write DISC\n");
                        return ERROR;
                    }
                    break;
                case 1:
                    if(read(fd, buffer, 1) < 0) {
                        if(tries < connection.numTries) {
                            tries++;
                            state = 0;
                        } else {
                            printf("Number of tries exceeded the limit...\n");
                            return -1;
                        }
                    } else {
                        if (memcmp(buffer, disc, 5) == 0) {
                            state = 2;
                        } else state = 0;
                    }
                    break;
                case 2:
                    if(write(fd, ua, 5) < 0) {
                        printf("ERROR: Cannot write UA\n");
                        return ERROR;
                    } else done = 1;
                default: break;
            }
        }
        return TRUE;
    }

    if (role == RECEIVER) {
        while(!done) {
            switch(state) {
                case 0: 
                    if(write(fd, disc, 5) < 0) {
                        printf("ERROR: Cannot write DISC\n");
                        return ERROR;
                    }
                    break;
                case 1:
                    if(read(fd, buffer, 1) < 0) {
                        if(tries < connection.numTries) {
                            tries++;
                            state = 0;
                        } else {
                            printf("Number of tries exceeded the limit...\n");
                            return -1;
                        }
                    } else {
                        if (memcmp(buffer, ua, 5) == 0) {
                            done = 1;
                        } else state = 0;
                    }
                    break;
                default: break;
            }
        }

        return TRUE;
    }
    return TRUE;
}

void startAlarm() {
    (void) signal(SIGALRM, alarmHandler);
}

void alarmHandler(int signal) {
    alarmEnabled = 1;
    printf("Attempt %d\n", alarmCount + 1);
}

int generateSUTrama(char* dest, char frameA, char frameC) {
    if(!frameA || !frameC) return -1;

    dest[0] = F;
    dest[1] = frameA;
    dest[2] = frameC;
    dest[3] = dest[1]^dest[2];
    dest[4] = F;

    return 1;
}

int generateITrama(char* tramaI, char* buffer, int bufSize) {
    int currentPosition = 0;
    tramaI[currentPosition] = F;
    tramaI[++currentPosition] = A_TX;
    tramaI[++currentPosition] = (sendNumber == 0) ? C_I_S0 : C_I_S1;
    tramaI[++currentPosition] = (sendNumber == 0) ? BCC_I_S0 : BCC_I_S1;

    char bcc2 = generateBCC2Frame(buffer, bufSize);
    
    // byte stuffing on data

    for(int i = 0; i < bufSize; i++) {
        switch(buffer[i]) {
            case F:
                tramaI[++currentPosition] = ESCAPE_CHAR;
                tramaI[++currentPosition] = 0x5E;
                break;
            
            case ESCAPE_CHAR:
                tramaI[++currentPosition] = ESCAPE_CHAR;
                tramaI[++currentPosition] = 0X5D;
                break;
            
            default:
                tramaI[++currentPosition] = buffer[i];
        }    
    }

    // byte stuffing on bcc frame

    switch(bcc2) {
        case F:
            tramaI[++currentPosition] = ESCAPE_CHAR;
            tramaI[++currentPosition] = 0x5E;
            break;

        case ESCAPE_CHAR:
            tramaI[++currentPosition] = ESCAPE_CHAR;
            tramaI[++currentPosition] = 0X5D;
            break;
        
        default: 
            tramaI[++currentPosition] = bcc2;
    }

    return currentPosition;
}

int generateRRandREJTramas(char* tramaRR, char* tramaREJ, int sendNumber) {
    if(sendNumber != 0 && sendNumber != 1) return -1;
    switch(sendNumber) {
        case 0:
            generateSUTrama(tramaRR, A_TX, C_RR_R0);
            generateSUTrama(tramaREJ, A_TX, C_REJ_R0);
            break;
    }
    return 1;
}

int sendUATrama(int fd) {
    printf("UA was sent\n");
    char ua[5];
    generateSUTrama(ua, A_RX, C_UA);
    int length = write(fd, ua, 5);
    if(length < 0) return -1;
    printf("%d bytes written\n", length);
    return 1;
}

int sendSETTrama(int fd) {
    char set[5];
    generateSUTrama(set, A_TX, C_SET);
    char flag;
    enum State state = START;
    startAlarm();
    do {
        write(fd, set, 5);

        if(alarmEnabled) {
            alarm(connection.timeOut);
            alarmEnabled = 0;
        }

        while(state != DONE && alarmEnabled == 0) {
            read(fd, &flag, 1);
            stateMachineSETMessage(&state, flag);
        }

        if(state == DONE) {
            printf("SET WAS SENT\n");
            return 1;
        }

        alarmCount++;
    } while (alarmCount < connection.numTries);

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

int sendDISCTrama(int fd) {
    printf("DISC was sent\n");
    char disc[5];
    generateSUTrama(disc, A_RX, C_DISC);
    int length = write(fd, disc, 5);

    if(length < 0) return -1;
    printf("%d bytes written\n", length);
    return 1;
}

int getDISCTrama(int fd) {

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

void sendRRtrama(char controlo,int fd){

    char C= C_RR_R0 + (controlo << 4);
    char buffer[5] = { F, ACK_A , C , ACK_A^C , F };
    int bytes_RR = write(fd, buffer, 5);
    printf("bytes enviados %d\n",bytes_RR);

}

void sendREJtrama(char controlo,int fd){

    char C = C_REJ_R0 + (controlo << 4);
    char buffer[5] = { F, ACK_A , C , ACK_A^C , F };
    int bytes_REJ = write(fd, buffer, 5);
    printf("bytes enviados %d\n",bytes_REJ);
    
}

char generateBCC2Frame(char* buffer, int bufSize) {
    char bcc2 = 0x00;
    for (int i = 0; i < bufSize; i++) bcc2 ^= buffer[i];
    return bcc2;
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

void printArrayHex(char* array, int length) {
    putchar('\n');
    for(int i = 0; i < length; i++)
        printf("%.2x ", array[i]);
    putchar('\n');
}