#ifndef FWRITEKADR_H
#define FWRITEKADR_H

#include "ui_fwritekadr.h"




class fWriteKadr : public QWidget, private Ui::fWriteKadr
{
    Q_OBJECT
    Q_ENUMS(qMResult)



    
public:

    enum qMResult{None = 0,
                 Ok = 1,
                 Cancel = 2};

    explicit fWriteKadr(QWidget *parent = 0);

    void setStartData(quint32 TimeVal);

    qMResult getModalResult(){return modalResult;}


private:

    qMResult modalResult;

public slots:

    void sl_update(quint32 time, quint32 kadrNum);
    void sl_Cancel();
    void sl_Stop();
};

#endif // FWRITEKADR_H
