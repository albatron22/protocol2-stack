#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include "Protocol2_call_back.h"
#include "../Protocol2.h"

Protocol2_Handle_t prtcl_2;            // протокол v2.0
Protocol2_Package_t prtcl_packages[] = // массив структур даннных для типов DL-пакетов
    {
        [0] = {"PT1", PT1_PKG_CallBack},  // пакет 1
        [1] = {"PT2", PT2_PKG_CallBack}}; // пакет 2

size_t SerialAvailable();
uint8_t SerialRead();
void SerialClean();
uint32_t Timestamp();

//////////////////////////////////////////
//////////////////////////////////////////
uint8_t rx_buffer[] = "$PT1$*\x11\r\n$PT2$30,50,-40*\x3D\r\n";
size_t indx_read = 0;
bool rx_flag = false;

int main()
{
    /* Протокол */
    prtcl_2.dl.pkg = prtcl_packages;
    prtcl_2.dl.num_of_pkg = sizeof(prtcl_packages) / sizeof(Protocol2_Package_t);
    prtcl_2.VPortAvailable = SerialAvailable;
    prtcl_2.VPortRead = SerialRead;
    prtcl_2.VPortClean = SerialClean;
    prtcl_2.VGetTick_ms = Timestamp;
    Protocol2_Init(&prtcl_2);

    /* Проверка генерации сообщения */
    uint8_t pt[] = "TELEM";
    uint8_t data[] = "prsd564c";
    uint8_t pkg[64] = {0};

    size_t pkg_size = Protocol2_CreatePKG(&prtcl_2, pkg, pt, data, strlen(data));
    printf("%sSize: %d\r\n\r\n", pkg, pkg_size);

    while (true)
    {
        Protocol2_Loop(&prtcl_2);

        if(rx_flag)
        {
            printf("\r\nts:\t%d", Timestamp());
            break;
        }
            
    }
    
    return 0;
}

size_t SerialAvailable()
{
    return strlen((char *)rx_buffer) - indx_read;
}

uint8_t SerialRead()
{
    uint8_t c = '\0';
    if (indx_read < sizeof(rx_buffer))
    {
        c = rx_buffer[indx_read];
        indx_read++;
    }
    return c;
}

void SerialClean()
{
    return;
}

uint32_t Timestamp()
{
    struct timeval te;
    gettimeofday(&te, NULL);                                         // get current time
    uint32_t milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;  // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}