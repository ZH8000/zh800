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
#include <wiringSerial.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <errno.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <net/if.h>

#define SHMSZ 2000
#define ZHNetworkType "eth0"

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

#define IN_P0 0x00
#define IN_P1 0x01
#define OUT_P0 0x02
#define OUT_P1 0x03
#define INV_P0 0x04
#define INV_P1 0x05
#define CONFIG_P0 0x06
#define CONFIG_P1 0x07

#define WatchDogCountValue 600000 //msec
#define InputLength 256
#define UPLoadFileLength 256
#define CountPeriod 100 //msec
#define WriteFileCountValue 4000 // msec
#define FTPCountValue 300 //sec
#define FTPWakeUpValue 60 //sec
#define zhMAXOUTPUT 10
#define ERRORCHECKMAXRETRY 20

#define Log(s,func, line, opt) StringCat(func);StringCat(opt)
#define RS232_Length 5
#define ErrorType 22

#define goodrate 1
//#define LogMode
#define PrintInfo
#define PrintMode

enum
{
    GoodNumber = 0,
    BadNumber,
    GoodTotalNumber
};

enum
{
    MachRUNNING = 1,
    MachREPAIRING,
    MachREPAIRDone,
    MachJobDone,
    MachLOCK,
    MachUNLOCK,
    MachSTOPForce1,
    MachSTOPForce2
};

short SerialThreadFlag;
short WatchDogFlag = 0;
short zhResetFlag = 0;
short PrintLeftLog = 0;
short FileFlag = 0;
short updateFlag = 0;
short zhTelnetFlag = 0;
short FTPFlag = 0;
short ButtonFlag = 0;
short MasterFlag = 0;

char *shm, *s, *tail;
char *shm_pop;
unsigned char output[RS232_Length];

pthread_cond_t cond,condFTP;
pthread_mutex_t mutex, mutex_3, mutex_log, mutex_2, mutexFTP, mutexFile;

long productCountArray[3];
long ExproductCountArray[3];
int messageArray[ErrorType];
int ExmessageArray[ErrorType];

char ISNo[InputLength], ManagerCard[InputLength], MachineCode[InputLength], UserNo[InputLength], CountNo[InputLength];
char RepairNo[InputLength];
char UPLoadFile[UPLoadFileLength];

void * zhLogFunction(void *argument);
void * FTPFunction(void *arguemnt);
void * ButtonListenFunction(void *argument);
void * SerialFunction(void *argument);
void * RemoteControl(void *argument);
void StringCat(const char *str);

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);
int transferFormatINT(unsigned char x);
long transferFormatLONG(unsigned char x);

void * RemoteControl(void * argument)
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
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)))
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
    /*if(close(socket_desc) < 0)
    {
        printf("erron %d",errno);
    }*/
    close(socket_desc);
    //free(socket_desc);
#ifdef PrintInfo
    printf("Telnet close\n"); 
#endif
}

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
    fprintf(pfile,"tsw303\t%s\t0\t", buff);
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
                    fprintf(pfile,"tsw303\t%s\t%ld\t",buff,productCountArray[GoodNumber]);
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

void * SerialFunction(void *argument)
{
#ifdef LogMode
    Log(s, __func__, __LINE__, "Serial Function entry\n");
#endif
    int fd;
    char temp_output[RS232_Length];
    int count1, count2;
    int string_count = 0;
    struct termios options;
    count1 = count2 = 0;

    memset(temp_output, 0, sizeof(char)*RS232_Length);

    if ((fd = serialOpen ("/dev/ttyAMA0", 9600)) < 0)
    {
        printf ("Unable to open serial device: %s\n", strerror (errno)) ;
        pthread_exit((void*)"fail");
    }

    tcgetattr(fd, &options);
    options.c_cflag |= PARENB;
    tcsetattr(fd, TCSANOW, &options);

    while(SerialThreadFlag)
    {
        while(serialDataAvail(fd))
        {   
            char temp_char_1;
            temp_char_1 = serialGetchar(fd);
            if(count1 == 2 && string_count < RS232_Length - 1)
            {
                temp_output[string_count] = temp_char_1;
                string_count++;
            }else if(count1 == 2 && string_count == (RS232_Length - 1))
            {
                //package
                temp_output[string_count] = '\0';
                pthread_mutex_lock(&mutex_2);
                memset(output, 0,sizeof(char)*RS232_Length);
                int vers_test_count = 0;
                //printf("Ready to Print:");
                for(vers_test_count = 0; vers_test_count < RS232_Length; vers_test_count++)
                {
                    output[vers_test_count] = temp_output[vers_test_count];
                    //printf(" %x", output[vers_test_count]);
                }
                //printf("\n");
                memset(temp_output, 0, sizeof(char)*RS232_Length);
                count1 = 0;
                string_count = 0;
                updateFlag = 1;
                pthread_mutex_unlock(&mutex_2);
            }else if(temp_char_1 == 0x08)
            {
                count1 = 1;
            }else if(count1 == 1 && temp_char_1 == 0x06)
            {
                count1 = 2;                
            }else count1 = 0;
    
            fflush (stdout) ;
            //if (SerialThreadFlag == 0) break;
        }
    }
    if(fd >= 0)
    {
        serialClose(fd);
    }
    printf("serial function exit\n");
#ifdef LogMode
    Log(s, __func__, __LINE__, "Serial Function exit\n");
#endif
}

