#include "linklayer.h"
#include <signal.h>
#include <unistd.h>
#include <time.h>

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

#define ESCAPE_CHAR 0x7D

struct termios oldtio;
struct termios newtio;

int fd;
struct linkLayer connection;
char Trama_lida;

int alarmEnabled = 1;
int alarmCount = 0;
int sendNumber = 0; // Ns
int readNumber = 0; // Rs

enum State { START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DONE };

int numberOfRRs = 0;
int numberOfREJs = 0;
int numberOfTimeOuts = 0;
int numberOfIframesSent = 0;
int numberOfIframesReceived = 0;

// @brief Set oldtio and newtio
// @return int - 1 if termios was configurated and -1 if occur error trying configurate termios
int configTermios();

// @brief Init link layer parameters
// @param connectionParameters: link layer settings
// @return void
void setConnectionParameters(linkLayer connectionParameters);

// @brief Start the alarm
// @return void
void startAlarm();

// @brief Activate the alarm
// @return void
void alarmHandler();

// @brief Generate supervision (S) and unnumbered (U) tramas
// @param dest: buffer to be filled with the generated SU trama
// @param frameA: Adrress field of a generic SU
// @param frameC: Control field of a generic SU
// @return int - 1 if success, -1 if error
int generateSUTrama(char* dest, char frameA, char frameC);

// @brief Generate information tramas
// @param tramaI: buffer to be filled with the generated I trama
// @param payload: payload to be introduced on the I trama
// @return int - length of I trama
int generateITrama(char* tramaI, char* payload, int payloadLength);

// @brief Generate RR and REJ based on Ns
// @param tramaRR: buffer to be filled with the generated RR trama
// @param tramaREJ: buffer to be filled with the generated REJ trama
// @param sendNumber: send number
// @return int - 1 if success, -1 if error
int generateRRandREJTramas(char* tramaRR, char* tramaREJ, int sendNumber);

// @brief Send generic trama
// @param fd: file descriptor
// @param trama: trama to be write in file
// @param length: length of the trama
// @return int - 1 if success, -1 if error
int sendTrama(int fd, char* trama, int length);

// @brief Send SET trama
// @param fd: file descriptor
// @return int - 1 if success, -1 if error
int sendSETTrama(int fd);

// @brief Get SET trama
// @param fd: file descriptor
// @return void
void getSETTrama(int fd);

// @brief Send a REJ trama
// @param control: control byte to create Control field
// @param fd: file descriptor
// @return int - length of REJ trama
int sendREJtrama(char control, int fd);

// @brief Send a RR trama
// @param control: control byte to create Control field
// @param fd: file descriptor
// @return int - length of RR trama
int sendRRtrama(char control, int fd);

// @brief Generate BCC2 frame
// @param buffer: data buffer used to be compared
// @param bufSize: buffer size
// @return char - generated BCC2
char generateBCC2Frame(char* buffer, int bufSize);

// @brief State machine (SET, UA, DISC)
// @param state: current state
// @param flag: flag to indicate when the state is changed
// @param A: address field
// @param C: control field
// @param BCC: block check character
// @return void
void stateMachine(enum State* state, char flag, char A, char C, char BCC);

// @brief Print an array in hexadecimal (debugging use only)
// @param array: array to be printed
// @param length: array size
// @param label: label to print
// @return void
void printArrayHex(char* array, int length, char* label);

// @brief Print protocol statistics
// @return void
void printStatistics();

// @brief Generate random error in data frame with a error prob = chance
// @param trama: trama to put the error
// @param length: trama size
// @param chance: probability to generate error
// @return int - error index position
int generateRandomError(char* trama, int length, int chance);

/***********************************************************************************/
/*                                  IMPLEMENTATION                                 */
/***********************************************************************************/

int configTermios() {
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

    return TRUE;
}

void setConnectionParameters(linkLayer connectionParameters) {
    sprintf(connection.serialPort, "%s", connectionParameters.serialPort);
    connection.role = connectionParameters.role;
    connection.numTries = connectionParameters.numTries;
    connection.timeOut = connectionParameters.timeOut;
    // connection.baudRate = connectionParameters.baudRate;
}

