#ifndef DNEWUSTAVKI_H
#define DNEWUSTAVKI_H

#include "ui_dnewustavki.h"

#include "spinboxdelegate.h"

#include <QSettings>
#include <QDebug>

#include <QTableWidgetItem>
#include <QStandardItemModel>
#include <QFileDialog>





class dNewUstavki : public QDialog, private Ui::dNewUstavki
{
    Q_OBJECT
    
public:
    explicit dNewUstavki(bool isNew, QWidget *parent = 0);

    void setUstFile(QString FileName);
    void setMASEnabel(int MAS, bool value, QString name);
    void setBLKEnable(int BLK, bool value, QString name);

    void setMAXMASNumber(int num) {MaxMASNumber = num;}


    void loadUstFile();
    void SaveToFile();
    QStandardItemModel blk_model[4];
    SpinBoxDelegate delegate;
    
protected:


    QSettings *ustFile;

    void changeEvent(QEvent *e);

    bool MASEnable[10];
    bool BLKEnable[4];

    int MaxMASNumber;
    int MaxLKNumber;
    int MaxBLKNumber;
    int MaxSensorNumber;

    int MASIndex;
    int BLKIndex;

    int MASIndexArray[10];
    int BLKIndexArray[4];


    QCheckBox *DualMASPS1[8];
    QCheckBox *DualMASPS2[8];
    QCheckBox *ZapMASPS1[8];
    QCheckBox *ZapMASPS2[8];
    quint8 MASAddr[10][4];
    quint8 MASBufferSize[10][4];
    quint8 MASConst1Pos[10][4];
    quint8 MASConst1[10][4];
    quint8 MASConst2Pos[10][4];
    quint8 MASConst2[10][4];
    quint8 MASAdder[10][4];
    QByteArray MASBuffer[10][4];


   // QTableWidgetItem *BufferItems[128];

    quint8 currentMAS;
    quint8 currentLK;

    bool isChangeMAS;
    bool isChangeLK;

    void SaveCurrentLK(quint8 MAS, quint8 LK);
    void LoadCurrentLK(quint8 MAS, quint8 LK);


public slots:

    void sl_ChangeMAS(int num);
    void sl_ChangeLK(int num);
    void sl_BufferSizeChange(int size);

    void sl_OkButton();
    void sl_CancelButton();
    void sl_ChangeBLK(int num);

    void sl_DualPS1Checked(bool val);
    void sl_DualPS2Checked(bool val);

    void sl_BrowsPZUFile();
};

#endif // DNEWUSTAVKI_H
