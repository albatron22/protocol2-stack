#ifndef _PROTOCOL2_H_
#define _PROTOCOL2_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define PRTCL2_PT_MAX_LENGTH 32
#define PRTCL2_DATA_MAX_LENGTH 512
/**
 * Минимальный пакет содержит в себе только служебные поля (без заголовка и блока данных)
 * 1 байт символа начала пакета + 1 байт окончания заголовка +
 * 1 байт окончания блока данннх + 1 байт контрольной суммы +
 * 2 байта окончания пакета CR LF.
 * Итого 6 служебных байт
 */
#define PRTCL2_PKG_MIN_LENGTH 6

    typedef struct Protocol2_Package_Struct
    {
        char *type;                                              // Тип пакета (дескриптор)
        void (*CallBack_pkg)(uint8_t *data, size_t data_legnth); // call-back функция обработки сообщения по типу пакета

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
                PRTCL2_DL_IDLE_STATE = 0,          // ожидание прихода стартового символа
                PRTCL2_DL_RECEPTION_PT_STATE,      // считывание заголовка - типа пакета
                PRTCL2_DL_RECEPTION_PAYLOAD_STATE, // считывание блока полезных пакета
                PRTCL2_DL_CHECKSUM_STATE,          // Вычисление контрольной суммы пакета и сверка его с принятым
                PRTCL2_DL_PROCESSING_STATE         // обработка пакета
            } state;                               // состояние КА DL-канала

            bool enable; // активация/деактивация работы DL-стека

            uint8_t pt[PRTCL2_PT_MAX_LENGTH];    // поле типа пакета <TYPE>
            size_t indx_pt;                      // индекс текущей записи в поле заголовка
            uint8_t msg[PRTCL2_DATA_MAX_LENGTH]; // поле данных
            size_t indx_msg;                     // индекс текущей записи в поле данных

            Protocol2_Package_t *pkg; // ссылка на массив структур зарегистрированных пакетов
            size_t num_of_pkg;        // количество зарегистрированных пакетов сообщений

            uint32_t ts;      // метка времени
            uint32_t timeout; // таймаут ожидания пакета
        } dl;
        //------------------------------------------------------------------------------------------------------------------------------
        //----------------------------------------------------------------------------------------------------------------------------
        /* Виртуальный порт (ссылки на внешние функции работы с потоком данных по uart) */
        size_t (*VPortAvailable)(); // возвращает количество доступных байт для чтения
        uint8_t (*VPortRead)();     // чтение одного байта
        void (*VPortClean)();       // очистка содержимого приёмного буфера порта

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
    size_t Protocol2_CreatePKG(Protocol2_Handle_t *prtcl2, uint8_t *pkg, uint8_t *pt, uint8_t *data, size_t data_length);

#ifdef __cplusplus
}
#endif

#endif