int llopen(linkLayer connectionParameters) {
    setConnectionParameters(connectionParameters);

    fd = open(connection.serialPort, O_RDWR | O_NOCTTY );
    if (fd < 0) {
        perror("Error: Cannot open the serial port.");
        return ERROR;
    }

    if(configTermios() == ERROR) {
        printf("Error: Cannot set termios structure.\n");
        return ERROR;
    }

    printf(connection.role == TRANSMITTER ? "Openning as transmitter\n" : "Openning as receiver\n");
    printf("New termios structure set\n");

    if(connection.role == NOT_DEFINED) {
        perror("Error: Role was not defined.\n");
        return ERROR;
    }

    if(connection.role == TRANSMITTER) {
        if(sendSETTrama(fd) == TRUE){ 
            return TRUE;

        }
    }

    if(connectionParameters.role == RECEIVER) {
        getSETTrama(fd);
        Trama_lida=C_SET;
        char ua[5];
        generateSUTrama(ua, A_RX, C_UA);
        if(sendTrama(fd, ua, 5) == ERROR) printf("Error: Cannot write UA\n");
        return TRUE;
    }

    return ERROR;
}

int llread(char *package){
    int frame_pos = 0, package_pos = 0, controlo, bytes_read = 0, flag_count = 0,flag_pos;
    char buffer[BUF_SIZE];
    char Bcc2 = 0x00;

    while(1) {
        int bytes_read = read(fd, buffer, BUF_SIZE);

        for (int i = 0 ; i < bytes_read; i++)
            if (buffer[i] == F) flag_count++;

        if (flag_count == 2) break;
        flag_count = 0;
    }
    numberOfIframesReceived++;

    while (buffer[frame_pos] != F) frame_pos++;

    while (buffer[frame_pos] == F) frame_pos++;  //avança até terminarem as flags no header(até o adress)
    

    frame_pos++;            // avança para o controlo do header
    controlo = frame_pos;

    if(buffer[controlo] != C_SET && buffer[controlo] != C_UA && buffer[controlo] != C_DISC)
        generateRandomError(buffer, bytes_read, 100);

    if(buffer[controlo] == Trama_lida) {      //verifica se a trama é repetida
        sendRRtrama(buffer[controlo], fd);   //se for envia ACK sem alterar o pacote
        return 0;
    }

    if(buffer[frame_pos]==C_DISC && buffer[frame_pos+1] == (BCC_DISC)) return ERROR;

    Trama_lida = buffer[controlo];   //grava o numero de sequência;
    frame_pos++;      //avança para a data do pacote

    if (buffer[frame_pos] != (buffer[controlo] ^ buffer[controlo - 1])) return ERROR;

    frame_pos++;
    flag_pos=frame_pos;

    while(buffer[flag_pos] != F) flag_pos++;


    while (frame_pos < flag_pos) {    //neste ciclo preenche-se o pacote com a data pretendida

        if ((frame_pos == flag_pos-2) && (buffer[frame_pos] == 0x7d)) { //faz o byte stuffing do controlo        
            frame_pos++;

            if (buffer[frame_pos] == 0x5e) {  // se o byte for 0x7e
                buffer[frame_pos] = F;
            } 
            else if (buffer[frame_pos]==0x5d) {  // se o byte for 0x7d
                buffer[frame_pos] = 0x7d;    
            }
        }

        else if (frame_pos ==  flag_pos-1) {   //se a data esta corrompida pede para o pacote ser enviado de novo;

            if (Bcc2 == buffer[frame_pos]) frame_pos++;

            else {
                memset(package, 0, package_pos+1);
                memset(buffer,0,bytes_read);
                sendREJtrama(buffer[controlo], fd);
                bytes_read = 0;
                Bcc2 = 0x00;
                frame_pos=0;
                flag_count=0;
                while (bytes_read==0) bytes_read = read(fd, buffer, MAX_PAYLOAD_SIZE); //fica constantemente a ler até obter um resultado de volta 
                for (int i = 0 ; i < bytes_read; i++)   if (buffer[i] == F) flag_count++;
                if(flag_count<2) return -1;
                numberOfIframesReceived++;
                while(buffer[frame_pos]!= F) frame_pos++;
                while(buffer[frame_pos]== F) frame_pos++;
                frame_pos++;
                if(buffer[frame_pos]!= Trama_lida) return -1;
                if(buffer[frame_pos+1]!= (buffer[frame_pos-1]^buffer[frame_pos])) return -1;
                package_pos = 0;
                frame_pos = controlo + 2;
                
            }
        } 

        else {

            if(buffer[frame_pos]==0x7d){
                frame_pos++;

                if (buffer[frame_pos] == 0x5e) {  // se o byte for 0x7e
                    package[package_pos] = F;
                } 
                else if (buffer[frame_pos] == 0x5d) {
                    package[package_pos] = 0x7d;
                }

            }
            
            else  package[package_pos] = buffer[frame_pos];
            Bcc2 ^= package[package_pos];
            frame_pos++;
            package_pos++;
        }
    }


    sendRRtrama(buffer[controlo], fd); //manda o ACK
    return package_pos;
}

