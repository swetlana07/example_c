#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QTime>
#include <windows.h>
#include "WDMTMKv2.h"
#include <QThread>
#include <QFile>
#include <QBuffer>
#include <QSharedMemory>
#include <math.h>
QFile stdIn;
QFile log_file;
HANDLE hEvent, hConsole;
COORD pos_cons={0,0};
TTmkEventData event;
FILETIME ft,ftt;
unsigned short data[34];
int subaddr=0;
QString diag, str_pub;
int tmkNumber=0;//номер устройства
int precision=10;//точность синхронизации в миллисекундах 10..100
int zone=0;//часовой пояс, 0-гринвич, 3-москва, 99 -пояс от СЕВ
int regim_ta=0;//режим работы устройства 0- монитор, иначе ответное устройство
struct Izmer{
    qint64 datetime;
    qint64 cev_dt;
    qint64 delta;
    bool dost;

};
QVector <struct Izmer> izm;
QSharedMemory *shared_mem;


bool (*get_data)();

bool get_data_mt(){

          GetSystemTimeAsFileTime(&ft);//return the number of 100-nanosecond intervals since January 1, 1601 (UTC).
          ResetEvent(hEvent);
          tmkgetevd(&event);

          if((event.nInt == 3) || (event.nInt == 4))
          {
             mtgetblk(event.mt.wBase,data,7);//1 командное слово + 6-слов данных
             return true;
          }


    return false;
}
bool get_data_rt(){

  //        GetSystemTimeAsFileTime(&ft);//return the number of 100-nanosecond intervals since January 1, 1601 (UTC).
          ResetEvent(hEvent);
          tmkgetevd(&event);

          if((event.nInt == 3)||(!event.nInt)){
             data[0] = event.rt.wStatus;// rtgetmsgsw()

           // узнаём по какому субадресу получили данные
          if(subaddr!=((data[0] & SUBADDR_MASK)>>5)){
              subaddr=(data[0] & SUBADDR_MASK)>>5;
                // rtdefbuf(cur_buf);
              rtdefsubaddr(RT_RECEIVE, subaddr);
          }
           // получение данных
             rtgetblk(0,data+1,6);
             return true;
          }

    return false;
}

