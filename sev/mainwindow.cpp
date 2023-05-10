#include<QDebug>
#include <windows.h>
#include <QSharedMemory>
//#include <C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.19041.0\\um\\ShlObj_core.h>
#include "mainwindow.h"
#include "WDMTMKv2.h"
struct Settings settings_default;

void loadSet(QString setFile, struct Settings &settings){
    settings=settings_default;
    QFile file_set;
    file_set.setFileName(setFile);
    if(file_set.open(QIODevice::ReadOnly | QIODevice::Text)){
         QStringList ba=QString(file_set.readAll()).split("\n");
         bool ok;
         int res;
         foreach(QString str,ba){
             if(str.contains("numTmk=")){
                res=str.remove("numTmk=").toUInt(&ok);
                if(ok)
                    settings.numTmk=res;
             }
             if(str.contains("prec=")){
                res=str.remove("prec=").toUInt(&ok);
                if(ok)
                    settings.prec=res;
             }
             if(str.contains("zone=")){
                res=str.remove("zone=").toUInt(&ok);
                if(ok)
                    settings.zone=res;
             }
             if(str.contains("is_prot=")){
                if((str.remove("is_prot=")=="0")||(str.remove("is_prot=")=="FALSE"))
                    settings.is_prot=false;
                else
                    settings.is_prot=true;
             }
             if(str.contains("is_avto=")){
                if((str.remove("is_avto=")=="0")||(str.remove("is_avto=")=="FALSE"))
                    settings.is_avto=false;
                else
                    settings.is_avto=true;
             }
             if(str.contains("reg=")){
                if((str.remove("reg=")=="0")||(str.remove("is_prot=")=="FALSE"))
                    settings.reg_is_ou=false;
                else
                    settings.reg_is_ou=true;
             }
         }
    }
    return;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    result=0;
    shared_mem= new QSharedMemory("shared_rel");
    if(!shared_mem->create(6,QSharedMemory::ReadWrite)){
        QMessageBox::critical(0,"Внимание","Программа синхронизации уже запущена! "
                                      , QMessageBox::Ok);
            result=1;
            return;

    }
   /* if(!IsUserAnAdmin()){QMessageBox::critical(0,"Внимание","Программа синхронизации должна быть запущена с правами администратора! "
                                               , QMessageBox::Ok);
       result=1;
       return;
   }*/
   setWindowTitle("Синхронизация с 'Гном-2М'");
   settings_default.numTmk=0;//default settings
   settings_default.prec=10;
   settings_default.is_prot=true;
   settings_default.is_avto=true;

   settings_default.reg_is_ou=0;
   settings=settings_default;
   SET_FILE_NAME="settings.txt";
   loadSet(SET_FILE_NAME, settings);
   set_dlg=NULL;
   about_dlg=NULL;
   proc_exit_code=0;
   setAction = new QAction(tr("Настройки"), this);   //    addFilesAction->setIcon(QIcon(":/images/document-open.png"));
   setAction->setShortcut(tr("Ctrl+S"));
   exitAction = new QAction(tr("Выход"), this);   //    exitAction->setIcon(QIcon(":/images/application-exit.png"));
   exitAction->setShortcut(tr("Ctrl+Q"));
   aboutAction = new QAction(tr("О программе"), this);   //    aboutAction->setIcon(QIcon(":/images/help-about.png"));
   aboutAction->setShortcut(tr("Ctrl+A"));   //    connect(addFilesAction, SIGNAL(triggered()), this, SLOT(addFiles()));
   connect(setAction, SIGNAL(triggered()), this, SLOT(set_wdg()));
   connect(exitAction, SIGNAL(triggered()), this, SLOT(cans()));
   connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
   menuBar()->addAction(setAction);
   menuBar()->addAction(aboutAction);
   menuBar()->addAction(exitAction);
   main_wdg=new QWidget();
   if(settings.is_avto)
       bt_pause=new QPushButton("Отключить синхронизацию",this);
   else
       bt_pause=new QPushButton("Включить синхронизацию",this);

   TIME_ZONE_INFORMATION tz;
   GetTimeZoneInformation(&tz);
   if(0!=tz.Bias){
     QMessageBox::critical(0,"Внимание","Часовые пояса из настроек компьютера и программы сложатся.\n Рекомендуется обнулить часовой пояс в настройке компьютера."
                                                             , QMessageBox::Ok);
   }
   QHBoxLayout *hlay=new QHBoxLayout;
   hlay->addWidget(bt_pause);
   QVBoxLayout *vlay=new QVBoxLayout;
   lb_diag.setMaximumHeight(30);
   lb_diag.setMinimumHeight(30);
   lb_tmk.setMaximumHeight(100);
   lb_img.setMaximumHeight(480);
   lb_img.setMinimumHeight(380);
   lb_img.setMaximumWidth(180);
   lb_img.setMinimumWidth(180);
   QAction* showHide= new QAction("&Показать/Скрыть окно", this);
   connect(showHide,SIGNAL(triggered()),this, SLOT(slotShowHide()));
   QAction* quitPrg= new QAction("&Завершить программу", this);
   menuIconTray=new QMenu(this);
   menuIconTray->addAction(showHide);
   menuIconTray->addAction(quitPrg);
   connect(quitPrg,SIGNAL(triggered()),this, SLOT(cans()));
   icon= new QSystemTrayIcon(this);
   icon->setContextMenu(menuIconTray);
   icon->setToolTip("Программа синхронизации");
   numIconTray=-1;

  /* icon_norma=QIcon("img\\normaTray.bmp");
   icon_err=QIcon("img\\errTray.bmp");
   icon_off=QIcon("img\\offTray.bmp");
   pix_start.load("img\\img-start.jpg");
   pix_norma.load("img\\img-norma.jpg");
   pix_erOpen.load("img\\img-erOpen.jpg");
   pix_erNotSinh.load("img\\img-erNotSinh.jpg");
   pix_erNotSignal.load("img\\img-erNotSignal.jpg");
   pix_erNotDost.load("img\\img-erNotDost.jpg");*/
   icon_norma=QIcon(":/img/normaTray.bmp");
   icon_err=QIcon(":/img/errTray.bmp");
   icon_off=QIcon(":/img/offTray.bmp");
   pix_start.load(":/img/img-start.jpg");
   pix_norma.load(":/img/img-norma.jpg");
   pix_erOpen.load(":/img/img-erOpen.jpg");
   pix_erNotSinh.load(":/img/img-erNotSinh.jpg");
   pix_erNotSignal.load(":/img/img-erNotSignal.jpg");
   pix_erNotDost.load(":/img/img-erNotDost.jpg");

   set_pix(PIX_START);
   vlay->addWidget(&lb_diag);
   vlay->addWidget(&lb_tmk);
   vlay->addWidget(&lb_verbose);
   vlay->addStretch(1);
   vlay->addLayout(hlay);
   connect(bt_pause, SIGNAL(clicked()),this,SLOT(pause()));
   connect(&timer_snh,SIGNAL(timeout()),this, SLOT(look_snh()));
   QHBoxLayout *hlay_main=new QHBoxLayout;
   hlay_main->addWidget(&lb_img,0,Qt::AlignAbsolute);
   hlay_main->addLayout(vlay);
   main_wdg->setLayout(hlay_main);
   setCentralWidget(main_wdg);
   this->setCursor(Qt::ArrowCursor);
   is_proc=false;
   connect(&proc_snh,SIGNAL(readyRead()), this,SLOT(msg_snh()));
   connect(&proc_snh, SIGNAL(started()), this, SLOT(start_proc()));
   connect(&proc_snh, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finish_proc(int, QProcess::ExitStatus)));
   if(bt_pause->text().contains("Отключить"))
      begin_proc();
   else
      end_proc();
  dt_last=QDateTime::currentMSecsSinceEpoch();
  timer_snh.start(1600);
  this->setMinimumWidth(430);
  quit_prog=false;
  icon->show();
}
void MainWindow::closeEvent(QCloseEvent *pe){
    if(quit_prog)pe->accept();
    else if(icon->isVisible()){
        hide();
        pe->ignore();
    }
}
void MainWindow::slotShowHide(){
    setVisible(!isVisible());
}
void MainWindow::set_pix(int cur){
    if(cur!=cur_pix){
        lb_img.clear();
        switch(cur){
        case PIX_START: lb_img.setPixmap(pix_start.scaled(QSize(180,400),Qt::IgnoreAspectRatio));
            if(numIconTray){
                 qDebug()<<"ok!!-2";
                 icon->setIcon(QIcon(icon_off));
                 qDebug()<<"ok!!-3";
                icon->showMessage("Сообщение от программы синхронизации","Синхронизация отключена!",
                                  QSystemTrayIcon::Information,3000);
                numIconTray=0;
            }
            break;
        case PIX_NORMA: lb_diag.clear();
            lb_img.setPixmap(pix_norma.scaled(QSize(180,400),Qt::IgnoreAspectRatio));
            if(numIconTray!=1){
                icon->setIcon(QIcon(icon_norma));
                icon->showMessage("Сообщение от программы синхронизации","Синхронизация - норма!",
                                  QSystemTrayIcon::Information,3000);
                numIconTray=1;
            }
            break;
        case PIX_ER_OPEN: lb_img.setPixmap(pix_erOpen.scaled(QSize(180,400),Qt::IgnoreAspectRatio));
            if(numIconTray!=2){
                icon->setIcon(QIcon(icon_err));
                icon->showMessage("Сообщение от программы синхронизации","Процесс синхронизации не запускается!",
                                  QSystemTrayIcon::Information,3000);
                numIconTray=2;
            }
            break;
        case PIX_ER_SIN: lb_img.setPixmap(pix_erNotSinh.scaled(QSize(180,400),Qt::IgnoreAspectRatio));
            if(numIconTray!=3){
                icon->setIcon(QIcon(icon_err));
                icon->showMessage("Сообщение от программы синхронизации","Ошибка в процессе синхронизации!",
                                  QSystemTrayIcon::Information,3000);
                numIconTray=3;
            }
            break;
        case PIX_ER_SIG: lb_img.setPixmap(pix_erNotSignal.scaled(QSize(180,400),Qt::IgnoreAspectRatio));
            if(numIconTray!=4){
                icon->setIcon(QIcon(icon_err));
                icon->showMessage("Сообщение от программы синхронизации","Ошибка формата пакетов при синхронизации!",
                                  QSystemTrayIcon::Information,3000);
                numIconTray=4;
            }
            break;
        case PIX_ER_DOST: lb_img.setPixmap(pix_erNotDost.scaled(QSize(180,400),Qt::IgnoreAspectRatio));
            if(numIconTray!=5){
                icon->setIcon(QIcon(icon_err));
                icon->showMessage("Сообщение от программы синхронизации","Нет достоверности при синхронизации!",
                                  QSystemTrayIcon::Information,3000);
                numIconTray=5;
            }
            break;
        default:
            lb_img.setPixmap(QPixmap());
            if(numIconTray!=6){
                icon->setIcon(QIcon(icon_err));
                icon->showMessage("Сообщение от программы синхронизации","Неизвестная ошибка!",
                                  QSystemTrayIcon::Information,3000);
                numIconTray=6;
            }
        }
        cur_pix=cur;
    }
}

