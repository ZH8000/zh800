#include "lcd.h"

//針對wf35m 的lcd 進行 操作的function 都集中在此
//因為老化機允許多單,在工單資料時, 會將切換螢幕的button 功能disable

//傳入screenIndex 確認是要顯示哪一頁
// 0 : 工單畫面
// 1 : 計數頁面
// 2 : 關機時的黑畫面, 以後可能會有其他用途, 不過現在僅拿來顯示關機
// 3,4,5 : 都是顯示選單畫面, 差別僅是"> <" 的位只有所不同
// 6 : config 畫面 顯示機台編號 ip address 軟體日期等等
// 7 : 工單畫面, 不過進入維修狀態才會跑道這個index

//status != 表示輸入工單中輸入到那個階段
int UpdateScreenFunction(int screenIndex, int status)
{
    unsigned char infoScreen[13] = {0x31, 0x04, 0x32, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x0d};
    unsigned char countScreen[13] = {0x31, 0x04, 0x32, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0a, 0x00, 0x0d};
    unsigned char powerOffScreen[13] = {0x31, 0x04, 0x32, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0a, 0x00, 0x0d};
    unsigned char menuScreen[13] = {0x31, 0x04, 0x32, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x0a, 0x00, 0x0d};
    unsigned char configScreen[13] = {0x31, 0x04, 0x32, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0a, 0x00, 0x0d};
    unsigned char popUpScreen[18] = {0x31, 0x04, 0x31, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0a, 0x00, 0x0d};
    unsigned char endString[3] = {0x0a, 0x00, 0x0d};
    unsigned char startString[5] = {0x31, 0x04, 0x31, 0x10, 0x00};
   
    pthread_mutex_lock(&MutexLinklist);
    //count menu
    if(screenIndex == 1)
    {       
        unsigned char countNoPositionColorString[10] = {0x00, 0x69, 0x00, 0x5a, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char goodNoPositionColorString[10] = {0x00, 0x69, 0x00, 0x89, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};   
        unsigned char totalBadNoPositionColorString[10] = {0x00, 0x69, 0x00, 0xb9, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};   

        unsigned char * commandArrayPtr;
        unsigned char * commandPtr, * countPtr;
        int arraySize = 0;
 
        SendCommandMessageFunction(countScreen, 13);
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
        //sleep(1);

        //count no.
        if(ZHList !=NULL)
        {
            arraySize = strlen(ZHList->CountNo);
#ifdef DEBUG
            printf("array size:%d\n", arraySize);
#endif
            commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(18 + arraySize)); 
            memset(commandArrayPtr, 0, sizeof(unsigned char)*(18 + arraySize));
            commandPtr = commandArrayPtr;
            memcpy(commandPtr, startString, 5);
            commandPtr = commandPtr + 5;
            memcpy(commandPtr, countNoPositionColorString, 10);
            commandPtr = commandPtr + 10;
            memcpy(commandPtr, ZHList->CountNo, arraySize);
            commandPtr = commandPtr + arraySize;
            memcpy(commandPtr, endString, 3);
            SendCommandMessageFunction(commandArrayPtr, 18 + arraySize);
            if(commandArrayPtr != NULL) 
            {
                free(commandArrayPtr);
                commandPtr = NULL;
            }
 
            //good count
            commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(18 + 7));
            memset(commandArrayPtr, 0, sizeof(unsigned char)*(18 + 7));
            countPtr = (unsigned char *)malloc(sizeof(unsigned char)*7);
            memset(countPtr, 0, sizeof(unsigned char)*7);

         
            sprintf(countPtr, "%ld", (GoodCount%1000000));

            commandPtr = commandArrayPtr;
            memcpy(commandPtr, startString, 5);
            commandPtr = commandPtr + 5;
            memcpy(commandPtr, goodNoPositionColorString, 10);
            commandPtr = commandPtr + 10;
            memcpy(commandPtr, countPtr, 7);
            commandPtr = commandPtr + 7;
            memcpy(commandPtr, endString, 3);
            SendCommandMessageFunction(commandArrayPtr, 18 + 7);
            if(commandArrayPtr != NULL) 
            {
                free(commandArrayPtr);
                commandPtr = NULL;
            }
            if(countPtr != NULL)
            {
                free(countPtr);
                countPtr = NULL;
            }

            commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(18 + 7));
            memset(commandArrayPtr, 0, sizeof(unsigned char)*(18 + 7));
            countPtr = (unsigned char *)malloc(sizeof(unsigned char)*7);
            memset(countPtr, 0, sizeof(unsigned char)*7);
         
            sprintf(countPtr, "%ld", (TotalBadCount%1000000));

            commandPtr = commandArrayPtr;
            memcpy(commandPtr, startString, 5);
            commandPtr = commandPtr + 5;
            memcpy(commandPtr, totalBadNoPositionColorString, 10);
            commandPtr = commandPtr + 10;
            memcpy(commandPtr, countPtr, 7);
            commandPtr = commandPtr + 7;
            memcpy(commandPtr, endString, 3);
            SendCommandMessageFunction(commandArrayPtr, 18 + 7);
            if(commandArrayPtr != NULL) 
            {
                free(commandArrayPtr);
                commandPtr = NULL;
            }
            if(countPtr != NULL)
            {
                free(countPtr);
                countPtr = NULL;
            }
        }
        SendCommandMessageFunction(popUpScreen, 18);
    }else if(screenIndex == 99)
    {
        unsigned char BarcodeError1[24] = {0x31, 0x04, 0x31, 0x13, 0x10, 
                   0x00, 0x5a, 0x00, 0x78, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 'E', 'r', 'r', 'o', 'r', '1', 0x0a, 0x00, 0x0d};

        unsigned char BarcodeError2[24] = {0x31, 0x04, 0x31, 0x13, 0x10, 
                   0x00, 0x5a, 0x00, 0x78, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 'E', 'r', 'r', 'o', 'r', '2', 0x0a, 0x00, 0x0d};

        unsigned char BarcodeError3[24] = {0x31, 0x04, 0x31, 0x13, 0x10, 
                   0x00, 0x5a, 0x00, 0x78, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 'E', 'r', 'r', 'o', 'r', '3', 0x0a, 0x00, 0x0d};

        unsigned char BarcodeError4[24] = {0x31, 0x04, 0x31, 0x13, 0x10, 
                   0x00, 0x5a, 0x00, 0x78, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 'E', 'r', 'r', 'o', 'r', '4', 0x0a, 0x00, 0x0d};

        unsigned char BarcodeError5[24] = {0x31, 0x04, 0x31, 0x13, 0x10, 
                   0x00, 0x5a, 0x00, 0x78, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 'E', 'r', 'r', 'o', 'r', '5', 0x0a, 0x00, 0x0d};

        if(status == 1)
            SendCommandMessageFunction(BarcodeError1, 28);
        else if(status == 2)
            SendCommandMessageFunction(BarcodeError2, 28);
        else if(status == 3)
            SendCommandMessageFunction(BarcodeError3, 28);
        else if(status == 4)
            SendCommandMessageFunction(BarcodeError4, 28);
        else
            SendCommandMessageFunction(BarcodeError5, 28);
    }else if(screenIndex == 2)
    {
        //poweroff
        SendCommandMessageFunction(powerOffScreen, 13);
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
        SendCommandMessageFunction(popUpScreen, 18);
    }else if(screenIndex == 3)
    {
        unsigned char bar1[34] = {0x31,0x04,0x31,0x03,0x00,0x00,0x48,0x00,0x37,0xFF,0xFF,0xFF,0x00,0x00,0x00,
                                           '>',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','<', 0x0a, 0x00, 0x0d};

        SendCommandMessageFunction(menuScreen, 13);
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);

        SendCommandMessageFunction(bar1, 34);
        SendCommandMessageFunction(popUpScreen, 18);
    }else if (screenIndex == 4)
    {
        unsigned char bar2[34] = {0x31,0x04,0x31,0x03,0x00,0x00,0x48,0x00,0x69,0xFF,0xFF,0xFF,0x00,0x00,0x00,
                                           '>',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','<', 0x0a, 0x00, 0x0d};
        SendCommandMessageFunction(menuScreen, 13);
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);

        SendCommandMessageFunction(bar2, 34);
        SendCommandMessageFunction(popUpScreen, 18);
    }else if(screenIndex == 5)
    {
        unsigned char bar3[34] = {0x31,0x04,0x31,0x03,0x00,0x00,0x48,0x00,0x9D,0xFF,0xFF,0xFF,0x00,0x00,0x00,
                                           '>',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','<', 0x0a, 0x00, 0x0d};
        SendCommandMessageFunction(menuScreen, 13);
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);

        SendCommandMessageFunction(bar3, 34);
        SendCommandMessageFunction(popUpScreen, 18);
    }else if(screenIndex == 6)
    {
        //input new order only
        unsigned char lotPostionColorString[10] = {0x00, 0x5f, 0x00, 0x3c, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char partPositionColorString1[10] = {0x00, 0x5f, 0x00, 0x5f, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char partPositionColorString2[10] = {0x00, 0x5f, 0x00, 0x82, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char countNoPositionColorString[10] = {0x00, 0x82, 0x00, 0xa5, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char userNoPositionColorString[10] = {0x00, 0x82, 0x00, 0xcf, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};

        unsigned char * commandArrayPtr;
        unsigned char * commandPtr,  * managerPtr;
        int arraySize = 0;

        SendCommandMessageFunction(infoScreen, 13);
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);

        if(ZHNode != NULL)
        {
            if(strlen(ZHNode->ISNo) > 0)
            { 
                commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*32); 
                memset(commandArrayPtr, 0, sizeof(unsigned char)*32);
                commandPtr = commandArrayPtr;
                memcpy(commandPtr, startString, 5);
                commandPtr = commandPtr + 5;
                memcpy(commandPtr, lotPostionColorString, 10);
                commandPtr = commandPtr + 10;
                memcpy(commandPtr, ZHNode->ISNo, 14);
                commandPtr = commandPtr +14;
                memcpy(commandPtr, endString, 3);

                //31 4 31 3 10 0 78 0 3c ff ff ff 0 0 0 31 32 33 34 35 36 37 38 39 30 31 32 33 34 a 0 d
                SendCommandMessageFunction(commandArrayPtr, 32);
                if(commandArrayPtr != NULL) 
                {
                    free(commandArrayPtr);
                    commandPtr = NULL;
                }
            }
            if(strlen(ZHNode->ManagerCard) > 0)
            {
                //part no. 1
                commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*28);
                memset(commandArrayPtr, 0, sizeof(unsigned char)*28);
                commandPtr = commandArrayPtr;
                memcpy(commandPtr, startString, 5);
                commandPtr = commandPtr + 5;
                memcpy(commandPtr, partPositionColorString1, 10);
                commandPtr = commandPtr + 10;
                memcpy(commandPtr, ZHNode->ManagerCard, 10);
                commandPtr = commandPtr + 10;
                memcpy(commandPtr, endString, 3);

                SendCommandMessageFunction(commandArrayPtr, 28);
                if(commandArrayPtr != NULL)
                {
                    free(commandArrayPtr);
                    commandPtr = NULL;
                }
        
                //part no. 2
                commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char )*32);
                memset(commandArrayPtr, 0, sizeof(unsigned char)*32);
                commandPtr = commandArrayPtr;
                memcpy(commandPtr, startString, 5);
                commandPtr = commandPtr + 5;
                memcpy(commandPtr, partPositionColorString2, 10);
                commandPtr = commandPtr + 10;
                managerPtr = ZHNode->ManagerCard;
                managerPtr = managerPtr + 10;
                memcpy(commandPtr, managerPtr, 14);
                commandPtr = commandPtr + 14;
                memcpy(commandPtr, endString, 3);

                SendCommandMessageFunction(commandArrayPtr, 32);
                if(commandArrayPtr != NULL)
                {
                    free(commandArrayPtr);
                    commandPtr = NULL;
                }
                if(managerPtr != NULL)
                {
                    managerPtr = NULL;
                }
            }

            //count no.
            arraySize = strlen(ZHNode->CountNo);
#ifdef DEBUG
            printf("array size:%d\n", arraySize);
#endif
            commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(18 + arraySize)); 
            memset(commandArrayPtr, 0, sizeof(unsigned char)*(18 + arraySize));
            commandPtr = commandArrayPtr;
            memcpy(commandPtr, startString, 5);
            commandPtr = commandPtr + 5;
            memcpy(commandPtr, countNoPositionColorString, 10);
            commandPtr = commandPtr + 10;
            memcpy(commandPtr, ZHNode->CountNo, arraySize);
            commandPtr = commandPtr + arraySize;
            memcpy(commandPtr, endString, 3);

            //31 4 31 3 10 0 78 0 3c ff ff ff 0 0 0 31 32 33 34 35 36 37 38 39 30 31 32 33 34 a 0 d
            SendCommandMessageFunction(commandArrayPtr, 18 + arraySize);
            if(commandArrayPtr != NULL) 
            {
                free(commandArrayPtr);
                commandPtr = NULL;
            }
                
            //user no
            arraySize = strlen(ZHNode->UserNo);
            char *userNoPtr = ZHNode->UserNo;
            if(arraySize > 25)
            {
                arraySize = arraySize - 24;
                userNoPtr = ZHNode->UserNo + 24;
            }
            commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(18 + arraySize)); 
            memset(commandArrayPtr, 0, sizeof(unsigned char)*(18 + arraySize));
            commandPtr = commandArrayPtr;
            memcpy(commandPtr, startString, 5);
            commandPtr = commandPtr + 5;
            memcpy(commandPtr, userNoPositionColorString, 10);
            commandPtr = commandPtr + 10;
            memcpy(commandPtr, userNoPtr, arraySize);
            commandPtr = commandPtr + arraySize;
            memcpy(commandPtr, endString, 3);

            SendCommandMessageFunction(commandArrayPtr, 18 + arraySize);
            if(commandArrayPtr != NULL) 
            {
                free(commandArrayPtr);
                commandPtr = NULL;
            }
        }
        SendCommandMessageFunction(popUpScreen, 18);
    }else if(screenIndex == 7)
    {
        //for user change
        unsigned char lotPostionColorString[10] = {0x00, 0x5f, 0x00, 0x3c, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char partPositionColorString1[10] = {0x00, 0x5f, 0x00, 0x5f, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char partPositionColorString2[10] = {0x00, 0x5f, 0x00, 0x82, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char countNoPositionColorString[10] = {0x00, 0x82, 0x00, 0xa5, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};

        unsigned char * commandArrayPtr;
        unsigned char * commandPtr,  * managerPtr;
        int arraySize = 0;

        SendCommandMessageFunction(infoScreen, 13);
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
        //sleep(1);
        
        //lot no

        if(ZHList != NULL)
        {
            if(strlen(ZHList->ISNo) > 0)
            { 
                commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*32); 
                memset(commandArrayPtr, 0, sizeof(unsigned char)*32);
                commandPtr = commandArrayPtr;
                memcpy(commandPtr, startString, 5);
                commandPtr = commandPtr + 5;
                memcpy(commandPtr, lotPostionColorString, 10);
                commandPtr = commandPtr + 10;
                memcpy(commandPtr, ZHList->ISNo, 14);
                commandPtr = commandPtr +14;
                memcpy(commandPtr, endString, 3);

                //31 4 31 3 10 0 78 0 3c ff ff ff 0 0 0 31 32 33 34 35 36 37 38 39 30 31 32 33 34 a 0 d
                SendCommandMessageFunction(commandArrayPtr, 32);
                if(commandArrayPtr != NULL) 
                {
                    free(commandArrayPtr);
                    commandPtr = NULL;
                }
            }
            if(strlen(ZHList->ManagerCard) > 0)
            {
                //part no. 1
                commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*28);
                memset(commandArrayPtr, 0, sizeof(unsigned char)*28);
                commandPtr = commandArrayPtr;
                memcpy(commandPtr, startString, 5);
                commandPtr = commandPtr + 5;
                memcpy(commandPtr, partPositionColorString1, 10);
                commandPtr = commandPtr + 10;
                memcpy(commandPtr, ZHList->ManagerCard, 10);
                commandPtr = commandPtr + 10;
                memcpy(commandPtr, endString, 3);

                SendCommandMessageFunction(commandArrayPtr, 28);
                if(commandArrayPtr != NULL)
                {
                    free(commandArrayPtr);
                    commandPtr = NULL;
                }
        
                //part no. 2
                commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char )*32);
                memset(commandArrayPtr, 0, sizeof(unsigned char)*32);
                commandPtr = commandArrayPtr;
                memcpy(commandPtr, startString, 5);
                commandPtr = commandPtr + 5;
                memcpy(commandPtr, partPositionColorString2, 10);
                commandPtr = commandPtr + 10;
                managerPtr = ZHList->ManagerCard;
                managerPtr = managerPtr + 10;
                memcpy(commandPtr, managerPtr, 14);
                commandPtr = commandPtr + 14;
                memcpy(commandPtr, endString, 3);

                SendCommandMessageFunction(commandArrayPtr, 32);
                if(commandArrayPtr != NULL)
                {
                    free(commandArrayPtr);
                    commandPtr = NULL;
                }
                if(managerPtr != NULL)
                {
                    managerPtr = NULL;
                }
            }

            //count no.
            arraySize = strlen(ZHList->CountNo);
#ifdef DEBUG
            printf("array size:%d\n", arraySize);
#endif
            commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(18 + arraySize)); 
            memset(commandArrayPtr, 0, sizeof(unsigned char)*(18 + arraySize));
            commandPtr = commandArrayPtr;
            memcpy(commandPtr, startString, 5);
            commandPtr = commandPtr + 5;
            memcpy(commandPtr, countNoPositionColorString, 10);
            commandPtr = commandPtr + 10;
            memcpy(commandPtr, ZHList->CountNo, arraySize);
            commandPtr = commandPtr + arraySize;
            memcpy(commandPtr, endString, 3);

            //31 4 31 3 10 0 78 0 3c ff ff ff 0 0 0 31 32 33 34 35 36 37 38 39 30 31 32 33 34 a 0 d
            SendCommandMessageFunction(commandArrayPtr, 18 + arraySize);
            if(commandArrayPtr != NULL) 
            {
                free(commandArrayPtr);
                commandPtr = NULL;
            }
        }
        SendCommandMessageFunction(popUpScreen, 18);
    }else if(screenIndex == 8)
    {
        //config
        unsigned char machineNoPositionColorString[10] = {0x00, 0x8c, 0x00, 0x8c, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char ipAddressPositionColorString[10] = {0x00, 0x64, 0x00, 0xc8, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};   
        unsigned char versionPositionColorString[18] = {0x00, 0x00, 0x00, 0x50, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 'V', 'E', 'R', 'S', 'I', 'N', ':'};   

        unsigned char * commandArrayPtr;
        unsigned char * commandPtr, * countPtr;
        int arraySize = 0;
 
        SendCommandMessageFunction(configScreen, 13);
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
        //sleep(1);

        arraySize = strlen(VERSION);
#ifdef DEBUG
        printf("array size:%d\n", arraySize);
#endif
        commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(26 + arraySize)); 
        memset(commandArrayPtr, 0, sizeof(unsigned char)*(26 + arraySize));
        commandPtr = commandArrayPtr;
        memcpy(commandPtr, startString, 5);
        commandPtr = commandPtr + 5;
        memcpy(commandPtr, versionPositionColorString, 18);
        commandPtr = commandPtr + 18;
        memcpy(commandPtr, VERSION, arraySize);
        commandPtr = commandPtr + arraySize;
        memcpy(commandPtr, endString, 3);
        SendCommandMessageFunction(commandArrayPtr, 26 + arraySize);
        if(commandArrayPtr != NULL) 
        {
            free(commandArrayPtr);
            commandPtr = NULL;
        }

        int fd2 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct ifreq ethreq;
        memset(&ethreq, 0, sizeof(ethreq));
        strncpy(ethreq.ifr_name, ZHNETWORKTYPE, IFNAMSIZ);
        ioctl(fd2, SIOCGIFFLAGS, &ethreq);

        if(ethreq.ifr_flags & IFF_RUNNING)
        {
            char ipAddrArray[17] ;
            int fd = socket(AF_INET, SOCK_DGRAM, 0);
            struct ifreq ifr;
            ifr.ifr_addr.sa_family = AF_INET;
            strncpy(ifr.ifr_name, ZHNETWORKTYPE, IFNAMSIZ-1);
            ioctl(fd, SIOCGIFADDR, &ifr);
            close(fd);

            memset(ipAddrArray, 0, sizeof(char)*17);
            strncpy(ipAddrArray, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), sizeof(char)*17);
            arraySize = 17;
            commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(18 + arraySize)); 
            memset(commandArrayPtr, 0, sizeof(unsigned char)*(18 + arraySize));
            commandPtr = commandArrayPtr;
            memcpy(commandPtr, startString, 5);
            commandPtr = commandPtr + 5;
            memcpy(commandPtr, ipAddressPositionColorString, 10);
            commandPtr = commandPtr + 10;
            memcpy(commandPtr, ipAddrArray, arraySize);
            commandPtr = commandPtr + arraySize;
            memcpy(commandPtr, endString, 3);
            SendCommandMessageFunction(commandArrayPtr, 18 + arraySize);
            if(commandArrayPtr != NULL) 
            {
                free(commandArrayPtr);
                commandPtr = NULL;
            }
        }else
        {
            ;//
        }
        close(fd2);

        //machine no.
        arraySize = strlen(MachineNo);
