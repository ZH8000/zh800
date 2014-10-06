#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <time.h>

#include <pthread.h>
#include <assert.h>

#include <wiringPi.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <errno.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <net/if.h>

#define I2C_IO_Extend_1 0x20
#define I2C_IO_Extend_2 0x21
#define I2C_IO_Extend_3 0x22

#define WiringPiPIN_11 0
#define WiringPiPIN_12 1
#define WiringPiPIN_13 2
#define WiringPiPIN_15 3
#define WiringPiPIN_16 4
#define WiringPiPIN_18 5
#define WiringPiPIN_22 6
#define WiringPiPIN_24 10

#define IN_P0 0x00
#define IN_P1 0x01
#define OUT_P0 0x02
#define OUT_P1 0x03
#define INV_P0 0x04
#define INV_P1 0x05
#define CONFIG_P0 0x06
#define CONFIG_P1 0x07

#define WatchDogCountValue 120
#define InputLength 20
#define UPLoadFileLength 21
#define CountPeriod 4
#define WriteFileCountValue 4200
#define SHMSZ 2000
#define WriteFilePerSize 300
//#define RS232_Length 55
//#define AgeCountNumber 5
#define goodrate 1

#define FTPCountValue 300
#define FTPWakeUpValue 60

#define ZHNetworkType "eth0"

#define Log(s,func, line, opt) StringCat(func);StringCat(opt)
//#define Logmode
#define PrintInfo

enum{
    MechSTART = 1,
    MechSTOP,
    MechRUNNING,
    MechREPAIR,
    MechHUMAN,
    MechLOCK,
    MechUNLOCK,
    MechEND
};

short zhInterruptEnable = 0;
short zhResetFlag = 0;
short zhTelnetFlag = 0;
short FTPFlag = 0;
short FileFlag = 0;
short WatchDogFlag = 0;
short updateFlag = 0;

char *shm, *s, *tail;
char *shm_pop;
int PrintLeftLog = 0;

long PINCount[6][8];
long PINEXCount[6][8];

int I2CEXValue[6];
short CutRoll[2];

pthread_cond_t cond, condFTP;
pthread_mutex_t mutex, mutex_2, mutex_log, Mutexlinklist, mutexFTP;

typedef struct INPUTNODE
{
  char ISNo[InputLength];
  char ManagerCard[InputLength];
  char MachineCode[InputLength];
  char UserNo[InputLength];
  char CountNo[InputLength];
  char UPLoadFile[UPLoadFileLength];
  
  struct InputNode *link;

} InputNode;

InputNode *list = NULL;

void * zhLogFunction(void *argument);
void StringCat(const char *str);
void * FTPFunction(void *argument);
void * FileFunction(void *argument);
void * WatchDogFunction(void *argument);
void * BarcodeInputFunction(void *argument);
void * zhINTERRUPT1(void *argument);

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);

void StringCat(const char *str)
{
    int x = strlen(str);
    int count;
    pthread_mutex_lock(&mutex_log);
    for(count = 0; count < x; count++)
    {
        *s = *str;
        //printf("%c",*s);
        s++;
        str++;
        if(s >= tail) {
            s = shm +1;
        }
    }
    pthread_mutex_unlock(&mutex_log);
}