void MainWindow::begin_proc(){
    shared_mem->lock();
    char *to=(char*)shared_mem->data();
    const char* from="+++";
    memcpy(to,from,4);
    shared_mem->unlock();
    QStringList list;
    list.append(QString::number(settings.numTmk));
    list.append(QString::number(settings.prec));
    list.append(QString::number(settings.zone));
    list.append(QString::number(settings.reg_is_ou));
    list.append(QString::number(settings.is_prot));
    proc_snh.start("sev_mt",list);
}

void MainWindow::end_proc(){
    shared_mem->lock();
    strncpy((char*)shared_mem->data(),"stop",4);
    shared_mem->unlock();
    lb_tmk.clear();
}

void MainWindow::start_proc(){
    is_proc=true;
}

void MainWindow::finish_proc(int code, QProcess::ExitStatus){
    is_proc=false;
    proc_exit_code=code;
}

void MainWindow::msg_snh(){
     dt_last=QDateTime::currentMSecsSinceEpoch();
     QStringList res=QString(proc_snh.readAll()).split("\n");
     if(!res.count()){
        return;
     }
     lb_verbose.setText("");
     if(res.at(0).contains("suc")){
         QString rs;
         rs="<h4><FONT COLOR=#229966>Синхронизация - норма<FONT COLOR=#229966></h4>";
         foreach(QString st,res){
             if(st.contains("delta=")){
                QString s=st.replace("delta=","");
                s=s.replace(" ","");
                if(!s.contains("-"))
                    s="&nbsp;"+s;
                rs+= " - текущая погрешность="+s+" мс";
            }
            if(st.contains("prec="))
                rs+= "<br> - заданная точность="+ st.replace("prec=","")+" мс";
             if(st.contains("zone="))
                rs+= "<br> - часовой пояс="+ st.replace("zone=","");
         }
         set_pix(PIX_NORMA);
         qDebug()<<"rs="<<rs;
         lb_tmk.setText(rs);
     }else if(res.at(0).contains("NO_PRVL")){
         set_pix(PIX_ER_SIN);
         lb_tmk.setText("<FONT COLOR=#992222>Нет прав для синхронизации!");
         lb_verbose.setText("Необходимо перезапустить <br>программу с правами <br>администратора.");
     }else if(res.at(0).contains("NO_SUC_COUNT")){
         QString rs;
         rs="Сообщения при приеме пакетов:\n";
         if(res.count()>1){
             QByteArray ba=res.at(1).toLocal8Bit();
             int i0=0;
             if((ba.length()-18)>0)
                 i0=ba.length()-18;
             for(int i=i0;i<ba.length();i++){
                switch(ba.data()[i]){
                    case '+':rs.append(QString::number(i-i0+1)+" - норма;\n");break;
                    case 'd':rs.append(QString::number(i-i0+1)+" - сообщение недостоверно;\n");break;
                    case 'f':rs.append(QString::number(i-i0+1)+" - неверный формат сообщения;\n");break;
                    case 'i':rs.append(QString::number(i-i0+1)+" - ошибка ожидания прерывания;\n");break;
                    case 't':rs.append(QString::number(i-i0+1)+" - нет собщений в ЛПИ;\n");break;
                    case 'e':rs.append(QString::number(i-i0+1)+" - неизвестная ошибка;\n");break;
                    default:rs.append(QString::number(i-i0+1)+" - недокументированная ошибка;\n");
                }

             }
            lb_verbose.setText(rs);
            ba.replace("+","");
            if(rs.contains("недостоверно")){set_pix(PIX_ER_DOST); rs="Информация СЕВ недостоверна.";}
            else if(rs.contains("неверный формат")){set_pix(PIX_ER_DOST);rs="Информация СЕВ не соответствует <br> протоколу.";}
            else if(rs.contains("прерывания")){set_pix(PIX_ER_SIN);rs="Ошибка прерывания.";}
            else if(rs.contains("неизвестная")){set_pix(PIX_ER_SIN);rs="Ошибка ожидания прерывания.";}
            else if(rs.contains("нет собщений")){set_pix(PIX_ER_SIG);rs="Нет передачи пакетов по ЛПИ.";}
            else {set_pix(PIX_ER_SIN);rs="Неизвестная ошибка.";}
          /*
            switch(ba.at(ba.length()-1)){
                case 'd':set_pix(PIX_ER_DOST); rs="Информация СЕВ недостоверна.";break;
                case 'f':set_pix(PIX_ER_DOST);rs="Информация СЕВ не соответствует <br> протоколу.";break;
                case 'i':set_pix(PIX_ER_SIN);rs="Ошибка прерывания.";break;
                case 't':set_pix(PIX_ER_SIG);rs="Нет передачи пакетов по ЛПИ.";break;
                case 'e':set_pix(PIX_ER_SIN);rs="Ошибка ожидания прерывания.";break;
                default:set_pix(PIX_ER_SIN);rs="Неизвестная ошибка.";
            }*/
            lb_tmk.setText(rs);
         }
         else{
            set_pix(PIX_ER_SIN);
            lb_tmk.setText("<FONT COLOR=#992222>Неизвестная ошибка.");
        }


     }else if(res.at(0).contains("err_sin=")){
         set_pix(PIX_ER_SIN);
         QString rs=res.at(0);
         lb_tmk.setText("<FONT COLOR=#992222>Ошибка "+rs.remove("err_sin=")+"!");
     }else{

     }

}
void MainWindow::look_snh(){
    if(is_proc && (bt_pause->text().contains("Отключить"))&&((QDateTime::currentMSecsSinceEpoch()-dt_last)>5000)){
        set_pix(PIX_ER_OPEN);
        lb_diag.setText("<FONT COLOR=#992222>Не отвечает процесс синхронизации!");
        end_proc();
        return;
    }
    if(!is_proc && (bt_pause->text().contains("Отключить"))){
         begin_proc();
         set_pix(PIX_ER_OPEN);
         lb_diag.setText("<FONT COLOR=#992222>Не запущен процесс синхронизации!");
         lb_tmk.clear();
         QString rs;
         switch(proc_exit_code){
         case -1:rs="<div>Проверьте подключение TA1- USB.</div>";break;
         case -2:rs="<div>Устройство занято.</div><div>Завершите процесс, занявший </div><div>устройство.</div>";break;
         case -3:rs="<div>Ошибка создания события.</div><div>Перезагрузите компьютер </div><div>и перезапустите программу.</div>";break;
         case -4:rs="<div>Не поддерживается режим ОУ.</div><div>Зайдите в настройки </div><div>и выберите режим монитора.</div>";break;
         case -5:rs="<div>Не поддерживается работа с прерываниями в режиме ОУ.</div><div>Зайдите в настройки и выберите </div><div>режим монитора.</div>";break;
         case -6:rs="<div>Не поддерживается режим МТ.</div><div>Зайдите в настройки и выберите режим ОУ</div>";
         }
         lb_verbose.setText(rs);
         return;
    }
    if(is_proc && (bt_pause->text().contains("Включить"))){
        end_proc();
        lb_diag.setText("<FONT COLOR=#992222>Не остановлен процесс синхронизации!"
                        "<div><FONT COLOR=#000000>Отключите модуль TA-USB от компьютера.</div>");
        return;
   }
   if(lb_diag.text().contains("Не запущен процесс синхронизации")
       ||lb_diag.text().contains("Не остановлен процесс синхронизации"))
       lb_diag.setText("");
   if(is_proc && bt_pause->text().contains("Отключить")){
       lb_diag.setText("СИНХРОНИЗАЦИЯ ВКЛЮЧЕНА");
       if(TmkOpen()){
           end_proc();
           lb_tmk.clear();
           lb_verbose.clear();

       }else TmkClose();
   }
   return;
}

