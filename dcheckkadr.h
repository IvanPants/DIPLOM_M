#ifndef DCHECKKADR_H
#define DCHECKKADR_H

#include "ui_dcheckkadr.h"

#include <QUdpSocket>
#include <QFile>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QSettings>
#include <QButtonGroup>
#include <QTimer>
#include <QMessageBox>
#include <QTextBrowser>
#include <QStringList>
#include <QtEndian>


#include <JlCompress.h>

#include "mas_data.h"
#include "fwritekadr.h"
#include "dviewerror.h"
#include "ds_ridsolomon.h"

#include "pult.h"
#include "kadr_def.h"

#define DELAY_MIN   2600
#define DELAY_MAX   3200

#define CLR_ERROR() KadrCheckNumber = 0; \
                    MMPError = 0;   \
                    SSKError = 0;   \
                    TimeError = 0;  \
                    M1Error = 0;    \
                    M2Error = 0; \
                    BMPError = 0;\
                    DelayError = 0; \
                    CRCError = 0;\
                    RSError = 0;\
                    AllError = 0

#define ADD_MMP_ERROR() MMPError++;     \
                        AllError++

#define ADD_BMP_ERROR() BMPError++;     \
                        AllError++

#define ADD_SSK_ERROR() SSKError++; \
                        AllError++

#define ADD_TIME_ERROR() TimeError++;   \
                        AllError++

#define ADD_M1_ERROR() M1Error++;   \
                        AllError++


#define ADD_M2_ERROR() M2Error++;   \
                        AllError++

#define ADD_DELAY_ERROR() DelayError++; \
                          AllError++

#define ADD_CRC_ERROR() CRCError++;  \
                        AllError++

#define ADD_RS_ERROR()  RSError++;\
                        AllError++;

#define GET_KADR_COUNTER(X, Y) X = ((Y.counter << 24) & 0xFF000000) | \
                                   ((Y.counter << 8) & 0x00FF0000) | \
                                   ((Y.counter >> 8) & 0x0000FF00) | \
                                   ((Y.counter >> 24) & 0x000000FF)




#define GET_SSK(X, Y) switch(KadrType) {      \
                    case KADR_RTSC: X = Y.KD.R_PACKAGE.KADR[RSSK_POSITION]; break; \
                    case KADR_RTSCM: X = Y.KD.M_PACKAGE.SSK; break; \
                    case KADR_RTSCM1: X = Y.KD.M_PACKAGE.SSK; break; \
                    case KADR_VAAR: X = Y.KD.V_PACKAGE.pCount & 0x3F; break;}

#define GET_SEK(X, Y) switch(KadrType) {      \
                        case KADR_RTSC: X = Y.KD.R_PACKAGE.KADR[RSEK_POSITION]; break; \
                        case KADR_RTSCM: X = Y.KD.M_PACKAGE.SEK; break; \
                        case KADR_RTSCM1: X = Y.KD.M_PACKAGE.SEK; break; \
                        case KADR_VAAR: X = Y.KD.V_PACKAGE.SEK; break; }

#define GET_MIN(X, Y) switch(KadrType) {      \
                        case KADR_RTSC: X = Y.KD.R_PACKAGE.KADR[RMIN_POSITION]; break; \
                        case KADR_RTSCM: X = Y.KD.M_PACKAGE.MIN; break; \
                        case KADR_RTSCM1: X = Y.KD.M_PACKAGE.MIN; break; \
                        case KADR_VAAR: X = Y.KD.V_PACKAGE.MIN; break;}
#define GET_KSS(X, Y) switch(KadrType) {      \
                        case KADR_RTSC: X = Y.KD.R_PACKAGE.KADR[RKSS_POSITION]; break; \
                        case KADR_RTSCM: X = Y.KD.M_PACKAGE.KSS; break; \
                        case KADR_RTSCM1: X = Y.KD.M_PACKAGE.KSS; break; \
                        case KADR_VAAR: break;}




class dCheckKadr : public QDialog, private Ui::dCheckKadr
{
    Q_OBJECT

public:
    explicit dCheckKadr(QWidget *parent = 0);

    void UpdateEtalonRTSCM();   // Формирование эталонного кадра РТСЦ-М
    void UpdateEtalonRTSC();    // Формирование эталонного кадра РТСЦ
    void UpdateEtalonRTSCM1();  // Формирование эталонного кадра РТСЦ-М1
    void UpdateEtalonVAAR();    // Формирование эталонного кадра ВААР

    void LoadUstavkiFile(QString ustFileName);


private:

    QTextStream error_stream;


    const quint64 blk_package_time[10] = {0,
                                          1024,
                                          512,
                                          256,
                                          128,
                                          64,
                                          32,
                                          16,
                                          8,
                                          4};

    void UpdateErrorData();
    void ViewError();


    void setRsvChannel(int num);

    quint16 crc16(quint8 *buff, quint16 len);        // Подсчет CRC16


// ==========================================================================

    ds_RidSolomon RSObject;


    quint8 getBLKData(quint8 blk_num, quint8 sensor_num);
    bool CheckBLKPackageCounter(quint8 blk_num, quint8 sensor_num, quint8 curr_counter, quint64 packageTime);

    quint16 calculatePZUPos(quint16 currPos, bool isPS1, quint8 currentInput);


    QButtonGroup *InputButtonGroup;

    QTimer *KadrReadTimer;

    QUdpSocket *send_socket;
    QUdpSocket *recv_socket;

    QFile *CheckKadrFile;
    QSettings *UstavkiFile;

    QTemporaryFile *kadrTmpFile;

    QString DevFileName;
    QString UstFileName;


    bool DualPS1[8];
    bool DualPS2[8];

    bool ZapretPS1[8];
    bool ZapretPS2[8];

    PZUData PRG_PS1;
    PZUData PRG_PS2;

    quint16 PRG_VAAR[1024];                 // Содержание ПЗУ ВААР

    quint8 KadrType;                        // Тип проверяемого файла
    quint8 Potok[4];                        // Номера потоков по входам. 0 - отключен
    quint8 currentPotokType;                // Тип текущего потока

    MAS_Data *mas_input[13];

    quint8 ImmKadr_PS1[128][512];           // Расчетный кадр имитатора ММП [ССК][Позиция] для ПС1
    quint8 ImmKadr_PS2[128][512];           // Расчетный кадр имитатора ММП [ССК][Позиция] для ПС2

    LK_PARAM lk_param[13][4];           // Параметры ЛК для расчета кадра
    ERROR_PARAM er_param_PS1[128][512];     // Параметры для формирования ошибки проверки для ПС1
    ERROR_PARAM er_param_PS2[128][512];     // Параметры для формирования ошибки проверки для ПС2

    BLK_PARAM BLK_param[4][24];             // Параметры для проверки БЛК
    BLK_ERROR BLK_Error[4][24];

    int mas_input_array[13];                // Соответствие имитаторов ко входа МАС
    int mas_input_convert[8];

    // Переменный для проверки

        quint8 old_ssk;                 // Предыдущее значение ССК
        quint8 old_sec;                 // Предыдущее значение секунд в кадре
        quint8 old_min;                 // Предыдущее значение минут в кадре
        quint16 old_msec;               // Предыдущее значение миллисекунд в кадре

        bool old_M1;                    // Предыдущее значение М1
        bool old_M2;                    // Предыдущее значение М2

        bool timeChecking;              // Идет подсчет кадров между секундой
        quint32 kadrTimeNumber;         // Число кадров между секундой


        quint32 RecvKadr;               // Число принятых кадров

        quint32 KadrReadTime;           // Время чтения кадра
        quint32 currentReadTime;        // Оставшееся время

        bool KadrReading;               // Идет чтение кадра

        bool blk_package_good;          // Пакет БЛК достоверный и можно
                                        // использовать для проверки задержки


   // Переменные ошибок

        quint32 KadrCheckNumber;        // Число проверенных кадров

        quint32 AllError;               // Общее число ошибок
        quint32 MMPError;               // Число ошибок ММП
        quint32 BMPError;               // Число ошибок БЛК
        quint32 SSKError;               // Число ошибок ССК
        quint32 TimeError;              // Число ошибок времени
        quint32 M1Error;                // Число ошибок метки М1
        quint32 M2Error;                // Число ошибок метки М2
        quint32 DelayError;             // Ошибка подсчета задержки БЗИ
        quint32 CRCError;               // Число ошибок CRC
        quint32 RSError;                // Число сбоев кода Рида-Соломона



 public slots:

        void sl_CheckKadr();
        void sl_SaveResult();
        void sl_Close();


        void sl_Recv_PackageM();
        void sl_Recv_PackageR();
        void sl_Recv_PackageV();
        void sl_startRecv(int num);

        void sl_KadrReadTimerTimeOut();
        void sl_FPGALoadOk();

signals:

        void UpdateReadingInfo(quint32 time, quint32 kadrNum);
        void StopReading();

};

#endif // DCHECKKADR_H