void * RemoteControl(void *argument)
{
    int socket_desc , new_socket , c;
    struct sockaddr_in server , client;
    int iSetOption = 1;
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
    //Bind
    //int x =  bind(socket_desc,(struct sockaddr *)&server , sizeof(server));
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)))
    //if( x < 0)
    {
        printf("%d ", errno);
        puts("bind failed");
        pthread_exit(NULL);
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    while( zhTelnetFlag == 1 && (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted\n");
        zhTelnetFlag = 0;
        break;
    }
    printf("Telnet close\n"); 
    /*if(close(socket_desc) < 0)
    {
        printf("erron %d",errno);
    }*/
    close(socket_desc);
    //free(socket_desc);
    sleep(1);
}

void * zhINTERRUPT1(void * argument)
{
    while(zhInterruptEnable)
    {
        char *dev = "/dev/i2c-1";

        int fd , r;
    
        fd = open(dev, O_RDWR);
    
        if(fd < 0)
        {
            perror("Open fail");
            exit(0);
        }

        r = ioctl(fd, I2C_SLAVE, I2C_IO_Extend_1);
        if (r < 0)
        {
            perror("Selecting i2c device\n");
            exit(0);
        }
        int x, y;
        int ForCount ,first, second;
    
        x = i2c_smbus_read_byte_data(fd, IN_P0);
        y = i2c_smbus_read_byte_data(fd, IN_P1);
        close(fd);
        
        if(I2CEXValue[0] != x || I2CEXValue[1] != y)
        {
            first = I2CEXValue[0] ^ 0xff;
            first = first & x;
            second = I2CEXValue[1] ^ 0xff;
            second = second & y;

            I2CEXValue[0] = x;
            I2CEXValue[1] = y;

            for(ForCount = 0; ForCount < 8; ++ForCount)
            {
                if(ForCount > 1)
                {
                    PINCount[1][ForCount] = PINCount[1][ForCount] + (second & 1);
                    if(ForCount == 5 && (first & 1) == 1)
                    {
                        CutRoll[0] = 1;
                    }
                    else if(ForCount == 6 && (first & 1) == 1)
                    {
                        CutRoll[1] = 1;
                    }
                    else if(ForCount == 7 && (first & 1) == 1)
                    {
                        PINCount[0][5] = PINCount[0][5] + CutRoll[0];
                        PINCount[0][6] = PINCount[0][6] + CutRoll[1];
                    
                        CutRoll[0] = CutRoll[1] = 0;
                    }
            }
            first = first >> 1;
            second = second >> 1;
        }
#ifdef PrintInfo 
        printf("reader 1: %3d, %3d | %ld %ld %ld %ld %ld %ld %ld %ld || %ld %ld %ld %ld %ld %ld %ld %ld \n",
                x , y, PINCount[0][0], PINCount[0][1], PINCount[0][2], 
                PINCount[0][3], PINCount[0][4], PINCount[0][5], PINCount[0][6], PINCount[0][7],
                PINCount[1][0], PINCount[1][1], PINCount[1][2], PINCount[1][3], PINCount[1][4], PINCount[1][5], PINCount[1][6], PINCount[1][7]);
#endif
        }
    }
}

void * zhLogFunction(void *argument)
{
    int shmid;
    long size;
    key_t key;
    char *s_pop;
    struct shmid_ds shmid_ds;
    struct timeval now;
    struct tm ts;
    struct stat st;
    char LogFileLocation[80];
    char LogString[300];
    int WriteFileCount = 0;
    char buff[40];

    gettimeofday(&now, NULL);
    ts = *localtime(&now.tv_sec);
    strftime(buff, sizeof(buff), "%Y/%m/%d_%H:%M:%S", &ts);
    
    key = 5678;
    if((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0)
    {
        perror("shmget");
        pthread_exit(NULL);
    }
    if((shm_pop = shmat(shmid, NULL, 0)) == (char *)-1)
    {
        perror("shmat");
        pthread_exit(NULL);
    }
    s_pop = shm_pop + 1;
    int count;

    for(count = 0 ; count < SHMSZ -1 ; count ++)
    {
        *s_pop = NULL;
        s_pop++;
    }
    s_pop = shm_pop + 1;
    //char *tail1 = shm_pop + (SHMSZ -1);
    tail = shm_pop + (SHMSZ -1);
    memset(LogString, 0, sizeof(char)*300);
    sprintf(LogFileLocation,"/media/usb0/Log_%ld.txt",(long)now.tv_sec);
    FILE *pfile;
    pfile = fopen(LogFileLocation, "a");
    fprintf(pfile,"CS-210\t%s\t0\t", buff);
    fclose(pfile);
    while(*shm_pop != '*')
    {
        if(*s_pop == NULL)
        {
            //usleep(5000000);
            usleep(500000);
            continue;
        }
        
        if(WriteFileCount == 299 || PrintLeftLog == 1)
        {
            printf("ready to write log\n");
            pfile = fopen(LogFileLocation,"a");
            
            int ForCount;
            for(ForCount = 0; ForCount <= WriteFileCount; ForCount++)
            {
                fprintf(pfile, "%c", LogString[ForCount]);
                if(LogString[ForCount] == '\n')
                {
                    gettimeofday(&now, NULL);
                    ts = *localtime(&now.tv_sec);
                    strftime(buff, sizeof(buff), "%Y/%m/%d_%H:%M:%S", &ts);
                    fprintf(pfile,"CS-210\t%s\t%ld\t",buff,PINCount[1][6]);
                }
            }      
    
            fclose(pfile);
            
            memset(LogString, 0, sizeof(char)*300);
            WriteFileCount = 0;
        }
        strncat(LogString, s_pop, 1);
        WriteFileCount++;
        *s_pop = NULL;
        if(s_pop >= tail)
        {
            s_pop = shm_pop + 1;
        }
        else s_pop++;

        stat(LogFileLocation, &st);
        size = st.st_size;
        if(size > 100000)
        {
            gettimeofday(&now, NULL);
            sprintf(LogFileLocation,"/media/usb0/Log_%ld.txt",(long)now.tv_sec);
        }
    }
    shmdt(shm_pop);
    shmctl(shmid, IPC_RMID, &shmid_ds);
}

void * FileFunction(void *argument)
{
#ifdef LogMode
    Log(s, __func__, __LINE__, " entry\n");
#endif
    struct timeval now;
    struct timespec outtime;
    struct ifreq ifr;

    int fd;

    long size; 
    int ForCount, ForCount2;
    FILE * pfile;

    while(FileFlag)
    {
        pthread_mutex_lock(&mutex);

        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + CountPeriod;
        outtime.tv_nsec = now.tv_usec * 1000; 

        pthread_cond_timedwait(&cond, &mutex, &outtime);
        pthread_mutex_unlock(&mutex);

#ifdef LogMode
        Log(s, __func__, __LINE__, " WatchDog wake\n");
#endif
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, ZHNetworkType, IFNAMSIZ-1);
        ioctl(fd, SIOCGIFADDR, &ifr);
        close(fd);
        
        pthread_mutex_lock(&Mutexlinklist);
        InputNode *p = list;
       
        if(p != NULL)
        { 
            pfile = fopen(p->UPLoadFile, "a");
            for(ForCount = 0; ForCount < 2; ++ForCount)
            {
                for(ForCount2 = 0; ForCount2 < 8; ++ForCount2)
                {
                    if(PINEXCount[ForCount][ForCount2] != PINCount[ForCount][ForCount2] && ForCount2 == 6 && ForCount == 1)
                    {
                        fprintf(pfile, "%s\t%s\t%s\t%ld\t%ld\t0\t%s\t%d\t%s\t%s\t0\t0\t0\t%02d\t%02d\n", p->ISNo, p->ManagerCard, p->CountNo, PINCount[1][6], (long)now.tv_sec,
                                                                                               inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), 
                                                                                               ForCount * 8 + ForCount2 + 2, p->MachineCode, p->UserNo, 
                                                                                               MechRUNNING, MechSTART);
                    }
                    else if(PINEXCount[ForCount][ForCount2] != PINCount[ForCount][ForCount2])
                    {    
                        fprintf(pfile, "%s\t%s\t%s\t%ld\t%ld\t%ld\t%s\t%d\t%s\t%s\t0\t0\t0\t%02d\t%02d\n", p->ISNo, p->ManagerCard, p->CountNo, PINCount[1][6], 
                                                                                               (long)now.tv_sec,  PINCount[ForCount][ForCount2],
                                                                                               inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), 
                                                                                               ForCount * 8 + ForCount2 + 2, p->MachineCode, p->UserNo, 
                                                                                               MechRUNNING, MechSTART);
                    }
                }
            }
            size = ftell(pfile);        
            fclose(pfile);
        }
        pthread_mutex_unlock(&Mutexlinklist);

        printf("%s %s %s %s %s %ld|| %ld %ld %ld %ld %ld %ld %ld %ld || %ld %ld %ld %ld %ld %ld %ld %ld \n",
                p->ISNo, p->ManagerCard, p->MachineCode, p->UserNo, p->CountNo, size, 
                PINCount[0][0], PINCount[0][1], PINCount[0][2], PINCount[0][3], PINCount[0][4], PINCount[0][5], PINCount[0][6], PINCount[0][7],
                PINCount[1][0], PINCount[1][1], PINCount[1][2], PINCount[1][3], PINCount[1][4], PINCount[1][5], PINCount[1][6], PINCount[1][7]);
        
        memcpy(PINEXCount, PINCount, sizeof(long)*40);
    }
}
int main(int argc, char *argv[])
{
    char inputString[InputLength] , *tempPtr;
    int fd, r, rc;
    char *dev = "/dev/i2c-1";

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutex_2, NULL);
    pthread_mutex_init(&mutex_log, NULL);
    pthread_mutex_init(&Mutexlinklist, NULL);
    pthread_mutex_init(&mutexFTP, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_cond_init(&condFTP, NULL);

    pthread_t barcodeInputThread, watchdogThread;

    wiringPiSetup(); 

    pinMode(WiringPiPIN_15, OUTPUT);
    pinMode(WiringPiPIN_16, OUTPUT);
    pinMode(WiringPiPIN_18, OUTPUT);
    pinMode(WiringPiPIN_22, INPUT);
    pinMode(WiringPiPIN_24, INPUT);
    pullUpDnControl (WiringPiPIN_22, PUD_UP); 
    pullUpDnControl (WiringPiPIN_24, PUD_UP); 

    fd = open(dev, O_RDWR);
    if(fd < 0)
    {
       perror("Open Fail");
       return 1;
    }
    r = ioctl(fd, I2C_SLAVE, I2C_IO_Extend_1);
    if(r < 0)
    {
       perror("Selecting i2c device");
       return 1;
    }
    i2c_smbus_write_byte_data(fd, OUT_P0, 0x00);
    i2c_smbus_write_byte_data(fd, INV_P0, 0x00);
    i2c_smbus_write_byte_data(fd, CONFIG_P0, 0x00);

    i2c_smbus_write_byte_data(fd, OUT_P1, 0x00);
    i2c_smbus_write_byte_data(fd, INV_P1, 0xFF);
    i2c_smbus_write_byte_data(fd, CONFIG_P1, 0xFF); 

    i2c_smbus_read_byte_data(fd, IN_P0);
    i2c_smbus_read_byte_data(fd, IN_P1);
    close(fd);
         
    fd = open(dev, O_RDWR);
    if(fd < 0)
    {
        perror("Open Fail");
        return 1;
    }
    r = ioctl(fd, I2C_SLAVE, I2C_IO_Extend_2);
    if(r < 0)
    {
       perror("Selection i2c device fail");
       return 1;
    }
    i2c_smbus_write_byte_data(fd, OUT_P0, 0x00);
    i2c_smbus_write_byte_data(fd, INV_P0, 0x00);
    i2c_smbus_write_byte_data(fd, CONFIG_P0, 0x00);

    i2c_smbus_write_byte_data(fd, OUT_P1, 0x00);
    i2c_smbus_write_byte_data(fd, INV_P1, 0x00);
    i2c_smbus_write_byte_data(fd, CONFIG_P1, 0x00);
    i2c_smbus_read_byte_data(fd, IN_P0);
    i2c_smbus_read_byte_data(fd, IN_P1);
    close(fd);

    fd = open(dev, O_RDWR);
    if(fd < 0)
    {
        perror("Open Fail");
        return 1;
    }
    r = ioctl(fd, I2C_SLAVE, I2C_IO_Extend_3);
    if(r < 0)
    {
         perror("Selection i2c device fail");
         return 1;
    }
    i2c_smbus_write_byte_data(fd, OUT_P0, 0x00);
    i2c_smbus_write_byte_data(fd, INV_P0, 0x00);
    i2c_smbus_write_byte_data(fd, CONFIG_P0, 0x00);
        
    i2c_smbus_write_byte_data(fd, OUT_P1, 0x00);
    i2c_smbus_write_byte_data(fd, CONFIG_P1, 0x00);
    i2c_smbus_read_byte_data(fd, IN_P0);
    i2c_smbus_read_byte_data(fd, IN_P1);
    close(fd);
   
    //reset count value and other;
    memset(PINCount, 0, sizeof(long)*48);
    memset(PINEXCount, 0, sizeof(long)*40);
    memset(I2CEXValue, 0, sizeof(int)*6);
    memset(CutRoll, 0, sizeof(short)*2);


    WatchDogFlag = 1; 
    rc = pthread_create(&watchdogThread, NULL, WatchDogFunction, NULL);
    assert(rc == 0);

    while(1)
    {
        rc = pthread_create(&barcodeInputThread, NULL, BarcodeInputFunction, NULL);
        assert(rc == 0);

        while(list == NULL)
        {
            ;
        }

        while(digitalRead(WiringPiPIN_24) == 0)
        {
            usleep(100000);
        }
        printf("ready to cancel barcode\n");
        pthread_cancel(barcodeInputThread);
        pthread_join(barcodeInputThread, NULL);
        printf("cancel done\n");
        while(list != NULL)
        {
            digitalWrite (WiringPiPIN_15, HIGH);
            digitalWrite (WiringPiPIN_16, HIGH);
            digitalWrite (WiringPiPIN_18, LOW);

            memset(inputString, 0, sizeof(char)*InputLength);
            gets(inputString);
            if(strncmp(inputString, "XXX", 3) == 0)
            {
                pthread_mutex_lock(&Mutexlinklist);
                InputNode *p = list;
                while(p != NULL)
                {
                    memset(p->UserNo, 0, sizeof(char)*InputLength);
                    tempPtr = inputString + 3;
                    memcpy(p->UserNo, tempPtr, sizeof(inputString)-2);
                    printf("%s %s %s %s %s %s\n", p->ISNo, p->ManagerCard, p->MachineCode, p->UserNo, p->CountNo, p->UPLoadFile);
                    p = p->link;
            
                    digitalWrite (WiringPiPIN_15, LOW);
                    digitalWrite (WiringPiPIN_16, HIGH);
                    digitalWrite (WiringPiPIN_18, LOW);

                }
                pthread_mutex_unlock(&Mutexlinklist);
                break;
            }
            printf("UserNo scan error code\n");
        }
    } 

    return 0;
}

