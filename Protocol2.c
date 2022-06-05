#include "Protocol2.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Специальные символы пакетов
*/
const char PRTCL2_START_SYMBOL = '$'; // символ начала пакета
const char PRTCL2_PT_SYMBOL = '$';    // символ окончания поля типа пакета
const char PRTCL2_END_PKG_r = '\r';   // первый символ окончания пакета
const char PRTCL2_END_PKG_n = '\n';   // второй символ окончания пакета

static void DL_Reset(Prtcl2_Handle_t *prtcl2);
static void DL_IdleState(Prtcl2_Handle_t *prtcl2);
static void DL_Reception_PT_State(Prtcl2_Handle_t *prtcl2);
static void DL_Reception_MSG_State(Prtcl2_Handle_t *prtcl2);
static void DL_ProcessingState(Prtcl2_Handle_t *prtcl2);
/* Состояния Up link канла */
static void UL_IdleState(Prtcl2_Handle_t *prtcl2);
static void UL_CreatePKG(Prtcl2_Handle_t *prtcl2);
static void UL_SendPKG(Prtcl2_Handle_t *prtcl2);
static void UL_WaitState(Prtcl2_Handle_t *prtcl2);

/**
 * Таблица функций состояний КА DL-канала
*/
void (*DL_State_Table[])(Prtcl2_Handle_t *prtcl2) = {
    [PRTCL2_DL_IDLE_STATE] = DL_IdleState,
    [PRTCL2_DL_RECEPTION_PT_STATE] = DL_Reception_PT_State,
    [PRTCL2_DL_RECEPTION_MSG_STATE] = DL_Reception_MSG_State,
    [PRTCL2_DL_PROCESSING_STATE] = DL_ProcessingState};

/**
 * Таблица функций состояний КА UL-канала
*/
void (*UL_State_Table[])(Prtcl2_Handle_t *prtcl2) = {
    [PRTCL2_UL_IDLE_STATE] = UL_IdleState,
    [PRTCL2_UL_CREATE_PKG] = UL_CreatePKG,
    [PRTCL2_UL_SEND_PKG] = UL_SendPKG,
    [PRTCL2_UL_WAIT_STATE] = UL_WaitState};

/**
 * @brief Инициализация стека протокола
 * @param prtcl2 ссылка на структуру данных протокола Prtcl2_Handle_t
 * @return None
*/
void Prtcl2_Init(Prtcl2_Handle_t *prtcl2)
{
    prtcl2->dl.state = PRTCL2_DL_IDLE_STATE;
    prtcl2->dl.enable = true;
    prtcl2->dl.timeout = 3000;
    prtcl2->dl.ts = HAL_GetTick();
    /* Up link канал */
    prtcl2->ul.state = PRTCL2_UL_IDLE_STATE;
    prtcl2->ul.enable = true;
    prtcl2->ul.ts = HAL_GetTick();
    DL_Reset(prtcl2);
}

/**
 * @brief Функция обработки стека протокола
 * @param prtcl2 ссылка на структуру данных протокола Prtcl2_Handle_t
 * @return None
*/
void Prtcl2_Loop(Prtcl2_Handle_t *prtcl2)
{
    /* Если ожидание конца пакета превышено - сброс КА */
    if (HAL_GetTick() - prtcl2->dl.ts >= prtcl2->dl.timeout)
        prtcl2->dl.state = PRTCL2_DL_IDLE_STATE;

    DL_State_Table[prtcl2->dl.state](prtcl2);
    UL_State_Table[prtcl2->ul.state](prtcl2);
}

/**
 * @brief Сброс буферов полей
 * @param prtcl2 ссылка на структуру данных протокола Prtcl2_Handle_t
 * @return None
*/
static void DL_Reset(Prtcl2_Handle_t *prtcl2)
{
    prtcl2->dl.indxpt = 0;
    prtcl2->dl.indxmsg = 0;
    memset(prtcl2->dl.pt, 0, sizeof(prtcl2->dl.pt));
    memset(prtcl2->dl.msg, 0, sizeof(prtcl2->dl.msg));
}

/**
 * @brief Функция состояния PRTCL2_DL_IDLE_STATE
 * @param prtcl2 ссылка на структуру данных протокола Prtcl2_Handle_t
 * @return None
*/
static void DL_IdleState(Prtcl2_Handle_t *prtcl2)
{
    if (!prtcl2->dl.enable)
        return;

    if (prtcl2->portAvailable())
    {
        if (prtcl2->portRead() == PRTCL2_PT_SYMBOL) // найден стартовый символ
        {
            DL_Reset(prtcl2);
            prtcl2->dl.state = PRTCL2_DL_RECEPTION_PT_STATE; // переход на состояние считывания дескриптора (типа пакета)
        }
    }
    prtcl2->dl.ts = HAL_GetTick();
}