void * FileFunction(void *argement)
{
#ifdef LogMode
    Log(s, __func__, __LINE__, "File Function entry\n");
#endif
    struct timeval now;
    struct timespec outtime;
    FILE *file_dst;
    char file_output_temp[RS232_Length];
    unsigned char countArray[9];

    int fd;
    struct ifreq ifr;
    int ForCount = 0;
    int WriteFileCount = 0;
    int watchdogCooldown = WatchDogCountValue;
    short errorCheckCount[3];
    char result[3]; //remeber last stage 
    memset(result, 0, sizeof(short)*3);
    memset(errorCheckCount , 0, sizeof(short)*3);   
    memset(countArray, 0 ,sizeof(char)*9); 
    memset(messageArray, 0, sizeof(int)*ErrorType);

    while(FileFlag)
    {
        pthread_mutex_lock(&mutex);
        gettimeofday(&now, NULL);
        //outtime.tv_sec = now.tv_sec + CountPeriod;
        outtime.tv_sec = now.tv_sec;
        //outtime.tv_nsec= now.tv_usec * 1000;
        outtime.tv_nsec= (now.tv_usec + (CountPeriod * 1000) ) * 1000;
        
        if(outtime.tv_nsec > 1000000000)
        {
            outtime.tv_sec += 1;
            outtime.tv_nsec = outtime.tv_nsec % 1000000000;
        }

        pthread_cond_timedwait(&cond, &mutex, &outtime);
        pthread_mutex_unlock(&mutex);
        if(updateFlag == 1)
        {
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            ifr.ifr_addr.sa_family = AF_INET;
            strncpy(ifr.ifr_name, ZHNetworkType, IFNAMSIZ-1);
            ioctl(fd, SIOCGIFADDR, &ifr);
            close(fd);

            memset(productCountArray, 0, sizeof(long)*3);
 
            memset(file_output_temp, 0, sizeof(char)*RS232_Length);

            pthread_mutex_lock(&mutex_2);
            strncpy(file_output_temp, output, RS232_Length);
            int vers_count;
            for(vers_count = 0; vers_count < RS232_Length; vers_count++)
            {
                file_output_temp[vers_count] = output[vers_count];
            }
            updateFlag = 0;
            pthread_mutex_unlock(&mutex_2);

            if(file_output_temp[1] == 0x06)
            {
                printf("0x06: ");
                for(ForCount = 0; ForCount < 5; ForCount++)
                {
                    printf("%x ", file_output_temp[ForCount]);
                }
                printf("\n");
                countArray[3] = file_output_temp[2];
                countArray[2] = file_output_temp[3];
            }else if(file_output_temp[1] == 0x05)
            {
                printf("0x05: ");
                for(ForCount = 0; ForCount < 5; ForCount++)
                {
                    printf("%x ", file_output_temp[ForCount]);
                }
                printf("\n");
                countArray[1] = file_output_temp[2];
                countArray[0] = file_output_temp[3];                
            }else if(file_output_temp[1] == 0x0C)
            {
                printf("0x0C: ");
                for(ForCount = 0; ForCount < 5; ForCount++)
                {
                    printf("%x ", file_output_temp[ForCount]);
                }
                printf("\n");

                countArray[6] = file_output_temp[2];
                countArray[5] = file_output_temp[3];
            }else if(file_output_temp[1] == 0x0D)
            {
                printf("0x0D: ");
                for(ForCount = 0; ForCount < 5; ForCount++)
                {
                    printf("%x ", file_output_temp[ForCount]);
                }
                printf("\n");

                countArray[8] = file_output_temp[2];
                countArray[7] = file_output_temp[3];
            }else if(file_output_temp[1] == 0x07)
            {
                printf("0x07: %x %x %x %x\n", file_output_temp[0], file_output_temp[1], file_output_temp[2], file_output_temp[3]);
                countArray[4] = file_output_temp[3];
            }else if(file_output_temp[1] == 0x00)
            {
                printf("0x00: ");
                for(ForCount = 0; ForCount < 5; ForCount++)
                {
                    printf("%x ", file_output_temp[ForCount]);
                }
                printf("\n");

                char result1 = result[0] ^ 0xff;
                result1 = result1 & file_output_temp[3];
                char result2 = result[1] ^ 0xff;
                result2 = result2 & file_output_temp[2];
                
                result[0] = file_output_temp[3];
                result[1] = file_output_temp[2];

                for(ForCount = 0; ForCount < 8; ForCount++)
                {
                    if((result1 & 1) == 1)
                    {
                        messageArray[ForCount] = messageArray[ForCount] + 1;
                    }
                    if((result2 & 1) == 1)
                    {
                        messageArray[ForCount+8] = messageArray[ForCount+8] + 1;
                    }
                    result1 = result1 >> 1;
                    result2 = result2 >> 1; 
                }
            }else if(file_output_temp[1] == 0x01)
            {
                printf("0x01: ");
                for(ForCount = 0; ForCount < 5; ForCount++)
                {
                    printf("%x ", file_output_temp[ForCount]);
                }
                printf("\n");

                char result3 = result[2] ^ 0xff;
                result3 = result3 & file_output_temp[3];
                result[2] = file_output_temp[3];
                for(ForCount = 16; ForCount < 22; ForCount++)
                {
                    if((result3 & 1) == 1)
                    {
                        messageArray[ForCount] = messageArray[ForCount] + 1;
                    }
                    result3 = result3 >> 1;
                }
            }
            else;

            //[vers|2014.09.02 |assign count value]
            for(ForCount = 0; ForCount < 5; ++ForCount)
            {
                if(ForCount == 4)
                {
                    productCountArray[BadNumber] = transferFormatLONG(countArray[4]);
                }else
                {
                    productCountArray[GoodNumber] = productCountArray[GoodNumber] + transferFormatLONG(countArray[ForCount]) * pow(256, ForCount);
                    productCountArray[GoodTotalNumber] = 
                                           productCountArray[GoodTotalNumber] + transferFormatLONG(countArray[ForCount + 5]) * pow(256, ForCount);
                }
            }
            //[vers|2014.09.02 |end]

            //[vers| avoid  wrong info send form machine]
            for(ForCount = 0; ForCount < 3; ++ForCount)
            {
                if(abs(ExproductCountArray[ForCount]-productCountArray[ForCount]) > 220 && errorCheckCount[ForCount] < ERRORCHECKMAXRETRY)
                {
                    productCountArray[ForCount] = ExproductCountArray[ForCount];
                    errorCheckCount[ForCount]++;
                }
                else
                {
                    errorCheckCount[ForCount] = 0;
                }
            }
            if(file_output_temp[1] == 0x00 || file_output_temp[1] == 0x01 || file_output_temp[1] == 0x05 || 
                                              file_output_temp[1] == 0x06 || file_output_temp[1] == 0x07 ||
                                              file_output_temp[1] == 0x0C || file_output_temp[1] == 0x0D)
            {
#ifdef PrintInfo
                printf("%s %s %s %s %s || ",ISNo, ManagerCard, CountNo, MachineCode, UserNo);
                printf("%ld %ld %ld\n", productCountArray[GoodNumber], productCountArray[BadNumber], productCountArray[GoodTotalNumber]);
            
                for(ForCount = 0; ForCount < 22; ForCount++)
                {
                    printf(" %d", messageArray[ForCount]);
                }
                printf("\n");
#endif
            }
        }
        WriteFileCount = (WriteFileCount + CountPeriod) % WriteFileCountValue;
        if(WriteFileCount == 0 || FileFlag == 0)
        {
            short newDataIncome = 0;

            //[vers|2014.10.25 | initial count number]
            for(ForCount = 0; ForCount < 3; ++ForCount)
            {
                //need set ExproductCountArray
                if(productCountArray[ForCount] < ExproductCountArray[ForCount])
                {
                    // count reset
                    ExproductCountArray[ForCount] = 0;
                }else if((productCountArray[ForCount] != 0) && (ExproductCountArray[ForCount] == 0))
                {
                    ExproductCountArray[ForCount] = productCountArray[ForCount];
                }
                else;
            }
            //[vers|2014.10.25|end] 

            pthread_mutex_lock(&mutexFile);
            file_dst = fopen(UPLoadFile,"a");
            for(ForCount = 0 ; ForCount < ErrorType; ++ForCount)
            {
                if(messageArray[ForCount] != ExmessageArray[ForCount] && messageArray[ForCount] != 0)
                {
#ifdef PrintMode
                    fprintf(file_dst, "%s %s %s 0 %ld %d %s %d %s %s 0 0 0 %02d\n", ISNo, ManagerCard, CountNo, 
                                                                                    (long)now.tv_sec, 
                                                                                    messageArray[ForCount] - ExmessageArray[ForCount],
                                                                                    inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                    (ForCount+1)+3, MachineCode, UserNo, MachRUNNING);
#else
                    fprintf(file_dst, "%s %s %s 0 %ld %d %s %d %s %s 0 0 0 %02d\n", ISNo, ManagerCard, CountNo, 
                                                                                    (long)now.tv_sec, messageArray[ForCount],
                                                                                    inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                    (ForCount+1)+3, MachineCode, UserNo, MachRUNNING);
#endif
                    if(newDataIncome == 0) newDataIncome = 1;
                }
            }
            for(ForCount = 0 ; ForCount < 3; ForCount++)
            {
               if(productCountArray[ForCount]!= 0 && ExproductCountArray[ForCount] != productCountArray[ForCount])
               {
                    if(ForCount == 0)
                    {
                        if(productCountArray[GoodNumber] - ExproductCountArray[GoodNumber] > zhMAXOUTPUT)
                        {
                            fprintf(file_dst, "%s %s %s -1 %ld 0 %s %d %s %s 0 0 0 %02d\n", ISNo, ManagerCard, CountNo, (long)now.tv_sec, 
                                                                                      inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                      ForCount + 1, MachineCode, UserNo, MachRUNNING);
                        }
                        else
                        {
#ifdef PrintMode
                            fprintf(file_dst, "%s %s %s %ld %ld 0 %s %d %s %s 0 0 0 %02d\n", 
                                                                            ISNo, ManagerCard, CountNo, 
                                                                            productCountArray[GoodNumber] - ExproductCountArray[GoodNumber],
                                                                            (long)now.tv_sec,
                                                                            inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                            ForCount + 1, MachineCode, UserNo, MachRUNNING);
#else
                            fprintf(file_dst, "%s %s %s %ld %ld 0 %s %d %s %s 0 0 0 %02d\n",
                                                                ISNo, ManagerCard, CountNo, productCountArray[GoodNumber], (long)now.tv_sec,
                                                                inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                ForCount + 1, MachineCode, UserNo, MachRUNNING);
#endif
                        }
                   }
                   else
                   {
#ifdef PrintMode
                       fprintf(file_dst, "%s %s %s 0 %ld %ld %s %d %s %s 0 0 0 %02d\n", 
                                                                     ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                     productCountArray[ForCount] - ExproductCountArray[ForCount],
                                                                     inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                     ForCount + 1, MachineCode, UserNo, MachRUNNING);
#else
                       fprintf(file_dst, "%s %s %s 0 %ld %ld %s %d %s %s 0 0 0 %02d\n", 
                                                                     ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                     productCountArray[ForCount],
                                                                     inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                     ForCount + 1, MachineCode, UserNo, MachRUNNING);
#endif
                   }
                   if(newDataIncome == 0) newDataIncome = 1;
               }
            }
            fclose(file_dst);
            pthread_mutex_unlock(&mutexFile);
            //check good count
            /*if(productCountArray[GoodNumber] < ExproductCountArray[GoodNumber])
            {
                int forCount = 0; 
                file_dst = fopen("aaaa", "a");
                
                for(forCount = 0; forCount < RS232_Length; forCount++)
                {
                    fprintf(file_dst, "%x ", file_output_temp[forCount]);
                }

                fprintf(file_dst, "%ld %ld \n", productCountArray[GoodNumber], ExproductCountArray[GoodNumber]);
                fclose(file_dst);
            }*/
            memcpy(ExproductCountArray, productCountArray, sizeof(long)*3);
            memcpy(ExmessageArray, messageArray, sizeof(int)*ErrorType);
            
            //a timeout mechanism
            if(newDataIncome == 1)
            {
                watchdogCooldown = WatchDogCountValue;
            }else 
            {
                watchdogCooldown = watchdogCooldown - WriteFileCountValue;
                printf("%d\n", watchdogCooldown);   
            }
            if(watchdogCooldown <= 0)
            {
                zhResetFlag = 1;
            }

            //check network status
            int fd2 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
            struct ifreq ethreq;
            memset(&ethreq, 0, sizeof(ethreq));
            strncpy(ethreq.ifr_name, ZHNetworkType, IFNAMSIZ);
            ioctl(fd2, SIOCGIFFLAGS, &ethreq);
            if(ethreq.ifr_flags & IFF_RUNNING)
            {
                digitalWrite (WiringPiPIN_15, HIGH);
                digitalWrite (WiringPiPIN_16, HIGH);
                digitalWrite (WiringPiPIN_18, LOW);
            }else
            {
                digitalWrite (WiringPiPIN_15, HIGH);
                digitalWrite (WiringPiPIN_16, LOW);
                digitalWrite (WiringPiPIN_18, LOW);
            }
            close(fd2);
        }
    }
#ifdef LogMode
    Log(s, __func__, __LINE__, "FileFunction exit\n");
#endif
}