void (*preparation)();
void preparation_mt(){
    memset(data,0,64);
    mtstop();
    do
      tmkgetevd(&event);
    while(event.nInt );
    mtstartx(0,CX_STOP);
}
void preparation_rt(){
    do
      tmkgetevd(&event);
    while(event.nInt );
}
static int (*ini_milstd)();
int ini_milstd_rt(){
    // инициализация MilStd для ОУ
    if (TmkOpen())
    {
         return -1;
     }
    if(tmkconfig(tmkNumber)){    // printf("er=%d\n",-2);//diag="ТА1-USB failed!";
        return -2;
    }

    hEvent=NULL;
    if (!(hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) // ручной сброс события + сброшено событие
    {       // printf("er=%d\n",-3);//diag="No event!";
        return -3;
    }
    ResetEvent(hEvent);
    tmkdefevent(hEvent,TRUE);// привязка события для текущей выбранной платы (т.е. канала)
    int ret = rtreset();// инициализация выбранной платы(канала) с переводом ее в режим Оконечное устройство.
    rtdefaddress(21);
    // включаем режим работы ОУ восприятия командных слов с групповым адресом
    int ret_mode = rtdefmode(RT_BRCST_MODE);//ret = rtdefmode(RT_FLAG_MODE|RT_BRCST_MODE);
    int ret_irq = rtdefirqmode(RT_GENER1_BL|RT_GENER2_BL);// прерывания по приёму данных получаем
    // работаем только с нулевой страницей
    rtdefpage(0);
    rtdefsubaddr(RT_RECEIVE, 0);
    if(ret){       // printf("er=%d\n",-4);//diag="Режим ОУ не поддерживается!";
        return -4;
    }
    if(ret_irq || ret_mode){       // printf("er=%d\n",-5);//diag="Работа с прерываниями не поддерживается!";
        return -5;
    }

    return 0;

}

int ini_milstd_mt(){
    // инициализация MilStd для mt

    if (TmkOpen())
    {
       return -1;
    }
    if(tmkconfig(tmkNumber)){       // printf("er=%d\n",-2);//diag="ТА1-USB failed!";
        return -2;
    }
    hEvent=NULL;
    if (!(hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) // ручной сброс события + сброшено событие
    {      //  printf("er=%d\n",-3);//diag="No event!";
        return -3;
    }
    ResetEvent(hEvent);
    tmkdefevent(hEvent,TRUE);// привязка события для текущей выбранной платы (т.е. канала)
    if(mtreset()){// инициализация выбранной платы(канала) с переводом ее в режим Монитор.
      //  printf("er=%d\n",-6);//diag="Режим monitor не поддерживается!";
        return -6;
    }
    mtdefbase(0);
    return 0;
}
bool is_stop(){
    if(!shared_mem->attach())
        return true;
    shared_mem->lock();
    char ch[10];
    memset(ch,0,10);
    strncpy(ch,(char*)shared_mem->data(),4);
    shared_mem->unlock();
    shared_mem->detach();   // printf("rel=%s\n",ch);
    if(QString(ch).contains("stop"))
        return true;
    else return false;


}
QString systemTime_to_str(SYSTEMTIME st){
    QString res;
    res=QString::number(st.wYear)+"/ "+QString::number(st.wMonth)+"/ "+QString::number(st.wDay)+" - ";
    res+=QString::number(st.wHour)+":"+QString::number(st.wMinute)+":"+QString::number(st.wSecond)+":"+QString::number(st.wMilliseconds);

    return res;
}

int main(int argc, char **argv[])
{   //Командная строка:
    //argv[1]-номер устройства tmkNumber,
    //argv[2]-погрешность синхронизации в миллисекундах 1..10
    //argv[3]-часовой пояс, 0-гринвич, 99-москва, 100 -пояс от СЕВ
    //argv[4]- режим работы устройства 0- монитор, иначе ответное устройство
    //argv[5]- 0-нет записи в журнал, иначе есть запись в журнал
    shared_mem= new QSharedMemory("shared_rel");
    bool is_log=true;//по умолчанию есть запись в журнал
    int zone_cur=0;
    if(argc==6){
        bool ok;
        char* ch1=(char*)argv[1];
        int res=QString(ch1).toInt(&ok);
        if(ok)
            tmkNumber=res;//tmkNumber=(int)argv[1];//tmkNumber=(int)argv[1];

        char* ch2=(char*)argv[2];
        res=QString(ch2).toInt(&ok);

        if(ok){//точность синхронизации в миллисекундах 1..10
            precision=res;
            if (precision>100)precision=10;
        }

        char* ch3=(char*)argv[3];

        res=QString(ch3).toInt(&ok);

        if(ok)
           zone=res;//100-CEB
        if(((zone>12)&&(zone!=100))||(zone<-11))
           zone=0;

        char* ch4=(char*)argv[4];

        if(QString(ch4)=="0")
            regim_ta=0;
        else
            regim_ta=1;

        char* ch5=(char*)argv[5];

       if(QString(ch5)=="0")
           is_log=false;
       else
           is_log=true;
    }

//если precision=1 мс, то PERCIS=4000 мкс/10
    const int PERCIS=precision*200;

    int intrv;
    //intrv=500;
    if(precision<20) intrv=100;//100
    else if(precision<30)intrv=150;
    else if(precision<40)intrv=200;
    else if(precision<50)intrv=250;
    else if(precision<60)intrv=300;
    else intrv=500;

    if(!regim_ta) {
        ini_milstd=ini_milstd_mt;
        preparation=preparation_mt;
        get_data=get_data_mt;
    } else {
        ini_milstd=ini_milstd_rt;
        preparation=preparation_rt;
        get_data=get_data_rt;
    }

    unsigned long long ft_last_sin;//время последней успешной синхронизации
    const int BASE_SINHRO=8;//10//количество результатов для расчета погрешности
    const int BASE_IGNOR=0;//6//игнорировать 2 самых больших и 2 самых маленьких результата

    SYSTEMTIME st, stt, stt_cur;

    //TIME_ZONE_INFORMATION tz_system;
    //GetTimeZoneInformation(&tz_system);

    memset(&event,0,sizeof(event));
    int len, count, count_suc;// len-число принятых слов, count-количество опросов устройства, count_suc- количество успешных опросов устройства
    struct Izmer izm_;//результаты одного текущего опроса
    GetSystemTimeAsFileTime(&ft);
    ft_last_sin=*((long long *)&ft);
    //ft_last_sin=*((long long *)&ft+tz_system.Bias*600000000ll);
    FileTimeToSystemTime(&ft, &st);

    if(is_stop())return 0;

    if(int res=ini_milstd()){
        qDebug()<<diag;
        if(is_log)
            log_file.close();
        return res;
    }

    if(is_log){
         QString name_log_file=QString("logsev_%1%2%3-%4%5%6")
                          .arg(st.wYear,4)
                          .arg(QString::number(st.wMonth).rightJustified(2,'0'))
                          .arg(QString::number(st.wDay).rightJustified(2,'0'))
                          .arg(QString::number(st.wHour).rightJustified(2,'0'))
                          .arg(QString::number(st.wMinute).rightJustified(2,'0'))
                          .arg(QString::number(st.wSecond).rightJustified(2,'0'));/**/
        // QString name_log_file=QString("logsev_mt");qDebug()<<name_log_file;
         log_file.setFileName(name_log_file);
         if(log_file.open(QIODevice::ReadWrite | QIODevice::Text )){
            //is_log=true;
             if(!regim_ta)
                 log_file.write(QString("Start sinchro: tmkNum=%1, prec=%2 mc, zone=%3, regim=mt, is_proto=%4\n").arg(tmkNumber).arg(precision).arg(zone).arg(is_log).toLocal8Bit());
             else
                 log_file.write(QString("Start sinchro: tmkNum=%1, prec=%2 mc, zone=%3, regim=rt, is_proto=%4\n").arg(tmkNumber).arg(precision).arg(zone).arg(is_log).toLocal8Bit());

         } else
             is_log=false;
    }

    QString log_str;
    long long delta, const_popravka=0;
    QVector<long long> ism_popravka;
    int size_popravka=5;
    unsigned long timer_ev;//apparate timer event
  //  unsigned long timer_sw_ev;//program time event

  //  unsigned long timer_sw;//program timer value
    unsigned long timer_;//apparate timer value
    bool timer_apparate=false;
    if(tmktimer(TIMER_32BIT | TIMER_64US ))timer_apparate=true;
    int ret=0;
   //Для работы с консолью
   // hConsole=GetStdHandle(STD_OUTPUT_HANDLE);
   //SetConsoleCursorPosition(hConsole, pos_cons);
   // SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    while(true){
            count_suc=0;
            count=0;
            delta=0;
            log_str.clear();
            if(is_stop())break;
            diag.clear();
            while((count_suc<BASE_SINHRO)&&(count<(BASE_SINHRO+4))){
                count++;
                timer_=0;timer_ev=0;
                izm_.delta=0;
                izm_.dost=true;
                izm_.datetime=0;
                izm_.cev_dt=0;
                Sleep(0);
                //сбор данных с устройства
                preparation();
                Sleep(0);
                SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
                ret=WaitForSingleObject(hEvent,10);
               // if(timer_apparate)timer_ev=tmkgettimer();
                GetSystemTimeAsFileTime(&ft);//return the number of 100-nanosecond intervals since January 1, 1601 (UTC).
               SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
                if(timer_apparate)//timer_=tmkgettimer();
                {
                    timer_=tmkgettimer();//apparate timer value
                    // timer_sw=tmkgetswtimer();//program timer value
                    if(!regim_ta) timer_ev=mtgetmsgtime();
                    else timer_ev=rtgetmsgtime();
                    //  timer_sw_ev=tmkgetevtime();//program time event
                }

                *((long long *)&ft)-= long(timer_- timer_ev)*640ll;//const_popravka;
                //log_file.write(QString("delta_timer=%1, timer_=%2, timer_ev=%3\n").arg((timer_- timer_ev)*640ll).arg(timer_).arg(timer_ev).toLocal8Bit());

                // SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

                switch(ret)
                {
                    case WAIT_OBJECT_0:
                    if(get_data())
                    {
                            // определяем длину сообщения из командного слова
                            len=data[0] & 31;
                            if (len==6){
                                if(data[5] & 1){
                                    stt.wYear =  ((data[1] & 0xf000)>>12)*10 + ((data[1] & 0x0f00)>>8)+2000;
                                    stt.wMonth = ((data[1] & 0x00f0)>>4 )*10 + (data[1] & 0x000f);
                                    stt.wDay = ((data[2] & 0xf000)>>12)*10 + ((data[2] & 0x0f00)>>8);
                                    stt.wHour = ((data[2] & 0x00f0)>>4 )*10 + (data[2] & 0x000f);// гринвический час
                                    // short hour_msk = ((data[3] & 0xf000)>>12)*10 + ((data[3] & 0x0f00)>>8);// московский час + 3ч
                                    short hour_zone = ((data[3] & 0x00f0)>>4 )*10 + (data[3] & 0x000f);// поясной час + 5ч
                                    stt.wMinute = ((data[4] & 0xf000)>>12)*10 + ((data[4] & 0x0f00)>>8);
                                    stt.wSecond = ((data[4] & 0x00f0)>>4 )*10 + (data[4] & 0x000f);
                                    stt.wMilliseconds =((data[5] & 0xf000)>>12)*100 + ((data[5] & 0x0f00)>>8)*10 + ((data[4] & 0x00f0)>>4);

                                  //  qDebug()<<stt.wYear<<"// "<<stt.wMonth<<"// "<<stt.wDay<<"  ";
                                  //  qDebug()<<stt.wHour<<":"<<stt.wMinute<<":"<<stt.wSecond<<":"<<stt.wMilliseconds;

                                    if(SystemTimeToFileTime( &stt,&ftt)){
                                    stt_cur=stt;
                                    switch(zone){
                                        case 0:   //Гринвич, gmt
                                            izm_.cev_dt=*((qint64*)&ftt);
                                            zone_cur=0;
                                            break;
                                        case 99: //Mocква, gmt+3
                                            izm_.cev_dt=*((qint64*)&ftt)+(qint64)(3*3600*10000000ll);//gmt+3часа
                                            zone_cur=3;
                                            break;
                                        case 100: //Текущий из ЛПИ
                                            if(abs(hour_zone-stt.wHour)<=12){
                                               zone_cur=hour_zone-stt.wHour;
                                            }else{
                                               if (hour_zone>=stt.wHour){
                                                  zone_cur=hour_zone-stt.wHour-24;
                                               }else{
                                                  zone_cur=hour_zone-stt.wHour+24;
                                               }
                                            }
                                            izm_.cev_dt=*((qint64*)&ftt)+(qint64)(zone_cur*3600ll*10000000ll);

                                        break;
                                        default:
                                            if((zone>12)||(zone<-11))
                                                zone=0;
                                            zone_cur=zone;
                                            izm_.cev_dt=*((qint64*)&ftt)+(qint64)(zone_cur*3600ll*10000000ll);
                                            break;
                                    }
                                    diag.append("+");
                                    count_suc++;
                                }else{
                                    izm_.dost=false;
                                    diag.append("f");//diag="Неверный формат времени
                                }
                                }else{
                                    izm_.dost=false;
                                    //qDebug()<<"NO DOST";
                                    diag.append("d"); //printf("er_lpi=NO DOST!\n");//diag="Недостоверная информация!";

                                }
                            }else{
                                izm_.dost=false;                                //printf("er_lpi=LEN<>6 ,%d\n", len);
                                diag.append("f");//diag="Неверный формат, длина пакета не равна 6 словам!";
                            }

                        }else{
                            izm_.dost=false;
                             diag.append("i");
                        }
                        izm_.datetime=*((unsigned long long *)&ft);
                        //izm_.datetime=*((unsigned long long *)&ft+tz_system.Bias*600000000ll);
                        izm.append(izm_);

                    break;
                    case WAIT_TIMEOUT:
                         diag.append("t");//diag="TIMEOUT";//"Нет периема по ЛПИ!";// printf("er_lpi=TIMEOUT\n");
                        izm_.dost=false;
                    break;
                    default:
                        diag.append("e");//diag="WAIT_ERROR"; // printf("er_lpi=WAIT_ERROR\n");
                        izm_.dost=false;
                }

            }//while count

            if(count_suc>=(BASE_SINHRO-BASE_IGNOR)){

             QVector<qint64> delta_izm;
             for(int i=0;i<izm.count();i++){
                if(izm.at(i).dost && izm.at(i).datetime){
                    izm[i].delta=izm[i].datetime - izm[i].cev_dt;
                    delta_izm.append(izm[i].delta);

                }else izm[i].delta=0;
             }

             //выкидываем два самых больших и два самых маленьких значений delta, берем среднее из остальных
            qSort(delta_izm.begin(),delta_izm.end());

             for(int i=0;i<BASE_IGNOR-1;i++)
                if(abs(delta_izm[0])<abs(delta_izm[delta_izm.count()-1]))
                    delta_izm.remove(delta_izm.count()-1);
                else delta_izm.remove(0);
             for(int i=0;i<delta_izm.count()-1;i++) //for(int i=0;i<delta_izm.count()-2*BASE_IGNOR;i++)
                 delta += delta_izm.at(i);
             delta=delta/delta_izm.count();


            log_str="delta="+QString::number(delta/10000)+"\n"+
                     "prec="+QString::number(precision)+"\n"+
                     "zone="+QString::number(zone_cur)+"\n";
             if(abs(delta)>0){//(abs(delta)>PERCIS){
                 DWORD  err=0;
                SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
                    GetSystemTimeAsFileTime(&ft);//return the number of 100-nanosecond intervals since January 1, 1601 (UTC).
                    *((long long *)&ft)-= delta;
                    *((long long *)&ft)-=const_popravka;
                    FileTimeToSystemTime(&ft, &st);
                   // qDebug()<<st.wYear<<"/// "<<st.wMonth<<"/// "<<st.wDay<<"  ";
                   // qDebug()<<st.wHour<<":"<<st.wMinute<<":"<<st.wSecond<<":"<<st.wMilliseconds;
                    if (!SetSystemTime(&st))
                        err = GetLastError();
                SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);


                if(!err){
                    ft_last_sin=*((long long *)&ft);
                    log_str="sinchro suc\n"+log_str;
                   // qDebug()<<systemTime_to_str(stt_cur)+";"+systemTime_to_str(st)+";"+QString::number(delta/10000);
                   // log_file.write((systemTime_to_str(stt_cur)+";"+systemTime_to_str(st)+";"+QString::number(delta/10000)+"\n").toLocal8Bit());

                    log_file.write((QString::number(ft_last_sin/10000)+";"+QString::number(delta/10000)+";+\n").toLocal8Bit());
                }
                else{
                    if (err == ERROR_PRIVILEGE_NOT_HELD){
                        log_str="err_sin=NO_PRVL\n"+log_str;
                        if(is_log){
                            log_file.write(QString("No privileges").toLocal8Bit());
                        }
                    }
                    else{
                        log_str="err_sin="+QString::number(err)+"\n"+log_str;
                        if(is_log){
                            log_file.write(QString("Error N").arg(err).toLocal8Bit());
                        }
                    }
                }
             }
             else {
                 log_str="suc\n"+log_str;
                 //log_file.write((systemTime_to_str(stt_cur)+";"+systemTime_to_str(st)+";"+QString::number(delta/10000)+"\n").toLocal8Bit());
             }

         }else{
             log_str="err_sin=NO_SUC_COUNT "+QString::number(count_suc)+" from "+QString::number(count)+"\n";
             if(is_log){
                 log_file.write(QString("Error count_suc=%1 from %2\n").arg(count_suc).arg(BASE_SINHRO).toLocal8Bit());
             }
        }
        izm.clear();

        log_str.append(diag);
        printf("%s",log_str.toLocal8Bit().data());
        if(is_log){
            log_file.write("+++");
            log_file.write(log_str.toLocal8Bit());
            log_file.write("---");
            log_file.flush();
        }

        fflush(stdout);
        if(size_popravka==ism_popravka.count()){
           qint64 sum=0;
           for(int j=0;j<ism_popravka.count();++j)
               sum+=ism_popravka[j];
           if(abs(const_popravka+sum/ism_popravka.count())<80000)//50000
              const_popravka+=sum/ism_popravka.count();
           log_file.write(QString("popravka=%1 mc\n").arg(const_popravka/10000).toLocal8Bit());
           ism_popravka.clear();
        }
        else {
            //if(abs(delta)<50000)
                   ism_popravka.append(delta);
           // else ism_popravka.clear();
        }
        Sleep(intrv);

   }
   tmkdone(tmkNumber);
   TmkClose();
   if(is_log){
       log_file.write(QString("Stop!").toLocal8Bit());
       log_file.flush();
   }
}
