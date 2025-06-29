#ifndef DMANUALLOADPZU_H
#define DMANUALLOADPZU_H

#include "ui_dmanualloadpzu.h"


#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>

class dManualLoadPZU : public QDialog, private Ui::dManualLoadPZU
{
    Q_OBJECT
    
public:
    explicit dManualLoadPZU(QWidget *parent = 0);

    QString fileDir;

private:

    quint8 LoadingType; /* 0 - ТА528 одним файлом
                         * 1 - ТА528 двумя файлами (прошивки внутренних микросхем)
                         * 2 - ТА528 файлом HTF
                         * 3 - TA539
                         */

    QByteArray PZUContent;

    quint8 loading;

    int ConvertHTF(QString FileName);

   public slots:

    void selectTA528_1(void);
    void selectTA528_2(void);
    void selectTA528_htf(void);
    void selectTA539(void);

    void browseFile1();
    void browseFile2();

    void LoadFile();
    void CloseForm();

    void LoadingOK();
    void LoadingError();

    void setProgress(int progress);

    void OnPZU();
    void OffPZU();

signals:

    void sWritePZU(QByteArray data, quint16 type, quint16 pause, bool interliving);
    void sStartPZU(bool status);
    void sOnOffPZU(bool status);


};

#endif // DMANUALLOADPZU_H
