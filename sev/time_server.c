#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#if defined(__QNXNTO__)
#include <unistd.h>
#include <sys/neutrino.h>
#endif
#include "ntp_server.h"
#include "lib_common/keys_api.h"
#include "lib_common/keys_api_util.h"
#include "lib_common/log_api.h"
#include "lib_control/control_client.h"
#include "lib_time/my_time.h"
#include "lib_milstd/milstd_lib.h"
#include "lib_net/sock_func.h"
// для отладки - без получения СЕВ по манчестеру
//#define	WITHOUT_MILSTD

// только для Visual Studio (for VS2013 _MSC_VER=1800)
#if defined(_MSC_VER) //&& (_MSC_VER >= 1020)
/*
 C:\Users\Александр>pkg-config --cflags glib-2.0
 -mms-bitfields -IC:/msys32/mingw32/include/glib-2.0 -IC:/msys32/mingw32/lib/glib-2.0/include -IC:/msys32/mingw32/include
 C:\Users\Александр>pkg-config --libs glib-2.0
 -LC:/msys32/mingw32/lib -lglib-2.0 -lintl
 */
#pragma comment(lib,"libwinpthread-1.lib")
#pragma comment(lib,"libglib-2.0-0.lib")
#pragma comment(lib,"libintl-8.lib")
#pragma comment(lib,"ws2_32.lib")
#endif

// для поправки к локальному времени относительно времени СЕВ
static pthread_mutex_t mutex_popravka = PTHREAD_MUTEX_INITIALIZER;
static volatile long long popravka_var = 0;

// определить поправку к локальному времени относительно времени СЕВ  в микросекундах
long long get_popravka() {
	pthread_mutex_lock(&mutex_popravka);
	long long var = popravka_var;
	pthread_mutex_unlock(&mutex_popravka);
	return var;
}

// задать поправку к локальному времени относительно времени СЕВ в микросекундах
static void set_popravka(long long var) {
	pthread_mutex_lock(&mutex_popravka);
	popravka_var = var;
	pthread_mutex_unlock(&mutex_popravka);
}

// установка системного времени
bool set_systemtime(void) {
	int ret;
	pthread_mutex_lock(&mutex_popravka);
	//сдвиг системного времени на смещение в микросекундах
	ret = set_clock_from_dusec(popravka_var);
	popravka_var = 1;
	pthread_mutex_unlock(&mutex_popravka);
	return (bool) ret;
}

