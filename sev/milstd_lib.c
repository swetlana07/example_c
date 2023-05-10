// milstd_lib.c : Библиотека по работе с интерфейсом Манчестер 1533b MilStd
//
/*#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifndef _WIN32*/
#include <unistd.h>	// for usleep()
/*#endif
#include <pthread.h>
#include <errno.h>
#include "my_time.h"
#include "milstd_lib.h"*/




// for Windows
/*#ifdef _WIN32
#define EOK              0  // No error
HANDLE hEvent[MAX_TMK_NUMBER+1];
HANDLE hEvent_break[MAX_TMK_NUMBER+1];
#define MILSTD_USE_MUTEX


#endif*/


// использование нескольких MilStd каналов в одном процессе
//#define MILSTD_USE_MUTEX

// для остановки регистрации
//static pthread_mutex_t mutex_stop = PTHREAD_MUTEX_INITIALIZER;
static volatile int mil_stop_var = 0;

//static GMutex *mutex_tmkselect = NULL;// для разделения по времени выбора плат

//#ifdef MILSTD_USE_MUTEX

//static pthread_mutex_t mutex_tmkselect = PTHREAD_MUTEX_INITIALIZER;


//#endif

/*static int milstd_mutex_lock(int channel)
{
#ifdef MILSTD_USE_MUTEX
	int cur_channel;
        int ret = pthread_mutex_trylock(&mutex_tmkselect);
	if(ret!=EOK )
	{
		//printf("pthread_mutex_trylock=%d\n",ret);
		ret = pthread_mutex_lock(&mutex_tmkselect);
	}
	//ret = pthread_mutex_lock(&mutex_tmkselect);
	//ret = usleep((useconds_t)1000000*50);
	if(ret!=EOK)
                printf("pthread_mutex_lock=%d",ret);/
	//if(rtgetpage()!=0)
	//	rtdefpage(0);// работаем только с нулевой страницей
	// меняем канал, если надо
	cur_channel = tmkselected();
	if (cur_channel != channel)
	{
		//printf("milstd_mutex_lock cur_channel=%d new_channel=%d\n",cur_channel, channel);
		tmkselect(channel);// выбор текущего канала
	}

	return ret;
	//g_mutex_lock(mil_get_mutex());
#else
	// меняем канал, если надо
	int cur_channel = tmkselected();
	if (cur_channel != channel)
	{
		//printf("milstd_mutex_lock cur_channel=%d new_channel=%d\n",cur_channel, channel);
		tmkselect(channel);// выбор текущего канала
	}
	return EOK;
#endif
}

static int milstd_mutex_unlock(void)
{
#ifdef MILSTD_USE_MUTEX
	return pthread_mutex_unlock(&mutex_tmkselect);
	//g_mutex_unlock(mil_get_mutex());
#else
	return EOK;
#endif
}
*/

/*/ для получения событий
#define MAX_CHANNELS	8
static pthread_mutex_t mutex_event[MAX_CHANNELS];// = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cond_wait_event[MAX_CHANNELS];// = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_wait_stop;// = PTHREAD_COND_INITIALIZER;
// потоковая функция чтения данных по Milstd
static void*  mil_thread_event(void*  args)
{
	printf("[mil_thread_event] start thread\n");
	while(1)
	{
		int channel;
		int events = tmkwaitevents(0xffff, 5000);
		// timeout
		if (!events)
			continue;
		// прерывание ожидания
		if (events<0)
			break;
		// сообщаем всем каналам о прерывании в которых оно произошло
		for(channel=0;channel<MAX_CHANNELS;channel++)
		{
			// есть прерывание для тек. канала
			if (events & (1<<channel))
			{
				// событие свершилось
				pthread_mutex_lock(&mutex_event[channel]);
				pthread_cond_broadcast(&cond_wait_event[channel]);
				pthread_mutex_unlock(&mutex_event[channel]);
			}
		}
	};
	printf("[mil_thread_event] exit thread\n");
	return NULL;
}*/

// Иннициализация работы с устройствами MilStd
// вызывается один раз
/*int mil_init(void)
{

	// иннициализация MilStd
	if (TmkOpen())
		return -1;
	// версия оборудования
	{
#ifndef _WIN32
		TMK_DATA_RET ver = tmkgethwver();
#else
		TMK_DATA ver = tmkgethwver();
#endif
		printf("TmkOpen Ok ver=%d\n",ver);
	}
	// сбрасываем признак прерывания ожидания
        //pthread_mutex_lock(&mutex_stop);
	mil_stop_var = 0;
        //pthread_mutex_unlock(&mutex_stop);
	/*
	pthread_condattr_t attr;
	pthread_condattr_init( &attr);
	pthread_condattr_setclock( &attr, CLOCK_MONOTONIC);
	int i;
	for(i=0;i<MAX_CHANNELS;i++)
	{
		pthread_mutex_init(&mutex_event[i], NULL);
		pthread_cond_init(&cond_wait_event[i], &attr);
	}
	pthread_mutex_init(&mutex_stop, NULL);
	pthread_cond_init(&cond_wait_stop, &attr);
	 */


	// запускаем поток ожидания событий
	//pthread_create(NULL, NULL, mil_thread_event, NULL);
	//hEvent_break = CreateEvent(NULL, TRUE, FALSE, NULL);
	//if (!(hEvent_break = CreateEvent(NULL, TRUE, FALSE, NULL)))
	//	return -2;// не было сопоставлено событие
