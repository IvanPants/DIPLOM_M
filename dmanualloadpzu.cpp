#include "dmanualloadpzu.h"

dManualLoadPZU::dManualLoadPZU(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);

    progressBar -> setVisible(false);
    connect(rbTA528_1, SIGNAL(clicked()), this, SLOT(selectTA528_1()));
    connect(rbTA528_2, SIGNAL(clicked()), this, SLOT(selectTA528_2()));
    connect(rbTA528_htf, SIGNAL(clicked()), this, SLOT(selectTA528_htf()));
    connect(rbTA539, SIGNAL(clicked()), this, SLOT(selectTA539()));

    connect(pbBrowseFile1, SIGNAL(clicked()), this, SLOT(browseFile1()));
    connect(pbBrowseFile2, SIGNAL(clicked()), this, SLOT(browseFile2()));

    connect(pbLoad, SIGNAL(clicked()), this, SLOT(LoadFile()));
    connect(pbCancel, SIGNAL(clicked()), this, SLOT(CloseForm()));

    connect(pbOnPZU, SIGNAL(clicked()), this, SLOT(OnPZU()));
    connect(pbOffPZU, SIGNAL(clicked()), this, SLOT(OffPZU()));


        rbTA528_1 -> setChecked(true);
        lFile1 -> setText(tr("Файл bin:"));
        lFile1 -> setVisible(true);
        leFile1 -> setVisible(true);
        pbBrowseFile1 -> setVisible(true);

        lFile2 -> setVisible(false);
        leFile2 -> setVisible(false);
        pbBrowseFile2 -> setVisible(false);
        LoadingType = 0;

}

int dManualLoadPZU::ConvertHTF(QString FileName)
{

    QFile htfFile;
    QString tmpStr;

    quint8 tmpLowByte;
    quint8 tmpHightByte;


    htfFile.setFileName(FileName);

    if (!htfFile.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::critical(this,
                             tr("pult"),
                             tr("Ошибка открытия файла HTF"),
                             QMessageBox::Ok);
        return -1;
    }


    QTextStream in(&htfFile);

    tmpStr = in.readLine();

    while (!in.atEnd())
    {
      tmpStr.clear();
      tmpStr = in.readLine();

      if (tmpStr.size() < 17)
          continue;

      tmpHightByte = 0;
      tmpLowByte = 0;

      for (int i = 0; i < 8; i++)
      {
          tmpHightByte = tmpHightByte << 1;

          if(tmpStr[i] == '1')
          {
              tmpHightByte = tmpHightByte | 0x01;
          }
      }

      for (int i = 9; i < 17; i++)
      {
          tmpLowByte = tmpLowByte << 1;
          if (tmpStr[i] == '1')
          {
              tmpLowByte = tmpLowByte | 0x01;
          }
      }

      PZUContent.append(tmpLowByte);
      PZUContent.append(tmpHightByte);

    }

    htfFile.close();
}

void dManualLoadPZU::selectTA528_1()
{
    if (rbTA528_1 -> isChecked())
    {
        lFile1 -> setText(tr("Файл bin:"));
        lFile1 -> setVisible(true);
        leFile1 -> setVisible(true);
        pbBrowseFile1 -> setVisible(true);

        lFile2 -> setVisible(false);
        leFile2 -> setVisible(false);
        pbBrowseFile2 -> setVisible(false);

        LoadingType = 0;


    }
}

void dManualLoadPZU::selectTA528_2()
{
    if (rbTA528_2 -> isChecked())
    {
        lFile1 -> setText(tr("Файл 01:"));
        lFile1 -> setVisible(true);
        leFile1 -> setVisible(true);
        pbBrowseFile1 -> setVisible(true);


        lFile2 -> setText(tr("Файл 02:"));
        lFile2 -> setVisible(true);
        leFile2 -> setVisible(true);
        pbBrowseFile2 -> setVisible(true);

        LoadingType = 1;


    }

}

void dManualLoadPZU::selectTA528_htf()
{
    if (rbTA528_htf -> isChecked())
    {
        lFile1 -> setText(tr("Файл HTF:"));
        lFile1 -> setVisible(true);
        leFile1 -> setVisible(true);
        pbBrowseFile1 -> setVisible(true);

        lFile2 -> setText("");
        lFile2 -> setVisible(false);
        leFile2 -> setVisible(false);
        pbBrowseFile2 -> setVisible(false);

        LoadingType = 2;
    }
}