int main(int argc ,char *argv[])
{
    char *dev = "/dev/i2c-1";
    int rc;    
    pthread_t LogThread, SerialThread, FileThread, TelnetControlThread, FTPThread;
    int shmid;
    key_t key;
    struct ifreq ifr;

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutex_2, NULL);
    //pthread_mutex_init(&mutex_3, NULL);
    pthread_mutex_init(&mutex_log, NULL);
    pthread_mutex_init(&mutexFTP, NULL);
    pthread_mutex_init(&mutexFile, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_cond_init(&condFTP, NULL);

    wiringPiSetup(); 
  
    pinMode(WiringPiPIN_15, OUTPUT);
    pinMode(WiringPiPIN_16, OUTPUT);
    pinMode(WiringPiPIN_18, OUTPUT);
    pinMode(WiringPiPIN_22, INPUT);
    pullUpDnControl (WiringPiPIN_22, PUD_UP); 

    int fd, r;
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
#ifdef LogMode
    rc = pthread_create(&LogThread, NULL, zhLogFunction, NULL);
    assert(rc == 0);
    key = 5678;
    sleep(1);

    if ((shmid = shmget(key, SHMSZ, 0666)) < 0)
    {
        perror("shmget main");
        return 1;
    }

    if ((shm = shmat(shmid, NULL, 0)) == (char *)-1)
    {
        perror("shmat");
        return 1;
    }
    s = shm + 1;
    Log(s, __func__, __LINE__, " ready to init\n");
#endif
    
    //the mechine always standby
    while(1)
    {
        unsigned char isNormalStop = 0;
        
        digitalWrite (WiringPiPIN_15, HIGH);
        digitalWrite (WiringPiPIN_16, HIGH);
        digitalWrite (WiringPiPIN_18, HIGH);
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
        i2c_smbus_write_byte_data(fd, OUT_P1, 0x07);
        i2c_smbus_write_byte_data(fd, CONFIG_P1, 0x00);
        close(fd);

#ifdef PrintInfo
        printf("Ready to work..\n");
#endif
        while(1)
        {
            sleep(1);
            memset(tempString, 0, sizeof(char)* InputLength);
            gets(tempString);
            //if(strncmp(tempString, "YYY", 3) == 0)
            if(strlen(tempString) == 14)
            {
                memset(ISNo, 0, sizeof(char)*InputLength);
                //tempPtr = tempString + 3;
                //memcpy(ISNo, tempPtr, sizeof(tempString)-2);
                tempPtr = tempString;
                memcpy(ISNo, tempPtr, sizeof(tempString));
                digitalWrite (WiringPiPIN_15, LOW);
                digitalWrite (WiringPiPIN_16, HIGH);
                digitalWrite (WiringPiPIN_18, HIGH);
                break;
            }
            printf("scan ISNo error code\n");
        }
        while(1)
        {
            sleep(1);
            memset(tempString, 0, sizeof(char)*InputLength);
            gets(tempString);
            //if(strncmp(tempString, "QQQ", 3) == 0)
            int x = strlen(tempString);
            printf("string length : %d\n", x);
            if(strlen(tempString) == 24)
            {
                memset(ManagerCard, 0, sizeof(char)*InputLength);
                //tempPtr = tempString + 3;
                //memcpy(ManagerCard, tempPtr, sizeof(tempString)-2);
                tempPtr = tempString;
                memcpy(ManagerCard, tempPtr, sizeof(tempString));
                digitalWrite (WiringPiPIN_15, HIGH);
                digitalWrite (WiringPiPIN_16, LOW);
                digitalWrite (WiringPiPIN_18, HIGH);
                break;
            }
            printf("ManagerCard scan error code\n");
        }
        while(1)
        {
            sleep(1);
            memset(tempString, 0, sizeof(char)*InputLength);
            gets(tempString);
            int stringLength = strlen(tempString);
            int arrayCount = 0;
            short flagFailPass = 0;
            while(arrayCount < stringLength)
            {
                if(tempString[arrayCount] == '0') ;
                else if(tempString[arrayCount] == '1');
                else if(tempString[arrayCount] == '2');
                else if(tempString[arrayCount] == '3');
                else if(tempString[arrayCount] == '4');
                else if(tempString[arrayCount] == '5');
                else if(tempString[arrayCount] == '6');
                else if(tempString[arrayCount] == '7');
                else if(tempString[arrayCount] == '8');
                else if(tempString[arrayCount] == '9');
                else 
                {
                    flagFailPass = 1;
                    break;
                }
                ++arrayCount;
            }
            if(flagFailPass == 0 && stringLength > 0)
            {
                memset(CountNo, 0, sizeof(char)*InputLength);
                memcpy(CountNo, tempPtr, sizeof(tempString));
                goodCount = (atoi(CountNo)*goodrate);
                if(goodCount > 0)
                {
                    printf("need finish: %ld\n", goodCount);
                    digitalWrite (WiringPiPIN_15, LOW);
                    digitalWrite (WiringPiPIN_16, LOW);
                    digitalWrite (WiringPiPIN_18, HIGH);
                    break;
                }
            }
            /*if(strncmp(tempString, "WWW", 3) == 0)
            {
                memset(CountNo, 0, sizeof(char)*InputLength);
                tempPtr = tempString + 3;
                memcpy(CountNo, tempPtr, sizeof(tempString)-2);
                goodCount = (atoi(CountNo)*goodrate);
                printf("need finish: %ld\n", goodCount);
                digitalWrite (WiringPiPIN_15, LOW);
                digitalWrite (WiringPiPIN_16, LOW);
                digitalWrite (WiringPiPIN_18, HIGH);
                
                break;
            }*/
            printf("CountNo scan error code\n");
        } 
     
        while(1)
        {
            sleep(1);
            memset(tempString, 0, sizeof(char)*InputLength);
            gets(tempString);
            if(strncmp(tempString, "XXXP", 4) == 0)
            {
                memset(UserNo, 0, sizeof(char)*InputLength);
                tempPtr = tempString + 4;
                memcpy(UserNo, tempPtr, sizeof(tempString)-3);
                
                digitalWrite (WiringPiPIN_15, HIGH);
                digitalWrite (WiringPiPIN_16, HIGH);
                digitalWrite (WiringPiPIN_18, LOW);
                
                break;
            }
            printf("UserNo scan error code\n");
        }

        char FakeInput[5][InputLength];
        memset(FakeInput, 0, sizeof(char)*(5*InputLength));
        int filesize, FakeInputNumber = 0;
        int FakeInputNumber_2 = 0;
        char * buffer, * charPosition;
        short FlagNo = 0;        

        pfile = fopen("/home/pi/works/tsw303/barcode","r");
        fseek(pfile, 0, SEEK_END);
        filesize = ftell(pfile);
        rewind(pfile);
        buffer = (char *) malloc (sizeof(char)*filesize);
        charPosition = buffer;
        fread(buffer, 1, filesize, pfile);
        fclose(pfile);
        while(filesize > 1)
        {
            if(*charPosition == ' ')
            {
                FlagNo = 1;
            }
            else if(*charPosition != ' ' && FlagNo == 1)
            {
                FakeInputNumber++;
                FakeInputNumber_2 = 0;

                FakeInput[FakeInputNumber][FakeInputNumber_2] = *charPosition;
                FakeInputNumber_2++;
                FlagNo = 0;
            }
            else
            {
                FakeInput[FakeInputNumber][FakeInputNumber_2] = *charPosition;
                FakeInputNumber_2++;
            }
            filesize--;
            charPosition++;
        }
        free(buffer);

        /*sleep(1);
        memset(ISNo, 0, sizeof(char)*InputLength);
        strcpy(ISNo, FakeInput[0]);
        digitalWrite (WiringPiPIN_15, LOW);
        digitalWrite (WiringPiPIN_16, HIGH);
        digitalWrite (WiringPiPIN_18, HIGH);

        sleep(1);
        memset(ManagerCard, 0, sizeof(char)*InputLength);
        strcpy(ManagerCard, FakeInput[1]);
        digitalWrite (WiringPiPIN_15, HIGH);
        digitalWrite (WiringPiPIN_16, LOW);
        digitalWrite (WiringPiPIN_18, HIGH);
        */
        sleep(1);
        memset(MachineCode, 0 , sizeof(char)*InputLength);
        strcpy(MachineCode, FakeInput[2]);
        /*digitalWrite (WiringPiPIN_15, LOW);
        digitalWrite (WiringPiPIN_16, LOW);
        digitalWrite (WiringPiPIN_18, HIGH);
        
        sleep(1);
        memset(CountNo, 0, sizeof(char)*InputLength);
        strcpy(CountNo, FakeInput[3]);
        goodCount = (atoi(CountNo)*goodrate);
        digitalWrite (WiringPiPIN_15, HIGH);
        digitalWrite (WiringPiPIN_16, HIGH);
        digitalWrite (WiringPiPIN_18, LOW);

        sleep(1);
        memset(UserNo, 0, sizeof(char)*InputLength);
        strcpy(UserNo, FakeInput[4]);
        digitalWrite (WiringPiPIN_15, LOW);
        digitalWrite (WiringPiPIN_16, HIGH);
        digitalWrite (WiringPiPIN_18, LOW);
        */

        gettimeofday(&now, NULL);
        memset(UPLoadFile, 0, sizeof(char)*UPLoadFileLength);
        sprintf(UPLoadFile,"%ld%s.txt",(long)now.tv_sec, MachineCode); 
        pfile = fopen(UPLoadFile, "a");
        if(pfile != NULL)
        {
            fclose(pfile);
        }

        printf("%s %s %s %s %s %s\n", ISNo, ManagerCard, MachineCode, UserNo, CountNo, UPLoadFile);
        MasterFlag = 1;
        
        memset(ExproductCountArray, 0, sizeof(long)*3);
        memset(ExmessageArray, 0, sizeof(int)*ErrorType);
       

        if(zhTelnetFlag == 0){
            //zhTelnetFlag = 1;
            //rc = pthread_create(&TelnetControlThread, NULL, RemoteControl, NULL);
            //assert(rc == 0);
        }
        while(MasterFlag)
        {
            //unlock 
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
            i2c_smbus_write_byte_data(fd, OUT_P1, 0x00);
            i2c_smbus_write_byte_data(fd, CONFIG_P1, 0x00);
            close(fd);

            PrintLeftLog = 0;
            zhResetFlag = 0; //reset flag clean
            
            SerialThreadFlag = 1;
            rc = pthread_create(&SerialThread, NULL, SerialFunction, NULL);
            assert(rc == 0);

            FileFlag = 1;
            rc = pthread_create(&FileThread, NULL, FileFunction, NULL);
            assert(rc == 0);

            FTPFlag = 1;
            rc = pthread_create(&FTPThread, NULL, FTPFunction, NULL);
            assert(rc == 0);

            while(zhResetFlag == 0)
            {
                usleep(100000);
                if(ExproductCountArray[GoodNumber] >= goodCount)
                //if(productCountArray[GoodNumber] >= 0)
                {
                    //finish job
                    sleep(10);
                    printf("Houston we are ready to back!\n");
                    zhResetFlag = 1;
                    MasterFlag = 0;
                    isNormalStop = 1;
                }
                /*else if(zhTelnetFlag == 0)
                {
                    printf("Houston remote user call me back to base!\n");
#ifdef LogMode
                    Log(s, __func__, __LINE__, " remote user call me back\n");
                    Log(s, __func__, __LINE__, " remote user call me back\n");
                    Log(s, __func__, __LINE__, " remote user call me back\n");
#endif
                    pthread_join(TelnetControlThread, NULL);
                    zhResetFlag = 1;
                    MasterFlag = 0;
                }
*/
                else if(digitalRead(WiringPiPIN_22) == 0)
                {
                    //finish job
#ifdef LogMode
                    Log(s, __func__, __LINE__, " PIN_22 call me back\n");
                    Log(s, __func__, __LINE__, " PIN_22 call me back\n");
                    Log(s, __func__, __LINE__, " PIN_22 call me back\n");
                    Log(s, __func__, __LINE__, " PIN_22 call me back\n");
                    Log(s, __func__, __LINE__, " PIN_22 call me back\n");
#endif
                    printf("Houston we are ready to back!\n");
                    zhResetFlag = 1;
                    MasterFlag = 0;
                }
                else;
            }

            PrintLeftLog = 1;
            SerialThreadFlag = 0;
            pthread_join(SerialThread, NULL);
            sleep(1);

            FileFlag = 0;
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);
            pthread_join(FileThread, NULL);
            sleep(1);

            //get ip address & time
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            ifr.ifr_addr.sa_family = AF_INET;
            strncpy(ifr.ifr_name, ZHNetworkType, IFNAMSIZ-1);
            ioctl(fd, SIOCGIFADDR, &ifr);
            close(fd);
            gettimeofday(&now, NULL);

            pthread_mutex_lock(&mutexFile);
            if(MasterFlag == 0 && isNormalStop == 1)
            {
                pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                                       inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                       MachineCode, UserNo, MachJobDone);
#else
                fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, , (long)now.tv_sec,
                                                                                       inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                       MachineCode, UserNo, MachJobDone);
#endif
                fclose(pfile);
            }
            else if(MasterFlag == 0)
            {
                if(productCountArray[GoodNumber] >= goodCount / 1.04 ) 
                {
                    pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                    fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                                       inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                       MachineCode, UserNo, MachSTOPForce1);
#else
                    fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, , (long)now.tv_sec,
                                                                                       inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                       MachineCode, UserNo, MachSTOPForce1);
