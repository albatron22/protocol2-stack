#include "Protocol2.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Специальные символы пакетов
*/
const char PRTCL2_START_SYMBOL = '$'; // символ начала пакета
const char PRTCL2_PT_SYMBOL = '$';    // символ окончания поля типа пакета
const char PRTCL2_END_PKG_CR = '\r';   // первый символ окончания пакета <CR>
const char PRTCL2_END_PKG_LF = '\n';   // второй символ окончания пакета <LF>

static void DL_Reset(Protocol2_Handle_t *prtcl2);
static void DL_IdleState(Protocol2_Handle_t *prtcl2);
static void DL_Reception_PT_State(Protocol2_Handle_t *prtcl2);
static void DL_Reception_MSG_State(Protocol2_Handle_t *prtcl2);
static void DL_ProcessingState(Protocol2_Handle_t *prtcl2);

/**
 * Таблица функций состояний КА DL-канала
*/
void (*DL_State_Table[])(Protocol2_Handle_t *prtcl2) = {
    [PRTCL2_DL_IDLE_STATE] = DL_IdleState,
    [PRTCL2_DL_RECEPTION_PT_STATE] = DL_Reception_PT_State,
    [PRTCL2_DL_RECEPTION_MSG_STATE] = DL_Reception_MSG_State,
    [PRTCL2_DL_PROCESSING_STATE] = DL_ProcessingState};

/**
 * @brief Инициализация стека протокола
 * @param prtcl2 ссылка на структуру данных протокола Protocol2_Handle_t
 * @return None
*/
void Protocol2_Init(Protocol2_Handle_t *prtcl2)
{
    /* Down-link канал */
    prtcl2->dl.state = PRTCL2_DL_IDLE_STATE;
    prtcl2->dl.enable = true;
    prtcl2->dl.timeout = 3000;
    prtcl2->dl.ts = prtcl2->VGetTick_ms();
}

/**
 * @brief Функция обработки стека протокола
 * @param prtcl2 ссылка на структуру данных протокола Protocol2_Handle_t
 * @return None
*/
void Protocol2_Loop(Protocol2_Handle_t *prtcl2)
{
    /* Если ожидание конца пакета превышено - сброс КА */
    if (prtcl2->VGetTick_ms() - prtcl2->dl.ts >= prtcl2->dl.timeout)
        prtcl2->dl.state = PRTCL2_DL_IDLE_STATE;

    DL_State_Table[prtcl2->dl.state](prtcl2);
}

/**
 * @brief Сброс буферов полей
 * @param prtcl2 ссылка на структуру данных протокола Protocol2_Handle_t
 * @return None
*/
static void DL_Reset(Protocol2_Handle_t *prtcl2)
{
    prtcl2->dl.indxpt = 0;
    prtcl2->dl.indxmsg = 0;
    memset(prtcl2->dl.pt, 0, sizeof(prtcl2->dl.pt));
    memset(prtcl2->dl.msg, 0, sizeof(prtcl2->dl.msg));
}

/**
 * @brief Функция состояния PRTCL2_DL_IDLE_STATE
 * @param prtcl2 ссылка на структуру данных протокола Protocol2_Handle_t
 * @return None
*/
static void DL_IdleState(Protocol2_Handle_t *prtcl2)
{
    if (!prtcl2->dl.enable)
        return;

    if (prtcl2->VPortAvailable())
    {
        if (prtcl2->VPortRead() == PRTCL2_PT_SYMBOL) // найден стартовый символ
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
 * @param prtcl2 ссылка на структуру данных протокола Protocol2_Handle_t
 * @return None
*/
static void DL_Reception_PT_State(Protocol2_Handle_t *prtcl2)
{
    if (prtcl2->VPortAvailable())
    {
        uint8_t c = prtcl2->VPortRead();
        if (c == PRTCL2_PT_SYMBOL)
            prtcl2->dl.state = PRTCL2_DL_RECEPTION_MSG_STATE;
        else if ((c == PRTCL2_END_PKG_CR) || (c == PRTCL2_END_PKG_LF))
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
 * @param prtcl2 ссылка на структуру данных протокола Protocol2_Handle_t
 * @return None
*/
static void DL_Reception_MSG_State(Protocol2_Handle_t *prtcl2)
{
    if (prtcl2->VPortAvailable())
    {
        uint8_t c = prtcl2->VPortRead();
        if (c == PRTCL2_END_PKG_LF)
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
        else if (c != PRTCL2_END_PKG_CR)
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
 * @param prtcl2 ссылка на структуру данных протокола Protocol2_Handle_t
 * @return None
*/
static void DL_ProcessingState(Protocol2_Handle_t *prtcl2)
{
    for (size_t i = 0; i < prtcl2->dl.numOfPKG; i++)
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