void MainWindow::cans(){
    shared_mem->lock();
    strncpy((char*)shared_mem->data(),"stop",4);
    shared_mem->unlock();
    shared_mem->detach();
    if(!proc_snh.waitForFinished(3100) && is_proc)
        lb_diag.setText("<FONT COLOR=#992222><div>Не остановлен процесс синхронизации.</div>"
                        "<div><FONT COLOR=#000000>Отключите модуль TA-USB от компьютера.</div>");
    timer_snh.stop();
    quit_prog=true;
    this->close();
}

void MainWindow::pause(){
    if(bt_pause->text().contains("Отключить")){
        bt_pause->setEnabled(false);
        end_proc();
        this->setCursor(Qt::WaitCursor);
        if(!proc_snh.waitForFinished(3500)&& is_proc)
            lb_diag.setText("<FONT COLOR=#992222><div>Не остановлен процесс синхронизации.</div>"
                            "<div><FONT COLOR=#000000>Отключите модуль TA-USB от компьютера.</div>");
        else{
            set_pix(PIX_START);
            lb_verbose.clear();
            lb_tmk.clear();
            lb_diag.setText("CИНХРОНИЗАЦИЯ ОТКЛЮЧЕНА.");
        }
        bt_pause->setText("Включить синхронизацию");
        bt_pause->setEnabled(true);
        this->setCursor(Qt::ArrowCursor);
    }else{
        bt_pause->setEnabled(false);
        this->setCursor(Qt::WaitCursor);
        begin_proc();
        if(!proc_snh.waitForStarted() && !is_proc)lb_diag.setText("Не запущен процесс синхронизации.");
        bt_pause->setText("Отключить синхронизацию");
        bt_pause->setEnabled(true);
        this->setCursor(Qt::ArrowCursor);
    }
}
void MainWindow::set_wdg(){
    if(set_dlg){
        set_close();
        return;
    }
    set_dlg=new Set_dlg(settings,this);
    connect(set_dlg->bt_imp, SIGNAL(clicked()), this, SLOT(restart_mt()));
    connect(set_dlg->bt_cns, SIGNAL(clicked()), this, SLOT(set_close()));
    set_dlg->show();
}
void MainWindow::restart_mt(){
    if(!set_dlg)return;
    int res; bool ok;
    QString str_wr;
    res=set_dlg->numTmk.currentText().toInt(&ok);
    if(ok)
        settings.numTmk=res;
    else
        settings.numTmk=settings_default.numTmk;
    str_wr+="numTmk="+QString::number(settings.numTmk)+"\n";

    res=set_dlg->prec.currentText().toInt(&ok);
    if(ok)
        settings.prec=res;
    else
        settings.prec=settings_default.prec;
    str_wr+="prec="+QString::number(settings.prec)+"\n";
    if(set_dlg->zone.currentText()=="CEB"){
        res=100;
        ok=true;
    }else{
        int pos= set_dlg->zone.currentText().indexOf(" ");
        if(pos>0)
           res=set_dlg->zone.currentText().mid(0,pos).toInt(&ok);
        else
         res=set_dlg->zone.currentText().toInt(&ok);
    }
    if(ok)
        settings.zone=res;
    else
        settings.zone=settings_default.zone;
    str_wr+="zone="+QString::number(settings.zone)+"\n";

    if(set_dlg->reg.currentText().contains("Монитор"))
        settings.reg_is_ou=false;
    else
        settings.reg_is_ou=true;
    str_wr+="reg="+QString::number(settings.reg_is_ou)+"\n";

    if(set_dlg->box_is_proto.isChecked()){
        settings.is_prot=true;
        str_wr+="is_prot=1\n";
    }else{
        settings.is_prot=false;
        str_wr+="is_prot=0\n";
    }
    if(set_dlg->box_is_avto.isChecked()){
        settings.is_avto=true;
        str_wr+="is_avto=1\n";
    }else{
        settings.is_avto=false;
        str_wr+="is_avto=0\n";
    }
    qDebug()<<str_wr;
    QFile settingFile(SET_FILE_NAME);
    if(settingFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)){
        settingFile.write(str_wr.toLocal8Bit());
        settingFile.flush();
        settingFile.close();
    }
    set_close();
    if( bt_pause->text().contains("Отключить")){
        end_proc();
        Sleep(1000);
        begin_proc();
    }
}
MainWindow::~MainWindow(){}