int llwrite(char* buf, int bufSize) {
    char tramaI[2008], tramaRR[5], tramaREJ[5], buffer[2008];

    alarmCount = 0;

    int response, state = 0, done = 0;;
    int tramaILength = generateITrama(tramaI, buf, bufSize);

    generateRRandREJTramas(tramaRR, tramaREJ, sendNumber);
    startAlarm();

    while(!done) {
        switch(state) {
            case 0:
                if ((response = write(fd, tramaI, tramaILength)) > 0) 
                    numberOfIframesSent++;
                state = 1;
                break;
            case 1:
                
                if(alarmEnabled) {
                    alarm(connection.timeOut);
                    alarmEnabled = 0;
                    alarmCount++;
                } 
                
                response = read(fd, buffer, 5);

                if(response <= 0) {
                    if(alarmCount < connection.numTries) {
                        state = 0;
                        numberOfTimeOuts++;
                    } else return ERROR;
                } else state = 2;

                break;
            case 2:
                if (memcmp(tramaRR, buffer, 5) == 0) {
                    sendNumber ^= 1;
                    done = 1;
                    numberOfRRs++;
                } else if (memcmp(tramaREJ, buffer, 5) == 0) {
                    state = 0;
                    alarmCount++;
                    numberOfTimeOuts++;
                    numberOfREJs++;
                } else state = 0;
                break;
            default: break;
        }
    }
    return bufSize;
}

int llclose(int showStatistics) {
    int role = connection.role;
    char disc[5], ua[5];
    
    if(role == NOT_DEFINED) {
        perror("Error: role was not defined.\n");
        return ERROR;
    }

    if (role == TRANSMITTER) {
        char discRX[5];
        
        generateSUTrama(disc, A_TX, C_DISC);
        generateSUTrama(ua, A_TX, C_UA);
        generateSUTrama(discRX,A_RX,C_DISC);

        char buffer[5];
        alarmCount = 0;
        startAlarm();

        while(1) {
            if(alarmEnabled){
                if(sendTrama(fd, disc, 5) == ERROR)  {
                    printf("Error: Cannot send DISC\n");
                    return ERROR;
                }
                printf("Transmitter sent DISC\n");
                alarm(connection.timeOut);
                alarmEnabled = 0;
                alarmCount++;
                numberOfTimeOuts++;
                printf("Attempt %d\n", alarmCount);
            } else read(fd, buffer, 5);

            if(memcmp(buffer,discRX,5) == 0) break;
            
            if(alarmCount == connection.numTries) {
                close(fd);
                printf("Error: Cannot close the connection with success.\n");
                return ERROR;
            }
        }

        printf("Transmitter received DISC\n");

        if(sendTrama(fd, ua, 5) == ERROR) {
            printf("Cannot send UA\n");
            return ERROR;
        } else{
            printf("Transmitter sent UA\n");
            close(fd);
            printf("Closing the connection...\n");
            printStatistics();
            return TRUE;
        }
        
        return TRUE;
    }

    if (role == RECEIVER) {
        generateSUTrama(disc, A_RX, C_DISC);

        enum State state = START;
        char flag;
        int currentState = 0;
        while(state != DONE) {
            read(fd, &flag, 1);
            stateMachine(&state, flag, A_TX, C_DISC, BCC_DISC);
            currentState++;
        }
        if(state == DONE) {
            printf("Receiver received DISC\n");
            if(sendTrama(fd, disc, 5) == ERROR) {
                printf("Cannot write DISC\n");
                return ERROR;
            } else {
                printf("Receiver sent DISC\n");
                printf("Closing the connection...\n");
                printStatistics();
                return TRUE;
            }
        }
        return TRUE;
    }
    return TRUE;
}

void startAlarm() {
    (void) signal(SIGALRM, alarmHandler);
}

