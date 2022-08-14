#include "Protocol2.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//#define DEBUG_PRINT

/**
 * Специальные символы пакетов
 */
const char PRTCL2_START_SYMBOL = '$'; // символ начала пакета
const char PRTCL2_PT_SYMBOL = '$';    // символ окончания поля типа пакета
const char PRTCL2_CS_SYMBOL = '*';    // символ окончания блока данных - разделения блока данных и байта контрольной суммы
const char PRTCL2_END_PKG_CR = '\r';  // первый символ окончания пакета <CR>
const char PRTCL2_END_PKG_LF = '\n';  // второй символ окончания пакета <LF>

uint8_t pkg[512] = {0};
bool dataIsSent = false;

static void DL_Reset(Protocol2_Handle_t *prtcl2);
static void DL_IdleState(Protocol2_Handle_t *prtcl2);
static void DL_Reception_PT_State(Protocol2_Handle_t *prtcl2);
static void DL_Reception_Payload_State(Protocol2_Handle_t *prtcl2);
static void DL_CheckSumState(Protocol2_Handle_t *prtcl2);
static void DL_ProcessingState(Protocol2_Handle_t *prtcl2);

static uint8_t CheckSumXOR(uint8_t *pt, uint8_t pt_symbol, uint8_t *data, size_t data_length);

/**
 * Таблица функций состояний КА DL-канала
 */
void (*DL_State_Table[])(Protocol2_Handle_t *prtcl2) = {
    [PRTCL2_DL_IDLE_STATE] = DL_IdleState,
    [PRTCL2_DL_RECEPTION_PT_STATE] = DL_Reception_PT_State,
    [PRTCL2_DL_RECEPTION_PAYLOAD_STATE] = DL_Reception_Payload_State,
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
 * @brief Функция создания пакета в соответствии с правилами протокола.
 * @param pkg массив для записи пакета
 * @param pkg_length длина массива записи пакета в байтах
 * @param pt строка заголовка пакета
 * @param data полезные данные в произвольном формате
 * @param data_length размер полезных данных в байтах
 * @return размер созданного пакета в байтах. Если 0 - ошибка
 */
size_t Protocol2_CreatePKG(Protocol2_Handle_t *prtcl2, uint8_t *pkg, size_t pkg_length, uint8_t *pt, uint8_t *data, size_t data_length)
{
    size_t pkg_size = 0;
    size_t pt_length = strlen(pt);

    /* Проверка размеров полея пакета */
    if (data_length > (PRTCL2_DATA_MAX_LENGTH - PRTCL2_CS_LENGTH) || pt_length > PRTCL2_PT_MAX_LENGTH || pt_length == 0)
        return 0;
    
    /**
     * Размер массива пакета должен вместить в себя все поля:
     * размер заголовка (pt)
     * размер блока данных (data_length)
     * служебные поля
     */
    if (pkg_length < (pt_length + data_length + PRTCL2_PKG_MIN_LENGTH))
        return 0;

    /* Вычисление контрольной суммы */
    uint8_t cs = CheckSumXOR(pt, PRTCL2_PT_SYMBOL, data, data_length);

    /* Сборка пакета */
    pkg_size = sprintf((char *)pkg, "%c%s%c%s*%X\r\n", PRTCL2_START_SYMBOL, pt, PRTCL2_PT_SYMBOL, data, cs);
    
    return pkg_size;
}

/**
 * @brief Сброс буферов полей
 * @param prtcl2 ссылка на структуру данных протокола Protocol2_Handle_t
 * @return None
 */
static void DL_Reset(Protocol2_Handle_t *prtcl2)
{
    prtcl2->dl.indx_pt = 0;
    prtcl2->dl.indx_msg = 0;
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
    prtcl2->dl.ts = prtcl2->VGetTick_ms();
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
            prtcl2->dl.state = PRTCL2_DL_RECEPTION_PAYLOAD_STATE;
        else if ((c == PRTCL2_END_PKG_CR) || (c == PRTCL2_END_PKG_LF))
        {
            /* Ошибка структуры пакета -> сброс в idle */
            prtcl2->dl.state = PRTCL2_DL_IDLE_STATE;
        }
        else
        {
            if (prtcl2->dl.indx_pt < sizeof(prtcl2->dl.pt)) // проверка на выход за границы буфера поля
            {
                prtcl2->dl.pt[prtcl2->dl.indx_pt] = c;
                prtcl2->dl.indx_pt++;
            }
        }
    }
}

/**
 * @brief Функция состояния PRTCL2_DL_RECEPTION_PAYLOAD_STATE.
 * Считывание блока полезных данных (то что лежит после поля типа пакета).
 * @param prtcl2 ссылка на структуру данных протокола Protocol2_Handle_t
 * @return None
 */