#ifdef DEBUG
        printf("array size:%d\n", arraySize);
#endif
        commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(18 + arraySize)); 
        memset(commandArrayPtr, 0, sizeof(unsigned char)*(18 + arraySize));
        commandPtr = commandArrayPtr;
        memcpy(commandPtr, startString, 5);
        commandPtr = commandPtr + 5;
        memcpy(commandPtr, machineNoPositionColorString, 10);
        commandPtr = commandPtr + 10;
        memcpy(commandPtr, MachineNo, arraySize);
        commandPtr = commandPtr + arraySize;
        memcpy(commandPtr, endString, 3);
        SendCommandMessageFunction(commandArrayPtr, 18 + arraySize);
        if(commandArrayPtr != NULL) 
        {
            free(commandArrayPtr);
            commandPtr = NULL;
        }
        SendCommandMessageFunction(popUpScreen, 18);
    }else
    {
        /*unsigned char lotPostionColorString[10] = {0x00, 0x78, 0x00, 0x3c, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char partPositionColorString1[10] = {0x00, 0x78, 0x00, 0x5f, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char partPositionColorString2[10] = {0x00, 0x64, 0x00, 0x82, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char countNoPositionColorString[10] = {0x00, 0x96, 0x00, 0xa5, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char userNoPositionColorString[10] = {0x00, 0x96, 0x00, 0xcf, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        */
        unsigned char lotPostionColorString[10] = {0x00, 0x5f, 0x00, 0x3c, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char partPositionColorString1[10] = {0x00, 0x5f, 0x00, 0x5f, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char partPositionColorString2[10] = {0x00, 0x5f, 0x00, 0x82, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char countNoPositionColorString[10] = {0x00, 0x82, 0x00, 0xa5, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        unsigned char userNoPositionColorString[10] = {0x00, 0x82, 0x00, 0xcf, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};


        unsigned char * commandArrayPtr;
        unsigned char * commandPtr,  * managerPtr;
        int arraySize = 0;

        SendCommandMessageFunction(infoScreen, 13);
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
        //sleep(1);
        
        //lot no

        if(ZHList != NULL)
        {
            if(strlen(ZHList->ISNo) > 0)
            { 
                commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*32); 
                memset(commandArrayPtr, 0, sizeof(unsigned char)*32);
                commandPtr = commandArrayPtr;
                memcpy(commandPtr, startString, 5);
                commandPtr = commandPtr + 5;
                memcpy(commandPtr, lotPostionColorString, 10);
                commandPtr = commandPtr + 10;
                memcpy(commandPtr, ZHList->ISNo, 14);
                commandPtr = commandPtr +14;
                memcpy(commandPtr, endString, 3);

                //31 4 31 3 10 0 78 0 3c ff ff ff 0 0 0 31 32 33 34 35 36 37 38 39 30 31 32 33 34 a 0 d
                SendCommandMessageFunction(commandArrayPtr, 32);
                if(commandArrayPtr != NULL) 
                {
                    free(commandArrayPtr);
                    commandPtr = NULL;
                }
            }
            if(strlen(ZHList->ManagerCard) > 0)
            {
                //part no. 1
                commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*28);
                memset(commandArrayPtr, 0, sizeof(unsigned char)*28);
                commandPtr = commandArrayPtr;
                memcpy(commandPtr, startString, 5);
                commandPtr = commandPtr + 5;
                memcpy(commandPtr, partPositionColorString1, 10);
                commandPtr = commandPtr + 10;
                memcpy(commandPtr, ZHList->ManagerCard, 10);
                commandPtr = commandPtr + 10;
                memcpy(commandPtr, endString, 3);

                SendCommandMessageFunction(commandArrayPtr, 28);
                if(commandArrayPtr != NULL)
                {
                    free(commandArrayPtr);
                    commandPtr = NULL;
                }
        
                //part no. 2
                commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char )*32);
                memset(commandArrayPtr, 0, sizeof(unsigned char)*32);
                commandPtr = commandArrayPtr;
                memcpy(commandPtr, startString, 5);
                commandPtr = commandPtr + 5;
                memcpy(commandPtr, partPositionColorString2, 10);
                commandPtr = commandPtr + 10;
                managerPtr = ZHList->ManagerCard;
                managerPtr = managerPtr + 10;
                memcpy(commandPtr, managerPtr, 14);
                commandPtr = commandPtr + 14;
                memcpy(commandPtr, endString, 3);

                SendCommandMessageFunction(commandArrayPtr, 32);
                if(commandArrayPtr != NULL)
                {
                    free(commandArrayPtr);
                    commandPtr = NULL;
                }
                if(managerPtr != NULL)
                {
                    managerPtr = NULL;
                }
            }

            //count no.
            arraySize = strlen(ZHList->CountNo);
#ifdef DEBUG
            printf("array size:%d\n", arraySize);
#endif
            commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(18 + arraySize)); 
            memset(commandArrayPtr, 0, sizeof(unsigned char)*(18 + arraySize));
            commandPtr = commandArrayPtr;
            memcpy(commandPtr, startString, 5);
            commandPtr = commandPtr + 5;
            memcpy(commandPtr, countNoPositionColorString, 10);
            commandPtr = commandPtr + 10;
            memcpy(commandPtr, ZHList->CountNo, arraySize);
            commandPtr = commandPtr + arraySize;
            memcpy(commandPtr, endString, 3);

            //31 4 31 3 10 0 78 0 3c ff ff ff 0 0 0 31 32 33 34 35 36 37 38 39 30 31 32 33 34 a 0 d
            SendCommandMessageFunction(commandArrayPtr, 18 + arraySize);
            if(commandArrayPtr != NULL) 
            {
                free(commandArrayPtr);
                commandPtr = NULL;
            }
                
            //user no
            arraySize = strlen(ZHList->UserNo);
            char *userNoPtr = ZHList->UserNo;
            if(arraySize > 25)
            {
                arraySize = arraySize - 24;
                userNoPtr = ZHList->UserNo + 24;
            }  
            commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(18 + arraySize)); 
            memset(commandArrayPtr, 0, sizeof(unsigned char)*(18 + arraySize));
            commandPtr = commandArrayPtr;
            memcpy(commandPtr, startString, 5);
            commandPtr = commandPtr + 5;
            memcpy(commandPtr, userNoPositionColorString, 10);
            commandPtr = commandPtr + 10;
            memcpy(commandPtr, userNoPtr, arraySize);
            commandPtr = commandPtr + arraySize;
            memcpy(commandPtr, endString, 3);

            SendCommandMessageFunction(commandArrayPtr, 18 + arraySize);
            if(commandArrayPtr != NULL) 
            {
                free(commandArrayPtr);
                commandPtr = NULL;
            }
        }
        SendCommandMessageFunction(popUpScreen, 18);
    }
    pthread_mutex_unlock(&MutexLinklist);
    return 0;
}

