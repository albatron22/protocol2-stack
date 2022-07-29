#ifndef _PROTOCOL2_H_
#define _PROTOCOL2_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

    typedef struct Protocol2_Package_Struct
    {
        char *type;                         // Тип пакета (дескриптор)
        void (*CallBack_pkg)(uint8_t *msg); // call-back функция обработки сообщения по типу пакета

    } Protocol2_Package_t;

    /**
     * @brief Главная структура данных стека протокола
     */
    typedef struct Protocol2_Handle_Struct
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

            Protocol2_Package_t *pkg; // ссылка на массив структур зарегистрированных пакетов
            size_t numOfPKG;          // количество зарегистрированных пакетов сообщений

            uint32_t ts;      // метка времени
            uint32_t timeout; // таймаут ожидания пакета
        } dl;
        //------------------------------------------------------------------------------------------------------------------------------
        //----------------------------------------------------------------------------------------------------------------------------
        /* Виртуальный порт (ссылки на внешние функции работы с потоком данных по uart) */
        size_t (*VPortAvailable)();         // возвращает количество доступных байт для чтения
        uint8_t (*VPortRead)();             // чтение одного байта
        void (*VPortClean)();               // очистка содержимого приёмного буфера порта

        /**
         * @brief Отправка массива байт данных в порт
         * @param pkg массив данных для отправки (массив должен быть глобальным)
         * @return Статус отправки: 0 - OK, 1 - NOK (busy or error)
         */
        int (*VPortSendData)(uint8_t *pkg); // 

        uint32_t (*VGetTick_ms)(); // системное время, мс
    } Protocol2_Handle_t;

    void Protocol2_Init(Protocol2_Handle_t *prtcl2);
    void Protocol2_Loop(Protocol2_Handle_t *prtcl2);
    void Protocol2_SendPKG(Protocol2_Handle_t *prtcl2, char *pt, char *data);

#ifdef __cplusplus
}
#endif

#endif