/*	return 1;
}

// Остановка передачи данных
// номер канала действителен только для Windows
/*void mil_stop(int channel)
{
#ifdef _WIN32		// WINDOWS
	SetEvent(hEvent_break[channel]);
#elif defined(__QNXNTO__)	// QNX
	// выставляем признак прерывания ожидания
        //pthread_mutex_lock(&mutex_stop);
	mil_stop_var = 1;
        //pthread_mutex_unlock(&mutex_stop);
	// прерываем ожидание прерывания
	int ret = tmkwaiteventsflag(0,0);
	ret= 0;

#else				// Linux
/*
В драйвере есть дополнительная функция tmkwaiteventsflag,
которая в интерфейсе ltmk.c/ltmk.h не определена, но неявно
используется в драйвере USB устройств как раз для останова
ожидания прерывания не-USB устройств по прерыванию от
USB устройств. Пример можно посмотреть в ltmk.c/ltmk.h в
папке include/usb/ драйвера. Если кратко, то надо определить
#define VTMK_tmkwaiteventsflag 115
#define TMK_IOCtmkwaiteventsflag _IOW(TMK_IOC_MAGIC, VTMK_tmkwaiteventsflag+TMK_IOC0, __u64)
а затем использовать как-то так:
int _VTMK4Arg[2];
_VTMK4Arg[0] = mainpid;
_VTMK4Arg[1] = 1;
ioctl(_hVTMK4VxD, TMK_IOCtmkwaiteventsflag, &_VTMK4Arg);
Здесь mainpid - pid того процесса, который вызвал tmkwaitevents,
а _hVTMK4VxD - хэндл открытого драйвера (определен в ltmk.c).
*/
/*	#define VTMK_tmkwaiteventsflag 115
	#define TMK_IOCtmkwaiteventsflag _IOW(TMK_IOC_MAGIC, VTMK_tmkwaiteventsflag+TMK_IOC0, __u64)
	{
		int _VTMK4Arg[2];
		_VTMK4Arg[0] = getpid();
		_VTMK4Arg[1] = 1;
		ioctl(_hVTMK4VxD, TMK_IOCtmkwaiteventsflag, &_VTMK4Arg);
	}
#endif
}
/*
// Определить режим работы канала
// BC_MODE, RT_MODE, MT_MODE
unsigned short mil_get_mode(int channel)
{
	unsigned short mode;
	milstd_mutex_lock(channel);
	mode = tmkgetmode();
	milstd_mutex_unlock();
	return mode;
}

// Инициализация устройства MilStd
// channel - номер канала (0-9)
// mode - режим (КК,МТ,ОУ)-> (BC_MODE,MT_MODE,RT_MODE)
// rtAddress - адрес ОУ, нужен если mode=RT_MODE
// use_buffering - использовать буферизацию
// return: 1(ok) <0(err)
int mil_open(int channel,int mode,unsigned short rtAddress, bool use_buffering)
{
	int ret=0,ret_irq=0,ret_mode=0;
	int maxp = tmkgetmaxn();// максимальный номер канала (0-MAX_TMK_NUMBER[9])
	if(channel<0 || channel>maxp || rtAddress<0)
		return -1;// канал отсутствует в системе
	// закрепление канала на плате за текущим процессом
	if(tmkconfig(channel))
		return -2;// канал уже занят другим процессом
#ifdef _WIN32
	// создание события и привязка его к выбранному каналу
	if (!(hEvent_break[channel] = CreateEvent(NULL, TRUE, FALSE, NULL)) || // ручной сброс события + сброшено событие
		!(hEvent[channel] = CreateEvent(NULL, TRUE, FALSE, NULL)) // ручной сброс события + сброшено событие
		)
		return -4;// не было сопоставлено событие
	else
	{
		ResetEvent(hEvent[channel]);
		milstd_mutex_lock(channel);
		tmkdefevent(hEvent[channel],TRUE);// привязка события для текущей выбранной платы (т.е. канала)
		milstd_mutex_unlock();
	}
#endif

	// сбрасываем признак прерывания ожидания
        //pthread_mutex_lock(&mutex_stop);
	mil_stop_var = 0;
        //pthread_mutex_unlock(&mutex_stop);

	milstd_mutex_lock(channel);
	switch(mode)
	{
	case BC_MODE:
		ret = bcreset();// инициализация выбранной платы(канала) с переводом ее в режим Контроллер канала.
		ret_irq = bcdefirqmode(BC_GENER1_BL|BC_GENER2_BL);
		break;
	case MT_MODE:
		ret = mtreset();// инициализация выбранной платы(канала) с переводом ее в режим Монитор.
		ret_irq = mtdefirqmode(MT_GENER2_BL|MT_GENER2_BL);
		break;
	case RT_MODE:
		ret = rtreset();// инициализация выбранной платы(канала) с переводом ее в режим Оконечное устройство.
		// включаем режим работы ОУ восприятия командных слов с групповым адресом
		ret_mode = rtdefmode(RT_BRCST_MODE);//ret = rtdefmode(RT_FLAG_MODE|RT_BRCST_MODE);
		ret_irq = rtdefirqmode(RT_GENER1_BL|RT_GENER2_BL);// прерывания по приёму данных получаем
		// работаем только с нулевой страницей
		rtdefpage(0);
		// выставляем адрес ОУ - только не для группового режима
		if(rtAddress!=31)
			if(rtdefaddress(rtAddress))
				ret = -5;// не установлен адрес ОУ
		// настраиваем буферизацию приёма
//#ifdef __QNXNTO__ // QNX 6
		//milstd_mutex_lock();
                /*if(use_buffering)
		{
			int i, subaddr;// subaddr с 1 по 30, 0 и 31 зарезервированы для команд управления
			for(subaddr=1;subaddr<31;subaddr++)
			{
				// максимальное число буферов для подадреса
				TMK_DATA_RET max_bufs;
				// переключаемся на страницу subaddr
				rtdefsubaddr(RT_RECEIVE, subaddr);
				//Узнать максимальное число буферов для каждого подадреса
				max_bufs = rtallocsabuf(SABUF_STD_R0+subaddr, GET_MAX_ALLOC);// = 16
				// Включить буферизацию на каждый отдельный подадрес приёма
				max_bufs = rtallocsabuf(SABUF_STD_R0+subaddr, max_bufs);// return - тек. число буферов
				for(i=0;i<max_bufs-1;i++)
				{
					// делаем активным текущий буфер
					rtdefbuf(i);
					// делаем ссылку на следующий буфер
					rtdeflink((i+1) | RT_LINK_IP);
				}
				// делаем активным последний буфер
				rtdefbuf(max_bufs-1);
				// зацикливаем последний буфер на первый
				rtdeflink(0 | RT_LINK_IP);
			}
                }*/
		//milstd_mutex_unlock();
//#endif

/*		break;
	default:
		ret = TMK_BAD_FUNC;
		break;
	}
	milstd_mutex_unlock();

	if(ret)
		return -6;// режим не поддерживается
//	if(mode!=tmkgetmode())//определить режим выбранной платы
//		return -7;// режим не выставился
	if(ret_irq || ret_mode)
		return -8;// работа с прерываниями не поддерживается
	return 1;
}

/*#ifdef _WIN32
//from wdmtmk.c
void TmkClose_i(int iTMK);
/ *void TmkClose_i(int iTMK)
{
	if (_hVTMK4VxD[iTMK])
		CloseHandle(_hVTMK4VxD[iTMK]);
	_ahVTMK4Event[iTMK] = 0;
	_hVTMK4VxD[iTMK] = 0;
}* /
#endif*/


