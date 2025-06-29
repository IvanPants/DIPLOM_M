#ifndef DSCHECKVAAR_H
#define DSCHECKVAAR_H

#include <QObject>
#include <QFileInfo>
#include <QDebug>
#include <QSettings>


#include "mas_data.h"


typedef struct _SECOND_HEADER {

    quint8 M2Min            : 1;        // Маркер М2 меандр 20 сек.
    quint8 M1Min            : 1;        // Маркер М1 меандр 2 сек.
    quint8 Min              : 6;        // Код минут

    quint8 M2Sec            : 1;        // Маркер М2 меандр 20 сек.
    quint8 M1Sec            : 1;        // Маркер М1 меандр 2 сек.
    quint8 Sec              : 6;        // Код секунд

    quint8 MSec_Hi;                     // Код миллисекунд старший байт
    quint8 MSec_Lo;                     // Код миллисекунд младший байт

} SECOND_HEADER;


typedef struct _PACKAGE_VAAR{

    quint16 Version              : 3;   // Номер версии "000"
    quint16 PackageType          : 1;   // Тип пакета "0" - телеметрический, "1" - телесигнализационный
    quint16 SecondFlag           : 1;   // Флаг второго заголовка "1" - используется второй заголовок
    quint16 SourceIdent          : 11;  // Идентификатор источника "11111111111" - пустой кадр
                                        //                         "01010101010" - идентификатор исочника

    quint16 GroupFlag            : 2;   // Флаг группы "01" - первый пакет, "10" - последний пакет, "00" - остальные пакета
    quint16 PackageCounter       : 14;  // Счетчик пакетов от 1 до 16384

    quint16 PackageLenght;              // Длина пакета

    SECOND_HEADER SecondHeader;         // Второй заголовок

    quint8 data[512];                   // Данные

    quint8 CRC[2];                      // Контрольная сумма CRC

} PACKAGE_VAAR;


typedef struct _KADR_VAAR{

    SECOND_HEADER Time;

    PACKAGE_VAAR UserData;
    quint8 Reserved[6];
    quint8 RS[96];

    quint32 SYNC;

} KADR_VAAR;

typedef  struct _DATA_STR {
    quint16 addr:4;
    quint16 pf:1;
    quint16 num_in:4;
    quint16 gl_faz:7;
} DATA_STR;

typedef union _PZUData{
    DATA_STR data_str[1024];
    quint8 buff[2048];
    quint16 word[1024];
} PZUData;


typedef struct _LK_PARAM
{
    bool Adder_en;        // Есть динамика для данного LK
    quint8 Addr;          // Адрес ЛК
    quint8 Adder;         // Значение сумматора
    quint16 BufferSize;   // Размер буфера ЛК
    quint16 poz_faz;      // Позиция фазировки ЛК в кадре
    quint8 gl_faz;        // Глубина фазировки данного ЛК

    quint16 lk_channel;
    quint8 current_Adder;
    bool clear_adder;    // Признак сброса сумматора
    bool mas_dm;         // Признак МАС-ЦМ

    quint16 const1Pos;
    quint8 const1;
    quint16 const2Pos;
    quint8 const2;


} LK_PARAM;

class dsCheckVAAR : public QObject
{
    Q_OBJECT

private:

    KADR_VAAR *kadrData;        // Буфер кадра для проверки


    QString ustFilePath;        // Путь к файлу уставок


    QString UstFileName;        // Имя файла уставок
    QString DevFileName;        // Имя файла прибора

    QString pzuFileName;        // Имя файла программы опроса

    QSettings *ustFile;         // Файл уставок
    QSettings *devFile;         // Файл прибора


    bool DualPS1[8];            // Признак сдвоенного МАС для ПС1
    bool DualPS2[8];            // Признак сдвоенного МАС для ПС2

    MAS_Data *MASData[13];      // Массив данных МАС и ЦМ

    PZUData PRG_PS1;            // Программа опроса ПС1
    PZUData PRG_PS2;            // Программа опроса ПС2


    quint8 ImmKadr_PS1[128][512];   // Эталонный кадр для ПС1
    quint8 ImmKadr_PS2[128][512];   // Эталонный кадр для ПС2


    LK_PARAM lk_param[13][4];           // Параметры ЛК для расчета кадра

    struct {
        quint8 in;
        quint8 type;

    } mas_input_array[13];    // Массив соответствий номеров


public:
    explicit dsCheckVAAR(QObject *parent = 0);

    void setKadrData(void *data) {kadrData = (KADR_VAAR *)data;}
    void setUstFileName(QString FileName);


    int loadUstFile();
    int UpdateEtalon();
    
signals:
    
public slots:
    
};

#endif // DSCHECKVAAR_H
