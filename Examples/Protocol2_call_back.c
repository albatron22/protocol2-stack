#include "Protocol2_call_back.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

extern bool rx_flag;

/**
 * @brief Call-back функция обработчика пакета по типу <PT1>.
 * --- Пример ---
 * @param data массив блока полезных данных, соответствущее типу пакета
 * @param data_length размер блока данных
 * @return None
 */
void PT1_PKG_CallBack(uint8_t *data, size_t data_length)
{
    printf("Recieve <PT1> PKG\nDATA:\t%s\r\nData size:\t%d bytes", data, data_length);
    rx_flag = true;
}

/**
 * @brief Call-back функция обработчика пакета по типу <PT3>.
 * --- Пример ---
 * @param data массив блока полезных данных, соответствущее типу пакета
 * @param data_length размер блока данных
 * @return None
 */
void PT2_PKG_CallBack(uint8_t *data, size_t data_length)
{
    printf("Recieve <PT2> PKG\nDATA:\t%s\r\nData size:\t%d bytes", data, data_length);
    rx_flag = true;
}
