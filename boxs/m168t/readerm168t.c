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

#define WatchDogCountValue 21000 //msec
#define InputLength 30 
#define UPLoadFileLength 21
#define CountPeriod 300
#define WriteFileCountValue 4200
#define FTPCountValue 300
#define FTPWakeUpValue 60
#define ERRORCHECKMAXRETRY 5 


#define Log(s,func, line, opt) StringCat(func);StringCat(opt)
#define RS232_Length 41

#define goodrate 1
//#define LogMode
#define PrintInfo
#define PrintMode

#define ProductCount 4
#define Information 4

enum
{
    GoodNumber = 0,
    BadTape,
    BadShort,
    BadCycle
};

enum
{
    MechSTART = 1,
    MechSTOP,
    MechRUNNING,
    MechREPAIR,
    MechHUMAN,
    MechLOCK,
    MechUNLOCK,
    MechEND
};

enum
{
    BadLayout_1 = 0,
    BadLayout_2,
    BadLayout_3,
    BadLayout_4,
};

long LayoutCount[Information][8];
long ExLayoutCount[Information][8];

int SerialThreadFlag;
short zhResetFlag = 0;
int PrintLeftLog = 0;
int FileFlag = 0;
int updateFlag = 0;
short zhTelnetFlag = 0;
short FTPFlag = 0;
short ButtonFlag = 0;
short MasterFlag = 0;

char *shm, *s, *tail;
char *shm_pop;
unsigned char output[RS232_Length];

pthread_cond_t cond, condFTP;
pthread_mutex_t mutex, mutex_3, mutex_log, mutex_2, mutexFTP, mutexFile;

long productCountArray[ProductCount];
long ExproductCountArray[ProductCount];
int messageArray[Information];
int ExmessageArray[Information];

char ISNo[InputLength], ManagerCard[InputLength], MachineCode[InputLength], UserNo[InputLength], CountNo[InputLength];
char UPLoadFile[UPLoadFileLength];

void * zhLogFunction(void *argument);
void * FTPFunction(void *arguemnt);
void StringCat(const char *str);
void * SerialFunction(void *argument);
void * FileFunction(void *argement);
void * RemoteControl(void *argument);
void * ButtonListenFunction(void *argument);

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