/**
 * @brief Функция состояния PRTCL2_DL_RECEPTION_PT_STATE.
 * Считывание типа пакета.
 * @param prtcl2 ссылка на структуру данных протокола Prtcl2_Handle_t
 * @return None
*/
static void DL_Reception_PT_State(Prtcl2_Handle_t *prtcl2)
{
    if (prtcl2->portAvailable())
    {
        uint8_t c = prtcl2->portRead();
        if (c == PRTCL2_PT_SYMBOL)
            prtcl2->dl.state = PRTCL2_DL_RECEPTION_MSG_STATE;
        else if ((c == PRTCL2_END_PKG_r) || (c == PRTCL2_END_PKG_n))
        {
            /* Ошибка структуры пакета -> сброс в idle */
            prtcl2->dl.state = PRTCL2_DL_IDLE_STATE;
        }
        else
        {
            if (prtcl2->dl.indxpt < sizeof(prtcl2->dl.pt)) // проверка на выход за границы буфера поля
            {
                prtcl2->dl.pt[prtcl2->dl.indxpt] = c;
                prtcl2->dl.indxpt++;
            }
        }
    }
}

/**
 * @brief Функция состояния PRTCL2_DL_RECEPTION_MSG_STATE.
 * Считывание сообщения пакета (то что лежит после поля типа пакета).
 * @param prtcl2 ссылка на структуру данных протокола Prtcl2_Handle_t
 * @return None
*/
static void DL_Reception_MSG_State(Prtcl2_Handle_t *prtcl2)
{
    if (prtcl2->portAvailable())
    {
        uint8_t c = prtcl2->portRead();
        if (c == PRTCL2_END_PKG_n)
        {
            prtcl2->dl.state = PRTCL2_DL_PROCESSING_STATE;
            /* Можем сразу вызвать процедуру обработки пакета */
            DL_ProcessingState(prtcl2);
        }
        else if ((c == PRTCL2_START_SYMBOL) || (c == PRTCL2_PT_SYMBOL))
        {
            /* Ошибка структуры пакета -> сброс в idle */
            prtcl2->dl.state = PRTCL2_DL_IDLE_STATE;
        }
        else if (c != PRTCL2_END_PKG_r)
        {
            if (prtcl2->dl.indxmsg < sizeof(prtcl2->dl.msg)) // проверка на выход за границы буфера поля
            {
                prtcl2->dl.msg[prtcl2->dl.indxmsg] = c;
                prtcl2->dl.indxmsg++;
            }
        }
    }
}

/**
 * @brief Функция состояния PRTCL2_DL_PROCESSING_STATE. В цикле 
 * происходит поиск обработчика по зарегистрированным пакетам,
 * соответствущему принятому типу пакета.
 * @param prtcl2 ссылка на структуру данных протокола Prtcl2_Handle_t
 * @return None
*/
static void DL_ProcessingState(Prtcl2_Handle_t *prtcl2)
{
    for (size_t i = 0; i < prtcl2->dl.NumOfPKG; i++)
    {
        if (!strncmp((const char *)prtcl2->dl.pt, prtcl2->dl.pkg[i].type, strlen(prtcl2->dl.pkg[i].type)))
        {
            /* Вызов обработчика пакета */
            prtcl2->dl.pkg[i].CallBack_pkg(prtcl2->dl.msg);
            break;
        }
    }
    prtcl2->dl.state = PRTCL2_DL_IDLE_STATE;
}

/* -------------------------------- Up link канал ------------------------------ */

/**
 * @brief Состояние ожидания КА Up link
 * @param prtcl2 ссылка на структуру данных протокола Prtcl2_Handle_t
 * @return None
*/
static void UL_IdleState(Prtcl2_Handle_t *prtcl2)
{
    if ((HAL_GetTick() - prtcl2->ul.ts >= prtcl2->ul.period) && prtcl2->ul.enable)
    {
        prtcl2->ul.ts = HAL_GetTick();
        prtcl2->ul.state = PRTCL2_UL_CREATE_PKG;
        prtcl2->ul.end_of_send = false;
        UL_CreatePKG(prtcl2); // сразу вызов
    }
}

/**
 * @brief Формирование пакета телеметрии. Внутри функции
 * происходит вызов пользовательской call-back 
 * функции формирования пакета, в котороу передаётся ссылка
 * на массив, в которой записать пакет
 * @param prtcl2 ссылка на структуру данных протокола Prtcl2_Handle_t
 * @return None
*/
static void UL_CreatePKG(Prtcl2_Handle_t *prtcl2)
{
    /*  */
    memset(prtcl2->ul.pkg, 0, sizeof(prtcl2->ul.pkg));
    prtcl2->portCreatePKG(prtcl2->ul.pt, prtcl2->ul.pkg);
    prtcl2->ul.state = PRTCL2_UL_SEND_PKG;
    UL_SendPKG(prtcl2); // сразу вызов
}

/**
 * @brief Отправка пакета
 * @param prtcl2 ссылка на структуру данных протокола Prtcl2_Handle_t
 * @return None
*/
static void UL_SendPKG(Prtcl2_Handle_t *prtcl2)
{
    if (prtcl2->portSendPKG(prtcl2->ul.pkg) == 0) // старт отправки успешен
    {
        prtcl2->ul.state = PRTCL2_UL_WAIT_STATE;
    }
}

/**
 * @brief Состояние ожидания завершения передачи
 * @param prtcl2 ссылка на структуру данных протокола Prtcl2_Handle_t
 * @return None
*/
static void UL_WaitState(Prtcl2_Handle_t *prtcl2)
{
    if (prtcl2->ul.end_of_send)
    {
        prtcl2->ul.end_of_send = false;
        prtcl2->ul.state = PRTCL2_UL_IDLE_STATE;
    }
}