// Закрытие всех устройств MilStd
/*int mil_close(void)
{
/*
#ifdef _WIN32
	int channel;
	for (channel = 0; channel <= MAX_TMK_NUMBER; ++channel)
	{
		tmkdone(channel);//Процесс освобождает платы, с которыми закончил работу
		if(hEvent[channel])
			CloseHandle(hEvent[channel]);
		hEvent[channel] = NULL;
		//TmkClose_i(channel);

	}
#else
	//TmkClose();
#endif*/
/*	if(tmkdone(ALL_TMKS))//Процесс освобождает все платы, которые захватывал через tmkconfig()
                return -1;
	// завершить работы с устройствами
	TmkClose();
	return 1;
}

// Закрытие одного устройства MilStd
// channel - номер канала (0-MAX_TMK_NUMBER)
// return: 1(ok) <0(err)
int mil_close_i(int channel)
{
	int ret;
	milstd_mutex_lock(channel);
	ret = tmkdone(channel);
	if(ret)//Процесс освобождает платы, с которыми закончил работу
	{
		milstd_mutex_unlock();
		return -1;
	}
#ifdef _WIN32
	if(hEvent[channel])
		CloseHandle(hEvent[channel]);
	hEvent[channel] = NULL;
	//TmkClose_i(channel);
//#else
	//TmkClose();
#endif
	milstd_mutex_unlock();
	return 1;
}

#ifdef _WIN32
// timeout - время ожидания в миллисекундах, (пакет из кс+32сд уходит за (1+32)*20usec = 660 микросекунд)
// return 1(сработало прерывание), 0(таймаут), -1(прерывание ожидания)
int CheckTmkEvent(int channel,int timeout)
{
	int ret = 0;
	DWORD value;
	HANDLE	HandlesToWaitFor[2];
	HandlesToWaitFor[0] = hEvent_break[channel];
	HandlesToWaitFor[1] = hEvent[channel];

	value = WaitForMultipleObjects(2, HandlesToWaitFor,FALSE,(DWORD)timeout);
	// чтобы не пропустить вариант, когда оба события сработали
	//while(value!=WAIT_TIMEOUT)
	{
		switch(value)
		{
			case WAIT_TIMEOUT:
				//printf("event 0x%x ch=%d timeout\n",HandlesToWaitFor[0],channel);
				ret = 0;
				//ret = 1;
				break;
			// прерывание ожидания
			case WAIT_OBJECT_0:
				ResetEvent(hEvent_break[channel]);
				ret = -1;
				//value = WaitForMultipleObjects(2, HandlesToWaitFor,FALSE,100);
				break;
			// дождались события
			case WAIT_OBJECT_0+1:
				ResetEvent(hEvent[channel]);
				//if(ret==-1)// уже было прерывание
				//	ret = 2;// прерывание и последний блок данных
				//else
					ret = 1;
				break;
		};
		//value = WaitForMultipleObjects(2, HandlesToWaitFor,FALSE,0);
		//value = WAIT_TIMEOUT;
	};
	return ret;
}
#else
// timeout - время ожидания в миллисекундах, (пакет из кс+32сд уходит за (1+32)*20usec = 660 микросекунд)
// return 1(сработало прерывание) 0(таймаут) -1(прерывание)
int CheckTmkEvent(int channel,int timeout)
{
	if(channel<0 || channel>31)
		return -1;
	/*
    struct timespec to;
    clock_gettime(CLOCK_MONOTONIC, &to);
	to.tv_sec += 0;
	to.tv_nsec += timeout*1000000;

	int events = tmkwaitevents(0xff, 5000000);

	pthread_mutex_lock(&mutex_event[channel]);
    int retval = pthread_cond_timedwait(&cond_wait_event[channel], &mutex_event[channel], &to);
    pthread_mutex_unlock(&mutex_event[channel]);
    printf("pthread_cond_timedwait ch=%d ret=%d\n",channel,retval);
    */
	//while(1)
/*	{
		//int events = tmkwaitevents(0xff, timeout);
		int events = tmkwaitevents(1<<channel, timeout);
		// проверка на прерывание ожидания
		{
			int stop_var;
			// выставляем признак прерывания ожидания
                        //pthread_mutex_lock(&mutex_stop);
			stop_var = mil_stop_var;
			//if(mil_stop_var) mil_stop_var = 0;
                        //pthread_mutex_unlock(&mutex_stop);
			if(stop_var)
			{
				events = 0;
				return -1;// прерывание ожидания
			}
		}
		// timeout
		if (!events)
			return 0;// нет прерываний, timeout сработал
		// прерывание ожидания
		if (events<0)
			return -1;
		//if (events !=(1<<channel) )
		//	log_write_printf("event=%d channel=%d!!!\n\n",events,channel);
		//else
		//if (channel == 0)
		//	log_write_printf("event=%d channel=%d\n",events,channel);
		// событие свершилось
		if (events & (1<<channel))
			return 1;// есть прерывание для нашего канала
		else
			printf("event=%d (dolsno=0 or %d) tak kak channel=%d !!!\n",events,(1<<channel), channel);
		//log_write_printf("exit event=%d channel=%d!!!\n",events,channel);
		// получено прерывание для другого канала, пускай будет как таймаут
		//usleep(10);
		//{
		//	TTmkEventData	event_data;
		//	// запросить структуру данных, описывающих прерывание
		//	tmkgetevd(&event_data);
		//}
		events = 0;
		return 0;
	}
	return 0;
}
#endif


static double get_cur_time(void)
{
	double usec = 0;
#ifdef _WIN32
	LARGE_INTEGER lpFrequency;
	LARGE_INTEGER lpPerformanceCount1;
	QueryPerformanceCounter(&lpPerformanceCount1);
	QueryPerformanceFrequency(&lpFrequency);
	usec = lpPerformanceCount1.QuadPart*1. / lpFrequency.QuadPart;
#endif
	//printf("usec2=%lf \n",fmod(usec2,10000000000));
	return usec;
}

// Послать массив данных в цепочке, один (отправитель или получатель) должен быть КК, другой - ОУ
// channel - номер канала с которого слать (0-MAX_TMK_NUMBER)
// bus - основной или резервный канал (CX_BUS_A или CX_BUS_B)
// adress_ou - адрес оконечного устройства (получатель или отправитель, неважно)
// data - данные
// [in]  data_len - длина данных для отправки, в словах не более 100*32
// [out] data_len - число реально отправленных слов
// return: 1(успешно), 0(таймаут или ошибка передачи), -1(прерывание отправки)
int mil_send_data_chain_nopause(int channel, int bus,int adress_ou, unsigned short *data, int *data_len)
{
	//static int dbg_count = 0;
	static int num_block = 0;// номер пакета
	//int n_base = 5;// номер используемой базы
	int ret;
	int len_prepared=0;// сколько подготовили для передачи слов
	int len_transmited=0;// сколько уже передали слов
	int len_total=*data_len;// сколько ещё осталось слов передать
	unsigned short ks,bc_sw=0;// командное слово, расширенное слово результата обмена
	unsigned char subaddr=0x10;// подадрес (10000b)
	TTmkEventData	event_data;
	unsigned short mode = mil_get_mode(channel);
	// пока ничего не отправлено
	*data_len = 0;
	if(tmkselect(channel))// выбор текущего канала
		return -1;
	if(mode!=BC_MODE)// контроллер канала
		return 0;// плата не в нужном режиме
	milstd_mutex_lock(channel);
	//unsigned short buffer[64];
	ret = bcdefbus(bus);// передача по основному каналу (не по резервному BUS_B)
	milstd_mutex_unlock();
	//static int num_block = 0;// номер пакета
	//!!!num_block++;

	//int len=len_total-len_transmited;
	do
	{
		bool is_break = false;
		TMK_DATA start_flags;
		unsigned short os=0;// ответное слово
		int n_base = 100;// номер используемой базы
		int len=0;
		printf("usec0=%lf \n",get_cur_time());
		// формируем цепочку сообщений
		while((len_total-len_prepared)>0 && n_base<255)
		{
			len=len_total-len_prepared;
			if(len>32)	len=32;
			if(len<0)
				break;
			if(!len)
				break;
			//subaddr = num_block%30+1;//всего м.б. 30 подадресов 1-30 (0 и 31 для команд управления)
			subaddr = num_block%15+1;//всего м.б. 30 подадресов 1-30 (0 и 31 для команд управления)
			num_block++;
			//subaddr = num_block%5+1;//всего 5 подадресов 1-4
			//printf("subaddr=%d\n", subaddr);
			ks = (adress_ou<<11) | RT_RECEIVE | (subaddr<<5);// ОУ будет принимать данные по адресу adress_ou

			milstd_mutex_lock(channel);
			// Задание базы (0-255)
			ret = bcdefbase(n_base);

			// запись командного слова
			bcputw(0,ks | len%32 );
			// заполняем слова данных
			bcputblk(1,data+len_prepared,len);// bcAddr=1 - начальный адрес внутри базы (0 - 63), данные, длина данных
			// переход на следующую базу
			n_base++;
			// увеличиваем число переданных данных
			len_prepared +=len;
			// будем формировать цепочку, а не отдельное сообщение
			// находимся в предпоследней базе
			// делаем ссылку из текущей базы на следующую базу - остановка
			if((len_total-len_prepared)>0 &&(len_total-len_prepared)<=32)
				bcdeflink(n_base, DATA_BC_RT | bus | CX_STOP );
			// делаем ссылку из текущей базы на следующую базу
			else if((len_total-len_prepared)>32)
				bcdeflink(n_base, DATA_BC_RT | bus | CX_CONT );

			// выбираем стартовую базу - чтобы в других потоках не было изменений
			//ret = bcdefbase(100);
			milstd_mutex_unlock();
		}
		
		//ret = channel;//tmkselected();

		start_flags = CX_CONT;
		// нет цепочки, только одно сообщение
		if(len_total<=32)
			start_flags = CX_STOP;
		
		//dbg_count += len_total;
		//printf("!send start byte=%d total=%d\n",len_total*2,dbg_count*2);

		milstd_mutex_lock(channel);
		// выбираем стартовую базу
		n_base = 100;
		// Задание базы (0-255)
		ret = bcdefbase(n_base);
		// запуск цепочки
		if(adress_ou==31)// широковещательный пакет
			ret = bcstartx(n_base, DATA_BC_RT_BRCST | bus | start_flags);//формат 1: передача данных КК-ОУ
		else
			//ret = bcstartx(n_base, DATA_BC_RT | bus | CX_CONT);//формат 1: передача данных КК-ОУ
			ret = bcstartx(n_base, DATA_BC_RT | bus | start_flags);//формат 1: передача данных КК-ОУ
		// выбираем стартовую базу - чтобы в других потоках не было изменений
		//ret = bcdefbase(0);
		milstd_mutex_unlock();

		printf("usec1=%lf \n",get_cur_time());
	

		// ждём прерывание об окончании отправки блока не более 200 милисек
		ret = CheckTmkEvent(channel, 200);
		printf("usec2=%lf \n",get_cur_time());
		// если таймаут
		while(!ret)
			ret = CheckTmkEvent(channel, 200);
		//printf("usec3=%lf \n",get_cur_time());
		//printf("usec4=%lf \n",get_cur_time());
		// если прерывание ожидания
		if(ret==-1)
		{
			// небольшая пауза (0,2 сек), чтобы при прерывании передачи дождаться гарантированной отправки данных
#ifdef _WIN32
			Sleep(200);// в милисек.
#else
			usleep(200000);// в микросек.
#endif
			// прервать ли операцию записи, но не сейчас, сначала проверил, отправились ли данные
			is_break = true;
		}

		//printf("!!!send stop byte=%d total=%d\n",len_total*2,dbg_count*2);

		// Если ret==1 то узнаём что за прерывание
		milstd_mutex_lock(channel);
		{
			memset(&event_data,0,sizeof(event_data));
			tmkgetevd(&event_data);
			if(event_data.nInt==4)
			{
				printf("[mil_send_data_chain] event_data.nInt==%d!!!\n",event_data.nInt);
				milstd_mutex_unlock();
				if(is_break)
					return -1;// прерывание передачи
				continue;
				//*data_len  = len;
				//milstd_mutex_unlock();
				//return 1;
			}
			if(event_data.nInt<=0 || event_data.nInt>4)
			{
				printf("[mil_send_data_chain] event_data.nInt==%d!!!\n",event_data.nInt);
				milstd_mutex_unlock();
				if(is_break)
				// это видимо последее сообщение, которое отправлено после нажатия стоп - считаем, что оно дошло
				{
					len_transmited+=len_prepared;
					*data_len = len_transmited;
					return -1;// прерывание передачи
				}
				break;

			}
			//  читаем ответное слово
			os = bcgetw(len+2);
			bc_sw = event_data.bcx.wResultX;// расширенное слово результата обмена
			// для широковещательного пакета игнорируем ошибку отсутствия ответного слова
			if(adress_ou==31 && (bc_sw & SX_TOA))// широковещательный пакет
				bc_sw&=~SX_TOA;//SX_TOA=2 - неверная пауза перед ответным словом (отсутствует ответное слово);

			if(event_data.nInt==3 && !bc_sw)
			{
				len_transmited+=len_prepared;
				*data_len = len_transmited;
				//num_block++;
			}
			// ошибка при передаче
			else
			{
				printf("[mil_send_data_chain] event_data.nInt=%d bc_sw=%d\n",event_data.nInt,bc_sw);
				ret = 0;
				milstd_mutex_unlock();
				if(is_break)
					return -1;// прерывание передачи
				return 0;
				break;
			}
			//printf("*send nInt=%d len=%d/%d \n",event_data.nInt, len_total, *data_len);
			if(event_data.nInt == 2 || event_data.nInt == 3)
			{
				/*
				// Может прийти сразу несколько прерываний, поэтому вызываем tmkgetevd() до тех пор, пока event_data.nInt!=0
				while(event_data.nInt!= 0)
				{
					tmkgetevd(&event_data);// забываем остальные прерывания, чтобы повторно не считывать тоже самое
					//printf("[mil_send_data_chain] reread event_data.nInt=%d\n",event_data.nInt);
				}
				*/