#endif
                    fclose(pfile);
                }else
                {
                    pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                    fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                                       inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                       MachineCode, UserNo, MachSTOPForce2);
#else
                    fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, , (long)now.tv_sec,
                                                                                       inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                       MachineCode, UserNo, MachSTOPForce2);
#endif
                    fclose(pfile);
                }   
            }else
            {
                pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                                       inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                       MachineCode, UserNo, MachLOCK);
#else
                fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                                       inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                       MachineCode, UserNo, MachLOCK);
#endif
                fclose(pfile);
            }
            pthread_mutex_unlock(&mutexFile);

            FTPFlag = 0;
            pthread_mutex_lock(&mutexFTP);
            pthread_cond_signal(&condFTP);
            pthread_mutex_unlock(&mutexFTP);
            pthread_join(FTPThread, NULL);
            sleep(1);
            
            if(MasterFlag)
            {
                //lock 
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
        
                i2c_smbus_write_byte_data(fd, OUT_P1, 0x07);
                i2c_smbus_write_byte_data(fd, CONFIG_P1, 0x00);
                close(fd);

                digitalWrite (WiringPiPIN_15, LOW);
                digitalWrite (WiringPiPIN_16, LOW);
                digitalWrite (WiringPiPIN_18, HIGH);
                while(1)
                {
                    sleep(1);
                    memset(tempString, 0, sizeof(char)*InputLength);
                    gets(tempString);
                    if(strncmp(tempString, "XXXP", 4) == 0)
                    {
                        //unlock 
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
                        i2c_smbus_write_byte_data(fd, OUT_P1, 0x00);
                        i2c_smbus_write_byte_data(fd, CONFIG_P1, 0x00);
                        close(fd);

                        memset(UserNo, 0, sizeof(char)*InputLength);
                        tempPtr = tempString + 4;
                        memcpy(UserNo, tempPtr, sizeof(tempString)-3);
                        break;
                    }
                    else if(strncmp(tempString,"XXXM", 4) == 0)
                    {
                        char FixerNo[InputLength];
                        struct timeval changeIntoRepairmodeTimeStemp;
                        pthread_t buttonThread;
                        memset(FixerNo, 0, sizeof(char)*InputLength);
                        tempPtr = tempString + 4;
                        memcpy(FixerNo, tempPtr, sizeof(tempString)-3);
                        
                        //unlock 
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
                        i2c_smbus_write_byte_data(fd, OUT_P1, 0x00);
                        i2c_smbus_write_byte_data(fd, CONFIG_P1, 0x00);
                        close(fd);

                        //get ip address & time
                        fd = socket(AF_INET, SOCK_DGRAM, 0);
                        ifr.ifr_addr.sa_family = AF_INET;
                        strncpy(ifr.ifr_name, ZHNetworkType, IFNAMSIZ-1);
                        ioctl(fd, SIOCGIFADDR, &ifr);
                        close(fd);
                        gettimeofday(&now, NULL);
                        gettimeofday(&changeIntoRepairmodeTimeStemp, NULL);

                        pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                        fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                                       inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                       MachineCode, FixerNo, MachREPAIRING);
#else
                        fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                                       inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                       MachineCode, FixerNo, MachREPAIRING);
#endif
                        fclose(pfile);

                        FTPFlag = 1;
                        rc = pthread_create(&FTPThread, NULL, FTPFunction, NULL);
                        assert(rc == 0);
                        sleep(1);

                        FTPFlag = 0;
                        pthread_mutex_lock(&mutexFTP);
                        pthread_cond_signal(&condFTP);
                        pthread_mutex_unlock(&mutexFTP);
                        pthread_join(FTPThread, NULL);

                        ButtonFlag = 1;
                        rc = pthread_create(&buttonThread, NULL, ButtonListenFunction, NULL);
                        assert(rc == 0);

                        while(1)
                        {
                            memset(tempString, 0, sizeof(char)*InputLength);
                            gets(tempString);
                            sleep(1);
                            if(strncmp(tempString, "XXXM", 4) == 0)
                            {
                                char doubleCheckFixerNo[InputLength];
                                memset(doubleCheckFixerNo, 0, sizeof(char)*InputLength);
                                tempPtr = tempString + 4;
                                memcpy(doubleCheckFixerNo, tempPtr, sizeof(tempString)-3);
                                if(strcmp(FixerNo, doubleCheckFixerNo) == 0)
                                {
                                    //get ip address & time
                                    fd = socket(AF_INET, SOCK_DGRAM, 0);
                                    ifr.ifr_addr.sa_family = AF_INET;
                                    strncpy(ifr.ifr_name, ZHNetworkType, IFNAMSIZ-1);
                                    ioctl(fd, SIOCGIFADDR, &ifr);
                                    close(fd);
                                    gettimeofday(&now, NULL);

                                    pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                                    fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s %ld 0 0 %02d\n", 
                                                                          ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                          inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                          MachineCode, FixerNo, (long)changeIntoRepairmodeTimeStemp.tv_sec, MachREPAIRDone);
#else
                                    fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s %ld 0 0 %02d\n", 
                                                                          ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                          inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                          MachineCode, FixerNo, (long)changeIntoRepairmodeTimeStemp.tv_sec, MachREPAIRDone);
#endif
                                    fclose(pfile);
            
                                    FTPFlag = 1;
                                    rc = pthread_create(&FTPThread, NULL, FTPFunction, NULL);
                                    assert(rc == 0);
                                    sleep(1);

                                    FTPFlag = 0;
                                    pthread_mutex_lock(&mutexFTP);
                                    pthread_cond_signal(&condFTP);
                                    pthread_mutex_unlock(&mutexFTP);
                                    pthread_join(FTPThread, NULL);
                                    break;
                                }
                                printf("FixerNo scan error code\n");
                            }else if(strncmp(tempString, "UUU", 3) == 0)
                            {
                                char fixItem[InputLength];
                                memset(fixItem, 0, sizeof(char)*InputLength);
                                tempPtr = tempString + 3;
                                memcpy(fixItem, tempPtr, sizeof(tempString)-2);

                                //get ip address & time
                                fd = socket(AF_INET, SOCK_DGRAM, 0);
                                ifr.ifr_addr.sa_family = AF_INET;
                                strncpy(ifr.ifr_name, ZHNetworkType, IFNAMSIZ-1);
                                ioctl(fd, SIOCGIFADDR, &ifr);
                                close(fd);
                                gettimeofday(&now, NULL);

                                pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                                fprintf(pfile, "%s %s %s 0 %ld 0 %s %d %s %s %ld 0 0 %02d\n",
                                                                             ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                             inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), atoi(fixItem),
                                                                             MachineCode, FixerNo, (long)changeIntoRepairmodeTimeStemp.tv_sec, MachREPAIRING);