void alarmHandler() {
    alarmEnabled = 1;
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

int generateITrama(char* tramaI, char* payload, int payloadLength) {
    int currentPosition = 0;
    tramaI[currentPosition] = F;
    tramaI[++currentPosition] = A_TX;
    tramaI[++currentPosition] = (sendNumber == 0) ? C_I_S0 : C_I_S1;
    tramaI[++currentPosition] = (sendNumber == 0) ? BCC_I_S0 : BCC_I_S1;

    char bcc2 = generateBCC2Frame(payload, payloadLength);

    
    // byte stuffing on data

    for(int i = 0; i < payloadLength; i++) {
        switch(payload[i]) {
            case F:

                tramaI[++currentPosition] = ESCAPE_CHAR;
                tramaI[++currentPosition] = 0x5E;
                break;
            
            case ESCAPE_CHAR:
                tramaI[++currentPosition] = ESCAPE_CHAR;
                tramaI[++currentPosition] = 0X5D;
                break;
            
            default:
                tramaI[++currentPosition] = payload[i];
                break;
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

    currentPosition++;
    tramaI[currentPosition] = F;
    return currentPosition + 1;
}

int generateRRandREJTramas(char* tramaRR, char* tramaREJ, int sendNumber) {
    if(sendNumber != 0 && sendNumber != 1) return -1;
    switch(sendNumber) {
        case 0:
            generateSUTrama(tramaRR, A_RX, C_RR_R1);
            generateSUTrama(tramaREJ, A_RX, C_REJ_R0);
            break;

        case 1:
            generateSUTrama(tramaRR, A_RX, C_RR_R0);
            generateSUTrama(tramaREJ, A_RX, C_REJ_R1);
            break;
    }
    return 1;
}

int sendTrama(int fd, char* trama, int length) {
    if(write(fd, trama, length) < 0) return ERROR;
    else return TRUE;
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
            stateMachine(&state, flag, A_RX, C_UA, BCC_UA);
        }

        if(state == DONE) {
            printf("SET WAS SENT\n");
            return 1;
        }
        
        alarmCount++;
        printf("Attempt %d\n", alarmCount);
        numberOfTimeOuts++;
    } while (alarmCount < connection.numTries);

    return -1;
}

void getSETTrama(int fd) {
    enum State state = START;
    char flag;
    int currentState = 0;
    while(state != DONE) {
        read(fd, &flag, 1);
        stateMachine(&state, flag, A_TX, C_SET, BCC_SET);
        currentState++;
    }
}

int sendRRtrama(char control, int fd){

    control = !(control ^ 0x00);
    char C= C_RR_R0 + (control << 5);
    char buffer[5] = { F, A_RX , C , A_RX^C , F };
    int bytes_RR = write(fd, buffer, 5);
    if(bytes_RR < 0) return ERROR;
    numberOfRRs++;
    return bytes_RR;
}

int sendREJtrama(char control, int fd){

    char C = C_REJ_R0 + (control << 5);
    char buffer[5] = { F, A_RX , C , A_RX^C , F };
    int bytes_REJ = write(fd, buffer, 5);
    if(bytes_REJ < 0) return ERROR;
    numberOfREJs++;
    return bytes_REJ;
}

char generateBCC2Frame(char* buffer, int bufSize) {
    char bcc2 = 0x00;
    for (int i = 0; i < bufSize; i++) bcc2 ^= buffer[i];
    return bcc2;
}

void stateMachine(enum State* state, char flag, char A, char C, char BCC) {
    switch(*state) {
        case START: 
            if(flag == F) *state = FLAG_RCV;
            break;
        
        case FLAG_RCV: 
            if(flag == F) *state = FLAG_RCV;
            else if(flag == A) *state = A_RCV;
            else *state = START;
            break;
        
        case A_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == C) *state = C_RCV;
            else *state = START;
            break;
        
        case C_RCV:
            if(flag == F) *state = FLAG_RCV;
            else if(flag == (BCC)) *state = BCC_OK;
            else *state = START;            
            break;
        
        case BCC_OK:
            if(flag == F) *state = DONE;
            break;

        case DONE:
            break;
    }
}

void printArrayHex(char* array, int length, char* label) {
    putchar('\n');
    printf("%s: ", label);
    for(int i = 0; i < length; i++)
        printf("%.2x ", array[i]);
    putchar('\n');
}

void printStatistics() {
    int role = connection.role;
    printf("======================================\n");
    printf("File transmission statistics ");
    printf(role == TRANSMITTER ? "(Tx side):\n\n" : "(Rx side):\n\n");
    printf("Number of I frames transmitted: %d\n", numberOfIframesSent);
    printf("Number of I frames received: %d\n", numberOfIframesReceived);
    printf("Number of positive ACK: %d\n", numberOfRRs);
    printf("Number of negative ACK: %d\n", numberOfREJs);
    printf("Number of time-outs: %d\n", numberOfTimeOuts);
    printf("======================================\n");
}

int generateRandomError(char* trama, int length, int chance) {
    if(!trama) return ERROR;
    if(length <= 0) return ERROR;
    if(chance < 0 && chance > 100) return ERROR;
    
    srand(time(0));
    int error = rand() % 101; 
    int errorIndex;

    if (error < chance) { // if occur error
        do {
            errorIndex = rand() % (length - 3) + 1; // calculate the index to set error
        } while (trama[errorIndex] == ESCAPE_CHAR || trama[errorIndex] == F || trama[errorIndex] == 0x5D || trama[errorIndex] == 0x5E);
        trama[errorIndex] = 0x00;
    } 
    return errorIndex;
}