/*			}
		}
		//while(event_data.nInt!=0);//Может прийти сразу несколько прерываний, поэтому вызываем tmkgetevd() до тех пор, пока event_data.nInt!=0, потом снова ждём прерывание
		milstd_mutex_unlock();

		// прервать ли операцию записи
		if(is_break)
		{
			return -1;// прерывание передачи
		}
		// ошибка при передаче
		if(ret==0)
			return 0;
		//if(ret==2)
		//	return 0;// прерывание передачи
	}
	while((len_total-len_transmited)>0);// пока не передали все данные или не получили ошибку
	// всё ок
	return 1;

}

// Послать массив данных в цепочке, один (отправитель или получатель) должен быть КК, другой - ОУ
// channel - номер канала с которого слать (0-MAX_TMK_NUMBER)
// bus - основной или резервный канал (CX_BUS_A или CX_BUS_B)
// adress_ou - адрес оконечного устройства (получатель или отправитель, неважно)
// data - данные
// [in]  data_len - длина данных для отправки, в словах
// [out] data_len - число реально отправленных слов
// return: 1(успешно), 0(таймаут или ошибка передачи), -1(прерывание отправки)
int mil_send_data_chain(int channel, int bus,int adress_ou, unsigned short *data, int *data_len)
{
	//static int dbg_count = 0;
	static int num_block = 0;// номер пакета
	//int n_base = 5;// номер используемой базы
	int ret;
	int len_prepared=0;// сколько подготовили для передачи слов
	int len_transmited=0;// сколько уже передали слов
	int len_total=*data_len;// сколько ещё осталось слов передать
	unsigned short ks,bc_sw=0;// командное слово, расширенное слово результата обмена
	unsigned char subaddr=0x10;// подадрес (10000b)
	TTmkEventData	event_data;
	unsigned short mode = mil_get_mode(channel);
	// пока ничего не отправлено
	*data_len = 0;
	if(tmkselect(channel))// выбор текущего канала
		return -1;
	if(mode!=BC_MODE)// контроллер канала
		return 0;// плата не в нужном режиме
	milstd_mutex_lock(channel);
	//unsigned short buffer[64];
	ret = bcdefbus(bus);// передача по основному каналу (не по резервному BUS_B)
	milstd_mutex_unlock();
	//static int num_block = 0;// номер пакета
	//!!!num_block++;

	//int len=len_total-len_transmited;
	do
	{
		bool is_break = false;
		TMK_DATA start_flags;
		unsigned short os=0;// ответное слово
		int n_base = 0;// номер используемой базы
		int len=0;
		// формируем цепочку сообщений
		while((len_total-len_prepared)>0 && n_base<255)
		{
			len=len_total-len_prepared;
			if(len>32)	len=32;
			if(len<0)
				break;
			if(!len)
				break;
			//subaddr = num_block%30+1;//всего м.б. 30 подадресов 1-30 (0 и 31 для команд управления)
			subaddr = num_block%15+1;//всего м.б. 30 подадресов 1-30 (0 и 31 для команд управления)
			num_block++;
			//subaddr = num_block%5+1;//всего 5 подадресов 1-4
			//printf("subaddr=%d\n", subaddr);
			ks = (adress_ou<<11) | RT_RECEIVE | (subaddr<<5);// ОУ будет принимать данные по адресу adress_ou

			milstd_mutex_lock(channel);
			// Задание базы (0-255)
			ret = bcdefbase(n_base);

			// запись командного слова
			bcputw(0,ks | len%32 );
			// заполняем слова данных
			bcputblk(1,data+len_prepared,len);// bcAddr=1 - начальный адрес внутри базы (0 - 63), данные, длина данных
			// переход на следующую базу
			n_base++;
			// увеличиваем число переданных данных
			len_prepared +=len;
			// будем формировать цепочку, а не отдельное сообщение
			// находимся в предпоследней базе
			// делаем ссылку из текущей базы на следующую базу - остановка
			if((len_total-len_prepared)>0 &&(len_total-len_prepared)<=32)
				bcdeflink(n_base, DATA_BC_RT | bus | CX_STOP );
			// делаем ссылку из текущей базы на следующую базу
			else if((len_total-len_prepared)>32)
				bcdeflink(n_base, DATA_BC_RT | bus | CX_CONT );

			// выбираем стартовую базу - чтобы в других потоках не было изменений
			ret = bcdefbase(0);
			milstd_mutex_unlock();
		}
		
		//ret = channel;//tmkselected();

		start_flags = CX_CONT;
		// нет цепочки, только одно сообщение
		if(len_total<=32)
			start_flags = CX_STOP;
		
		//dbg_count += len_total;
		//printf("!send start byte=%d total=%d\n",len_total*2,dbg_count*2);

		milstd_mutex_lock(channel);
		// выбираем стартовую базу
		n_base = 0;
		// Задание базы (0-255)
		ret = bcdefbase(n_base);
		// запуск цепочки
		if(adress_ou==31)// широковещательный пакет
			ret = bcstartx(n_base, DATA_BC_RT_BRCST | bus | start_flags);//формат 1: передача данных КК-ОУ
		else
			//ret = bcstartx(n_base, DATA_BC_RT | bus | CX_CONT);//формат 1: передача данных КК-ОУ
			ret = bcstartx(n_base, DATA_BC_RT | bus | start_flags);//формат 1: передача данных КК-ОУ
		milstd_mutex_unlock();

		// ждём прерывание об окончании отправки блока не более 2000 милисек
		ret = CheckTmkEvent(channel, 2000);
		// если таймаут
		while(!ret)
			ret = CheckTmkEvent(channel, 200);
		// если прерывание ожидания
		if(ret==-1)
		{
			// небольшая пауза, чтобы при прерывании передачи дождаться гарантированной отправки данных
#ifdef _WIN32
			Sleep(1);
#else
			usleep(1);
#endif
			// прервать ли операцию записи, но не сейчас, сначала проверил, отправились ли данные
			is_break = true;
		}

		//printf("!!!send stop byte=%d total=%d\n",len_total*2,dbg_count*2);

		// Если ret==1 то узнаём что за прерывание
		milstd_mutex_lock(channel);
		{
			memset(&event_data,0,sizeof(event_data));
			tmkgetevd(&event_data);
			if(event_data.nInt==4)
			{
				printf("[mil_send_data_chain] event_data.nInt==%d!!!\n",event_data.nInt);
				milstd_mutex_unlock();
				if(is_break)
					return -1;// прерывание передачи
				continue;
				//*data_len  = len;
				//milstd_mutex_unlock();
				//return 1;
			}
			if(event_data.nInt<=0 || event_data.nInt>4)
			{
				printf("[mil_send_data_chain] event_data.nInt==%d!!!\n",event_data.nInt);
				milstd_mutex_unlock();
				if(is_break)
				// это видимо последее сообщение, которое отправлено после нажатия стоп - считаем, что оно дошло
				{
					len_transmited+=len_prepared;
					*data_len = len_transmited;
					return -1;// прерывание передачи
				}
				break;

			}
			//  читаем ответное слово
			os = bcgetw(len+2);
			bc_sw = event_data.bcx.wResultX;// расширенное слово результата обмена
			// для широковещательного пакета игнорируем ошибку отсутствия ответного слова
			if(adress_ou==31 && (bc_sw & SX_TOA))// широковещательный пакет
				bc_sw&=~SX_TOA;//SX_TOA=2 - неверная пауза перед ответным словом (отсутствует ответное слово);

			if(event_data.nInt==3 && !bc_sw)
			{
				len_transmited+=len_prepared;
				*data_len = len_transmited;
				//num_block++;
			}
			// ошибка при передаче
			else
			{
				printf("[mil_send_data_chain] event_data.nInt=%d bc_sw=%d\n",event_data.nInt,bc_sw);
				ret = 0;
				milstd_mutex_unlock();
				if(is_break)
					return -1;// прерывание передачи
				return 0;
				break;
			}
			//printf("*send nInt=%d len=%d/%d \n",event_data.nInt, len_total, *data_len);
			if(event_data.nInt == 2 || event_data.nInt == 3)
			{
				/*
				// Может прийти сразу несколько прерываний, поэтому вызываем tmkgetevd() до тех пор, пока event_data.nInt!=0
				while(event_data.nInt!= 0)
				{
					tmkgetevd(&event_data);// забываем остальные прерывания, чтобы повторно не считывать тоже самое
					//printf("[mil_send_data_chain] reread event_data.nInt=%d\n",event_data.nInt);
				}
				*/