void dManualLoadPZU::selectTA539()
{
    if (rbTA539 -> isChecked())
    {
        lFile1 -> setText(tr("Файл bin:"));
        lFile1 -> setVisible(true);
        leFile1 -> setVisible(true);
        pbBrowseFile1 -> setVisible(true);

        lFile2 -> setVisible(false);
        leFile2 -> setVisible(false);
        pbBrowseFile2 -> setVisible(false);

        LoadingType = 3;


    }
}

void dManualLoadPZU::browseFile1()
{
    QString fileName;

    qDebug() << fileDir;

    switch (LoadingType)
    {
    case 0:
    case 3:

        fileName = QFileDialog::getOpenFileName(this,
                                                tr("Выбирите файл ПЗУ"),
                                                fileDir,
                                                tr("Файл ПЗУ (*.bin);;Все файлы (*.*)")
                                                );


        break;

    case 1:

        fileName = QFileDialog::getOpenFileName(this,
                                                tr("Выбирите файл младшего ПЗУ"),
                                                fileDir,
                                                tr("Файл ПЗУ (*.01M *.01);;Все файлы (*.*)")
                                                );

        break;
    case 2:

        fileName = QFileDialog::getOpenFileName(this,
                                                tr("Выбирите файл HTF"),
                                                fileDir,
                                                tr("Файл имитатора (*.HTF);;Все файлы (*.*)")
                                                );

        break;



    default:
        break;
    }

    if (!fileName.isEmpty())
    {
        leFile1 -> setText(fileName);
    }
}

void dManualLoadPZU::browseFile2()
{
    QString fileName;

    qDebug() << fileDir;

    switch (LoadingType)
    {
    case 0:

        break;

    case 1:

        fileName = QFileDialog::getOpenFileName(this,
                                                tr("Выбирите файл старшего ПЗУ"),
                                                fileDir,
                                                tr("Файл ПЗУ (*.02M *.02);;Все файлы (*.*)")
                                                );

        break;

    case 2:
        break;

    default:
        break;
    }

    if (!fileName.isEmpty())
    {
        leFile2 -> setText(fileName);
    }

}

