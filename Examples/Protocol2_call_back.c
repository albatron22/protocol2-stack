#include "Protocol2_call_back.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Call-back функция обработчика пакета по типу <PT1>.
 * --- Пример ---
 * @param msg массив сообщения, соответствущее типу пакета
 * @return None
*/
void PT1_PKG_CallBack(uint8_t *msg)
{
    printf("Recieve <PT1> PKG");
}

/**
 * @brief Call-back функция обработчика пакета по типу <PT3>.
 * --- Пример ---
 * @param msg массив сообщения, соответствущее типу пакета
 * @return None
*/
void PT2_PKG_CallBack(uint8_t *msg)
{
    printf("Recieve <PT2> PKG");
}