/*			}
		}
		//while(event_data.nInt!=0);//Может прийти сразу несколько прерываний, поэтому вызываем tmkgetevd() до тех пор, пока event_data.nInt!=0, потом снова ждём прерывание
		milstd_mutex_unlock();

		// прервать ли операцию записи
		if(is_break)
		{
			return -1;// прерывание передачи
		}
		// ошибка при передаче
		if(ret==0)
			return 0;
		//if(ret==2)
		//	return 0;// прерывание передачи
	}
	while((len_total-len_transmited)>0);// пока не передали все данные или не получили ошибку
	// всё ок
	return 1;

}
// Послать массив данных, один (отправитель или получатель) должен быть КК, другой - ОУ
// channel - номер канала с которого слать (0-MAX_TMK_NUMBER)
// bus - основной или резервный канал (CX_BUS_A или CX_BUS_B)
// adress_ou - адрес оконечного устройства (получатель или отправитель, неважно)
// data - данные
// [in]  data_len - длина данных для отправки, в словах
// [out] data_len - число реально отправленных слов
// return: 1(успешно), 0(таймаут или ошибка передачи), -1(прерывание отправки)
int mil_send_data(int channel, int bus,int adress_ou, unsigned short *data, int *data_len)
{
	int n_base = 5;// номер используемой базы
	int ret,len;
	int len_transmited=0;// сколько уже передали
	int len_transmit=*data_len;// сколько ещё осталось передать
	unsigned short ks,bc_sw=0;// командное слово, расширенное слово результата обмена
	unsigned char subaddr=0x10;// подадрес (10000b)
	TTmkEventData	event_data;
	unsigned short mode = mil_get_mode(channel);
	// пока ничего не отправлено
	*data_len = 0;
	if(tmkselect(channel))// выбор текущего канала
		return -1;
	//adress_ou=1;
	if(mode==BC_MODE)// контроллер канала
	{
		static int num_block = 0;// номер пакета
		milstd_mutex_lock(channel);
		//unsigned short buffer[64];
		// Задание базы (0-255)
		ret = bcdefbase(n_base);
		ret = bcdefbus(bus);// передача по основному каналу (не по резервному BUS_B)
		milstd_mutex_unlock();

		//!!!num_block++;
		do
		{
			unsigned short os=0;// ответное слово
			len=len_transmit-len_transmited;
			if(len>32)	len=32;
			if(len<0)
				break;
			if(!len)	
				break;
			subaddr = num_block%30+1;//всего м.б. 30 подадресов 1-30 (0 и 31 для команд управления)
			//subaddr = num_block%5+1;//всего 5 подадресов 1-4
			//printf("subaddr=%d\n", subaddr);
			ks = (adress_ou<<11) | RT_RECEIVE | (subaddr<<5);// ОУ будет принимать данные по адресу adress_ou
			
			milstd_mutex_lock(channel);
			// запись командного слова
			bcputw(0,ks | len%32 );
			// заполняем слова данных
			bcputblk(1,data+len_transmited, len);// bcAddr=1 - начальный адрес внутри базы (0 - 63), данные, длина данных
			//ret = channel;//tmkselected();
			//mutex_unlock();
			//do
//			{
//				mutex_lock();
//#ifdef MILSTD_USE_MUTEX
//				tmkselect(channel);// выбор текущего канала
//#endif
			// запуск одиночной посылки
			if(adress_ou==31)// широковещательный пакет
				ret = bcstartx(n_base, DATA_BC_RT_BRCST | bus | CX_STOP | CX_NOSIG);//формат 1: передача данных КК-ОУ
			else
				ret = bcstartx(n_base, DATA_BC_RT | bus | CX_STOP | CX_NOSIG);//формат 1: передача данных КК-ОУ
			milstd_mutex_unlock();
			//num_block++;// номер переданного (или не переданного блока)

			// ждём прерывание об окончании отправки блока не более 200 милисек
			ret = CheckTmkEvent(channel,2000);
			if(ret!=1)
			{
				printf("[mil_send_data] break!!! ret=%d subaddr=%d \n",ret,subaddr);
				return ret;// timeout или прерывание передачи
			}

			// Если ret==1 то узнаём что за прерывание
			//if(ret==1)// || ret==2)
			milstd_mutex_lock(channel);
			//do
			{
				memset(&event_data,0,sizeof(event_data));
				tmkgetevd(&event_data);
				//if(event_data.nInt==0)
				//	break;
				// лишнее прерывание - пропускаем его
				if(!event_data.nInt)
				{
					printf("[mil_send_data] err nInt=%d start!!!\n",event_data.nInt);					
					// Может прийти сразу несколько прерываний, поэтому вызываем tmkgetevd() до тех пор, пока event_data.nInt!=0
					while(event_data.nInt!= 0)
						tmkgetevd(&event_data);// забываем остальные прерывания, чтобы повторно не считывать тоже самое
					//milstd_mutex_unlock();
					printf("[mil_send_data] err nInt=%d stop!!!\n",event_data.nInt);					
					continue;
					//*data_len  = len;
					//milstd_mutex_unlock();
					//return 1;
				}
				if(event_data.nInt<=0 || event_data.nInt>4)
				{
					milstd_mutex_unlock();
					printf("[mil_send_data] err nInt=%d!!!\n",event_data.nInt);
					break;
				}
				//  читаем ответное слово
				os = bcgetw(len+2);
				bc_sw = event_data.bcx.wResultX;// расширенное слово результата обмена
				// для широковещательного пакета игнорируем ошибку отсутствия ответного слова
				if(adress_ou==31 && (bc_sw & SX_TOA))// широковещательный пакет
					bc_sw&=~SX_TOA;//SX_TOA=2 - неверная пауза перед ответным словом (отсутствует ответное слово);

				if(event_data.nInt==3 && !bc_sw)
				{
					len_transmited+=len;
					*data_len = len_transmited;
					num_block++;
					//printf("[mil_send_data] event_data.nInt=%d bc_sw=%d\n",event_data.nInt,bc_sw);
				}
				// ошибка при передаче
				else
				{
					printf("[mil_send_data] event_data.nInt=%d bc_sw=%d\n",event_data.nInt,bc_sw);
					ret = 0;
					milstd_mutex_unlock();
					return 0;
					break;
				}
				//printf("mil_send_data event_data.nInt=%d\n",event_data.nInt);
				//if(event_data.nInt == 2 || event_data.nInt == 3)
				//{
					// Может прийти сразу несколько прерываний, поэтому вызываем tmkgetevd() до тех пор, пока event_data.nInt!=0
					//while(event_data.nInt!= 0)
					//	tmkgetevd(&event_data);// забываем остальные прерывания, чтобы повторно не считывать тоже самое
				//}
			}
			//while(event_data.nInt!=0);//Может прийти сразу несколько прерываний, поэтому вызываем tmkgetevd() до тех пор, пока event_data.nInt!=0, потом снова ждём прерывание
			milstd_mutex_unlock();

			// ошибка при передаче
			if(ret==0)
			{
				printf("[mil_send_data] err=%d!!!\n",ret);
				return 0;
			}
			//if(ret==2)
			//	return 0;// прерывание передачи
		}
		while(len>0);// пока не передали все данные или не получили ошибку
		return 1;

	}
	return 0;// плата не в нужном режиме
}

//#ifdef __QNXNTO__ // QNX 6 & win32

// Принять массив данных с таймаутом, не более одной посылки, получатель должен быть ОУ
// channel - номер канала с которого слать (0-MAX_TMK_NUMBER)
// bus - по основному или резервному каналу получены данные (BUS_A=0 или BUS_B=1)
// adress_ou - адрес оконечного устройства
// [out] buf - массив структур для приёма milstd данных
// [in/out] buf_count - количество входных/заполненых буферов данных
// timeout_ms - время ожидания данных в мс, -1 вечное ожидание
// return: 1 ОК, 0 если ошибка или прерывание, (-1 или 0xffff или 65535) timeout
int  mil_recv_data_timed(int channel, int bus,int adress_ou, milstd_buf_t *buf, int *buf_count, int timeout_ms)
{
	//static int dbg_count = 0;
	int ret=0;//,err;
	//unsigned short summ;// контрольная сумма
	unsigned short rt_sw=0;//,rt_sf=0;// слово состояния оу, флаговое слово оу
	// тек. режим - BC_MODE или RT_MODE
	unsigned short mode = mil_get_mode(channel);
	// число буферов с данными
	int buf_count_max = *buf_count;
	// число принятых кадров - пока ничего не получили
	*buf_count = 0;
	// ошибка
	if(!buf || buf_count_max<1 || channel<0 || adress_ou<0)
		return 0;
	if(mode!=RT_MODE)// оконечное устройство
	{
		mode = mil_get_mode(channel);
		return 0;// плата не в нужном режиме
	}
	do
	{
		int d_timeout = 50;// 50 мс
		int d_count = 0;// сколько прошло интервалов по d_timeout мс

		unsigned short subaddr;
		// номер буфера для подадреса
                //TMK_DATA_RET cur_buf = -1;
		TTmkEventData	event_data;
		memset(&event_data,0,sizeof(event_data));
		milstd_mutex_lock(channel);
		//if(rtgetpage()!=0)
		//	rtdefpage(0);// работаем только с нулевой страницей
		milstd_mutex_unlock();

		// если уже получили хотя бы один буфер, не ждём данные, а просто проверяем наличие
		// это на случай, если пришло сразу много блоков данных и все их нужно принять сразу
		if((*buf_count)>0)
		{
			// проверяем наличие прерывания
			int ret = 1;//CheckTmkEvent(channel, 2);//INFINITE
			if(ret==1)
			{
				milstd_mutex_lock(channel);
				// запросить структуру данных, описывающих прерывание
				tmkgetevd(&event_data);
				milstd_mutex_unlock();
			}
			if(ret==-1)
			{
				return 0;// прерывание работы платы
			}
			// если пока нет прерываний - выходим
			if(ret!=1 || event_data.nInt != 3)
				break;
			// время получения кадра
			buf[*buf_count].time = my_time_get_cur_usec2000();
		}
		// если ещё не получили ни одного буфера, ждём данные
		else
		{
			// цикл ожидания данных
			while(1)
			{
				memset(&event_data,0,sizeof(event_data));
				// может прийти сразу несколько прерываний, поэтому вызываем tmkgetevd() до тех пор, пока event_data.nInt!=0, потом снова ждём прерывание
				//if(event_data.nInt)
				//	continue;
				//printf("mil read  block channel=%d event_data.nInt=%d timeout_ms=%d\n",channel, event_data.nInt,timeout_ms);
				// проверка на таймаут при ожидании данных
				if(timeout_ms!=-1)
				{
					// ждём новое прерывание d_timeout милисек.
					ret = CheckTmkEvent(channel, d_timeout);//INFINITE
					// проверка на таймаут
					if(!ret && d_timeout*d_count>=timeout_ms)
					{
						return 0xffff;//-1 timeout работы функции
					}
					d_count++;
				}
				else
				{
					// ждём новое прерывание 2 сек., ret=1 значит пришло прерывание
					ret = CheckTmkEvent(channel,3000);//INFINITE
				}
				// timeout
				if(!ret)
				{
					// если уже получили какие-то данные
					if(*buf_count>0)
						return 1;
					// еще ничего не получили - переходим снова на ожидание данных
					else
					{
						milstd_mutex_lock(channel);
						// запросить структуру данных, описывающих прерывание
						tmkgetevd(&event_data);
						milstd_mutex_unlock();
						if(event_data.nInt == 3)
							printf("*** was timeout!!!! event_data.nInt=%d\n",event_data.nInt);
						continue;
					}
				}
				if(ret==-1)
				{
					return 0;// прерывание работы платы
				}
				// время получения кадра
				buf[*buf_count].time = my_time_get_cur_usec2000();

				milstd_mutex_lock(channel);
				// запросить структуру данных, описывающих прерывание
				tmkgetevd(&event_data);
				milstd_mutex_unlock();

				//printf("read channel=%d nInt=%d buf=%d subaddr=%d\n",channel, event_data.nInt,event_data.rt.wBuf,((event_data.rt.wStatus) & SUBADDR_MASK)>>5);

				if(event_data.nInt == 3)
				//if(event_data.nInt == 2 || event_data.nInt == 3)
				{
					// определяем текущий буфер для подадреса
					//cur_buf = event_data.rt.wBuf;//rtgetbuf();
					// определяем слово состояния оу
					//rt_sw = event_data.rt.wStatus;// rtgetmsgsw()

					// забываем остальные прерывания, чтобы повторно не считывать тоже самое
					/*while(event_data.nInt == 2 || event_data.nInt == 3)
					{
						tmkgetevd(&event_data);// забываем остальные прерывания, чтобы повторно не считывать тоже самое
						// узнаём номер буфера для подадреса и переходим в него
						TMK_DATA_RET cur_buf = event_data.rt.wBuf;//rtgetbuf();
						printf("wait cur_buf=%d nInt=%d \n",cur_buf, event_data.nInt);
					}*/