void * BarcodeInputFunction(void *argument)
{
    char *dev = "/dev/i2c-1";

    int fd;
    long goodCount;
    char tempString[InputLength], *tempPtr;
    struct timeval now;
    FILE *pfile;

    /*scanner check
    * 1. ISNO
    * 2. manager card
    * 3. machine code
    * 4. user No
    * 5. Count No
    */

    //the mechine always standby
    while(1)
    {
        InputNode *node;
        node = (InputNode *) malloc(sizeof(InputNode));
        if(node == NULL)
        {
            continue;
        }

        node->link = NULL;

        digitalWrite (WiringPiPIN_15, HIGH);
        digitalWrite (WiringPiPIN_16, HIGH);
        digitalWrite (WiringPiPIN_18, HIGH);

        /*fd = open(dev, O_RDWR);
        if(fd < 0)
        {
            perror("Open Fail");
            return 1;
        }
        r = ioctl(fd, I2C_SLAVE, I2C_IO_Extend_3);
        if(r < 0)
        {
            perror("Selection i2c device fail");
            return 1;
        }
        i2c_smbus_write_byte_data(fd, OUT_P0, 0x00);
        i2c_smbus_write_byte_data(fd, CONFIG_P0, 0x03);
        
        i2c_smbus_write_byte_data(fd, OUT_P1, 0x00);
        i2c_smbus_write_byte_data(fd, CONFIG_P1, 0x00);
        close(fd);*/

        //3rd i3c board will control 3*8 control
        printf("Ready to work!!\n");
        while(1)
        {
            memset(tempString, 0, sizeof(char)* InputLength);
            gets(tempString);
            if(strncmp(tempString, "YYY", 3) == 0)
            {
                memset(node->ISNo, 0, sizeof(char)*InputLength);
                tempPtr = tempString + 3;
                memcpy(node->ISNo, tempPtr, sizeof(tempString)-2);
                // ready for light some led;
                digitalWrite (WiringPiPIN_15, LOW);
                digitalWrite (WiringPiPIN_16, HIGH);
                digitalWrite (WiringPiPIN_18, HIGH);
                break;
            }
            printf("ISNo scan error code\n");
        }
        while(1)
        {
            memset(tempString, 0, sizeof(char)*InputLength);
            gets(tempString);
            if(strncmp(tempString, "QQQ", 3) == 0)
            {
                memset(node->ManagerCard, 0, sizeof(char)*InputLength);
                tempPtr = tempString + 3;
                memcpy(node->ManagerCard, tempPtr, sizeof(tempString)-2);
                digitalWrite (WiringPiPIN_15, HIGH);
                digitalWrite (WiringPiPIN_16, LOW);
                digitalWrite (WiringPiPIN_18, HIGH);
                break;
            }
            printf("ManagerCard scan error code\n");
        }
        while(1)
        {
            memset(tempString, 0 , sizeof(char)*InputLength);
            gets(tempString);
            if(strncmp(tempString, "ZZZ", 3) == 0)
            {
                memset(node->MachineCode, 0 , sizeof(char)*InputLength);
                tempPtr = tempString + 3;
                memcpy(node->MachineCode, tempPtr, sizeof(tempString)-2);
                digitalWrite (WiringPiPIN_15, LOW);
                digitalWrite (WiringPiPIN_16, LOW);
                digitalWrite (WiringPiPIN_18, HIGH);
                break;
            }
            printf("MachineCode scan error code\n");
        }
        
        while(1)
        {
            memset(tempString, 0, sizeof(char)*InputLength);
            gets(tempString);
            if(strncmp(tempString, "WWW", 3) == 0)
            {
                memset(node->CountNo, 0, sizeof(char)*InputLength);
                tempPtr = tempString + 3;
                memcpy(node->CountNo, tempPtr, sizeof(tempString)-2);
                digitalWrite (WiringPiPIN_15, HIGH);
                digitalWrite (WiringPiPIN_16, HIGH);
                digitalWrite (WiringPiPIN_18, LOW);
                
                break;
            }
            printf("CountNo scan error code\n");
        }

        while(1)
        {
            memset(tempString, 0, sizeof(char)*InputLength);
            gets(tempString);
            if(strncmp(tempString, "XXX", 3) == 0)
            {
                memset(node->UserNo, 0, sizeof(char)*InputLength);
                tempPtr = tempString + 3;
                memcpy(node->UserNo, tempPtr, sizeof(tempString)-2);
                digitalWrite (WiringPiPIN_15, LOW);
                digitalWrite (WiringPiPIN_16, HIGH);
                digitalWrite (WiringPiPIN_18, LOW);
                break;
            }
            printf("UserNo scan error code\n");
        }

        memset(node->UPLoadFile, 0, sizeof(char)*UPLoadFileLength);
        gettimeofday(&now, NULL);
        sprintf(node->UPLoadFile,"%ld%s.txt", (long)now.tv_sec, node->MachineCode); 

        printf("%s %s %s %s %s %s\n", node->ISNo, node->ManagerCard, node->MachineCode, node->UserNo, node->CountNo, node->UPLoadFile);
 
        pthread_mutex_lock(&Mutexlinklist);
        if(list == NULL) 
        {
            list = node;
        }
        else
        {
            InputNode *p = list;
            while(p->link!=NULL)
            {
                p = p->link;
            }
            p->link = node;
            
        }
        pthread_mutex_unlock(&Mutexlinklist);

        if(zhTelnetFlag == 0)
        {
            //zhTelnetFlag = 1;
            //rc = pthread_create(&TelnetControlThread, NULL, RemoteControl, NULL);
            //assert(rc == 0);
        }
    }
}