Set_dlg::Set_dlg(struct Settings &settings, QWidget *pwdg):QDialog(pwdg,Qt::WindowTitleHint | Qt::WindowSystemMenuHint){
    this->setWindowTitle("Настройки");
    QStringList list;
    if(TmkOpen()){
        list<<QString::number(settings.numTmk);
        numTmk.setCurrentIndex(0);
    } else{
        for(int i=0;i<tmkgetmaxn();i++)
            list<<QString::number(i);
        if(!list.count())
            list<<"0";
        TmkClose();
    }
    numTmk.addItems(list);
    int pos=list.indexOf(QString::number(settings.numTmk));
    if(pos!=-1)
        numTmk.setCurrentIndex(pos);
    else{
        numTmk.addItem(QString::number(pos));
        numTmk.setCurrentIndex(numTmk.count()-1);
    }
    numTmk.setEditable(false);
    list.clear();
    list<<"10"<<"25"<<"50"<<"75"<<"100";
    prec.addItems(list);
    pos=list.indexOf(QString::number(settings.prec));
    if(pos!=-1)
        prec.setCurrentIndex(pos);
    else{
        bool ok;
        if(settings.prec>100){
            settings.prec=100;
            prec.setCurrentIndex(prec.count()-1);
        }
        else{
            prec.setCurrentIndex(0);
            for(int i=0;i<list.count();i++){
                int res= list.at(i).toInt(&ok);
                if(settings.prec<=res && ok){
                     settings.prec=res;
                     prec.setCurrentIndex(i);
                     break;
                 }
           }
        }
    }
    prec.setEditable(false);
    list.clear();
    list<<"0 Гринвич"<<"+1"<<"+2"<<"+3 Москва"<<"+4"<<"+5 Миасс"<<"+6"<<"+7"
            <<"+8"<<"+9"<<"+10"<<"+11"<<"+12"
            <<"-1"<<"-2"<<"-3"<<"-4"<<"-5"<<"-6"<<"-7"
            <<"-8"<<"-9"<<"-10"<<"-11"<<"CEB";
    zone.addItems(list);
    zone.setCurrentIndex(0);
    if(settings.zone<0){
        for(int i=0;i<zone.count();i++)
            if(QString::number(settings.zone)==zone.itemText(i)){
                zone.setCurrentIndex(i);
                break;
            }
    }
    else{
        QString str_zn;
        if(settings.zone>0){
            if(settings.zone==100){
                str_zn="CEB";
                zone.setCurrentIndex(zone.count()-1);
            }else str_zn="+"+QString::number(settings.zone);
        }else
            str_zn=QString::number(settings.zone);
        if(settings.zone!=100)
         for(int i=0;i<zone.count()/2+1;i++){
            if(str_zn==zone.itemText(i).mid(0,str_zn.length())){
                zone.setCurrentIndex(i);
                break;
            }
        }
    }
    zone.setEditable(false);
    list.clear();
    list<<"Монитор"<<"Ответное устройство";
    reg.addItems(list);
    if(settings.reg_is_ou)
        reg.setCurrentIndex(1);
    else
        reg.setCurrentIndex(0);
    reg.setEditable(false);
    list.clear();
    if(settings.is_prot)
        box_is_proto.setChecked(true);
    else
        box_is_proto.setChecked(false);
    if(settings.is_avto)
        box_is_avto.setChecked(true);
    else
        box_is_avto.setChecked(false);
    QLabel* lb_numTmk= new QLabel("Номер устройства:");
    QLabel* lb_prec= new QLabel("Точность синхронизации, мс:");
    QLabel* lb_zone= new QLabel("Часовой пояс:");
    QLabel* lb_reg= new QLabel("Режим работы устройства:");
    QLabel* lb_proto= new QLabel("Ведение журнала:");
    QLabel* lb_avto= new QLabel("Автозапуск синхронизации:");
    bt_imp= new QPushButton("Применить");
    bt_cns= new QPushButton("Выход");
    QGridLayout* lay=new QGridLayout;
    lay->addWidget(lb_numTmk,0,0);
    lay->addWidget(lb_prec,1,0);
    lay->addWidget(lb_zone,2,0);
    lay->addWidget(lb_reg,3,0);
    lay->addWidget(lb_proto,4,0);
    lay->addWidget(lb_avto,5,0);
    lay->addWidget(bt_imp,6,0);

    lay->addWidget(&numTmk,0,1);
    lay->addWidget(&prec,1,1);
    lay->addWidget(&zone,2,1);
    lay->addWidget(&reg,3,1);
    lay->addWidget(&box_is_proto,4,1);
    lay->addWidget(&box_is_avto,5,1);
    lay->addWidget(bt_cns,6,1);
    setLayout(lay);
    connect(bt_cns, SIGNAL(clicked()), this,SIGNAL(set_cans()));
}

