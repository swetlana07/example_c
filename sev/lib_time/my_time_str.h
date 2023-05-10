// my_time_str.h
// Функции для работы со временем
#ifndef _MYTIMESTR_H_
#define _MYTIMESTR_H_

#include "my_time.h"

// Make sure we can call this stuff from C++.
#if __cplusplus
extern "C" {
#endif

// получить GTM дату и время
// потом требует удаления g_date_time_unref()
//GDateTime* date_time_get_gmt(struct tm *timeptr);

// получить местную дату и время
// потом требует удаления g_date_time_unref()
//GDateTime* date_time_get_local(struct tm *timeptr);

// получить строку с локальной датой и временем из UTC времени
// timeptr - локальное время
// строка потом требует удаления g_free()
//char* date_time_get_local_from_gmt_str(struct tm *timeptr);

// получить строку с локальной датой и временем из местного времени
// timeptr - локальное время
// строка потом требует удаления g_free()
//char* date_time_get_local_from_local_str(struct tm *timeptr);

// получить строку с датой и временем
// timeptr - локальное время
// строка потом требует удаления g_free()
char* date_time_get_str(struct tm *timeptr, long ms);

// перевести из секунд в строку типа: 3 дня 5 часов 42 минуты 12 секунд
char *get_time_str(int n_sec);

// перевести из градусов в строку
// hi_tocnost - секунды с точностью до 2 знаков(TRUE) или одного(FALSE)
char *get_lat_grad_str(double gr, gboolean hi_tocnost);
char *get_lon_grad_str(double gr, gboolean hi_tocnost);

#if __cplusplus
}  // End of the 'extern "C"' block
#endif

#endif // #ifndef _MYTIMESTR_H_