void * WatchDogFunction(void *argument)
{
    pthread_t interruptThread, FileThread, TelnetControlThread, FTPThread;
    int rc = 0;

    while(WatchDogFlag)
    {
        while(list== NULL)
        {
            //printf("we wait\n");
            sleep(1);
        }       

        zhInterruptEnable = 1;
        rc = pthread_create(&interruptThread, NULL, zhINTERRUPT1, NULL);
        assert(rc == 0);

        FileFlag = 1;
        rc = pthread_create(&FileThread, NULL, FileFunction, NULL);
        assert(rc == 0);

        FTPFlag = 1;
        rc = pthread_create(&FTPThread, NULL, FTPFunction, NULL);
        assert(rc == 0);

        while(digitalRead(WiringPiPIN_22)== 0)
        {
            usleep(100000);
        }
        printf("get finish event\n");
   
        zhInterruptEnable = 0;
        pthread_join(interruptThread, NULL);
        sleep(1);

        FileFlag = 0;
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
        pthread_join(FileThread, NULL);
        sleep(1);

        FTPFlag = 0;
        pthread_mutex_lock(&mutexFTP);
        pthread_cond_signal(&condFTP);
        pthread_mutex_unlock(&mutexFTP);
        pthread_join(FTPThread, NULL);
        sleep(1);

        //reset count value and other;
        memset(PINCount, 0, sizeof(long)*48);
        memset(PINEXCount, 0, sizeof(long)*40);
        memset(I2CEXValue, 0, sizeof(int)*6);
        memset(CutRoll, 0, sizeof(short)*2);

        pthread_mutex_lock(&Mutexlinklist);
        InputNode *p = list;
        if(p->link!=NULL)
        {
            list = list->link;
            free(p);
        }
        else if(p->link == NULL)
        {
            //printf("link is empty\n");
            free(p);
            list = NULL;
        
        }else;
        pthread_mutex_unlock(&Mutexlinklist);
    }
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    curl_off_t nread;
    /* in real-world cases, this would probably get this data differently
       as this fread() stuff is exactly what the library already would do
       by default internally */
    size_t retcode = fread(ptr, size, nmemb, stream);

    nread = (curl_off_t)retcode;

    fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
            " bytes from file\n", nread);
    return retcode;
}

