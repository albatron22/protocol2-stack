#include "Protocol2_call_back.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

extern bool rx_flag;

/**
 * @brief Call-back функция обработчика пакета по типу <PT1>.
 * --- Пример ---
 * @param msg массив сообщения, соответствущее типу пакета
 * @return None
*/
void PT1_PKG_CallBack(uint8_t *msg)
{
    printf("Recieve <PT1> PKG\nDATA:\t%s", msg);
    rx_flag = true;
}

/**
 * @brief Call-back функция обработчика пакета по типу <PT3>.
 * --- Пример ---
 * @param msg массив сообщения, соответствущее типу пакета
 * @return None
*/
void PT2_PKG_CallBack(uint8_t *msg)
{
    printf("Recieve <PT2> PKG\nDATA:\t%s", msg);
    rx_flag = true;
}