void dManualLoadPZU::LoadFile()
{
    QFile PZUFile;
    QFile PZUFile2;
    QByteArray low_b, hight_b;
    
    switch (LoadingType) {
    case 0:
        
        PZUFile.setFileName(leFile1->text());
        if(!PZUFile.open(QFile::ReadOnly))
        {
            QMessageBox::critical(this,
                                  tr("ТК170"),
                        tr("Ошибка открытия файла прошивки ПЗУ"),
                        QMessageBox::Ok);
            return;
        }

        PZUContent.clear();
        PZUContent.append(PZUFile.readAll());

        progressBar -> setMinimum(0);
        progressBar -> setMaximum(PZUContent.size());
        progressBar -> setValue(0);
        progressBar -> setVisible(true);

        PZUFile.close();

        qDebug() << "Load " << PZUContent.size() << " byte of data in file";

        loading = 255;

        emit sWritePZU(PZUContent, 0, spinBox -> value(), cbInterliving -> isChecked());

        while (loading == 255)
        {
            QApplication::processEvents();
        }

        if (loading != 0)
        {
            QMessageBox::critical(this,
                                  tr("pult"),
                                  tr("Ошибка загрузки имитатора ПЗУ"),
                                  QMessageBox::Ok);
            progressBar -> setVisible(false);
            return;
        }

        loading = 255;

        emit sStartPZU(true);

        while(loading == 255)
        {
            QApplication::processEvents();
        }

        if (loading != 0)
        {
            QMessageBox::critical(this,
                                  tr("pult"),
                                  tr("Ошибка запуска имитатора ПЗУ"),
                                  QMessageBox::Ok);
            progressBar -> setVisible(false);
            return;
        }
        
        QMessageBox::information(this,
                                 tr("pult"),
                                 tr("Имитатор ПЗУ успешно загружен"),
                                 QMessageBox::Ok);
        break;

    case 1:

        PZUFile.setFileName(leFile1->text());
        if(!PZUFile.open(QFile::ReadOnly))
        {
            QMessageBox::critical(this,
                                  tr("pult"),
                        tr("Ошибка открытия файла прошивки ПЗУ"),
                        QMessageBox::Ok);
            progressBar -> setVisible(false);
            return;
        }

        PZUFile2.setFileName(leFile2->text());
        if(!PZUFile2.open(QFile::ReadOnly))
        {
            QMessageBox::critical(this,
                                  tr("pult"),
                        tr("Ошибка открытия файла прошивки ПЗУ"),
                        QMessageBox::Ok);
            progressBar -> setVisible(false);
            return;
        }




        low_b.append(PZUFile.readAll());
        hight_b.append(PZUFile2.readAll());

        PZUFile.close();
        PZUFile2.close();

        qDebug() << "Low bytes: " << low_b.size() << " hight bytes: " << hight_b.size();

        for (int i = 0; i < 1536; i++)
        {
            PZUContent.append(low_b.at(i));
            PZUContent.append(hight_b.at(i));

           // qDebug() << i << " - " << QString::number(hight_b.at(i),16) << QString::number(low_b.at(i),16);
        }


        qDebug() << "Load " << PZUContent.size() << " byte of data in file";

        loading = 255;

        emit sWritePZU(PZUContent, 1, spinBox -> value(), cbInterliving -> isChecked());

        while (loading == 255)
        {
            QApplication::processEvents();
        }

        if (loading != 0)
        {
            QMessageBox::critical(this,
                                  tr("pult"),
                                  tr("Ошибка загрузки имитатора ПЗУ"),
                                  QMessageBox::Ok);
            progressBar -> setVisible(false);
            return;
        }

        loading = 255;

        emit sStartPZU(true);

        while(loading == 255)
        {
            QApplication::processEvents();
        }

        if (loading != 0)
        {
            QMessageBox::critical(this,
                                  tr("pult"),
                                  tr("Ошибка запуска имитатора ПЗУ"),
                                  QMessageBox::Ok);
            progressBar -> setVisible(false);
            return;
        }

        QMessageBox::information(this,
                                 tr("pult"),
                                 tr("Имитатор ПЗУ успешно загружен"),
                                 QMessageBox::Ok);

        break;

    case 2:

        ConvertHTF(leFile1->text());

        progressBar -> setMinimum(0);
        progressBar -> setMaximum(PZUContent.size());
        progressBar -> setValue(0);
        progressBar -> setVisible(true);

        qDebug() << "Load " << PZUContent.size() << " byte of data in file";

        loading = 255;

        emit sWritePZU(PZUContent, 2, spinBox -> value(), cbInterliving -> isChecked());

        while(loading == 255)
        {
            QApplication::processEvents();
        }

        if (loading != 0)
        {
            QMessageBox::critical(this,
                                  tr("ТК170"),
                                  tr("Ошибка загрузки имитатора ПЗУ"),
                                  QMessageBox::Ok);
            progressBar -> setTextVisible(false);
            return;
        }

        loading = 255;

        emit sStartPZU(true);

        while(loading == 255)
        {
            QApplication::processEvents();
        }

        if (loading != 0)
        {
            QMessageBox::critical(this,
                                  tr("pult"),
                                  tr("Ошибка запуска имитатора ПЗУ"),
                                  QMessageBox::Ok);
            progressBar -> setTextVisible(false);
            return;
        }

        QMessageBox::information(this,
                                 tr("pult"),
                                 tr("Имитатор ПЗУ успешно загружен"),
                                 QMessageBox::Ok);
        break;

    case 3:

        break;


    default:
        break;
    }

    progressBar -> setVisible(false);
}

void dManualLoadPZU::CloseForm()
{
    close();
}

void dManualLoadPZU::LoadingOK()
{
    loading = 0;
}

void dManualLoadPZU::LoadingError()
{
    loading = 1;
}

void dManualLoadPZU::setProgress(int progress)
{
    progressBar -> setValue(progress);
}

void dManualLoadPZU::OnPZU()
{
    loading = 255;

    emit sOnOffPZU(true);

    while(loading == 255)
    {
        QApplication::processEvents();
    }

    if (loading != 0)
    {
        QMessageBox::critical(this,
                              tr("pult"),
                              tr("Ошибка включения имитатора ПЗУ"),
                              QMessageBox::Ok);
        return;
    }
}

void dManualLoadPZU::OffPZU()
{
    loading = 255;

    emit sOnOffPZU(false);

    while(loading == 255)
    {
        QApplication::processEvents();
    }

    if (loading != 0)
    {
        QMessageBox::critical(this,
                              tr("pult"),
                              tr("Ошибка выключения имитатора ПЗУ"),
                              QMessageBox::Ok);
        return;
    }
}
