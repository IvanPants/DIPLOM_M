#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

#include "pult.h"

#include <QStringList>
#include <QSettings>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextCodec>
#include <QMessageBox>
#include <QTime>
#include <QTimer>
#include <QInputDialog>

#include "dmanualpk_icm.h"
#include "dadddevice.h"
#include "dnewustavki.h"
#include "dcheckkadr.h"
#include "dselftest.h"
#include "dconvert.h"
#include "dmanualreceiver.h"
#include "dviewustavki.h"
#include "dmanualloadpzu.h"

#include "modbustcp.h"


#define ADD_NUMBER(X, Y)    (X + QString::number(Y))


typedef struct {

    QString Name;

    bool MAS485_OP;

    int MAS_Number;

    bool MAS_Enable[10];
    QString MAS_name[10];

    bool DMAS2_Enable;
    QString DMAS2_Name;
    bool DMAS6_Enable;
    QString DMAS6_Name;

    bool PZU_Enable;

    int BLK_number;
    bool BLK_Enable[4];
    QString BLK_name[4];

    quint8 KadrType;
    bool KadrInputType;


} DEVICE_CONFIG;


class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT
    
public:

    enum BLK_NUMBER
    {
        BLK1 = 0,
        BLK2,
        BLK3,
        BLK4
    };

    enum BLK_FREQ
    {
        BOFF = 0,
        F500Hz,
        F1000Hz,
        F2000Hz,
        F4000Hz,
        F8000Hz,
        F16000Hz,
        F32000Hz,
        F64000Hz,
        F128000Hz
    };

    enum BLK_TYPE
    {
        BCONST = 0,
        BINC,
        BDEC,
        BSIN,
        BSUM,
        BRIGHT,
        BLEFT
    };

    bool isConnected;

    explicit MainWindow(QWidget *parent = 0);


private:

    const int KADR_TYPE[4] = {2, 1, 1, 1};

    volatile quint8 MBResult;

    ModBusTCP *MBTcp;

    QSettings *MainSettingFile;
    QSettings *DeviceSettingsFile;

    int currentDeviceNum;
    int NumberOfUst;

    QString currentUSTFile;
    DEVICE_CONFIG currentDeviceConfig;

    QTableWidgetItem *newItem;
    int currentLogRow;
    int LogRowForResult;

    void EnableControl();               // Включение контролов после коннекта
    void DisableControl();              // Отключение контролов после дисконнекта

    QLabel *MAS_label[10];
    QLabel *BLK_label[4];
    QTimer *waitLoadTimer;
    
protected:
    void changeEvent(QEvent *e);

    void LoadDevice();       // Загружает список приборов

    int LoadMAS_LKNumber(int ImmAddr, int MASNumber, int LKNumber);

    int LoadLKAddr(int ImmAddr, int LKAddr);             // Загрузка Адреса ЛК
    int LoadLKBuffer(int ImmAddr,  QByteArray LKBuffer);  // Загрузка буфера ЛК
    int LoadLKAdder(int ImmAddr, int Adder);
    int LoadLKConst1(int ImmAddr, int pos, int value);
    int LoadLKConst2(int ImmAddr, int pos, int value);
    int LoadDuration(int ImmAddr, int duration);

    int LoadDMAS2(int ImmAddr, int Start, int Stop, int increment, int delay);
    int LoadDMAS6(int ImmAddr, int Start, int Stop, int increment, int delay);

    int OnOffBLK(bool blk1, bool blk2, bool blk3, bool blk4);
    int selectBLK(quint8 blk_num);
    int addSensorBLK(quint8 freq, quint8 sensor_num, quint8 type, quint8 a, quint8 b, quint8 c);

    void errorLoadUstavki(int MAS, bool dm2, bool dm6, int blk, bool pzu);

    int SelectKadrType(int kadrType, bool InputType);


public slots:


    void sl_Connect();
    void sl_DisConnect();


    void sl_NewDevice();                // Создание нового прибора
    void sl_EditDevice();               // Редактирование прибора

    void sl_newUstavki();               // Создание нового файла уставок
    void sl_EditUstavki();              // Редактирование текущего файла уставок

    void sl_Convert528();               // Конвертирование файлов ПЗУ ТА528 в один файл

    void sl_ManualPK_ICM();
    void sl_ManualImmMAS_485();
    void sl_ManualImmMAS_Opt();
    void sl_ManualImmBLK();
    void sl_ManualImmPZU();
    void sl_ManualReceiver();           // Ручное управление приемником кадров

    void sl_SelfTest();

    void sl_LoadDevice(int device);   // Загружает в программы список уставок выбранного прибора
    void sl_SelectUst(int ust);         // Выбираем новую уставку для дальнейшей работы


    void sl_LoadUst();                  // Загрузка уставок в пульт
    void sl_CheckKadr();                // Проверка кадра
    void sl_ViewUst();                  // Просмотр уставок

    void sl_AddLog(QString text, bool WaitForResult);
    void sl_LogResult(bool result);
    void sl_clearLog();


    void sl_setRegimeMAS(quint16 slave, quint16 regime);
    void sl_LoadSTMASData(quint16 slave, quint16 addr, quint8 data1, quint8 data2, quint8 data3, quint8 data4);
    void sl_LoadIMMMASData(quint16 slave, quint16 addr, quint8 data1, quint8 data2, quint8 data3, quint8 data4);

    void sl_LoadDM2(int start, int stop, int increment, int delay);
    void sl_LoadDM6(int start, int stop, int increment, int delay);

    void sl_ModBusOK();
    void sl_ModBusError(int err);

    void sl_FPGALoadiang();

    void sl_LoadPZU(QByteArray data, quint16 type, quint16 pause, bool interliving);
    void sl_StartPZU(bool status);
    void sl_OnOffPZU(bool status);


signals:

    void sFPGALoadOK();
    void sPZULoadOK();
    void sPZULoadError();

    void sSetProgress(int progress);




};

#endif // MAINWINDOW_H
