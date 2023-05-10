// milstd_lib.h : Библиотека по работе с интерфейсом Манчестер 1533b MilStd

#ifndef _MILSTD_LIB_H_
#define _MILSTD_LIB_H_
#include "WDMTMKv2.h"
/*#ifdef _WIN32
#include <windows.h>
//#include"WDMTMKv2.h"
#define TMK_DATA_RET TMK_DATA

/*
#elif  defined(__QNXNTO__)
#define INFINITE            -1     //0xFFFFFFFF  // Infinite timeout
#include"driver/qnx/qnx6tmk.h"
#else
#define INFINITE            0xFFFFFFFF  // Infinite timeout
#include"driver/linux/ltmk.h"*/
/*#endif

#include <stdint.h>
#include <stdbool.h>

// содержимое одного кадра Milstd
typedef struct _milstd_buf_
{
	uint16_t	ks;// слово состояния (командное слово)
	int64_t		time;//	gmt время получения кадра
	uint16_t	milstd_data[32];// кадр манчестара
	uint8_t		milstd_data_len;// длина кадра (в словах)
}milstd_buf_t;*/

// Иннициализация работы с устройствами MilStd
// вызывается один раз
//int mil_init(void);

// Остановка передачи данных
//void mil_stop(int channel);

// Инициализация устройства MilStd
// channel - номер канала (0-9)
// mode - режим (КК,МТ,ОУ)-> (BC_MODE,MT_MODE,RT_MODE)
// rtAddress - адрес ОУ, нужен если mode=RT_MODE
// use_buffering - использовать буферизацию
// return: 1(ok) <0(err)
//int mil_open(int channel,int mode,unsigned short rtAddress, bool use_buffering);

// Закрытие всех устройств MilStd
// return: 1(ok) <0(err)
//int mil_close(void);

// Закрытие одного устройства MilStd (только для win)
// channel - номер канала (0-9)
// return: 1(ok) <0(err)
//int mil_close_i(int channel);

// Определить режим работы канала - BC_MODE, RT_MODE, MT_MODE
//unsigned short mil_get_mode(int channel);

// return 1(сработало прерывание) 0(таймаут) -1(прерывание)
//int CheckTmkEvent(int channel,int timeout);

// Послать массив данных, один (отправитель или получатель) должен быть КК, другой - ОУ
// return: число реально отправленных байт
//int mil_send_data_chain_nopause(int channel, int bus,int adress_ou, unsigned short *data, int *data_len);
//int mil_send_data_chain(int channel, int bus,int adress_ou, unsigned short *data, int *data_len);
//int mil_send_data(int channel, int bus,int adress_ou, unsigned short *data, int *data_len);

// Принять массив данных с таймаутом, не более одной посылки, получатель должен быть ОУ
// channel - номер канала с которого слать (0-MAX_TMK_NUMBER)
// bus - по основному или резервному каналу получены данные (BUS_A=0 или BUS_B=1)
// adress_ou - адрес оконечного устройства
// [out] buf - массив структур для приёма milstd данных
// [in/out] buf_count - количество входных/заполненых буферов данных
// timeout_ms - время ожидания данных в мс, -1 вечное ожидание
// return: 1 ОК, 0 если ошибка или прерывание, (-1 или 0xffff или 65535) timeout
//int  mil_recv_data_timed(int channel, int bus,int adress_ou, milstd_buf_t *buf, int *buf_count, int timeout_ms);
// Принять массив данных, не более одной посылки, получатель должен быть ОУ
// channel - номер канала с которого слать (0-MAX_TMK_NUMBER)
// bus - по основному или резервному каналу получены данные (BUS_A=0 или BUS_B=1)
// adress_ou - адрес оконечного устройства
// [out] buf - массив структур для приёма milstd данных
// [in/out] buf_count - количество входных/заполненых буферов данных
// timeout_ms - время ожидания данных в мс, -1 вечное ожидание
// return: 1 ОК, 0 если ошибка или прерывание
//int  mil_recv_data(int channel, int bus,int adress_ou, milstd_buf_t *buf, int *buf_count);

// удаляем все данные из буфера
// channel - номер канала(0-MAX_TMK_NUMBER)
//void mil_clear_buf(int channel);

#endif // _MILSTD_LIB_H_
