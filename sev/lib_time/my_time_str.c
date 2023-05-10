// my_time_str.c
// Функции для перевода времени в строки

//#ifdef _WIN32
//#define _CRT_SECURE_NO_WARNINGS
//#include <windows.h>
//#endif
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <glib.h>
#include <locale.h>
//#include <libintl.h>//
#include "../lib_common/package_locale.h"
#include "my_time_str.h"


// Make sure we can call this stuff from C++.
#if __cplusplus
extern "C" {
#endif

/*/ получить GTM дату и время
// потом требует удаления g_date_time_unref()
GDateTime* date_time_get_gmt(struct tm *timeptr)
{
	static GTimeZone *tz = NULL;
	if(!tz)
			tz = g_time_zone_new_utc();
	if(!tz)
		return NULL;
	GDateTime *datetime = g_date_time_new(tz,
		timeptr->tm_year + 1900,
		timeptr->tm_mon + 1,
		timeptr->tm_mday,
		timeptr->tm_hour,
		timeptr->tm_min,
		timeptr->tm_sec);
	//g_time_zone_unref(tz);
	return datetime;
}

// получить местную дату и время
// потом требует удаления g_date_time_unref()
GDateTime* date_time_get_local(struct tm *timeptr)
{
	static GTimeZone *tz = NULL;
	if(!tz)
		tz = g_time_zone_new_local();
	if(!tz)
		return NULL;
	GDateTime *datetime = g_date_time_new(tz,
		timeptr->tm_year + 1900,
		timeptr->tm_mon + 1,
		timeptr->tm_mday,
		timeptr->tm_hour,
		timeptr->tm_min,
		timeptr->tm_sec);
	//g_time_zone_unref(tz);
	return datetime;
}

// получить строку с локальной датой и временем из UTC времени
// timeptr - локальное время
// строка потом требует удаления g_free()
char* date_time_get_local_from_gmt_str(struct tm *timeptr)
{
	//gchar *ansi = (gchar*)g_malloc(256);
	//strftime(ansi, 250, "%F", timeptr);//дата
	//ret_str = g_locale_to_utf8(ansi,-1,0,0,0);
	GDateTime *datetime = (timeptr) ? date_time_get_gmt(timeptr) : NULL;
	GDateTime *datetime_l = (datetime) ? g_date_time_to_local(datetime) : NULL;
	gchar *str = (datetime_l) ? g_date_time_format(datetime_l, "%d.%m.%Y %X") : NULL;//"%x %X" "%F %T"
	//gchar *str = g_date_time_format(datetime, "%X");
	
	if(datetime) g_date_time_unref(datetime);
	if (datetime_l) g_date_time_unref(datetime_l);
	return str;
}

// получить строку с локальной датой и временем из местного времени
// timeptr - локальное время
// строка потом требует удаления g_free()
char* date_time_get_local_from_local_str(struct tm *timeptr)
{
	//gchar *ansi = (gchar*)g_malloc(256);
	//strftime(ansi, 250, "%F", timeptr);//дата
	//ret_str = g_locale_to_utf8(ansi,-1,0,0,0);
	GDateTime *datetime = date_time_get_local(timeptr);
	gchar *str = (datetime) ? g_date_time_format(datetime, "%d.%m.%Y %X") : NULL;//"%x %X" "%F %T"
	//gchar *str = g_date_time_format(datetime, "%X");

	if(datetime) g_date_time_unref(datetime);
	return str;
}*/

// получить строку с датой и временем
// timeptr - локальное время
// строка потом требует удаления g_free()
char* date_time_get_str(struct tm *timeptr, long ms)
{
	if(!timeptr)
		return NULL;
	char *str = g_strdup_printf("%4d-%02d-%02d %02d:%02d:%02d.%03ld",
			timeptr->tm_year + 1900,
			timeptr->tm_mon + 1,
			timeptr->tm_mday,
			timeptr->tm_hour,
			timeptr->tm_min,
			timeptr->tm_sec,
			ms);
	return str;
}

// перевести из секунд в строку типа: 3 дня 5 часов 42 минуты 12 секунд
char *get_time_str(int n_sec_in)
{
	char *val_str;
	int n_sec = abs(n_sec_in);
	if (n_sec <= 60)// менее минуты (секунду, секунд)
	{
		val_str = g_strdup_printf("%d %s", n_sec_in, g_ngettext("second", "secons", n_sec));
	}
	else if (n_sec <= 3600)// от 1 минуты до 60 минут
	{
		int minutes = n_sec_in / 60;// если число отрицательное, то только первое число со знаком -
		n_sec -= abs(minutes) * 60;
		const char *val_descr_m = g_ngettext("minute", "minutes", minutes);
		const char *val_descr_s = g_ngettext("second", "seconds", n_sec);
		val_str = g_strdup_printf("%d %s %02d %s", minutes, val_descr_m, n_sec, val_descr_s);
		//g_free(val_descr_m);
		//g_free(val_descr_s);
	}
	else if (n_sec <= 3600*24)// от 1 часа до 24 часов
	{
		int hour = n_sec_in / 3600;// если число отрицательное, то только первое число со знаком -
		n_sec -= abs(hour) * 3600;
		int minutes = n_sec / 60;
		n_sec -= minutes * 60;
		const char *val_descr_h = g_ngettext("hour", "hours", hour);
		const char *val_descr_m = g_ngettext("minute", "minutes", minutes);
		const char *val_descr_s = g_ngettext("second", "seconds", n_sec);
		val_str = g_strdup_printf("%d %s %02d %s %02d %s", hour, val_descr_h, minutes, val_descr_m, n_sec, val_descr_s);
		//g_free(val_descr_h); 
		//g_free(val_descr_m);
		//g_free(val_descr_s);
	}
	else // больше суток
	{
		int days = n_sec_in / (3600 * 24);// если число отрицательное, то только первое число со знаком -
		n_sec -= abs(days) * (3600 * 24);
		int hour = n_sec / 3600;
		n_sec -= hour * 3600;
		int minutes = n_sec / 60;
		n_sec -= minutes * 60;
		const char *val_descr_d = g_ngettext("day", "days", days);
		const char *val_descr_h = g_ngettext("hour", "hours", hour);
		const char *val_descr_m = g_ngettext("minute", "minutes", minutes);
		const char *val_descr_s = g_ngettext("second", "seconds", n_sec);
		val_str = g_strdup_printf("%d %s %02d %s %02d %s %02d %s", days, val_descr_d, hour, val_descr_h, minutes, val_descr_m, n_sec, val_descr_s);
		//g_free(val_descr_h); 
		//g_free(val_descr_m);
		//g_free(val_descr_s);
	}
	// чтобы добавить в файл переводов
	if (0) 
	{
		ngettext("day", "days", 0);
		ngettext("hour", "hours", 0);
		ngettext("minute", "minutes", 0);
		ngettext("second", "seconds", 0);
	}
	return val_str;
}

// перевести из градусов в строку
// ns_ew - символ (North South East West)
// hi_tocnost - секунды с точностью до 2 знаков(TRUE) или одного(FALSE)
static char *get_grad_str(double gr, gboolean hi_tocnost, const char *ns_ew)
{
	int grad = (int)gr;
	double min = (gr - grad) * 60.;
	double sec = (min - (int)min) * 60.;
	if (hi_tocnost)
		return g_strdup_printf("%3.2d° %02d′ %05.2lf″%s", grad, (int)min, sec, ns_ew);// °(degree) ⁰(superscrypt zero)
	return g_strdup_printf("%3.2d° %02d′ %04.1lf″%s", grad, (int)min, sec, ns_ew);
}

// перевести долготу из градусов в строку
// hi_tocnost - секунды с точностью до 2 знаков(TRUE) или одного(FALSE)
char *get_lon_grad_str(double gr, gboolean hi_tocnost)
{
	//int grad = (int)gr;
	const char *ns_ew;
	if (gr >= 0)// East
		ns_ew = _("E");
	else		// West
		ns_ew = _("W");
	return get_grad_str(fabs(gr), hi_tocnost, ns_ew);
}

// перевести широту из градусов в строку
// hi_tocnost - секунды с точностью до 2 знаков(TRUE) или одного(FALSE)
char *get_lat_grad_str(double gr, gboolean hi_tocnost)
{
	//int grad = (int)gr;
	const char *ns_ew;
	if (gr >= 0)// North
		ns_ew = _("N");
	else		// South
		ns_ew = _("S");
	return get_grad_str(fabs(gr), hi_tocnost, ns_ew);
}

#if __cplusplus
}  // End of the 'extern "C"' block
#endif