#else
                                fprintf(pfile, "%s %s %s 0 %ld 0 %s %d %s %s %ld 0 0 %02d\n", 
                                                                             ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                             inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), atoi(fixItem),
                                                                             MachineCode, FixerNo, (long)changeIntoRepairmodeTimeStemp.tv_sec, MachREPAIRING);
#endif
                                fclose(pfile);
                            }else
                            {
                                printf("FixerNo scan eror code\n");
                            }
                        }
                        ButtonFlag = 0;
                        pthread_join(buttonThread, NULL);
                        break;
                    }
                    printf("UserNo scan error code\n");
                }
                digitalWrite (WiringPiPIN_15, HIGH);
                digitalWrite (WiringPiPIN_16, HIGH);
                digitalWrite (WiringPiPIN_18, LOW);

                //get ip address & time
                fd = socket(AF_INET, SOCK_DGRAM, 0);
                ifr.ifr_addr.sa_family = AF_INET;
                strncpy(ifr.ifr_name, ZHNetworkType, IFNAMSIZ-1);
                ioctl(fd, SIOCGIFADDR, &ifr);
                close(fd);
                gettimeofday(&now, NULL);

                pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                     ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                     inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                     MachineCode, UserNo, MachUNLOCK);
#else
                fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                     ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                     inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                     MachineCode, UserNo, MachUNLOCK);
