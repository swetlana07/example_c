#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>

struct Settings{
    int numTmk;//номер устройства tmk
    int prec;//погрешность синхронизации
    bool is_prot;//ведение протоколов
    bool is_avto;//синхронизация начинается сразу после загрузки программы
    short int zone;//часовой пояс
    bool reg_is_ou;//false- монитор, иначе ответное устройство
};

enum{
    PIX_START=1,
    PIX_NORMA,
    PIX_ER_OPEN,
    PIX_ER_SIN,
    PIX_ER_SIG,
    PIX_ER_DOST

};
class QSystemTrayIcon;
class About_dlg:public QDialog{
    Q_OBJECT
private:
    QListWidget *contents_label;//список кнопок с изображением
    QStackedWidget *pages;//стек виджетов- страниц
public:
    About_dlg(QWidget *pwdg=0);
    QPushButton* bt_cns;
public slots:
    void changePage(QListWidgetItem *current,
                    QListWidgetItem *previous);
};

class Set_dlg : public QDialog{
    Q_OBJECT

   public:
    QComboBox numTmk;
    QComboBox prec;
    QComboBox zone;
    QComboBox reg;
    QCheckBox box_is_proto;
    QCheckBox box_is_avto;
    QPushButton* bt_imp;
    QPushButton* bt_cns;
    Set_dlg(struct Settings &settings, QWidget *pwdg=0);
  signals:
    void set_cans();
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    int result;
protected:
    virtual void closeEvent(QCloseEvent* pe);
private:
    QSystemTrayIcon* icon;
    QMenu *menuIconTray;
    int numIconTray;
    QAction *setAction;
    QAction *exitAction;
    QAction *aboutAction;
    Set_dlg *set_dlg;
    About_dlg *about_dlg;
    QWidget *main_wdg;
    QTimer* timer_err;
    QLabel lb_img,lb_diag,lb_tmk,lb_verbose;
    bool quit_prog;
    QPushButton* bt_pause;
    QPushButton* bt_stop;
    QProcess proc_snh;
    QTimer timer_snh;
    QSharedMemory *shared_mem;
    bool is_proc;
    int proc_exit_code;
    quint64 dt_last;
    int cur_pix;
    QPixmap pix_start, pix_norma, pix_erOpen, pix_erNotSinh, pix_erNotSignal, pix_erNotDost;
    QIcon icon_norma,icon_err, icon_off;
    QString SET_FILE_NAME;
    struct Settings settings;
    int ini_milstd();
    void begin_proc();
    void end_proc();
    void set_pix(int cur);
private slots:
    void pause();
    void cans();
    void look_snh();
    void msg_snh();
    void start_proc();
    void finish_proc(int code, QProcess::ExitStatus);
    void set_wdg();
    void restart_mt();
    void set_close();
    void about();
    void about_close();
    void slotShowHide();
};

#endif // MAINWINDOW_H