/*void * RemoteControl(void * argument)
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
    while( TelnetFlag == 1 && (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted\n");
        TelnetFlag = 0;
        break;
    }
    printf("Telnet close\n"); 
    if(close(socket_desc) < 0)
    {
        printf("erron %d",errno);
    }
    //close(socket_desc);
    //free(socket_desc);
    sleep(1);
}*/

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
    fprintf(pfile,"m168t\t%s\t0\t", buff);
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
                    fprintf(pfile,"m168t\t%s\t%ld\t",buff,productCountArray[GoodNumber]);
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
    unsigned char temp_output[RS232_Length];
    int count1, count2;
    count1 = count2 = 0;

    memset(temp_output, 0, sizeof(char)*RS232_Length);

    if ((fd = serialOpen ("/dev/ttyAMA0", 9600)) < 0)
    {
        printf ("Unable to open serial device: %s\n", strerror (errno)) ;
        pthread_exit((void*)"fail");
    }
    /*if (wiringPiSetup () == -1)
    {
        printf ("Unable to start wiringPi: %s\n", strerror (errno)) ;
        pthread_exit((void*)"fail");
    }*/
    
    int string_count = 0;
    while(SerialThreadFlag)
    {
        while(serialDataAvail(fd))
        {   
            unsigned char temp_char_1;
            temp_char_1 = serialGetchar(fd);
            //printf("%x ", temp_char_1);
            if(count1 < 4)
            {
                if(temp_char_1 == 0xdd)
                {
                    count1++;
                }
                else count1 = 0;
            }
            else if(count2 == 3 && temp_char_1 == 0xee && string_count < RS232_Length )
            { 
                temp_output[string_count] = temp_char_1;
                string_count++;
                //a lock for avoid read write at the same time
                int vers_test_count = 0;
		        pthread_mutex_lock(&mutex_2);
                memset(output, 0, sizeof(output));
                
                for(vers_test_count = 0; vers_test_count < RS232_Length; vers_test_count++)
                {
                    output[vers_test_count] = temp_output[vers_test_count];
                    //printf("%x ", output[vers_test_count]);
                }
                //printf("\n");
                memset(temp_output, 0, sizeof(temp_output));
                count1 = 0;
                count2 = 0;
                string_count = 0;
                updateFlag = 1;
                pthread_mutex_unlock(&mutex_2);
            }
            else if( string_count < RS232_Length)
            {
                if(temp_char_1 == 0xee)
                {
                    count2++;
                    temp_output[string_count] = temp_char_1;
                    string_count++;
                }
                else
                {
                    count2 = 0;
                    temp_output[string_count] = temp_char_1;
                    string_count++;
                }
            }
            else
            {
                printf("array overflow\n");
                memset(temp_output, 0, sizeof(char)* RS232_Length);
                count1 = 0;
                count2 = 0;
                string_count = 0;
            }
            fflush (stdout) ;
            //if (SerialThreadFlag == 0) break;
        }
    }
    if(fd != NULL)
    {
        serialClose(fd);
    }
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
    unsigned char * string_target;
    int WriteFileCount = 0;
    unsigned char file_output_temp[RS232_Length];
    int watchdogCooldown = WatchDogCountValue;

    int fd;
    struct ifreq ifr;
    int ForCount = 0;
    int vers_count;
    short errorCheckCount[4];
    memset(errorCheckCount, 0, sizeof(short)*4);
 
    while(FileFlag)
    {
        pthread_mutex_lock(&mutex);
        gettimeofday(&now, NULL);
        //outtime.tv_sec = now.tv_sec + CountPeriod;
        outtime.tv_sec = now.tv_sec;
        //outtime.tv_nsec= now.tv_usec * 1000;
        outtime.tv_nsec= (now.tv_usec + CountPeriod *1000) * 1000;

        if(outtime.tv_nsec > 1000000000)
        {
            outtime.tv_sec = outtime.tv_sec + 1;
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

            memset(productCountArray, 0, sizeof(long)*ProductCount);
            memset(messageArray, 0, sizeof(int)*Information);
 
            memset(file_output_temp, 0, sizeof(char)*RS232_Length);

            pthread_mutex_lock(&mutex_2);
            strncpy(file_output_temp, output, RS232_Length);
            for(vers_count = 0; vers_count < RS232_Length; vers_count++)
            {
                file_output_temp[vers_count] = output[vers_count];
            }
            updateFlag = 0;
            pthread_mutex_unlock(&mutex_2);

            for(ForCount = 6 ; ForCount < 9 ; ForCount++)
            {
                string_target = &file_output_temp[ForCount];
		        productCountArray[GoodNumber] = productCountArray[GoodNumber] + transferFormatLONG(*string_target) * pow(256, (ForCount-6));
            }
            for(ForCount = 9 ; ForCount < 11 ; ForCount++)
            {
                string_target = &file_output_temp[ForCount];
                productCountArray[BadTape] =  productCountArray[BadTape] + transferFormatLONG(*string_target) * pow(256, (ForCount - 9));
            }
            for(ForCount = 11 ; ForCount < 13 ; ForCount++)
            {
                string_target = &file_output_temp[ForCount];
                productCountArray[BadShort] =  productCountArray[BadShort] + transferFormatLONG(*string_target) * pow(256, (ForCount - 11));
            }
            for(ForCount = 13 ; ForCount < 15 ; ForCount++)
            {
                string_target = &file_output_temp[ForCount];
                productCountArray[BadCycle] =  productCountArray[BadCycle] + transferFormatLONG(*string_target) * pow(256, (ForCount - 13));
            }

            string_target = &file_output_temp[18];
            messageArray[BadLayout_1] = (int)*string_target;
       
            string_target = &file_output_temp[19];
            messageArray[BadLayout_2] = (int)*string_target;
       
            string_target = &file_output_temp[20];
            messageArray[BadLayout_3] =  (int)*string_target;
       
            string_target = &file_output_temp[21];
            messageArray[BadLayout_4] =  (int)*string_target;

            for(ForCount = 0; ForCount < 4; ++ForCount)
            {
                if(abs(ExproductCountArray[ForCount] - productCountArray[ForCount]) > 220 && errorCheckCount[ForCount] < ERRORCHECKMAXRETRY)
                {
                    productCountArray[ForCount] = ExproductCountArray[ForCount];
                    errorCheckCount[ForCount]++;
                }else
                {
                    errorCheckCount[ForCount] = 0;
                }
            }         

#ifdef PrintInfo
            printf("%s %s %s %s %s || ",ISNo, ManagerCard, CountNo, MachineCode, UserNo);
            printf("1:%x ", messageArray[BadLayout_1]);   
            printf("2:%x ", messageArray[BadLayout_2]);   
            printf("3:%x ", messageArray[BadLayout_3]);   
            printf("4:%x ", messageArray[BadLayout_4]);   
            printf("%ld %ld %ld %ld\n",productCountArray[GoodNumber],productCountArray[BadTape],productCountArray[BadShort], productCountArray[BadCycle]);
#endif
            
            for(ForCount = 0; ForCount < Information; ForCount++)
            {
                if(messageArray[ForCount]!=0)
                {
                    int ForCount2;
                    int messagepaser = messageArray[ForCount];
                    int messagepaser2 = ExmessageArray[ForCount];
                    int flag = 0;
                    for(ForCount2 = 0; ForCount2 < 8; ForCount2++)
                    {
                        if(((messagepaser & 1) == 1) &&((messagepaser2 & 1) == 0))
                        {
                            LayoutCount[ForCount][ForCount2]++; 
                            flag = 1;
                        }
                        messagepaser = messagepaser >> 1; 
                        messagepaser2 = messagepaser2 >> 1; 
                    }
                    if(flag == 1) ExmessageArray[ForCount] = messageArray[ForCount];
                }
                //[vers || no need othrea like angle or precess]
                /*else if(messageArray[ForCount] != 0 && (ExmessageArray[ForCount] !=  messageArray[ForCount]))
                {
                    //LayoutCount[ForCount][0] = messageArray[ForCount];
                    fprintf(file_dst, "%s %s %s %ld %ld %d %s %d %s %s\n", ISNo, ManagerCard, CountNo, productCountArray[GoodNumber],
                                                                                     (long)now.tv_sec,
                                                                                   //LayoutCount[ForCount][0],
                                                                                   messageArray[ForCount],
                                                                                   inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                   (ForCount*8+1)+3, MachineCode, UserNo);
                }else ;*/
                //[vers || end]
            }
       }

       WriteFileCount = (WriteFileCount + CountPeriod) % WriteFileCountValue;
       if(WriteFileCount == 0 || FileFlag == 0)
       {
            short newDataIncome = 0;

            //[vers|2014.10.25 | initial count number]
            for(ForCount = 0; ForCount < ProductCount; ++ForCount)
            {
                //need set ExproductCountArray
                if(productCountArray[ForCount] < ExproductCountArray[ForCount])
                {
                    ExproductCountArray[ForCount] = 0;
                }else if((productCountArray[ForCount] != 0) && (ExproductCountArray[ForCount] == 0))
                {
                    ExproductCountArray[ForCount] = productCountArray[ForCount];
                }
                else;
            }
            //[vers|2014.10.25|end] 

            pthread_mutex_lock(&mutexFile);
            file_dst = fopen(UPLoadFile, "a");
            for(ForCount = 0 ; ForCount < Information; ForCount++)
            {
                int ForCount2;
                for(ForCount2 = 0; ForCount2 < 8; ForCount2++)
                {
                    if(ExLayoutCount[ForCount][ForCount2] != LayoutCount[ForCount][ForCount2])
                    {
#ifdef PrintMode
                        fprintf(file_dst, "%s %s %s 0 %ld %ld %s %d %s %s 0 0 0 %02d\n", 
                                                                           ISNo, ManagerCard, CountNo, 
                                                                           (long)now.tv_sec, 
                                                                           LayoutCount[ForCount][ForCount2]-ExLayoutCount[ForCount][ForCount2],
                                                                           inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                           (ForCount*8 + ForCount2+1)+ProductCount, MachineCode, UserNo, 
                                                                           MechRUNNING);
#else
                        fprintf(file_dst, "%s %s %s %ld %ld %ld %s %d %s %s 0 0 0 %02d\n", 
                                                                                  ISNo, ManagerCard, CountNo, productCountArray[GoodNumber], 
                                                                                  (long)now.tv_sec, LayoutCount[ForCount][ForCount2],
                                                                                  inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                                  (ForCount*8 + ForCount2+1)+ProductCount, MachineCode, UserNo,
                                                                                  MechRUNNING);
#endif
                        if(newDataIncome == 0) newDataIncome = 1;
                    } 
                }
            }
            for(ForCount = 0 ; ForCount < ProductCount; ForCount++)
            {
               if(productCountArray[ForCount]!= 0 && ExproductCountArray[ForCount] != productCountArray[ForCount])
               {
                   if(ForCount == 0)
                   {
#ifdef PrintMode
                       fprintf(file_dst, "%s %s %s %ld %ld 0 %s %d %s %s 0 0 0 %02d\n", 
                                                                      ISNo, ManagerCard, CountNo, 
                                                                      productCountArray[GoodNumber] - ExproductCountArray[GoodNumber], 
                                                                      (long)now.tv_sec,
                                                                      inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                      ForCount+1, MachineCode, UserNo, MechRUNNING);
#else
                       fprintf(file_dst, "%s %s %s %ld %ld 0 %s %d %s %s 0 0 0 %02d\n", 
                                                                      ISNo, ManagerCard, CountNo, productCountArray[GoodNumber], (long)now.tv_sec,
                                                                      inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                      ForCount+1, MachineCode, UserNo, MechRUNNING);
#endif
                   }
                   else
                   {
#ifdef PrintMode
                       fprintf(file_dst, "%s %s %s 0 %ld %ld %s %d %s %s 0 0 0 %02d\n", 
                                                                     ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                     productCountArray[ForCount] - ExproductCountArray[ForCount],
                                                                     inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                     ForCount+1, MachineCode, UserNo, MechRUNNING);
#else
                       fprintf(file_dst, "%s %s %s %ld %ld %ld %s %d %s %s 0 0 0 %02d\n", 
                                                                     ISNo, ManagerCard, CountNo, productCountArray[GoodNumber], (long)now.tv_sec,
                                                                     productCountArray[ForCount],
                                                                     inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                     ForCount+1, MachineCode, UserNo, MechRUNNING);
#endif
                   }
                   if(newDataIncome == 0) newDataIncome = 1;
               }
            }
            fclose(file_dst);
            pthread_mutex_unlock(&mutexFile);

            /*if(abs(ExproductCountArray[GoodNumber] - productCountArray[GoodNumber]) > 256)
            {
                file_dst = fopen("/home/pi/works/m168t/log","a");
                for(vers_count = 0; vers_count < RS232_Length; vers_count++)
                {
                    fprintf(file_dst, "%x ", file_output_temp[vers_count]);
                }
                fprintf(file_dst, "%ld %ld\n", ExproductCountArray[GoodNumber], productCountArray[GoodNumber]);
                fclose(file_dst);
            }*/
            memcpy(ExproductCountArray, productCountArray, sizeof(long)*ProductCount);
            memcpy(ExmessageArray, messageArray, sizeof(int)*Information);
            memcpy(ExLayoutCount, LayoutCount, sizeof(long)*Information*8);

            //a timeout mechanism
            /*if(newDataIncome == 1)
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
            }*/
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
    pthread_t WatchDogThread, LogThread, SerialThread, FileThread, FTPThread;
    int shmid;
    key_t key;

    //Log(str, "part_1",__FILE__, __func__,__LINE__, "");

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
    struct ifreq ifr;
  
    printf("oooooooooooooooooooooooooooooooooooo\n");

 
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
    
    //wiringPiISR(WiringPiPIN_11, INT_EDGE_FALLING, &Interrupt1);
    //wiringPiISR(WiringPiPIN_12, INT_EDGE_FALLING, &Interrupt2);
    //wiringPiISR(WiringPiPIN_13, INT_EDGE_FALLING, &Interrupt3);
    
    //the mechine always standby
    while(1)
    {
        digitalWrite (WiringPiPIN_15, HIGH);
        digitalWrite (WiringPiPIN_16, HIGH);
        digitalWrite (WiringPiPIN_18, HIGH);
        /*fd = open(dev, O_RDWR);
        if(fd < 0)
        {
            error("Open Fail");
            return 1;
        }
        r = ioctl(fd, I2C_SLAVE, I2C_IO_Extend_1);
        if(r < 0)
        {
            perror("Selecting i2c device");
            return 1;
        }
        i2c_smbus_write_byte_data(fd, OUT_P0, 0x00);
        i2c_smbus_write_byte_data(fd, CONFIG_P0, 0x03);

        i2c_smbus_write_byte_data(fd, OUT_P1, 0x00);
        i2c_smbus_write_byte_data(fd, CONFIG_P1, 0x00); 

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
        i2c_smbus_write_byte_data(fd, CONFIG_P0, 0x03);

        i2c_smbus_write_byte_data(fd, OUT_P1, 0x00);
        i2c_smbus_write_byte_data(fd, CONFIG_P1, 0x00);
        close(fd);
*/
/*        fd = open(dev, O_RDWR);
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
        i2c_smbus_write_byte_data(fd, OUT_P1, 0x01);
        i2c_smbus_write_byte_data(fd, CONFIG_P1, 0x00);
        close(fd);
*/
        //3rd i3c board will control 3*8 control

        printf("Ready to work..\n");
/* 
        while(1)
        {
            sleep(1);
            memset(tempString, 0, sizeof(char)* InputLength);
            gets(tempString);
            if(strncmp(tempString, "YYY", 3) == 0)
            {
                memset(ISNo, 0, sizeof(char)*InputLength);
                tempPtr = tempString + 3;
                memcpy(ISNo, tempPtr, sizeof(tempString)-2);
                // ready for light some led;
                digitalWrite(WiringPiPIN_15, LOW);
                digitalWrite(WiringPiPIN_16, HIGH);
                digitalWrite(WiringPiPIN_18, HIGH);
                break;
            }
            printf("scan ISNo error code\n");
        }
        while(1)
        {
            sleep(1);
            memset(tempString, 0, sizeof(char)*InputLength);
            gets(tempString);
            if(strncmp(tempString, "QQQ", 3) == 0)
            {
                memset(ManagerCard, 0, sizeof(char)*InputLength);
                tempPtr = tempString + 3;
                memcpy(ManagerCard, tempPtr, sizeof(tempString)-2);
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
            if(strncmp(tempString, "WWW", 3) == 0)
            {
                memset(CountNo, 0, sizeof(char)*InputLength);
                tempPtr = tempString + 3;
                memcpy(CountNo, tempPtr, sizeof(tempString)-2);
                goodCount = (atoi(CountNo)*goodrate);
                digitalWrite (WiringPiPIN_15, HIGH);
                digitalWrite (WiringPiPIN_16, HIGH);
                digitalWrite (WiringPiPIN_18, LOW);
                break;
            }
            printf("CountNo scan error code\n");
        } 

        while(1)
        {
            sleep(1);
            memset(tempString, 0 , sizeof(char)*InputLength);
            gets(tempString);
            if(strncmp(tempString, "ZZZ", 3) == 0)
            {
                memset(MachineCode, 0 , sizeof(char)*InputLength);
                tempPtr = tempString + 3;
                memcpy(MachineCode, tempPtr, sizeof(tempString)-2);
                digitalWrite (WiringPiPIN_15, LOW);
                digitalWrite (WiringPiPIN_16, LOW);
                digitalWrite (WiringPiPIN_18, HIGH);
                break;
            }
            printf("MachineCode scan error code\n");
        }
 
        while(1)
        {
            sleep(1);
            memset(tempString, 0, sizeof(char)*InputLength);
            gets(tempString);
            if(strncmp(tempString, "XXX", 3) == 0)
            {
                memset(UserNo, 0, sizeof(char)*InputLength);
                tempPtr = tempString + 3;
                memcpy(UserNo, tempPtr, sizeof(tempString)-2);
                digitalWrite (WiringPiPIN_15, LOW);
                digitalWrite (WiringPiPIN_16, HIGH);
                digitalWrite (WiringPiPIN_18, LOW);
                break;
            }
            printf("UserNo scan error code\n");
        }
*/       

        char FakeInput[5][InputLength];
        memset(FakeInput, 0, sizeof(char)*(5*InputLength));
        int filesize, FakeInputNumber = 0;
        int FakeInputNumber_2 = 0;
        char * buffer, * charPosition;
        short FlagNo = 0;        

        pfile = fopen("/home/pi/works/m168t/barcode","r");
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
        sleep(1);
        memset(ISNo, 0, sizeof(char)*InputLength);
        //strcpy(ISNo, "01");
        strcpy(ISNo, FakeInput[0]);
        digitalWrite (WiringPiPIN_15, LOW);
        digitalWrite (WiringPiPIN_16, HIGH);
        digitalWrite (WiringPiPIN_18, HIGH);

        sleep(1);
        memset(ManagerCard, 0, sizeof(char)*InputLength);
        //strcpy(ManagerCard, "BL20123456790");
        strcpy(ManagerCard, FakeInput[1]);
        digitalWrite (WiringPiPIN_15, HIGH);
        digitalWrite (WiringPiPIN_16, LOW);
        digitalWrite (WiringPiPIN_18, HIGH);

        sleep(1);
        memset(MachineCode, 0 , sizeof(char)*InputLength);
        //strcpy(MachineCode, "E35");
        strcpy(MachineCode, FakeInput[2]);
        digitalWrite (WiringPiPIN_15, LOW);
        digitalWrite (WiringPiPIN_16, LOW);
        digitalWrite (WiringPiPIN_18, HIGH);

        sleep(1);
        memset(CountNo, 0, sizeof(char)*InputLength);
        //strcpy(CountNo, "100000");
        strcpy(CountNo, FakeInput[3]);
        goodCount = (atoi(CountNo)*goodrate);
        digitalWrite (WiringPiPIN_15, HIGH);
        digitalWrite (WiringPiPIN_16, HIGH);
        digitalWrite (WiringPiPIN_18, LOW);

        sleep(1);
        memset(UserNo, 0, sizeof(char)*InputLength);
        //strcpy(UserNo, "6957");
        strcpy(UserNo, FakeInput[4]);
        digitalWrite (WiringPiPIN_15, LOW);
        digitalWrite (WiringPiPIN_16, HIGH);
        digitalWrite (WiringPiPIN_18, LOW);


        memset(UPLoadFile, 0, sizeof(char)*UPLoadFileLength);
        gettimeofday(&now, NULL);
        sprintf(UPLoadFile,"%ld%s.txt",(long)now.tv_sec, MachineCode); 
        
        printf("%s %s %s %s %s %s\n", ISNo, ManagerCard, MachineCode, UserNo, CountNo, UPLoadFile);
 
        MasterFlag = 1;
        
        memset(ExproductCountArray, 0, sizeof(long)*ProductCount);
        memset(ExmessageArray, 0, sizeof(int)*Information);
        memset(LayoutCount, 0, sizeof(long)*(Information*8));
        memset(ExLayoutCount, 0, sizeof(long)*(Information*8));
        
        if(zhTelnetFlag == 0){
            //zhTelnetFlag = 1;
            //rc = pthread_create(&TelnetControlThread, NULL, RemoteControl, NULL);
            //assert(rc == 0);
        }

        while(MasterFlag)
        {
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
                sleep(1);
                if(productCountArray[GoodNumber]  >= goodCount)
                //if(productCountArray[GoodNumber]  >= 1000)
                {
                    //finish job
                    sleep(10);
                    printf("Houston we are ready to back!\n");
                    zhResetFlag = 1;
                    MasterFlag = 0;
                }
/*              else if(digitalRead(WiringPiPIN_22) == 1)
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
                    SerialThreadFlag = 0;
                    FileFlag = 0;
                    MasterFlag = 0;
                }
*/
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
                }*/
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

            //MechSTOP & MechHUMAN
            //get ip address
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            ifr.ifr_addr.sa_family = AF_INET;
            strncpy(ifr.ifr_name, ZHNetworkType, IFNAMSIZ-1);
            ioctl(fd, SIOCGIFADDR, &ifr);
            close(fd);

            if(MasterFlag == 0)
            {
                pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                    inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                    MachineCode, UserNo, MechEND);
#else
                fprintf(pfile, "%s %s %s %ld %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, productCountArray[GoodNumber], (long)now.tv_sec,
                                                                    inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                    MachineCode, UserNo, MechEND);
#endif
                fclose(pfile);
            }else
            {
                pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                    inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                    MachineCode, UserNo, MechLOCK);
#else
                fprintf(pfile, "%s %s %s %ld %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, productCountArray[GoodNumber], (long)now.tv_sec,
                                                                    inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                    MachineCode, UserNo, MechLOCK);
#endif
                fclose(pfile);
            }

            FTPFlag = 0;
            pthread_mutex_lock(&mutexFTP);
            pthread_cond_signal(&condFTP);
            pthread_mutex_unlock(&mutexFTP);
            pthread_join(FTPThread, NULL);
            sleep(1);

            //vers end
            /*
            if(MasterFlag)
            {
                digitalWrite (WiringPiPIN_15, HIGH);
                digitalWrite (WiringPiPIN_16, HIGH);
                digitalWrite (WiringPiPIN_18, LOW);
                while(1)
                {
                    sleep(1);
                    memset(tempString, 0, sizeof(char)*InputLength);
                    gets(tempString);
                    if(strncmp(tempString, "XXX", 3) == 0)
                    {
                        memset(UserNo, 0, sizeof(char)*InputLength);
                        tempPtr = tempString + 3;
                        memcpy(UserNo, tempPtr, sizeof(tempString)-2);
                      
                        digitalWrite (WiringPiPIN_15, LOW);
                        digitalWrite (WiringPiPIN_16, HIGH);
                        digitalWrite (WiringPiPIN_18, LOW);
                        pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                        fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                    inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                    MachineCode, UserNo, MechHUMAN);
#else
                        fprintf(pfile, "%s %s %s %ld %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, productCountArray[GoodNumber], (long)now.tv_sec,
                                                                    inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                    MachineCode, UserNo, MechHUMAN);
#endif
                        fclose(pfile);
                        break;
                    }else if(strncmp(tempString, "VVV", 3) == 0)
                    {
                        char FixerNo[InputLength];
                        pthread_t buttonThread;
                        memset(FixerNo, 0, sizeof(char)*InputLength);
                        tempPtr = tempString + 3;
                        memcpy(FixerNo, tempPtr, sizeof(tempString)-2);
                        pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                        fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                    inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                    MachineCode, FixerNo, MechREPAIR);
#else
                        fprintf(pfile, "%s %s %s %ld %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, productCountArray[GoodNumber], (long)now.tv_sec,
                                                                    inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                    MachineCode, FixerNo, MechREPAIR);
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
                            if(strncmp(tempString, "VVV", 3) == 0)
                            {
                                memset(FixerNo, 0, sizeof(char)*InputLength);
                                tempPtr = tempString + 3;
                                memcpy(FixerNo, tempPtr, sizeof(tempString)-2);
                                
                                pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                                fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                    inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                    MachineCode, FixerNo, MechREPAIR);
#else
                                fprintf(pfile, "%s %s %s %ld %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                    ISNo, ManagerCard, CountNo, productCountArray[GoodNumber], (long)now.tv_sec,
                                                                    inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                    MachineCode, FixerNo, MechREPAIR);
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
                        }

                        ButtonFlag = 0;
                        pthread_join(buttonThread, NULL);
                        break;
                    }
                    printf("UserNo scan error code\n");
                }
                pfile = fopen(UPLoadFile, "a");
#ifdef PrintMode
                fprintf(pfile, "%s %s %s 0 %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                     ISNo, ManagerCard, CountNo, (long)now.tv_sec,
                                                                     inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                     MachineCode, UserNo, MechUNLOCK);
#else
                fprintf(pfile, "%s %s %s %ld %ld 0 %s 1 %s %s 0 0 0 %02d\n", 
                                                                     ISNo, ManagerCard, CountNo, productCountArray[GoodNumber], (long)now.tv_sec,
                                                                     inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
                                                                     MachineCode, UserNo, MechUNLOCK);
#endif
                fclose(pfile);
            }*/
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
    //FILE *hd_src;
    struct stat file_info, file_info_2;
    //curl_off_t fsize;
    char UPLoadFile_3[UPLoadFileLength];
    struct timeval now;
    struct timespec outtime;
    int FTPCount = 0;

    while(FTPFlag){
        //char Remote_url[80] = "ftp://192.168.10.254:21/home/";
        //char Remote_url[80] = "ftp://192.168.2.223:8888/";
        long size = 0;
        pthread_mutex_lock(&mutexFTP);
        //struct curl_slist *headerlist=NULL;
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + FTPWakeUpValue;
        outtime.tv_nsec = now.tv_usec * 1000;
        pthread_cond_timedwait(&condFTP, &mutexFTP, &outtime);
        pthread_mutex_unlock(&mutexFTP);
        FTPCount = (FTPCount + FTPWakeUpValue) % FTPCountValue;
        pthread_mutex_lock(&mutexFile);
        if(!stat(UPLoadFile, &file_info_2))
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

            printf("%s\n", UPLoadFile_3);
            //strcat(Remote_url,UPLoadFile_3);
            if(stat(UPLoadFile_3, &file_info)) {
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
                pid_t proc = fork();
                if(proc < 0)
                {
                    printf("fork child fail\n");
                    return 0;
                }
                else if(proc == 0)
                {
                    char filePath[80];
                    char *pfile2;
                    memset(filePath, 0, sizeof(char)*80);
                    //strcpy(filePath, "/home/pi/zhlog/");
                    //strcpy(filePath, "/home/pi/works/m168t/");
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
            /*if(file_info.st_size > 0)
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
            }*/
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