void MainWindow::about(){
    if(about_dlg){
        about_close();
        return;
    }
    about_dlg=new About_dlg(this);
    connect(about_dlg->bt_cns, SIGNAL(clicked()), this,SLOT(about_close()));
    about_dlg->show();
    about_dlg->move((QApplication::desktop()->width()-about_dlg->width())/2,(QApplication::desktop()->height()-about_dlg->height())/2);
}

About_dlg::About_dlg(QWidget(*pwdg)):QDialog(pwdg,Qt::WindowTitleHint | Qt::WindowSystemMenuHint){
    this->setWindowTitle("О программе");
    contents_label = new QListWidget(this);
    contents_label->setViewMode(QListView::ListMode);
    contents_label->setMaximumWidth(250);
    contents_label->setMinimumWidth(230);
    contents_label->setSpacing(6);
    QListWidgetItem *programm = new QListWidgetItem(contents_label);
    programm->setTextColor(QColor(250,250,250));
    programm->setText(tr("Назначение и характеристики"));
    programm->setTextAlignment(Qt::AlignLeft);
    programm->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    QListWidgetItem *reabily  = new QListWidgetItem(contents_label);
    reabily->setTextColor(QColor(250,250,250));
    reabily->setText(tr("Настройки программы"));
    reabily->setTextAlignment(Qt::AlignLeft);
    reabily->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    QListWidgetItem *info = new QListWidgetItem(contents_label);
    info->setTextColor(QColor(250,250,250));
    info->setText(tr("Устранение неисправностей"));
    info->setTextAlignment(Qt::AlignLeft);
    info->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    contents_label->setCurrentRow(0);
    contents_label->setStyleSheet("QListWidget {background-color: white; background-image: url(./img/img-about.jpg);background-repeat: no-repeat;background-size: 100%;}");


    pages=new QStackedWidget(this);
   // pages->setMaximumWidth(380);
    pages->setMinimumWidth(400);
    QLabel *lb_work = new QLabel(this);
    lb_work->setMargin(10);
    lb_work->setFrameStyle(QFrame::Box);
    lb_work->setText("<p><font size=4><div>Эта программа предназначена для синхронизации IBM PC/AT совместимого компьютера"
                     "<div>с шиной USB под управлением ОС 'Windows 7' с системой единого времени 'Гном-2М'</div>"
                     "<div>по интерфейсу ГОСТ P 52070-2003 c заданной точностью.</div><div> Подключение к линии передачи информации (ЛПИ) осуществляется</div>"
                     "<div>посредством модуля сопряжения TA-USB производства АО 'ЭК Элкус'.</div></p>"
                     "<p><font size=4><div>Сигналы точного времени поступают в ЛПИ с циклической частотой 1кГц </div>"
                     "<div>в соответствии с Протоколом информационно-технического взаимодействия </div>"
                     "<div>с СЕВ и ЭЧ 'Гном-2М' по последовательному коду времени (проект 09852).</div>"
                     "<div>Частота синхронизации 2Гц,максимальная точность синхронизации 10 мс,</div></p>"
                     "<p><font size=4><div>В программе реализована возможность вести журнал, содержащий результаты работы.</div>"
                     "<div>Файл журнала создается в каталоге программы, имя файла имеет формат:</div>"
                     "<div>logsev_ггггммдд-ччммсс, где</div> "
                     "<div>    ггггммдд- дата старта синхронизации (гггг-год, мм-месяц, дд-день),</div> "
                     "<div>    ччммсс- время старта синхронизации (чч-часы, мм-минуты, сс-секунды).</div></p> "
                     "<p><font size=4><div>Программа отображает текущее состояние процесса синхронизации.</div> "
                     "<div>При возникновении неисправности выдаетcя соответствующее предупреждение.</div></p> "
                     "<h3><font color=#109030 >Внимание! Программа должна запускаться с правами администратора.</h3>"
                     );

    pages->addWidget(lb_work);

    QLabel *lb_reab = new QLabel(this);
    lb_reab->setAlignment(Qt::AlignTop);
    lb_reab->setMargin(10);
    lb_reab->setFrameStyle(QFrame::Box);
    lb_reab->setText("<p><font size=4><div>Для просмотра или изменения настроек программы следует</div>"
                     "<div> выбрать пункт главного меню  'Настройки'.</div></p>"
                     "<p><font size=4><div>В окне 'Настройки' можно задать: <div>-  номер устройства TA-USB, "
                     "если в компьютере установлено несколько модулей TA,</div>"
                     "<div>-  точность синхронизации (в интервале 10 - 100 мс),</div>"
                     "<div>-  часовой пояс (от -11 до +12),</div>"
                     "<div>-  режим работы TA-USB (монитор или ответное устройство),</div>"
                     "<div>-  ведение журнала, </div> "
                     "<div>-  автозапуск синхронизации при старте программы.</div> "
                     "<p><font size=4><div>Чтобы запустить программу с измененными настройками,следует </div> "
                     "<div> нажать кнопку 'Применить', настройки автоматически сохраняются <br>при следующем запуске программы.</div></p> "
                     "<h3><font color=#109030 >Внимание! Если программа запускается с журналированием, оператору <br>необходимо"
                     " самостоятельно удалять ненужные файлы журналов.</h3>"
                     );
    pages->addWidget(lb_reab);

    QLabel *lb_prt = new QLabel(this);
    lb_prt->setMargin(10);
    lb_prt->setFrameStyle(QFrame::Box);
    lb_prt->setAlignment(Qt::AlignTop);
    lb_prt->setText("<p><font size=4><div>Неисправность при старте программы возникает при отключенном от компьютера</div>"
                     "<div>модуле TA-USB, при этом на экран выдается соответствующее сообщение.</div>"
                     "<div>В этом случае проверьте подключение модуля к компьютеру и наличие </div>"
                     "<div>на компьютере драйвера модуля TA-USB C:\\Windows\\System32\\drivers\\ezusb.sys.</div></p>"
                     "<p><font size=4><div>Неисправность в процессе работы программы может возникнуть:</div>"
                     "<div>-  в изделии СЕВ 'Гном-2М'. В этом случае транспарант СЕВ 'Гном-2М'</div>"
                     "<div>   окрашен в красный цвет,</div>"
                     "<div>-  в изделии 'Суфлер-К'. В этом случае транспарант 'Суфлер-К'</div>"
                     "<div>   окрашен в красный цвет,</div>"
                     "<div>-  в ЛПИ. В этом случае информация от СЕВ не поступает. </div>"
                     "<p><font size=4><div>При возникновении неисправности программа выдает поясняющее сообщение. </div>"
                     "<div>Устраните неисправность, следуя рекомендации. </div></p>"
                     );
    pages->addWidget(lb_prt);
    connect(contents_label, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));
    QHBoxLayout *hLay = new QHBoxLayout;
    hLay->addWidget(contents_label,1);
    hLay->addWidget(pages,2);
    QVBoxLayout* lay=new QVBoxLayout;
    bt_cns=new QPushButton("Выход");
    bt_cns->setMaximumWidth(200);
    lay->addLayout(hLay);
    lay->addWidget(bt_cns,0,Qt::AlignHCenter);
    setLayout(lay);
}

void About_dlg::changePage(QListWidgetItem *current,
                QListWidgetItem *previous) {
        if (!current)
                current = previous;
        pages->setCurrentIndex(contents_label->row(current));
}

void MainWindow::about_close(){
    about_dlg->close();
    delete about_dlg;
    about_dlg=NULL;
}
void MainWindow::set_close(){
    set_dlg->close();
    delete set_dlg;
    set_dlg=NULL;
}