/*					break;// пришло нужное прерывание
				}
				printf("*** not to be here!!!! event_data.nInt=%d\n",event_data.nInt);
			};
		}

		// определяем текущий буфер для подадреса

                //cur_buf = event_data.rt.wBuf;//rtgetbuf();
		// определяем слово состояния оу
		rt_sw = event_data.rt.wStatus;// rtgetmsgsw()

		// получение данных

		// узнаём по какому субадресу получили данные
		subaddr=(rt_sw & SUBADDR_MASK)>>5;
		// читаем из области флагов флаговоe слово ОУ
		//rt_sf = rtgetflag(RT_RECEIVE,subaddr);

		milstd_mutex_lock(channel);
		// читаем полученные данные
		{
			TMK_DATA_RET sel_buf;
			// число принятых слов
			int len = (rt_sw & NWORDS_MASK);
			if(!len) len=32;
			// переключаемся на страницу subaddr
			//unsigned short cur_subaddr = rtgetsubaddr();
			//if(subaddr!=cur_subaddr)// нет смысла проверять, т.к. subadr постоянно меняется
			rtdefsubaddr(RT_RECEIVE, subaddr);

			// переходим в номер буфера, в котором д.б. данные
			sel_buf = rtgetbuf();
                        /*if(cur_buf!=sel_buf)
			{
				rtdefbuf(cur_buf);
				sel_buf =rtgetbuf();
                        }*/
			// если входной буфер недостаточной длины, читаем не все данные
			//if((*data_len+len)>data_len_max)
			//	len = data_len_max-*data_len;

			/*/ ждём освобождения подадреса
			while(rtbusy())
			{
				printf("busy()! *buf_count=%d cur_buf=%d subaddr=%d\n",*buf_count, cur_buf,subaddr);
			}*/
			// читаем блок данных