// установить/сбросить индикатор приёма СЕВ
bool set_ind_sev(int new_state) {
	static int cur_state = -1;
	if (new_state == cur_state)
		return true;
	log_write_printf("set_ind_sev new_state=%d in", new_state);
	// изменить световой индикатор
	bool ret = contol_client_signal_set_ind("127.0.0.1", IND_SEV, new_state);
	if (ret)
		cur_state = new_state;
	log_write_printf("set_ind_sev end ret=%d", ret);
	return ret;
}
// отправить массив состояния СЕВ
bool mdi_sev(ADDR_PU *adr, char* state) {
//int TIMEOUT_MS=		9000;	//timeout отправки и получения ответа - в миллисекундах (9 сек)
    int timeout_ms=	500;		//timeout соединения - в миллисекундах
	bool ret=false;
	SOCKET sockfd;
	if(!s_connect(adr->ip_1, adr->port, &sockfd, timeout_ms)){
		s_send(sockfd, (unsigned char *)state, 8, timeout_ms);
	}else if(!s_connect(adr->ip_2, adr->port, &sockfd, timeout_ms)){
		s_send(sockfd, (unsigned char *)state, 8, timeout_ms);
	}
	s_close(sockfd, timeout_ms);
	return ret;
}
int main(int argc, char *argv[]) {
#ifdef _WIN32
	// иннициализация файлов настроек
	//key_set_file("./config.ini");
	log_set_file("./log_time_server.txt");
#else
	// иннициализация файлов настроек
	//key_set_file("/usr/local/etc/config.ini");
	log_set_file("/usr/local/var/log_time_server.txt");
#endif
	log_write(NULL);
	{
		char *kreit_name = NULL;
		bool is_init = kreit_key_set_file(&kreit_name);
		// пишем в log-файл о начале запуска приложения
		if (is_init)
			log_write_printf("Start TimeServer, крейт=%s", kreit_name);
		else
			log_write_printf(
					"Start TimeServer fail!!!, not found file для крейта=%s",
					kreit_name);
		g_free(kreit_name);
	}
	//узнаем, какое время использовать:  московское(=1), поясное(=2),гринвич(=0 и другое)
	int8_t t_z=key_util_get_tz("t_z");
	if ((t_z !=1)&&(t_z!=2)) t_z=0;
	printf("TZ=%d\n",t_z);
	//узнаем адрес и порт для отправки массива состояния state_sev
	ADDR_PU *adr_1 = key_util_get_mdi("pu1");
	ADDR_PU *adr_2 = key_util_get_mdi("pu2");
	//узнаем, какое время использовать
// увеличиваем приоритет текущего потока и меняем алгоритм планирования
#if defined(__QNXNTO__)
	{
		struct sched_param param;
		sched_getparam(0, &param);
		// меняем алгоритм планирования текущего потока, чтобы его никто не вытеснил по истечении кванта времени
		sched_setscheduler(0, SCHED_FIFO, &param);
		// повышаем приоритет текущего потока регистрации данных до 20f (default 10r)
		pthread_setschedprio(pthread_self(), 20);
	}
#endif

#if defined(__QNXNTO__)
	// меняем время одного кванта времени до 100 микросекунд (default 1 миллисекунда)
	if (0) {
		int ret;
		struct _clockperiod old;
		struct _clockperiod new;
		new.fract = 0;
		new.nsec = 1000000;// 1ms
		//new.nsec=1000000000;
		ret = ClockPeriod(CLOCK_REALTIME, &new, &old, 0);
		/*struct _clockadjust canew;
		 struct _clockadjust caold;
		 canew.tick_count = 0;/// за сколько милисек
		 canew.tick_nsec_inc =0;
		 ClockAdjust(CLOCK_REALTIME,&canew,&caold);
		 ClockAdjust(CLOCK_REALTIME,NULL,&caold);*/
		log_write_printf("[main] ClockPeriod new: %d prev: %d\n", new.nsec,
				old.nsec);
		ret = 0;
	}
#endif
	// определяем разрешение таймера
	{
		struct timespec res;
		if (clock_getres(CLOCK_REALTIME, &res) == -1)
			log_write("[time_server] clock_getres = -1!!!");
		else
			log_write_printf("[time_server] Разрешение таймера %ld наносек.",
					res.tv_nsec);
	}

	// запускаем поток раздачи времени СЕВ по сети
	//ntp_server_start();


	// Инициализация работы с устройствами MilStd - вызывается один раз
#ifndef	WITHOUT_MILSTD

	int channel1 = -1;// на какой канал принимать данные - основной канал
	int addr_ou1 = -1;// на какой адрес принимать данные - основной канал
	int channel2 = -1;// на какой канал принимать данные - резервный канал
	int addr_ou2 = -1;// на какой адрес принимать данные - резервный канал
	// чтение параметров каналов milstd из файла неастроек
	CHANNEL_MILSTD *milstd_osn = key_util_get_milstd("ch-sev1");
	CHANNEL_MILSTD *milstd_rez = key_util_get_milstd("ch-sev2");
	if (!milstd_osn)
		log_write("не найдены настройки основного канала milstd");
	else {
		channel1 = milstd_osn->channel;
		addr_ou1 = milstd_osn->addr_ou;
	}
	if (!milstd_rez)
		log_write("не найдены настройки резервного канала milstd");
	else {
		channel2 = milstd_rez->channel;
		addr_ou2 = milstd_rez->addr_ou;
	}

	if (mil_init() < 0) {
		log_write("Tmk not found");
		return EXIT_FAILURE;
	} else
		log_write("Tmk init ok");
	// настроить основной milstd канал
	int ret1 = mil_open(channel1, RT_MODE, addr_ou1, false);// без буферизации, нужен последний полученный пакет
	if (ret1 < 0)
		log_write_printf("milstd channel %d init error=%d", channel1, ret1);
	// настроить резервный milstd канал
	int ret2 = mil_open(channel2, RT_MODE, addr_ou2, false);// без буферизации, нужен последний полученный пакет
	if (ret2 < 0)
		log_write_printf("milstd channel %d init error=%d", channel2, ret2);
#endif

	char state_sev[8];//массив состояния, байты 0-1-первая магистраль, 2-3 вторая, 5-zone
	int buf_count = 2;
	milstd_buf_t buf[2];
	uint64_t count_step = 0, count_recv = 0;// номер помученной посылки со временем
	int tm_hour_zone=0;//текущий час заданного в t_z времени
	char zone=0;//текущий временной пояс
	strncpy(state_sev,"tsv",3);

	while (1) {
/*#ifdef	WITHOUT_MILSTD
		// пауза 1 секунда
#ifdef _WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
		continue;
#endif
#ifdef _WIN32
			Sleep(1000);
#else
			sleep(1);
#endif
		continue;*/
		sleep(1);
		// основной канал СЕВ - ждём не более 50 милисек (данные должны приходить раз в милисекунду)
		buf_count = 2;// восстанавливаем первоначальный размер буфера (в словах)
		// удаляем все старые данные из буфера - чтобы время было самое новое
		mil_clear_buf(channel1);
		int i;
		for ( i = 3; i < 7; i++)
			state_sev[i] = 0;//обнуляем массив состояния, кроме преамбулы 0..2 байт и временного пояса 7 байт
		int res = mil_recv_data_timed(channel1, BUS_A, addr_ou1, buf,
				&buf_count, 50);
		if (buf_count > 1)
			printf("buf_count=%d \n", buf_count);
		// прерывание или таймаут
		if (!res || res == 0xffff)
			log_write("sev1 no\n");
		state_sev[3] = (char) res;
		// если основной канал не отвечает, пробуем по резервному
		if (!res || res == 0xffff) {
			// удаляем все старые данные из буфера - чтобы время было самое новое
			mil_clear_buf(channel2);
			// резервный канал СЕВ -  ждём не более 50 милисек
			buf_count = 2;
			res = mil_recv_data_timed(channel2, BUS_A, addr_ou2, buf,
					&buf_count, 50);
			//ks = mil_recv_data(channel2,BUS_A,addr_ou2,data,&data_len);
			if (!res || res == 0xffff)
				log_write("sev2 no\n");
			state_sev[5] = (char) res;
			if ((state_sev[5]==1) &&(buf[buf_count - 1].milstd_data[4] & 1))
				state_sev[6] = 1;
		}
		if (!res || res == 0xffff) {
			// сбросить индикатор приёма СЕВ
			set_ind_sev(0);
		}
		//printf("do state_sev[3]=%d, state_sev[4]=%d, state_sev[5]=%d, state_sev[6]=%d\n",state_sev[3],state_sev[4], state_sev[5],state_sev[6]);
		// расшифровка СЕВа
		if (res == 1 && buf_count > 0) {
			if (state_sev[3] == 1) {
				if (!(buf[buf_count - 1].milstd_data[4] & 1)) {//если инфа, принятая по первой магистрали недостоверна, переходим на вторую
					// удаляем все старые данные из буфера - чтобы время было самое новое
					mil_clear_buf(channel2);
					// резервный канал СЕВ -  ждём не более 50 милисек
					buf_count = 2;
					res = mil_recv_data_timed(channel2, BUS_A, addr_ou2, buf,
							&buf_count, 50);
					//ks = mil_recv_data(channel2,BUS_A,addr_ou2,data,&data_len);
					if (!res || res == 0xffff)
						log_write("sev2 no\n");

					state_sev[5] = (char) res;
					if ((state_sev[5]==1) &&(buf[buf_count - 1].milstd_data[4] & 1))
						state_sev[6] = 1;
				} else
					state_sev[4] = 1;
			}

			if (((state_sev[3]==1) && state_sev[4]) || ((state_sev[5]==1) && state_sev[6])) {//есть достоверный сев
				// используем последний принятый блок
				uint16_t *data = buf[buf_count - 1].milstd_data;
				struct tm tm_gmt;
				tm_gmt.tm_year = ((data[0] & 0xf000) >> 12) * 10 + ((data[0]
						& 0x0f00) >> 8) + 100;
				tm_gmt.tm_mon = ((data[0] & 0x00f0) >> 4) * 10 + (data[0]
						& 0x000f) - 1;
				tm_gmt.tm_mday = ((data[1] & 0xf000) >> 12) * 10 + ((data[1]
						& 0x0f00) >> 8);
				tm_gmt.tm_hour = ((data[1] & 0x00f0) >> 4) * 10 + (data[1]
						& 0x000f);// гринвический час
				//tm_gmt.tm_hour = ((data[2] & 0xf000)>>12)*10 + ((data[2] & 0x0f00)>>8);// московский час + 3ч
				//tm_gmt.tm_hour = ((data[2] & 0x00f0)>>4 )*10 + (data[2] & 0x000f);// поясной час + 5ч
				tm_gmt.tm_min = ((data[3] & 0xf000) >> 12) * 10 + ((data[3]
						& 0x0f00) >> 8);
				tm_gmt.tm_sec = ((data[3] & 0x00f0) >> 4) * 10 + (data[3]
						& 0x000f);
				int msec = ((data[4] & 0xf000) >> 12) * 100 + ((data[4]
						& 0x0f00) >> 8) * 10 + ((data[4] & 0x00f0) >> 4);
				if(t_z==1)
					tm_hour_zone=((data[2] & 0xf000)>>12)*10 + ((data[2] & 0x0f00)>>8);// московский час + 3ч
				else if(t_z==2)
				    tm_hour_zone=((data[2] & 0x00f0)>>4 )*10 + (data[2] & 0x000f);// поясной час + 5ч
				// локальное время, в микросекундах от 2000г.
				//static long long cur_usec_prev = 0;
				long long cur_usec = my_time_get_cur_usec2000();
				// время СЕВ, в микросекундах от 2000г.
				//static long long sev_usec_prev = 0;
				long long sev_usec = my_time_time_to_msec2000(&tm_gmt, msec)
						* 1000;
				// поправка к локальному времени, в микросекундах
				long long delta = sev_usec - cur_usec;
				// только в первый раз получили время СЕВ

				//если задано московское или поясное время
				if(t_z){
				  if(abs(tm_hour_zone-tm_gmt.tm_hour)<=12){
					  delta+=(long long)(tm_hour_zone-tm_gmt.tm_hour)*3600*1000*1000;
					  zone=(char)(tm_hour_zone-tm_gmt.tm_hour);
				  }else{
					  if (tm_hour_zone>=tm_gmt.tm_hour){
						  delta+=(long long)(tm_hour_zone-tm_gmt.tm_hour-24)*3600*1000*1000;
						  zone=(char)(tm_hour_zone-tm_gmt.tm_hour-24);
					  }else{
						  delta+=(long long)(tm_hour_zone-tm_gmt.tm_hour+24)*3600*1000*1000;
						  zone=(char)(tm_hour_zone-tm_gmt.tm_hour+24);
					  }
				  }
				  state_sev[7]=zone;
				 // printf("hr_grinv=%d, hr_msk=%d, hr_pojs=%d, ZONE=%d\n",tm_gmt.tm_hour,((data[2] & 0xf000)>>12)*10 + ((data[2] & 0x0f00)>>8),((data[2] & 0x00f0)>>4 )*10 + (data[2] & 0x000f),state_sev[7]);
				}state_sev[7]=0;
				printf("t_z=%d, hr_grinv=%d, hr_msk=%d, hr_pojs=%d, ZONE=%d\n",state_sev[7],tm_gmt.tm_hour, ((data[2] & 0xf000)>>12)*10 + ((data[2] & 0x0f00)>>8),((data[2] & 0x00f0)>>4 )*10 + (data[2] & 0x000f),state_sev[7]);
				if (!count_recv) {
					log_write_printf("Before set system time delta=%lld usec",
							delta);
					// установка системного времени
					if (set_clock_from_dusec(delta))// return: 1 OK, 0 Err (нет прав для установки времени)
					{
						// пересчитывыем локальное время снова
						cur_usec = my_time_get_cur_usec2000();
						// новая поправка поправка к локальному времени, в микросекундах
						delta = sev_usec - cur_usec;
						log_write_printf(
								"After set system time: delta=%lld usec OK",
								delta);
					} else
						log_write_printf(
								"Set system time: delta=%lld usec False!!!",
								delta);
				}
				// запоминаем поправку (разницу между локальным временем и временем СЕВ)
				set_popravka(delta);
				//проверить параметры сев2 при достоверном сев1
				if (((state_sev[3]==1) & state_sev[4])){
					mil_clear_buf(channel2);
					// резервный канал СЕВ -  ждём не более 50 милисек
					buf_count = 2;
					res = mil_recv_data_timed(channel2, BUS_A, addr_ou2, buf,
											&buf_count, 50);
									//ks = mil_recv_data(channel2,BUS_A,addr_ou2,data,&data_len);
					state_sev[5] = (char) res;
					if ((buf[buf_count - 1].milstd_data[4] & 1))
							state_sev[6] = 1;
				}else;
				// установить индикатор приёма СЕВ
				{
					long long cur_usec_prev = 0;
					if ((cur_usec - cur_usec_prev) > 1000)// меняем состояние не чаще раза в секунду
					{
						set_ind_sev(1);
						cur_usec_prev = cur_usec;
					}
				}
				if (!(count_recv % 60000))// 60000 - раз в минуту
				log_write_printf("delta=%lld,%03lld,%03lld sec", delta
						/ 1000000, (delta / 1000) % 1000, (delta) % 1000);
				count_recv++;
			}


			/*/ поправка к локальным часам
			 #if defined(__QNXNTO__)
			 {
			 struct _clockadjust canew;
			 struct _clockadjust caold;
			 canew.tick_count = 1000;// сколько тиков применять поправку (1 микросек.)
			 canew.tick_nsec_inc = delta*10;// поправка на каждом тике, в наносекундах
			 // поправка не более 10% от точности часов, (10000 наносек.= 10 микросек.)
			 if(canew.tick_nsec_inc>10000)	canew.tick_nsec_inc = 10000;
			 if(canew.tick_nsec_inc<-10000)	canew.tick_nsec_inc = -10000;
			 ClockAdjust(CLOCK_REALTIME,&canew,&caold);
			 ClockAdjust(CLOCK_REALTIME,NULL,&caold);
			 //ret = 0;

			 }
			 #endif

			if (0) //(!(tm_gmt.tm_sec) && !msec) || // ровное число минут и секунд - индикатор работы программы
			//!msec ||  // ровное число секунд - индикатор работы программы
			//(sev_usec-sev_usec_prev)!=1000 ||// пропуск/повтор пакета с СЕВом
			//(cur_usec-cur_usec_prev)<700 || (cur_usec-cur_usec_prev)>1300 ) // сдвиг приёма пакета по времени
			{

				// получаем локальную дату/время
				struct tm tm_gmt_loc;
				int msec_loc_int = my_time_msec2000_to_time((cur_usec + 500)
						/ 1000, &tm_gmt_loc);
				double msec_loc = (cur_usec % 1000000) / 1000.;
				// печатаем разницу, время СЕВ и локальное время
				printf("\n");
				printf("popravka=%lf sec\n", delta / 1000000.);
				printf("sev %02d:%02d:%02d,%03d\n", tm_gmt.tm_hour,
						tm_gmt.tm_min, tm_gmt.tm_sec, msec);
				printf("loc %02d:%02d:%02d,%07.3lf (%03d)\n",
						tm_gmt_loc.tm_hour, tm_gmt_loc.tm_min,
						(int) tm_gmt_loc.tm_sec, msec_loc, msec_loc_int);
				//printf("d_sev=%.3lf msec (sev_usec=%lld sev_usec_prev=%lld)\n",(sev_usec-sev_usec_prev)/1000.,sev_usec,sev_usec_prev);
				//printf("d_loc=%.3lf msec (cur_usec=%lld cur_usec_prev=%lld)\n",(cur_usec-cur_usec_prev)/1000.,cur_usec,cur_usec_prev);

			}
			*/
		} else {
			// если не таймаут - прерывание ожидания сообщения по milstd каналу
			if (res != 0xffff)
				break;
		}
		// если только первый раз получили данные (или не получили)
		if (!count_step) {
			// запускаем поток раздачи времени СЕВ по сети
			ntp_server_start();
		}
		count_step++;
		//отправить state_sev раз в 10 секунд
		if(!(count_step % 10)){
			printf("count_step=%lld\n",count_step);
			mdi_sev(adr_1, state_sev);
			mdi_sev(adr_2, state_sev);
			printf("pu1=%s, pu2=%s\n",adr_1->ip_1,adr_2->ip_1);
		}
	};
	// закрытие всех используемых устройств milstd
	mil_close();
	// остановка ntp сервера
	ntp_server_stop();
	log_write("Exit time server");
	printf("Exit time server\n");
	return EXIT_SUCCESS;
}