void * ChangeScreenEventListenFunction(void *argument)
{
    int screenIndex = 3;
    int listOffset = 0;
    char flagForZHPIN32, flagForZHPIN36, flagForZHPIN38, flagForZHPIN22;

    flagForZHPIN32 = flagForZHPIN36 = flagForZHPIN38 = flagForZHPIN22 = 0;
    //ScreenIndex = screenIndex;

    while(1)
    {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        if(ScreenIndex == 6 || ScreenIndex  == 7)
        {
            nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
        }
        else
        {
            if(DisableUpDown == 0 && flagForZHPIN32 == 0 && digitalRead(ZHPIN32) == 0)
            {
                printf("get ZHPIN_32 event\n");
                //screenIndex = (screenIndex + 1) % 2;
                screenIndex = (screenIndex + 1) % 3 + 3;
                pthread_mutex_lock(&MutexScreen);
                ScreenIndex = screenIndex;
                flagForZHPIN32 = 1; 
                UpdateScreenFunction(screenIndex, 0);
                pthread_mutex_unlock(&MutexScreen);
                listOffset = 0;
            }else if(flagForZHPIN32 == 1 && digitalRead(ZHPIN32) == 1)
            {
                flagForZHPIN32 = 0;
            }else if(DisableUpDown == 0 && flagForZHPIN22 == 0 && digitalRead(ZHPIN22) == 0)
            {
                printf("get ZHPIN_22 event\n");
                //screenIndex = (screenIndex + 1) % 2;
                pthread_mutex_lock(&MutexScreen);
                screenIndex = (screenIndex - 1) % 3 + 3;
                ScreenIndex = screenIndex;
                flagForZHPIN36 = 1; 
                UpdateScreenFunction(screenIndex, 0);
                pthread_mutex_unlock(&MutexScreen);
                listOffset = 0;
            }else if(flagForZHPIN22 == 1 && digitalRead(ZHPIN22) == 1)
            {
                flagForZHPIN22 = 0;
            }else if(flagForZHPIN36 == 0 && digitalRead(ZHPIN36) == 0)
            {
                printf("get ZHPIN_36 event\n");
                pthread_mutex_lock(&MutexScreen);
                ScreenIndex = screenIndex;
                flagForZHPIN36 = 1;
                UpdateScreenFunction(screenIndex, 0);
                pthread_mutex_unlock(&MutexScreen);
                listOffset = 0;
            }else if(flagForZHPIN36 == 1 && digitalRead(ZHPIN36) == 1)
            {
                flagForZHPIN36 = 0;
                DisableUpDown = 0;
            }else if(DisableUpDown == 0 && flagForZHPIN38 == 0 && digitalRead(ZHPIN38) == 0)
            {
                pthread_mutex_lock(&MutexScreen);
                printf("get ZHPIN_38 event\n");
                if(screenIndex == 3)
                {
                    ScreenIndex = 0;
                    UpdateScreenFunction(0, 0);
                }
                else if(screenIndex == 4)
                {
                    ScreenIndex = 1;
                    UpdateScreenFunction(1, 0);
                }
                else
                {
                    ScreenIndex = 8;
                    UpdateScreenFunction(8, 0);
                }
                pthread_mutex_unlock(&MutexScreen);
                flagForZHPIN38 = 1;
                DisableUpDown = 1;
                listOffset = 0;
            }else if(flagForZHPIN38 == 1 && digitalRead(ZHPIN38) == 1)
            {
                flagForZHPIN38 = 0;
                listOffset = 0;
            }else if((ScreenIndex == 0) && DisableUpDown == 1 && flagForZHPIN32 == 0 && digitalRead(ZHPIN32) == 0)
            {
                if(ZHList != NULL)
                {
                    InputNode *nodePtr;
                    nodePtr = ZHList;
                    pthread_mutex_lock(&MutexScreen);
                    if(listOffset == 0) listOffset = 1;
                    else
                    {
                        if(nodePtr->link != NULL) nodePtr = nodePtr->link;
                        else nodePtr = ZHList;
                        listOffset = 0;
                    }
                    printf("listOffset: %d\n", listOffset);
                    unsigned char infoScreen[13] = {0x31, 0x04, 0x32, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x0d};
                    unsigned char popUpScreen[18] = {0x31, 0x04, 0x31, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0a, 0x00, 0x0d};
                    unsigned char endString[3] = {0x0a, 0x00, 0x0d};
                    unsigned char startString[5] = {0x31, 0x04, 0x31, 0x10, 0x00};

                    /*unsigned char lotPostionColorString[10] = {0x00, 0x78, 0x00, 0x3c, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
                    unsigned char partPositionColorString1[10] = {0x00, 0x78, 0x00, 0x5f, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
                    unsigned char partPositionColorString2[10] = {0x00, 0x64, 0x00, 0x82, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
                    unsigned char countNoPositionColorString[10] = {0x00, 0x96, 0x00, 0xa5, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
                    unsigned char userNoPositionColorString[10] = {0x00, 0x96, 0x00, 0xcf, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
                    */

                    unsigned char lotPostionColorString[10] = {0x00, 0x5f, 0x00, 0x3c, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
                    unsigned char partPositionColorString1[10] = {0x00, 0x5f, 0x00, 0x5f, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
                    unsigned char partPositionColorString2[10] = {0x00, 0x5f, 0x00, 0x82, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
                    unsigned char countNoPositionColorString[10] = {0x00, 0x82, 0x00, 0xa5, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
                    unsigned char userNoPositionColorString[10] = {0x00, 0x82, 0x00, 0xcf, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};

                    unsigned char * commandArrayPtr;
                    unsigned char * commandPtr,  * managerPtr;
                    int arraySize = 0;

                    SendCommandMessageFunction(infoScreen, 13);
                    nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
                    //sleep(1);
        
                    //lot no
                    if(strlen(nodePtr->ISNo) > 0)
                    { 
                        commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*32); 
                        memset(commandArrayPtr, 0, sizeof(unsigned char)*32);
                        commandPtr = commandArrayPtr;
                        memcpy(commandPtr, startString, 5);
                        commandPtr = commandPtr + 5;
                        memcpy(commandPtr, lotPostionColorString, 10);
                        commandPtr = commandPtr + 10;
                        memcpy(commandPtr, nodePtr->ISNo, 14);
                        commandPtr = commandPtr +14;
                        memcpy(commandPtr, endString, 3);

                        //31 4 31 3 10 0 78 0 3c ff ff ff 0 0 0 31 32 33 34 35 36 37 38 39 30 31 32 33 34 a 0 d
                        SendCommandMessageFunction(commandArrayPtr, 32);
                        if(commandArrayPtr != NULL) 
                        {
                            free(commandArrayPtr);
                            commandPtr = NULL;
                        }
                    }
                    if(strlen(nodePtr->ManagerCard) > 0)
                    {
                        //part no. 1
                        commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*28);
                        memset(commandArrayPtr, 0, sizeof(unsigned char)*28);
                        commandPtr = commandArrayPtr;
                        memcpy(commandPtr, startString, 5);
                        commandPtr = commandPtr + 5;
                        memcpy(commandPtr, partPositionColorString1, 10);
                        commandPtr = commandPtr + 10;
                        memcpy(commandPtr, nodePtr->ManagerCard, 10);
                        commandPtr = commandPtr + 10;
                        memcpy(commandPtr, endString, 3);

                        SendCommandMessageFunction(commandArrayPtr, 28);
                        if(commandArrayPtr != NULL)
                        {
                            free(commandArrayPtr);
                            commandPtr = NULL;
                        }
        
                        //part no. 2
                        commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char )*32);
                        memset(commandArrayPtr, 0, sizeof(unsigned char)*32);
                        commandPtr = commandArrayPtr;
                        memcpy(commandPtr, startString, 5);
                        commandPtr = commandPtr + 5;
                        memcpy(commandPtr, partPositionColorString2, 10);
                        commandPtr = commandPtr + 10;
                        managerPtr = nodePtr->ManagerCard;
                        managerPtr = managerPtr + 10;
                        memcpy(commandPtr, managerPtr, 14);
                        commandPtr = commandPtr + 14;
                        memcpy(commandPtr, endString, 3);

                        SendCommandMessageFunction(commandArrayPtr, 32);
                        if(commandArrayPtr != NULL)
                        {
                            free(commandArrayPtr);
                            commandPtr = NULL;
                        }
                        if(managerPtr != NULL)
                        {
                            managerPtr = NULL;
                        }
                    }

                    //count no.
                    arraySize = strlen(nodePtr->CountNo);
#ifdef DEBUG
                    printf("array size:%d\n", arraySize);
#endif
                    commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(18 + arraySize)); 
                    memset(commandArrayPtr, 0, sizeof(unsigned char)*(18 + arraySize));
                    commandPtr = commandArrayPtr;
                    memcpy(commandPtr, startString, 5);
                    commandPtr = commandPtr + 5;
                    memcpy(commandPtr, countNoPositionColorString, 10);
                    commandPtr = commandPtr + 10;
                    memcpy(commandPtr, nodePtr->CountNo, arraySize);
                    commandPtr = commandPtr + arraySize;
                    memcpy(commandPtr, endString, 3);

                    //31 4 31 3 10 0 78 0 3c ff ff ff 0 0 0 31 32 33 34 35 36 37 38 39 30 31 32 33 34 a 0 d
                    SendCommandMessageFunction(commandArrayPtr, 18 + arraySize);
                    if(commandArrayPtr != NULL) 
                    {
                        free(commandArrayPtr);
                        commandPtr = NULL;
                    }
                
                    //user no
                    arraySize = strlen(nodePtr->UserNo);
                    char *userNoPtr = nodePtr->UserNo;
                    if(arraySize > 25)
                    {
                        arraySize = arraySize - 24;
                        userNoPtr = nodePtr->UserNo + 24;
                    }  
                    commandArrayPtr = (unsigned char *)malloc(sizeof(unsigned char)*(18 + arraySize)); 
                    memset(commandArrayPtr, 0, sizeof(unsigned char)*(18 + arraySize));
                    commandPtr = commandArrayPtr;
                    memcpy(commandPtr, startString, 5);
                    commandPtr = commandPtr + 5;
                    memcpy(commandPtr, userNoPositionColorString, 10);
                    commandPtr = commandPtr + 10;
                    memcpy(commandPtr, userNoPtr, arraySize);
                    commandPtr = commandPtr + arraySize;
                    memcpy(commandPtr, endString, 3);

                    SendCommandMessageFunction(commandArrayPtr, 18 + arraySize);
                    if(commandArrayPtr != NULL) 
                    {
                       free(commandArrayPtr);
                       commandPtr = NULL;
                    }
                    SendCommandMessageFunction(popUpScreen, 18);
                    pthread_mutex_unlock(&MutexScreen);
                }else;
            }
            else;
        }
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
        //sleep(0);
    }
}