/*			rtgetblk(0,buf[*buf_count].milstd_data,len);
			buf[*buf_count].milstd_data_len = len;

			//printf("*read channel=%d nInt=%d buf=%d subaddr=%d\n",channel, event_data.nInt,event_data.rt.wBuf,((event_data.rt.wStatus) & SUBADDR_MASK)>>5);
			//printf("*buf_count=%d cur_buf=%d sel_buf=%d subaddr=%d\n",*buf_count, cur_buf,sel_buf,subaddr );
		}
		milstd_mutex_unlock();

		//dbg_count +=buf[*buf_count].milstd_data_len;
		//printf("!!recv byte=%d total=%d\n",buf[*buf_count].milstd_data_len*2,dbg_count*2);

		// debug crc света
		/*if(0)
		{
			uint16_t *data = (char*)buf[*buf_count].milstd_data;
			int data_len = (char*)buf[*buf_count].milstd_data_len;
			uint16_t crc32_calc = 0;
			uint16_t crc32 = 0;
			int j;
			for (j = 0; j < data_len - 1; j++)
			{
				crc32_calc += data[j];
			}
			crc32 = data[data_len-1];
			if (crc32_calc != crc32)
			{
				printf("*buf_count=%d data_len=%d cur_buf=%d sel_buf=%d subaddr=%d",*buf_count,data_len, cur_buf,rtgetbuf(),subaddr );
				printf(" crc32_calc=0x%x crc32=0x%x!!!\n",crc32_calc, crc32);
			}
			else
			{
				//printf("*buf_count=%d data_len=%d cur_buf=%d sel_buf=%d subaddr=%d",*buf_count,data_len, cur_buf,rtgetbuf(),subaddr );
				//printf(" crc32_calc=0x%x crc32=0x%x\n",crc32_calc,crc32);
			}
		}*/
