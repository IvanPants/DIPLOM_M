#ifndef MAS_DATA_H
#define MAS_DATA_H

#include <QObject>
#include <QSettings>
#include <QDebug>
#include <QString>

class MAS_Data : public QObject
{
    Q_OBJECT

private:

    quint8 LK_DATA[4][128];
    bool Loading;

    bool isDualPS1;
    bool isDualPS2;

    quint8 MasNum;
    quint8 Addr[4];
    quint8 Adder[4];
    quint16 Const1Pos[4];
    quint8 Const1[4];
    quint16 Const2Pos[4];
    quint8 Const2[4];
    quint16 BufferSize[4];
    QString MASName;

    bool MAS_DM;        // true - МАС, false - ЦМ



public:
    explicit MAS_Data(QString Name, QObject *parent = 0);

    bool isLoading() {return Loading;}

    quint8 MASData[128][128];

    int LoadData(QString FileName, quint8 MASNumber, bool mas_dm); // Загрузка данных из файла уставок
    int RepackData();       // Перепаковка в масив данных

    void setMASName(QString name) {MASName = name;}
    QString getMASName() {return MASName;}

    void setMASNum(quint8 num){ MasNum = num;}
    quint8 getMASNum(){return MasNum;}

    bool getAdd_en(quint8 lk_num) {if (Adder[lk_num] == 0) return false; else return true;}
    quint8 getAddr(quint8 lk_num) {return Addr[lk_num];}
    quint8 getAdder(quint8 lk_num) {return Adder[lk_num];}
    quint16 getBufferSize(quint8 lk_num) {return BufferSize[lk_num];}

    bool getDualPS1() {return isDualPS1;}
    bool getDualPS2() {return isDualPS2;}

    quint8 getLKData(quint8 lk_num, quint16 lkChannel) {return LK_DATA[lk_num][lkChannel];}

    quint16 getConst1Pos(quint8 lk_num){if (MAS_DM) return Const1Pos[lk_num]; else return 300;}
    quint8 getConst1(quint8 lk_num){return Const1[lk_num];}

    quint16 getConst2Pos(quint8 lk_num){if (MAS_DM) return Const2Pos[lk_num]; else return 300;}
    quint8 getConst2(quint8 lk_num){return Const2[lk_num];}

    bool isMAS(){return MAS_DM;}
    
signals:
    
public slots:
    
};

#endif // MAS_DATA_H