static void DL_Reception_Payload_State(Protocol2_Handle_t *prtcl2)
{
    if (prtcl2->VPortAvailable())
    {
        uint8_t c = prtcl2->VPortRead();
        if (c == PRTCL2_END_PKG_LF)
        {
            prtcl2->dl.state = PRTCL2_DL_PROCESSING_STATE;
            /* Можем сразу вызвать процедуру проверка контрольной суммы пакета */
            DL_CheckSumState(prtcl2);
        }
        else if ((c == PRTCL2_START_SYMBOL) || (c == PRTCL2_PT_SYMBOL))
        {
            /* Ошибка структуры пакета -> сброс в idle */
            prtcl2->dl.state = PRTCL2_DL_IDLE_STATE;
        }
        else if (c != PRTCL2_END_PKG_CR)
        {
            if (prtcl2->dl.indx_msg < sizeof(prtcl2->dl.msg)) // проверка на выход за границы буфера поля
            {
                prtcl2->dl.msg[prtcl2->dl.indx_msg] = c;
                prtcl2->dl.indx_msg++;
            }
        }
    }
}

/**
 * @brief Вычисление контрольной суммы пакета и сверка его с принятым.
 * Контрольная сумма содержится в одном массиве с блоком полезных данных
 * в самом его конце. Последние три байта массива блока полезных данных -
 * символ окончания блока данных '*' и два hex-символа байта контрольной суммы.
 * @param prtcl2 ссылка на структуру данных протокола Protocol2_Handle_t
 * @return None
 */
static void DL_CheckSumState(Protocol2_Handle_t *prtcl2)
{
    char cs_symbol[3] = "\0";
    uint8_t checksum_pkg = 0;
    uint8_t _checksum = 0;

    if (prtcl2->dl.indx_msg >= PRTCL2_CS_LENGTH) // обязательных три байта минимум - символ разделения '*' и символы байта cs
    {
        prtcl2->dl.indx_msg--;                              // смещаемся символ контрольной суммы
        cs_symbol[1] = prtcl2->dl.msg[prtcl2->dl.indx_msg]; // читаем hex-символ младших бит байта контрольной суммы
        prtcl2->dl.msg[prtcl2->dl.indx_msg] = '\0';         // очищаем байт
        prtcl2->dl.indx_msg--;                              // смещаемся на следующий символ
        cs_symbol[0] = prtcl2->dl.msg[prtcl2->dl.indx_msg]; // читаем hex-символ старших бит байта контрольной суммы
        prtcl2->dl.msg[prtcl2->dl.indx_msg] = '\0';         // очищаем байт
        prtcl2->dl.indx_msg--;                              // смещаемся на байт символа разделения чтобы получить длину блока данных
        prtcl2->dl.msg[prtcl2->dl.indx_msg] = '\0';         // очищаем байт. Теперь в массиве блока данных нет лишних символов

        /* Парсинг контрольной суммы */
        checksum_pkg = (uint8_t)strtol(cs_symbol, NULL, 16);

        /* Сверка контрольной суммы */
        _checksum = CheckSumXOR(prtcl2->dl.pt, PRTCL2_PT_SYMBOL, prtcl2->dl.msg, prtcl2->dl.indx_msg);
        if (_checksum == checksum_pkg) // checksum OK
        {
#ifdef DEBUG_PRINT
            printf("Checksum OK!\r\n");
#endif
            prtcl2->dl.state = PRTCL2_DL_PROCESSING_STATE;
            /* Можем сразу вызвать процедуру проверка обработки пакета */
            DL_ProcessingState(prtcl2);
        }
        else // checksum NOK
        {
#ifdef DEBUG_PRINT
            printf("Checksum NOK!\r\n");
#endif
            /* Ошибка проверки котнрольной суммы -> сброс в idle */
            prtcl2->dl.state = PRTCL2_DL_IDLE_STATE;
        }
    }
    else
    {
#ifdef DEBUG_PRINT
        printf("Checksum ERROR!\r\n");
#endif
        /* Ошибка структуры пакета -> сброс в idle */
        prtcl2->dl.state = PRTCL2_DL_IDLE_STATE;
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
    for (size_t i = 0; i < prtcl2->dl.num_of_pkg; i++)
    {
        if (!strncmp((const char *)prtcl2->dl.pt, prtcl2->dl.pkg[i].type, strlen(prtcl2->dl.pkg[i].type)))
        {
            /* Вызов обработчика пакета */
            prtcl2->dl.pkg[i].CallBack_pkg(prtcl2->dl.msg, prtcl2->dl.indx_msg);
            break;
        }
    }
    prtcl2->dl.state = PRTCL2_DL_IDLE_STATE;
}

/**
 * @brief Процедура вычсиления контрольной суммы пакета.
 * Алгоритм вычисления контрольной суммы - это XOR всех байтов и символов между началом пакета '$' и концом блока данных '*':
 * $ pt---pt_symbol---data *
 * @param pt строка типа пакета
 * @param pt_symbol символ разделения типа пакета и блока данных ('$')
 * @param data массив блока данных (строка или байтовый массив)
 * @param data_length длина фрагмента
 * @return байт контрольной суммы
 */
static uint8_t CheckSumXOR(uint8_t *pt, uint8_t pt_symbol, uint8_t *data, size_t data_length)
{
    uint8_t checksum = 0;

    /* Контрольная сумма от заголовка пакета */
    size_t pt_length = strlen((char *)pt);
    for (int i = 0; i < pt_length; i++)
    {
        checksum = checksum ^ pt[i];
    }
    /* + символ разделения */
    checksum = checksum ^ pt_symbol;
    /* + сумма байтов блока данных */
    for (int i = 0; i < data_length; i++)
    {
        checksum = checksum ^ data[i];
    }

    return checksum;
}