#endif
                fclose(pfile);
            }
        }
    }
    //*shm = '*';
    return 0;
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
    //CURL *curl;
    //CURLcode res;
    //curl_off_t fsize;
    //FILE *hd_src;
    struct stat file_info, file_info_2;
    char UPLoadFile_3[UPLoadFileLength];
    struct timeval now;
    struct timespec outtime;
    int FTPCount = 0;

    while(FTPFlag){
        //char Remote_url[UPLoadFileLength] = "ftp://192.168.10.254:21/home/";
        //char Remote_url[UPLoadFileLength] = "ftp://192.168.2.223:8888/";
        //struct curl_slist *headerlist=NULL;        
        long size = 0;
        
        pthread_mutex_lock(&mutexFTP);
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + FTPWakeUpValue;
        outtime.tv_nsec = now.tv_usec * 1000;
        pthread_cond_timedwait(&condFTP, &mutexFTP, &outtime);
        pthread_mutex_unlock(&mutexFTP);
        FTPCount = (FTPCount + FTPWakeUpValue) % FTPCountValue;
        pthread_mutex_lock(&mutexFile);
        if(stat(UPLoadFile, &file_info_2) == 0)
        {
            size = file_info_2.st_size;
            //printf("size:%ld\n", size);
        }
        pthread_mutex_unlock(&mutexFile);

        if(FTPCount == 0 || FTPFlag == 0 || size > 100000)
        {
            pthread_mutex_lock(&mutexFile);
            memset(UPLoadFile_3, 0, sizeof(char)*UPLoadFileLength);
            strcpy(UPLoadFile_3, UPLoadFile);
            gettimeofday(&now, NULL);
            sprintf(UPLoadFile,"%ld%s.txt",(long)now.tv_sec, MachineCode);
            pthread_mutex_unlock(&mutexFile);

            //hd_src = fopen(UPLoadFile, "a");
            //if(hd_src != NULL)
            //{
            //    fclose(hd_src);
            //}

            printf("%s\n", UPLoadFile_3);
            if(stat(UPLoadFile_3, &file_info) < 0) {
                printf("Couldnt open %s: %s\n", UPLoadFile_3, strerror(errno));
#ifdef LogMode
                Log(s, __func__, __LINE__, " FTP fail_1\n");
#endif
            }
            else if(file_info.st_size > 0)
            {
                pid_t proc = fork();
                if(proc < 0)
                {
                    printf("fork child fail\n");
                    return 0;
                }
                else if(proc == 0)
                {
                    char filePath[UPLoadFileLength];
                    char *pfile2;
                    memset(filePath, 0, sizeof(char)*UPLoadFileLength);
                    //strcpy(filePath, "/home/pi/zhlog/");
                    //strcpy(filePath, "/home/pi/works/tsw303/");
                    strcpy(filePath, UPLoadFile_3);
                    pfile2 = filePath;                       
                    printf("%s\n", pfile2);

                    execl("../.nvm/v0.10.25/bin/node", "node", "../mongodb/SendDataClient.js", filePath, (char *)0);
                    //execl("../../.nvm/v0.10.25/bin/node", "node", "../../mongodb/SendDataClient.js", filePath, (char *)0);
                }
                else
                {
                    int result = -1;
                    wait(&result);
                }
            }
            /*else if(file_info.st_size > 0)
            {
                strcat(Remote_url,UPLoadFile_3);
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
            }*/
            else;
            unlink(UPLoadFile_3);
        }
    }
#ifdef LogMode
    Log(s, __func__, __LINE__, " FTP exit\n");
#endif
}

int transferFormatINT(unsigned char x)
{
    //int ans = ((int)x /16) * 10 + ((int) x % 16);
    int ans = (int)x;
    return ans;
}
long transferFormatLONG(unsigned char x)
{
    long ans = (long)x;
    return ans;
}

void * ButtonListenFunction(void *argument)
{
    while(ButtonFlag)
    {
       if(digitalRead(WiringPiPIN_22) == 0)
       {
           MasterFlag = 0;
           printf("MasterFlag = 0\n");
       } 
    }
}
