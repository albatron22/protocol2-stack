#ifndef _PROTOCOL2_H_
#define _PROTOCOL2_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

    extern const char PRTCL2_START_SYMBOL; // символ начала пакета
    extern const char PRTCL2_PT_SYMBOL;    // символ окончания поля типа пакета
    extern const char PRTCL2_END_PKG_r;    // первый символ окончания пакета
    extern const char PRTCL2_END_PKG_n;    // второй символ окончания пакета

    typedef struct Prtcl2_Package_Struct
    {
        char *type;                         // Тип пакета (дескриптор)
        void (*CallBack_pkg)(uint8_t *msg); // call-back функция обработки сообщения по типу пакета

    } Prtcl2_Package_t;

    /**
     * @brief Главная структура данных стека протокола
     */
    typedef struct Prtcl2_Handle_Struct
    {
        /**
         * DL-канал
         */
        struct
        {
            enum
            {
                PRTCL2_DL_IDLE_STATE = 0,      // ожидание прихода стартового символа
                PRTCL2_DL_RECEPTION_PT_STATE,  // считывание заголовка - типа пакета
                PRTCL2_DL_RECEPTION_MSG_STATE, // считывание сообщения пакета
                PRTCL2_DL_PROCESSING_STATE     // обработка пакета
            } state;                           // состояние КА DL-канала

            bool enable; // активация/деактивация работы DL-стека

            uint8_t pt[32];   // поле типа пакета <TYPE>
            size_t indxpt;    // индекс текущей записи в поле заголовка
            uint8_t msg[512]; // поле данных
            size_t indxmsg;   // индекс текущей записи в поле данных

            Prtcl2_Package_t *pkg; // ссылка на массив структур зарегистрированных пакетов
            size_t NumOfPKG;          // количество зарегистрированных пакетов сообщений

            uint32_t ts;      // метка времени
            uint32_t timeout; // таймаут ожидания пакета
        } dl;
        /**
         * UL-канал
         */
        struct
        {
            enum // состояние UL-канала (передача телеметрии)
            {
                PRTCL2_UL_IDLE_STATE = 0,
                PRTCL2_UL_CREATE_PKG,
                PRTCL2_UL_SEND_PKG,
                PRTCL2_UL_WAIT_STATE
            } state;

            bool enable;     // активация/деактивация передачи телметрии
            uint32_t period; // период отправки телеметрии, мс
            uint32_t ts;

            char *pt;         // заголовок пакета телеметрии согласно спецификации протокола
            uint8_t pkg[512]; // пакет сообщения телеметрии

            bool end_of_send; // флаг завершения отправки (поднимается в HAL_UART_TxCpltCallback)
        } ul;
        /* Внешние функции работы с потоком данных (для портирования протокола) */
        size_t (*portAvailable)(); // возвращает количество доступных байт в приёмном буфере интерфейса
        uint8_t (*portRead)();     // считывает новый байт из буфера
        void (*portClean)();       // очистка содержимого приёмного буфера
        /**
         * @brief формирование пакета
         * @param pt строка заголовка пакета
         * @param pkg пакет телеметриии, который нужно сформировать
         */
        void (*portCreatePKG)(char *pt, uint8_t *pkg);
        int (*portSendPKG)(uint8_t *pkg); // отправка пакета по интерфейсу

    } Prtcl2_Handle_t;

    void Prtcl2_Init(Prtcl2_Handle_t *prtcl2);
    void Prtcl2_Loop(Prtcl2_Handle_t *prtcl2);

#ifdef __cplusplus
}
#endif

#endif