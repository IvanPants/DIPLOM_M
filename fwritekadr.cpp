#include "fwritekadr.h"

fWriteKadr::fWriteKadr(QWidget *parent) :
    QWidget(parent),
    modalResult(None)
{
    setupUi(this);

    connect(pbCancel, SIGNAL(clicked()), this, SLOT(sl_Cancel()));
}

void fWriteKadr::setStartData(quint32 TimeVal)
{
    progressBar -> setMaximum(TimeVal);
    progressBar -> setValue(0);
    lKadrNum -> setText(QString::number(0));
    lTime -> setText(QString::number(TimeVal)  + " c.");
}

void fWriteKadr::sl_update(quint32 time, quint32 kadrNum)
{
    lTime -> setText(QString::number(progressBar->maximum() - time)  + " c.");
    progressBar -> setValue(time);
    lKadrNum -> setText(QString::number(kadrNum));
}

void fWriteKadr::sl_Cancel()
{
    modalResult = Cancel;
    this -> hide();

}

void fWriteKadr::sl_Stop()
{
    modalResult = Ok;
    this -> hide();
}