int SendCommandMessageFunction (unsigned char *message, int arrayLength)
{
    unsigned char *copyMessageArray;
    int forCount = 0;

#ifdef ZHCHECKSCREENBUSY
    unsigned char x[2] = {0x00, 0x00};
    int maxRetry = 100;

    while(x[1] != 0x01)
    {
        x[0] = 0x01;
        x[1] = 0x00;
        wiringPiSPIDataRW(CSCHANNEL, x, 2);
        printf("we wait %x\n", x[1]);
        if(x[1] == 0x01) break;
        nanosleep((const struct timespec[]){{0, 10000000L}}, NULL);
        maxRetry--;
        if(maxRetry <= 0)
        {
            digitalWrite (ZHPIN33, LOW);
            //nanosleep((const struct timespec[]){{0, 200000000L}}, NULL);
            sleep(2);    

            digitalWrite (ZHPIN33, HIGH);
            //nanosleep((const struct timespec[]){{0, 1500000000L}}, NULL);
            sleep(2);    
            maxRetry = 100;
        }

    }
    printf("lcd idle\n");
#endif

    copyMessageArray = (unsigned char *)malloc(sizeof(unsigned char)*arrayLength);

    memset(copyMessageArray, 0, sizeof(unsigned char)*arrayLength);
    memcpy(copyMessageArray, message, arrayLength);

#ifdef DEBUG   
    printf("%s ", __func__); 
    for(forCount = 0; forCount < arrayLength; forCount++)
    {
        printf("%x ", copyMessageArray[forCount]);
    }
    printf("\n");
#endif

    wiringPiSPIDataRW(CSCHANNEL, copyMessageArray, arrayLength);
    //nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
    //sleep(1);
    if(copyMessageArray != NULL)
    {
        //printf("free alloc memory\n");
        free(copyMessageArray);
    }
    /*while(x[0] != 0x00)
    {
        x[0] = 0x01;
        wiringPiSPIDataRW(CSCHANNEL, x, 1);
        nanosleep((const struct timespec[]){{0, 10000000L}}, NULL);
    }
    printf("lcd idle\n");*/
    return 0;
}
