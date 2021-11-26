#include "linklayer.h"
#include "helpers.h"
#include <signal.h>

#define ERROR -1

#define BUF_SIZE 2008

volatile int STOP = FALSE;
char read_data[BUF_SIZE];
int fd;
struct linkLayer connection;
char *Tramas_lidas[1000];
int read_count=0;


int llopen(linkLayer connectionParameters) { 

    connection.role = connectionParameters.role;
    connection.numTries =connectionParameters.timeOut;
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );
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

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
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
    printf(connectionParameters.role == TRANSMITTER ? "Openning as transmitter\n" : "Openning as receiver\n");
    printf("New termios structure set\n");

    if(connectionParameters.role == NOT_DEFINED) {
        perror("Error: role was not defined.\n");
        return ERROR;
    }

    if(connectionParameters.role == TRANSMITTER) {
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


    int frame_pos=0,package_pos=0,controlo;
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

            Tramas_lidas[read_count-1]= 0;
            read_count--;
            memset(package,0,package_pos+1);
            sendREJtrama(buffer[controlo],fd);
            return 0;

        }

        package[package_pos]=buffer[frame_pos];
        frame_pos++;
        package_pos++;
    }

    sendRRtrama(buffer[controlo],fd); //manda o ACK
    return package_pos ;
}


int llclose(int showStatistics) {
    int role = connection.role;

    if(role == NOT_DEFINED) {
        perror("Error: role was not defined.\n");
        return ERROR;
    }

    char disc[5] = { F, A_TX, C_DISC, BCC_DISC, F };
    char ua[5] = { F, A_RX, C_UA, BCC_UA, F };

    if (role == TRANSMITTER) {
        
    }

    if (role == RECEIVER) {

    }
    
    return 1;
}