void * FTPFunction(void *argument)
{
#ifdef LogMode
    Log(s, __func__, __LINE__, " FTP entry\n");
#endif
    CURL *curl;
    CURLcode res;
    FILE *hd_src;
    struct stat file_info;
    curl_off_t fsize;
    char UPLoadFile_3[21];
    struct timeval now;
    struct timespec outtime;
    int FTPCount = 0;
    short checkFlag = 0;

    while(FTPFlag){
        char Remote_url[80] = "ftp://192.168.10.254:21/home/";
        //char Remote_url[80] = "ftp://192.168.0.110:8888/";
        long size = 0;
        pthread_mutex_lock(&mutexFTP);
        //struct curl_slist *headerlist=NULL;
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + FTPWakeUpValue;
        outtime.tv_nsec = now.tv_usec * 1000;
        pthread_cond_timedwait(&condFTP, &mutexFTP, &outtime);
        pthread_mutex_unlock(&mutexFTP);
       
        FTPCount = (FTPCount + FTPWakeUpValue) % FTPCountValue;

        if(FTPCount == 0 || FTPFlag == 0 || size > 100000)
        {
            pthread_mutex_lock(&Mutexlinklist);
            
            InputNode *p = list;
            if(p != NULL)
            {
                memset(UPLoadFile_3, 0, sizeof(char)*21);
                strcpy(UPLoadFile_3, p->UPLoadFile);
                gettimeofday(&now, NULL);
                sprintf(p->UPLoadFile,"%ld%s.txt",(long)now.tv_sec, p->MachineCode);
                checkFlag = 1;
            }
            pthread_mutex_unlock(&Mutexlinklist);
            if(checkFlag == 1)
            {
                printf("%s\n", UPLoadFile_3);
                strcat(Remote_url,UPLoadFile_3);
                if(stat(UPLoadFile_3, &file_info)) 
                {
                    printf("Couldnt open %s: %s\n", UPLoadFile_3, strerror(errno));
#ifdef LogMode
                    Log(s, __func__, __LINE__, " FTP fail_1\n");
#endif
                    digitalWrite (WiringPiPIN_15, LOW);
                    digitalWrite (WiringPiPIN_16, LOW);
                    digitalWrite (WiringPiPIN_18, LOW);
                }
                if(file_info.st_size > 0)
                {
                    fsize = (curl_off_t)file_info.st_size;

                    curl_global_init(CURL_GLOBAL_ALL);

                    curl = curl_easy_init();
                    if(curl)
                    {
                        hd_src = fopen(UPLoadFile_3, "rb");
                        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
                        //curl_easy_setopt(curl, CURLOPT_USERPWD, "raspberry:1234");
                        curl_easy_setopt(curl, CURLOPT_USERPWD, "taicon_ftp:2769247");
                        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
                        curl_easy_setopt(curl,CURLOPT_URL, Remote_url);
                        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
                        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
                        res = curl_easy_perform(curl);

                        if(res != CURLE_OK)
                        {
                            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
#ifdef LogMode
                            Log(s, __func__, __LINE__, " FTP fail_2\n");
#endif
                            digitalWrite (WiringPiPIN_15, LOW);
                            digitalWrite (WiringPiPIN_16, LOW);
                            digitalWrite (WiringPiPIN_18, LOW);
                        }
                        else
                        {
                            digitalWrite (WiringPiPIN_15, LOW);
                            digitalWrite (WiringPiPIN_16, HIGH);
                            digitalWrite (WiringPiPIN_18, LOW);
                        }

                        //curl_slist_free_all (headerlist);
                        curl_easy_cleanup(curl);
                        if(hd_src)
                        {
                            fclose(hd_src);
                        }
                    }    
                    curl_global_cleanup();
                }
                unlink(UPLoadFile_3);
                checkFlag = 0;
            }
        }
    }
#ifdef LogMode
    Log(s, __func__, __LINE__, " FTP exit\n");
#endif
}