/*		if(0)
		{
			static int subaddr_prev=-1, cur_buf_prev = -1;
			char *data = (char*)buf[*buf_count].milstd_data;
			//static int cur_buf_prev[8];// = -1;
			static int64_t gl_count_prev[8];// = 0;
			int64_t gl_count = *((int64_t*)data + 1);
			//if(gl_count1-gl_count1_prev==0)//
			//if(gl_count_prev[channel] && gl_count-gl_count_prev[channel]!=1)
			if( (gl_count_prev[channel] && gl_count-gl_count_prev[channel]!=1) || (subaddr_prev!=-1 && (subaddr_prev!=30 || subaddr!=1) && (subaddr-subaddr_prev!=1)))
			{
				printf("***channel=%d cur=%lld prev=%lld (d=%lld)\n",channel,gl_count,gl_count_prev[channel], gl_count-gl_count_prev[channel]);
                        //	printf("***         subaddr=%d(prev=%d) cur_buf=%d(prev=%d)\n",subaddr,subaddr_prev,cur_buf,cur_buf_prev);
				//printf("subaddr=%d(prev=%d) cur_buf=%d(prev=%d) cur_buf_prev=%d t1=%d t_prev=%d d_t=%d\n",subaddr, cur_buf, cur_buf_prev,t1,t1_prev,t1-t1_prev);
			}
			gl_count_prev[channel] = gl_count;
			//cur_buf_prev = cur_buf;
			//t1_prev = t1;
			subaddr_prev = subaddr;
//			cur_buf_prev = cur_buf;
		}
		// запоминаем командное слово, вернее слово состояния
		buf[*buf_count].ks = rt_sw;
		// переходим на следующий кадр манчестнра
		(*buf_count)++;
		//usleep(0*1000*1000);
	}
	// проверяем, сколько осталось буферов
	while(buf_count_max > *buf_count);
	return 1;
}

// Принять массив данных, не более одной посылки, получатель должен быть ОУ
// channel - номер канала с которого слать (0-MAX_TMK_NUMBER)
// bus - по основному или резервному каналу получены данные (BUS_A=0 или BUS_B=1)
// adress_ou - адрес оконечного устройства
// [out] buf - массив структур для приёма milstd данных
// [in/out] buf_count - количество входных/заполненых буферов данных
// timeout_ms - время ожидания данных в мс, -1 вечное ожидание
// return: 1 ОК, 0 если ошибка или прерывание
int  mil_recv_data(int channel, int bus,int adress_ou, milstd_buf_t *buf, int *buf_count)
{
	int timeout_ms = -1;// вечное ожидание
	return mil_recv_data_timed(channel, bus, adress_ou, buf, buf_count, timeout_ms);
}

/*#else // кроме QNX

// Принять массив данных, не более одной посылки, получатель должен быть ОУ
// channel - номер канала с которого слать (0-MAX_TMK_NUMBER)
// bus - по основному или резервному каналу получены данные (BUS_A=0 или BUS_B=1)
// adress_ou - адрес оконечного устройства
// [out] buf - массив структур для приёма milstd данных
// [in/out] buf_count - количество входных/заполненых буферов данных
// timeout_ms - время ожидания данных в мс, -1 вечное ожидание
// return: 1 ОК, 0 если ошибка или прерывание
int  mil_recv_data(int channel, int bus,int adress_ou, milstd_buf_t *buf, int *buf_count)
{
	int ret=0;//,err;
	//unsigned short summ;// контрольная сумма
	unsigned short rt_sw=0;//,rt_sf=0;// слово состояния оу, флаговое слово оу
	// тек. режим - BC_MODE или RT_MODE
	unsigned short mode = mil_get_mode(channel);
	// число буферов с данными
	int buf_count_max = *buf_count;
	// число принятых кадров - пока ничего не получили
	*buf_count = 0;
	if(!buf || buf_count_max<1)	return 0;

	if(mode==RT_MODE)// оконечное устройство
	{
		unsigned short subaddr;
		TTmkEventData	event_data;
		memset(&event_data,0,sizeof(event_data));
		milstd_mutex_lock(channel);
		tmkselect(channel);// выбор текущего канала
		rtdefpage(0);// работаем только с нулевой страницей
		milstd_mutex_unlock();
		while(1)
		{
			milstd_mutex_lock(channel);
#ifdef MILSTD_USE_MUTEX
			tmkselect(channel);// выбор текущего канала
#endif
			// запросить структуру данных, описывающих прерывание
			tmkgetevd(&event_data);
			milstd_mutex_unlock();


			//printf("mil read  block event_data.nInt=%d\n",event_data.nInt);
			if(event_data.nInt == 2 || event_data.nInt == 3)
			{
				while(event_data.nInt == 2 || event_data.nInt == 3)
					tmkgetevd(&event_data);// забываем остальные прерывания, чтобы повторно не считывать тоже самое
				break;// пришло нужное прерывание
			}
			if(event_data.nInt)
				continue;// может прийти сразу несколько прерываний, поэтому вызываем tmkgetevd() до тех пор, пока event_data.nInt!=0, потом снова ждём прерывание
			// ждём новое прерывание
			ret = CheckTmkEvent(channel,5000);//INFINITE
			//printf("mil read  CheckTmkEvent=%d\n",ret);
			if(!ret)
				continue;// timeout
			if(ret==-1)
				return 0;// прерывание работы платы
		};
		rt_sw = event_data.rt.wStatus;// слово состояния оу
		// узнаём по какому субадресу получили данные
		subaddr=(rt_sw & SUBADDR_MASK)>>5;
		// читаем из области флагов флаговоe слово ОУ
		//rt_sf = rtgetflag(RT_RECEIVE,subaddr);

		milstd_mutex_lock(channel);
		// читаем полученные данные
		{
			int len = (rt_sw & NWORDS_MASK);// число принятых слов
			if(!len) len=32;
			// переключаемся на страницу subaddr
			rtdefsubaddr(RT_RECEIVE, subaddr);
			// читаем блок данных
			rtgetblk(0,buf[*buf_count].milstd_data,len);
			buf[*buf_count].milstd_data_len = len;
			// если входной буфер недостаточной длины, читаем не все данные
			//if((*data_len+len)>data_max_len)
			//	len = data_max_len-*data_len;
			// читаем блок данных
			//rtgetblk(0,data+*data_len,len);
			// *data_len+=len;
		}
		milstd_mutex_unlock();

		return rt_sw;
	}
	return 0;// плата не в нужном режиме
}

//#endif // QNX 6
*/


// удаляем все данные из буфера
// channel - номер канала(0-MAX_TMK_NUMBER)
/*void mil_clear_buf(int channel)
{
	TTmkEventData	event_data;
	memset(&event_data,0,sizeof(event_data));
	milstd_mutex_lock(channel);
	// запросить структуру данных, описывающих прерывание
	do{
		tmkgetevd(&event_data);
	}
	while(event_data.nInt);
	milstd_mutex_unlock();
}
*/
