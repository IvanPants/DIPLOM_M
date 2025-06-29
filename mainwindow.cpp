#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    setupUi(this);


    this -> setWindowTitle(tr("Программа управления и контроля ") + QString::number(NVER1, 10)
                                                        + "." + QString::number(NVER2, 10)
                                                        + "." + QString::number(NVER3, 10)
                                                        + "." + QString::number(NVER4, 10));



    MAS_label[0] = lMAS1; MAS_label[1] = lMAS2;
    MAS_label[2] = lMAS3; MAS_label[3] = lMAS4;
    MAS_label[4] = lMAS5; MAS_label[5] = lMAS6;
    MAS_label[6] = lMAS7; MAS_label[7] = lMAS8;
    MAS_label[8] = lMAS9; MAS_label[9] = lMAS10;

    BLK_label[0] = lBLK1; BLK_label[1] = lBLK2;
    BLK_label[2] = lBLK3; BLK_label[3] = lBLK4;

    MBTcp = new ModBusTCP(this);
    MBTcp -> setRepeat(3);
    isConnected = false;

    MainSettingFile = new QSettings(QApplication::applicationDirPath() + "//pult.ini", QSettings::IniFormat);

    MainSettingFile -> setIniCodec("Windows-1251");
    for (int i = 0; i < 10; i++)
    {
        currentDeviceConfig.MAS_name[i] = "MAC" + QString::number(i);
        MAS_label[i] -> setText("---");
    }

    for (int i = 0; i < 4; i++)
    {
        currentDeviceConfig.BLK_name[i] = "БЛК" + QString::number(i);
        BLK_label[i] -> setText("---");
    }

    QStringList header;

    header << tr("Время") << tr("Событие") << tr("Результат");

    twLog -> setColumnCount(3);
    twLog -> setHorizontalHeaderLabels(header);

    twLog -> setColumnWidth(1, 277);
    twLog -> setColumnWidth(2, 60);

    currentLogRow = 0;
// Подключение слотов верхнего меню

    connect(aManualPK_ICM, SIGNAL(triggered()), this, SLOT(sl_ManualPK_ICM()));

    connect(aManualReceiver, SIGNAL(triggered()), this, SLOT(sl_ManualReceiver()));
    connect(aManualImmPZU, SIGNAL(triggered()), this, SLOT(sl_ManualImmPZU()));

    connect(aSelfTest, SIGNAL(triggered()), this, SLOT(sl_SelfTest()));

    connect(aNewDevice, SIGNAL(triggered()), this, SLOT(sl_NewDevice()));
    connect(aEditDevice, SIGNAL(triggered()), this, SLOT(sl_EditDevice()));

    connect(aNewSettings, SIGNAL(triggered()), this, SLOT(sl_newUstavki()));
    connect(aEditSettings, SIGNAL(triggered()), this, SLOT(sl_EditUstavki()));

    connect(aConvert528, SIGNAL(triggered()), this, SLOT(sl_Convert528()));


    connect(pbConnect, SIGNAL(clicked()), this, SLOT(sl_Connect()));
    connect(pbDisConnect, SIGNAL(clicked()), this, SLOT(sl_DisConnect()));

    connect(MBTcp, SIGNAL(ModBusOK()), this, SLOT(sl_ModBusOK()));
    connect(MBTcp, SIGNAL(ModBusError(int)), this, SLOT(sl_ModBusError(int)));


    connect(cbSelectDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(sl_LoadDevice(int)));
    connect(lwSettings, SIGNAL(currentRowChanged(int)), this, SLOT(sl_SelectUst(int)));

    connect(pbLoadSettings, SIGNAL(clicked()), this, SLOT(sl_LoadUst()));
    connect(pbCheckKadr, SIGNAL(clicked()), this, SLOT(sl_CheckKadr()));
    connect(pbViewSettings, SIGNAL(clicked()), this, SLOT(sl_ViewUst()));


    connect(pbClearLog, SIGNAL(clicked()), this, SLOT(sl_clearLog()));

    LoadDevice();
}

void MainWindow::EnableControl()
{
    pbConnect -> setDisabled(true);
    pbDisConnect -> setEnabled(true);

    pbLoadSettings -> setEnabled(true);
    pbCheckKadr -> setEnabled(true);
   // pbViewSettings -> setEnabled(true);


    aManualPK_ICM -> setEnabled(true);
    aManualReceiver -> setEnabled(true);

    for (int i = 0; i < 10; i++)
    {

        if (currentDeviceConfig.MAS_Enable[i])
            MAS_label[i] -> setStyleSheet("color: yellow");
        else
            MAS_label[i] -> setStyleSheet("color: grey");
    }

    for (int i = 0; i < 4; i++)
    {
        if (currentDeviceConfig.BLK_Enable[i])
            BLK_label[i] -> setStyleSheet("color: yellow");
        else
            BLK_label[i] -> setStyleSheet("color: grey");
    }


    if (currentDeviceConfig.DMAS2_Enable)
        lDM2 -> setStyleSheet("color: yellow");
    else
        lDM2 -> setStyleSheet("color: grey");

    if (currentDeviceConfig.DMAS6_Enable)
        lDM6 -> setStyleSheet("color: yellow");
    else
        lDM6 -> setStyleSheet("color: grey");
}

void MainWindow::DisableControl()
{

    pbConnect -> setEnabled(true);
    pbDisConnect -> setDisabled(true);

    pbLoadSettings -> setDisabled(true);
#ifndef __DEBUG__
    pbCheckKadr -> setDisabled(true);
#endif
  //  pbViewSettings -> setDisabled(true);


    aManualPK_ICM -> setDisabled(true);
    aManualReceiver -> setDisabled(true);
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::LoadDevice()
{

    cbSelectDevice -> clear();

    int MaxDeviceNum = MainSettingFile -> beginReadArray("Device");

    for (int i = 0; i < MaxDeviceNum; ++i)
    {
        MainSettingFile -> setArrayIndex(i);
        qDebug() << "Found device " << i;
        qDebug() << "Device name: " << MainSettingFile -> value("devName", "").toString();

        cbSelectDevice -> addItem(MainSettingFile -> value("DevName", "").toString());
    }

    MainSettingFile -> endArray();

    MainSettingFile -> beginGroup("LastState");

    cbSelectDevice -> setCurrentIndex(MainSettingFile -> value("Device", 0).toInt());

    MainSettingFile -> endGroup();
}

int MainWindow::LoadMAS_LKNumber(int ImmAddr, int MASNumber, int LKNumber)
{

    qDebug() << "Set imm: " << ImmAddr << "MAS number: " << MASNumber << " LK number: " << LKNumber;

    MBResult = 255;

    MBTcp -> WriteHoldingRegister(ImmAddr, MAS_NUMBER_ADDR, MASNumber);

    while (MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
        return MBResult;

    MBResult = 255;

    MBTcp -> WriteHoldingRegister(ImmAddr, LK_NUMBER_ADDR, LKNumber);

    while (MBResult == 255)
        QApplication::processEvents();

    return MBResult;
}

int MainWindow::LoadLKAddr(int ImmAddr, int LKAddr)
{

    qDebug() << "Imm: " << ImmAddr << " Load addr: " << LKAddr;

    MBResult = 255;

    MBTcp -> WriteHoldingRegister(ImmAddr, ADDR_LK_ADDR, LKAddr);

    while (MBResult == 255)
        QApplication::processEvents();

    return MBResult;
}

int MainWindow::LoadLKBuffer(int ImmAddr, QByteArray LKBuffer)
{

    quint16 data[64];

    int t_size;

    qDebug() << "Imm: " << ImmAddr << " Load buffer size: " << LKBuffer.size();
    qDebug() << "Buffer: " << LKBuffer.toHex();

    MBResult = 255;

    MBTcp -> WriteHoldingRegister(ImmAddr, BUFFER_SIZE_ADDR, LKBuffer.size() - 1);

    while (MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
        return MBResult;

    MBResult = 255;

    if (LKBuffer.size() > 64)
        t_size = 32;
    else
        t_size = LKBuffer.size() / 2;

    qDebug() << "t_size: " << t_size;

    for (int i = 0; i < t_size; i++)
    {
        data[i] = (((quint16)LKBuffer.at(i * 2 + 1) << 8) & 0xFF00) | (((quint16)LKBuffer.at(i*2)) & 0x00FF);


    }

    if ((ImmAddr == IMM_OP_1_ADDR) || (ImmAddr == IMM_OP_2_ADDR))

        MBTcp -> WriteMultipleHoldingRegisters(ImmAddr, BUFFER_ADDR_OP, t_size, data);
    else
        MBTcp -> WriteMultipleHoldingRegisters(ImmAddr, BUFFER_ADDR_RS, t_size, data);


    while (MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
        return MBResult;

    if (LKBuffer.size() > 64)
    {
        for(int i = 32; i < (LKBuffer.size()/2); i++)
        {
            data[i - 32] = ((quint16)LKBuffer.at(i * 2 + 1) << 8) | ((quint16)LKBuffer.at(i*2));
        }

        MBResult = 255;

        if ((ImmAddr == IMM_OP_1_ADDR) || (ImmAddr == IMM_OP_2_ADDR))

            MBTcp -> WriteMultipleHoldingRegisters(ImmAddr, BUFFER2_ADDR_OP, (LKBuffer.size() - 64)/2, data);
        else
            MBTcp -> WriteMultipleHoldingRegisters(ImmAddr, BUFFER2_ADDR_RS, (LKBuffer.size() - 64)/2, data);

        while(MBResult == 255)
            QApplication::processEvents();

        return MBResult;
    }

    return MBResult;

}

int MainWindow::LoadLKAdder(int ImmAddr, int Adder)
{
    qDebug() << "Imm: " << ImmAddr << " Load adder: " << Adder;

    MBResult = 255;

    MBTcp -> WriteHoldingRegister(ImmAddr, ADDER_ADDR, Adder);

    while (MBResult == 255)
        QApplication::processEvents();

    return MBResult;

}

int MainWindow::LoadLKConst1(int ImmAddr, int pos, int value)
{
    quint16 data;

    qDebug() << "Imm: " << ImmAddr << " Load const1: " << value << " at pos: " << pos;


    data = (pos << 8) | value;

    MBResult = 255;

    MBTcp -> WriteHoldingRegister(ImmAddr, CONST1_ADDR, data);

    while (MBResult == 255)
        QApplication::processEvents();

    return MBResult;

}

int MainWindow::LoadLKConst2(int ImmAddr, int pos, int value)
{
    quint16 data;

    qDebug() << "Imm: " << ImmAddr << " Load const2: " << value << " at pos: " << pos;


    data = (pos << 8) | value;

    MBResult = 255;

    MBTcp -> WriteHoldingRegister(ImmAddr, CONST2_ADDR, data);

    while (MBResult == 255)
        QApplication::processEvents();

    return MBResult;
}

int MainWindow::LoadDuration(int ImmAddr, int duration)
{
    qDebug() << "Imm: " << ImmAddr << " Loadr duration: " << duration;


    MBResult = 255;

    MBTcp -> WriteHoldingRegister(ImmAddr, DURATION_ADDR, duration);

    while (MBResult == 255)
        QApplication::processEvents();

    return MBResult;

}

int MainWindow::LoadDMAS2(int ImmAddr, int Start, int Stop, int increment, int delay)
{
    quint16 data[3];

    qDebug() << "DMAS2 Imm: " << ImmAddr;
    qDebug() << "Start: " << Start << " Stop: " << Stop;
    qDebug() << "Increment: " << increment << " Delay: " << delay;

    data[0] = qToBigEndian<quint16>(Start);
    data[1] = qToBigEndian<quint16>(Stop);
    data[2] = qToBigEndian<quint16>(increment);

    MBResult = 255;

    MBTcp -> WriteMultipleHoldingRegisters(ImmAddr, DMAS2_START_ADDR, 3, data);

    while(MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
        return MBResult;

    MBResult = 255;

    MBTcp -> WriteHoldingRegister(ImmAddr, DMAS2_DELAY_ADDR, delay);

    while (MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
        return MBResult;

    MBResult = 255;

    MBTcp -> WriteCoil(ImmAddr, DMAS2_ENABLE_ADDR, 1);

    while (MBResult == 255)
        QApplication::processEvents();

    return MBResult;

}

int MainWindow::LoadDMAS6(int ImmAddr, int Start, int Stop, int increment, int delay)
{
    quint16 data[3];

    qDebug() << "DMAS6 Imm: " << ImmAddr;
    qDebug() << "Start: " << Start << " Stop: " << Stop;
    qDebug() << "Increment: " << increment << " Delay: " << delay;

    data[0] = qToBigEndian<quint16>(Start);
    data[1] = qToBigEndian<quint16>(Stop);
    data[2] = qToBigEndian<quint16>(increment);

    MBResult = 255;

    MBTcp -> WriteMultipleHoldingRegisters(ImmAddr, DMAS6_START_ADDR, 3, data);

    while(MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
        return MBResult;

    MBResult = 255;

    MBTcp -> WriteHoldingRegister(ImmAddr, DMAS6_DELAY_ADDR, delay);

    while (MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
        return MBResult;

    MBResult = 255;

    MBTcp -> WriteCoil(ImmAddr, DMAS6_ENABLE_ADDR, 1);

    while (MBResult == 255)
        QApplication::processEvents();

    return MBResult;
}

int MainWindow::OnOffBLK(bool blk1, bool blk2, bool blk3, bool blk4)
{

    quint8 data[4];

    qDebug() << "On/Off BLK - BLK1: " << blk1 << " BLK2: " << blk2 << " BLK3: " << blk3 << " BLK4: " << blk4;

    if (blk4) data[0] = 0xff; else data[0] = 0;
    if (blk3) data[1] = 0xff; else data[1] = 0;
    if (blk2) data[2] = 0xff; else data[2] = 0;
    if (blk1) data[3] = 0xff; else data[3] = 0;

    MBResult = 255;

    MBTcp -> WriteMultipleCoils(IMM_BLK_ADDR, BLK_ON_OFF_ADDR, 4, data);

    while(MBResult == 255)
        QApplication::processEvents();

    return MBResult;

}

int MainWindow::selectBLK(quint8 blk_num)
{
    quint16 data;

    qDebug() << "Select BLK" << (blk_num + 1);

    data  = 0xFF00 | ((quint16)blk_num & 0x00FF);

    MBResult = 255;

    MBTcp -> WriteHoldingRegister(IMM_BLK_ADDR, BLK_SELECT_ADDR, data);

    while (MBResult == 255)
        QApplication::processEvents();

    return MBResult;
}

int MainWindow::addSensorBLK(quint8 freq, quint8 sensor_num, quint8 type, quint8 a, quint8 b, quint8 c)
{

    quint16 data[3];

    qDebug() << "Add sensor: " << sensor_num << " FREQ: " << freq << " Type: " << type;
    qDebug() << " a = " << a << " b = " << b << " c = " << c;

    data[0] = (((quint16)sensor_num << 8 ) & 0xFF00) | ((quint16)freq & 0x00FF);
    data[1] = (((quint16)a << 8 ) & 0xFF00) | ((quint16)type & 0x00FF);
    data[2] = (((quint16)c << 8 ) & 0xFF00) | ((quint16)b & 0x00FF);

    MBResult = 255;

    MBTcp -> WriteMultipleHoldingRegisters(IMM_BLK_ADDR, BLK_SENSOR_SET_ADDR, 3, data);

    while (MBResult == 255)
        QApplication::processEvents();

    return MBResult;
}

void MainWindow::errorLoadUstavki(int MAS, bool dm2, bool dm6, int blk, bool pzu)
{
    sl_LogResult(false);
    lLoading -> setText(tr("Ошибка загрузки уставок"));
    if (MAS < 10)
        MAS_label[MAS] -> setStyleSheet("color: red");

    if (dm2)
        lDM2 -> setStyleSheet("color: red");

    if (dm6)
        lDM6 -> setStyleSheet("color: red");

    if (blk < 4)
        BLK_label[blk] -> setStyleSheet("color: red");

    if (pzu)
        lPZU -> setStyleSheet("color: red");

    QMessageBox::critical(this, "TK170", tr("Ошибка загрузки уставок"), QMessageBox::Ok);
    pbLoadSettings -> setEnabled(true);
    return;

}

int MainWindow::SelectKadrType(int kadrType, bool InputType)
{
    quint16 data;

    qDebug() << "Select KadrType" << kadrType << "set: " << KADR_TYPE[kadrType];

    data  = KADR_TYPE[kadrType];



    MBResult = 255;

    if (kadrType == 255)
    {
        MBTcp -> WriteHoldingRegister(BU_ADDR, BU_KADR_TYPE_ADDR, 0);

        while (MBResult == 255)
            QApplication::processEvents();
         return MBResult;
    } else
    {

        MBTcp -> WriteHoldingRegister(BU_ADDR, BU_KADR_TYPE_ADDR, data);

        while (MBResult == 255)
            QApplication::processEvents();
        if (MBResult != 0)
            return MBResult;

        MBResult = 255;

        if (kadrType == KADR_RTSC)
        {
            MBTcp -> WriteHoldingRegister(IMM_DMAS_ADDR, INPUT_SWITCH_ADDR, 0xFF00);
        }

        if ((kadrType == KADR_RTSCM) || (kadrType == KADR_RTSCM1))
        {
            if (InputType)
                MBTcp -> WriteHoldingRegister(IMM_DMAS_ADDR, INPUT_SWITCH_ADDR, 0xFF02);
            else
                MBTcp -> WriteHoldingRegister(IMM_DMAS_ADDR, INPUT_SWITCH_ADDR, 0xFF01);
        }

        if (kadrType == KADR_VAAR)
        {
            MBTcp -> WriteHoldingRegister(IMM_DMAS_ADDR, INPUT_SWITCH_ADDR, 0xFF02);
        }

        while (MBResult == 255)
            QApplication::processEvents();
        return MBResult;
    }
}

void MainWindow::sl_Connect()
{
    qDebug() << "Connect to TK170";

    lLoading -> setText(tr("Подключение к ТК170"));

    MBResult = 255;

    MBTcp -> Connect();

    while(MBResult == 255)
        QApplication::processEvents();

    if (MBResult == 0)
    {
        isConnected = true;
        EnableControl();
        lLoading -> setText(tr("Уставки не загружены"));

    } else
    {
        QMessageBox::critical(this, "TK170", tr("Ошибка соединения с пультом ТК170"), QMessageBox::Ok);
        lLoading -> setText(tr("Ошибка подключения к ТК170"));
    }


}

void MainWindow::sl_DisConnect()
{
    MBResult = 255;

    MBTcp -> DisConnect();

    while(MBResult == 255)
        QApplication::processEvents();

    if (MBResult == 0)
    {
        isConnected = false;
        DisableControl();
    } else
    {
        QMessageBox::critical(this, "TK170", tr("Ошибка отсоединения от ТК170"), QMessageBox::Ok);
    }

    lLoading -> setText(tr("Не подключения к ТК170"));

    for (int i = 0; i < 10; i++)
    {

        if (currentDeviceConfig.MAS_Enable[i])
            MAS_label[i] -> setStyleSheet("color: yellow");
        else
            MAS_label[i] -> setStyleSheet("color: grey");
    }

    for (int i = 0; i < 4; i++)
    {
        if (currentDeviceConfig.BLK_Enable[i])
            BLK_label[i] -> setStyleSheet("color: yellow");
        else
            BLK_label[i] -> setStyleSheet("color: grey");
    }


    if (currentDeviceConfig.DMAS2_Enable)
        lDM2 -> setStyleSheet("color: yellow");
    else
        lDM2 -> setStyleSheet("color: grey");

    if (currentDeviceConfig.DMAS6_Enable)
        lDM6 -> setStyleSheet("color: yellow");
    else
        lDM6 -> setStyleSheet("color: grey");



}

void MainWindow::sl_NewDevice()
{
    dAddDevice dialog(true);


    int MaxDeviceNumber;


    QDir *DeviceDir;

    if (dialog.exec() == QDialog::Accepted)
    {
        MaxDeviceNumber = MainSettingFile -> beginReadArray("Device");

        MainSettingFile -> endArray();
        MainSettingFile -> sync();

        MainSettingFile -> beginWriteArray("Device");

        qDebug() << "MaxDeviceNumber: " << MaxDeviceNumber;

        MainSettingFile -> setArrayIndex(MaxDeviceNumber);
        MainSettingFile -> setValue("devName", dialog.getDeviceName());

        MainSettingFile -> endArray();
        MainSettingFile -> sync();

        DeviceDir = new QDir(QApplication::applicationDirPath());

        qDebug() << DeviceDir;

        if (DeviceDir -> exists(ADD_NUMBER("Dev", MaxDeviceNumber)))
        {
            qDebug() << "Device directory exist";
        }

        if (!DeviceDir -> mkdir(ADD_NUMBER("Dev", MaxDeviceNumber)))
        {
            qDebug() << "Error create directory";
        }

        DeviceSettingsFile = new QSettings(DeviceDir->absolutePath() + "//" + ADD_NUMBER("Dev", MaxDeviceNumber) + "//Device.ini", QSettings::IniFormat);

        DeviceSettingsFile -> setIniCodec("Windows-1251");

        DeviceSettingsFile -> beginWriteArray("MAS");

        if (dialog.isMAS485())
        {
            for (int i = 0; i < 10; ++i)
            {
                DeviceSettingsFile -> setArrayIndex(i);

                DeviceSettingsFile -> setValue("Enable", dialog.getMASEnable(i));
                if (dialog.getMASEnable(i))
                    DeviceSettingsFile -> setValue("Name", dialog.getMASName(i));
                else
                    DeviceSettingsFile -> setValue("Name", "NO MAC");
            }
        } else
        {
            for (int i = 0; i < 8; ++i)
            {
                DeviceSettingsFile -> setArrayIndex(i);

                DeviceSettingsFile -> setValue("Enable", dialog.getMASEnable(i));
                if (dialog.getMASEnable(i))
                    DeviceSettingsFile -> setValue("Name", dialog.getMASName(i));
                else
                    DeviceSettingsFile -> setValue("Name", "NO MAC");

            }
        }

        DeviceSettingsFile -> endArray();

        DeviceSettingsFile -> setValue("MAS485_OP", dialog.isMAS485());

        DeviceSettingsFile -> sync();

        delete DeviceSettingsFile;
        LoadDevice();


    } else
    {
        qDebug() << "Cancel";
    }
}

void MainWindow::sl_EditDevice()
{
    dAddDevice dialog(false);

    MainSettingFile -> beginReadArray("Device");
    MainSettingFile -> setArrayIndex(currentDeviceNum);

    dialog.setDeviceName(MainSettingFile -> value("devName", "").toString());

    MainSettingFile->endArray();

    dialog.setDeviceNameFile(DeviceSettingsFile -> fileName());
    dialog.LoadDevNameFile();

    if (dialog.exec() == QDialog::Accepted)
    {
        dialog.saveDevNameFile();
    }
}

void MainWindow::sl_newUstavki()
{

    dNewUstavki *newUDialog = new dNewUstavki(true, this);

    int masNumber;

    if (DeviceSettingsFile -> value("MAS485_OP", true).toInt())
    {
        masNumber = 10;
    } else
    {
        masNumber = 8;
    }

    //newUDialog -> set
    int maxUstNumber = DeviceSettingsFile -> beginReadArray("Ustavki");

    DeviceSettingsFile -> endArray();

    qDebug() << "New usstavki number: " << maxUstNumber;

    QString ustName = QInputDialog::getText(this, tr("Добавление файла уставок"), tr("Название уставок:"), QLineEdit::Normal, "");

    qDebug() << "New ustavki name: " << ustName;

    QFileInfo fi(DeviceSettingsFile -> fileName());

    newUDialog -> setUstFile(fi.path() + "//" + ADD_NUMBER("Ust", maxUstNumber) + ".ini");
    newUDialog -> setMAXMASNumber(masNumber);

    for (int i = 0; i < masNumber; i++)
    {
        newUDialog -> setMASEnabel(i, currentDeviceConfig.MAS_Enable[i], currentDeviceConfig.MAS_name[i]);
        QApplication::processEvents();
    }

    for (int i = 0; i < 4; ++i)
    {
        newUDialog -> setBLKEnable(i, currentDeviceConfig.BLK_Enable[i], currentDeviceConfig.BLK_name[i]);
        QApplication::processEvents();
    }

    if (newUDialog -> exec() == QDialog::Accepted)
    {
        newUDialog -> SaveToFile();
        DeviceSettingsFile -> beginWriteArray("Ustavki");
            DeviceSettingsFile -> setArrayIndex(maxUstNumber);
            DeviceSettingsFile -> setValue("ust_Name", ustName);
        DeviceSettingsFile -> endArray();
    }

    DeviceSettingsFile -> sync();

    sl_LoadDevice(currentDeviceNum);

}

void MainWindow::sl_EditUstavki()
{
    dNewUstavki *editUDialog = new dNewUstavki(false, this);

    QApplication::processEvents();

    editUDialog -> setUstFile(currentUSTFile);

    int masNumber;

    if (DeviceSettingsFile -> value("MAS485_OP", true).toInt())
    {
        masNumber = 10;
    } else
    {
        masNumber = 8;
    }

    editUDialog -> setMAXMASNumber(masNumber);

    for (int i = 0; i < masNumber; ++i)
    {

        editUDialog -> setMASEnabel(i, currentDeviceConfig.MAS_Enable[i], currentDeviceConfig.MAS_name[i]);
        QApplication::processEvents();

    }


    for (int i = 0; i < 4; ++i)
    {
        editUDialog -> setBLKEnable(i, currentDeviceConfig.BLK_Enable[i], currentDeviceConfig.BLK_name[i]);
        QApplication::processEvents();
    }


    QApplication::processEvents();

    editUDialog -> loadUstFile();


    if (editUDialog -> exec() == QDialog::Accepted)
    {
        editUDialog -> SaveToFile();
    }
}

void MainWindow::sl_Convert528()
{
    dConvert528 dialog;

    dialog.exec();
}

void MainWindow::sl_LoadDevice(int device)
{
    // В зависимости от выбранного прибора, загрузить в ListWidget (lwSettings) все его уставки
    // Наверное уставки лучше хранить в отдельной папке.
    QListWidgetItem *itm;

    lwSettings -> clear();

    DeviceSettingsFile = new QSettings(QApplication::applicationDirPath() +
                                       "//" + ADD_NUMBER("Dev", device) +
                                       "//device.ini", QSettings::IniFormat);
    DeviceSettingsFile -> setIniCodec("Windows-1251");

    qDebug() << DeviceSettingsFile->fileName();

  //  DeviceSettingsFile -> setIniCodec(QTextCodec::codecForName("utf8"));

    currentDeviceNum = device;

    MainSettingFile -> beginReadArray("Device");
    MainSettingFile -> setArrayIndex(device);
    currentDeviceConfig.Name = MainSettingFile -> value("devName", "").toString();

    MainSettingFile->endArray();

    NumberOfUst = DeviceSettingsFile -> beginReadArray("Ustavki");

    for (int i = 0; i < NumberOfUst; ++i)
    {
        DeviceSettingsFile -> setArrayIndex(i);
//   // qDebug() << UstListFile.value(ADD_NUMBER("UST_name", i), "error").toString();

        itm = new QListWidgetItem(DeviceSettingsFile -> value("ust_Name", "error").toString());
        lwSettings -> addItem(itm);
    }

    DeviceSettingsFile -> endArray();

    currentDeviceConfig.MAS_Number = DeviceSettingsFile -> beginReadArray("MAS");

    for (int i = 0; i < 10; i++)
    {
        DeviceSettingsFile -> setArrayIndex(i);

        currentDeviceConfig.MAS_Enable[i] = DeviceSettingsFile -> value("Enable", false).toBool();
        currentDeviceConfig.MAS_name[i] = DeviceSettingsFile -> value("Name", "NoMAC").toString();
        if (currentDeviceConfig.MAS_Enable[i])
        {
            MAS_label[i] -> setText(currentDeviceConfig.MAS_name[i]);
            MAS_label[i] -> setStyleSheet("color: yellow");
        }
        else
        {
            MAS_label[i] -> setText(QString("NO MAC%1").arg(QString::number(i)));
            MAS_label[i] -> setStyleSheet("color: grey");
        }

    }

   DeviceSettingsFile -> endArray();

   currentDeviceConfig.BLK_number = DeviceSettingsFile -> beginReadArray("BLK");

   for (int i = 0; i < 4; i++)
   {
       DeviceSettingsFile -> setArrayIndex(i);

       currentDeviceConfig.BLK_Enable[i] = DeviceSettingsFile -> value("Enable", false).toBool();
       currentDeviceConfig.BLK_name[i] = DeviceSettingsFile -> value("Name", "NoBLK").toString();

       if (currentDeviceConfig.BLK_Enable[i])
       {
           BLK_label[i] -> setText(currentDeviceConfig.BLK_name[i]);
           BLK_label[i] -> setStyleSheet("color: yellow");
       } else
       {
           BLK_label[i] -> setText(QString("NO BLK%1").arg(QString::number(i + 1)));
           BLK_label[i] -> setStyleSheet("color: grey");
       }

   }
   DeviceSettingsFile -> endArray();



   currentDeviceConfig.DMAS2_Enable = DeviceSettingsFile -> value("DMAS2_Enable", false).toBool();
   currentDeviceConfig.DMAS2_Name = DeviceSettingsFile -> value("DMAS2_Name", "No DM").toString();

   currentDeviceConfig.DMAS6_Enable = DeviceSettingsFile -> value("DMAS6_Enable", false).toBool();
   currentDeviceConfig.DMAS6_Name = DeviceSettingsFile -> value("DMAS6_Name", "No DM").toString();

   if (currentDeviceConfig.DMAS2_Enable)
       lDM2 -> setStyleSheet("color: yellow");
   else
       lDM2 -> setStyleSheet("color: grey");

   if (currentDeviceConfig.DMAS6_Enable)
       lDM6 -> setStyleSheet("color: yellow");
   else
       lDM6 -> setStyleSheet("color: grey");


   // DeviceSettingsFile -> beginGroup("General");

    currentDeviceConfig.MAS485_OP = DeviceSettingsFile -> value("MAS485_OP", false).toBool();

    qDebug() << currentDeviceConfig.MAS485_OP;

    currentDeviceConfig.KadrType = DeviceSettingsFile -> value("KadrType", 0).toInt();
    currentDeviceConfig.KadrInputType = DeviceSettingsFile -> value("KadrInputType", true).toBool();

  // DeviceSettingsFile -> endGroup();

    MainSettingFile -> beginGroup("LastState");
        MainSettingFile -> setValue("Device", device);
        lwSettings -> setCurrentRow(MainSettingFile -> value("Ustavki", 0).toInt());
    MainSettingFile -> endGroup();


}

void MainWindow::sl_SelectUst(int ust)
{
    qDebug() << Q_FUNC_INFO << "ust: " << ust;

    if (ust < 0) return;

    currentUSTFile = QApplication::applicationDirPath() +
            ADD_NUMBER("/dev", currentDeviceNum) +
            ADD_NUMBER("/Ust", ust) + ".ini";

    qDebug() << Q_FUNC_INFO << currentUSTFile;
}

void MainWindow::sl_LoadUst()
{
// Загрузка имитаторов МАС


    QByteArray tmp_array; // Массив для временного хранения буфера
    int FileBufferSize;     // Размер буфера записанный в файл

    int ImmADDR[10];        // Массив адресов имитаторов на ModBus

    int currentMAS = 0;     // Текущий номер МАС-а
    int currentLK = 0;      // Текущий номерк ЛК в МАС-е

    int currentBLK = 0;    // Текущий номер БЛК
    int currentSensor = 0; // Текущий номер датчика БЛК

    int tmp_data;           // переменная для чтения из файла уставок
    int tmp_data2;           // переменная для чтения из файла уставок
    int tmp_data3;           // переменная для чтения из файла уставок
    int tmp_data4;           // переменная для чтения из файла уставок
    int tmp_data5;           // переменная для чтения из файла уставок

    int MAX_MAS;            // Максимальное число МАС-ов в текущей конфигурации
    int MAX_BLK;            // Максимальное число БЛК в текущей конфигурации

    pbLoadSettings -> setDisabled(true);


    for (int i = 0; i < 10; i++)
    {

        if (currentDeviceConfig.MAS_Enable[i])
            MAS_label[i] -> setStyleSheet("color: yellow");
        else
            MAS_label[i] -> setStyleSheet("color: grey");
    }

    for (int i = 0; i < 4; i++)
    {
        if (currentDeviceConfig.BLK_Enable[i])
            BLK_label[i] -> setStyleSheet("color: yellow");
        else
            BLK_label[i] -> setStyleSheet("color: grey");
    }


    if (currentDeviceConfig.DMAS2_Enable)
        lDM2 -> setStyleSheet("color: yellow");
    else
        lDM2 -> setStyleSheet("color: grey");

    if (currentDeviceConfig.DMAS6_Enable)
        lDM6 -> setStyleSheet("color: yellow");
    else
        lDM6 -> setStyleSheet("color: grey");


    lLoading -> setText(tr("Идет загрузка уставок..."));

    sl_AddLog(QString(tr("Начало загрузки уставок")), false);

    QSettings UstFile(currentUSTFile, QSettings::IniFormat);
    UstFile.setIniCodec("Windows-1251");

    if (currentDeviceConfig.MAS485_OP)
    {
        MAX_MAS = 10;
        MAX_BLK = 4;

        ImmADDR[0] =
        ImmADDR[1] =
        ImmADDR[2] =
        ImmADDR[3] = IMM_RS485_1_ADDR;

        ImmADDR[4] =
        ImmADDR[5] =
        ImmADDR[6] =
        ImmADDR[7] = IMM_RS485_2_ADDR;

        ImmADDR[8] =
        ImmADDR[9] = IMM_RS485_3_ADDR;



    } else
    {
        ImmADDR[0] =
        ImmADDR[1] =
        ImmADDR[2] =
        ImmADDR[3] = IMM_OP_1_ADDR;

        ImmADDR[4] =
        ImmADDR[5] =
        ImmADDR[6] =
        ImmADDR[7] = IMM_OP_2_ADDR;


        MAX_MAS = 8;
        MAX_BLK = 0;
    }
    tmp_data = UstFile.beginReadArray("MAS");

    for (currentMAS = 0; currentMAS < MAX_MAS; currentMAS++)
    {
        UstFile.setArrayIndex(currentMAS);

        if (currentDeviceConfig.MAS_Enable[currentMAS])
        {

            UstFile.beginReadArray("LK");

                for (currentLK = 0; currentLK < 4; currentLK++)
                {
                    sl_AddLog(QString(tr("Загрузка %1 ЛК%2")).arg(currentDeviceConfig.MAS_name[currentMAS]).arg(currentLK + 1), true);
                    if (LoadMAS_LKNumber(ImmADDR[currentMAS], currentMAS % 4, currentLK))
                    {
                        errorLoadUstavki(currentMAS, false, false, 10, false);
                        return;
                    }

                    UstFile.setArrayIndex(currentLK);


                    tmp_data = UstFile.value("Duration", 10).toInt();
                    sl_AddLog(QString(tr("Установка задержки в %1")).arg(tmp_data), true);

                    if (LoadDuration(ImmADDR[currentMAS], tmp_data))
                    {
                        errorLoadUstavki(currentMAS, false, false, 10, false);
                        return;
                    }
                    // Грузим массив

                                        tmp_data = UstFile.value("BufferSize", 64).toInt();

                                        FileBufferSize = UstFile.beginReadArray("Buffer");

                                            // Если в файле уставок нет столько данных для буфера

                                            if (tmp_data > FileBufferSize)
                                            {
                                                tmp_data = FileBufferSize;

                                                // Надо еще в очет написать ошибку
                                            }

                                            sl_AddLog(QString(tr("Загрузка буфера размер: %1")).arg(tmp_data), true);

                                            tmp_array.resize(tmp_data);

                                            for(int i = 0; i < tmp_data; i++)
                                            {
                                                UstFile.setArrayIndex(i);
                                                tmp_array[i] = (quint8)UstFile.value("Buf", 0).toInt();
                                            }

                                        UstFile.endArray(); // Вышли из массмва буфера

                                        if (LoadLKBuffer(ImmADDR[currentMAS], tmp_array))
                                        {
                                            errorLoadUstavki(currentMAS, false, false, 10, false);
                                            return;

                                        }

                                        sl_LogResult(true);

// Загрузка адреса ЛК

                    tmp_data = UstFile.value("Addr", 0).toInt();
                    sl_AddLog(QString(tr("Установка адреса в %1")).arg(tmp_data), true);

                    if (LoadLKAddr(ImmADDR[currentMAS], tmp_data))
                    {
                        errorLoadUstavki(currentMAS, false, false, 10, false);
                        return;
                    }

                    sl_LogResult(true);

// Загрузка сумматора

                    tmp_data = UstFile.value("Adder", 0).toInt();
                    sl_AddLog(QString(tr("Установка сумматора в %1")).arg(tmp_data), true);

                    if (LoadLKAdder(ImmADDR[currentMAS], tmp_data))
                    {
                        errorLoadUstavki(currentMAS, false, false, 10, false);
                        return;
                    }

                    sl_LogResult(true);

// Загрузка константы 1

                    if ((currentLK == 0) || (currentLK == 1))
                    {
                        tmp_data = UstFile.value("Const1Pos", 200).toInt();

                        tmp_data2 = UstFile.value("Const1", 0).toInt();
                        sl_AddLog(QString(tr("Установка константы1 значение %1 в канале %2")).arg(tmp_data2).arg(tmp_data), true);
                        if (LoadLKConst1(ImmADDR[currentMAS], tmp_data, tmp_data2))
                        {
                            errorLoadUstavki(currentMAS, false, false, 10, false);
                            return;
                        }

                        sl_LogResult(true);



// Загрузка константы 2

                    tmp_data = UstFile.value("Const2Pos", 200).toInt();

                        tmp_data2 = UstFile.value("Const2", 0).toInt();
                        sl_AddLog(QString(tr("Установка константы2 значение %1 в канале %2")).arg(tmp_data2).arg(tmp_data), true);

                        if (LoadLKConst2(ImmADDR[currentMAS], tmp_data, tmp_data2))
                        {
                            errorLoadUstavki(currentMAS, false, false, 10, false);
                            return;
                        }

                        sl_LogResult(true);
                    }

            }

            UstFile.endArray(); // LK
            MAS_label[currentMAS] -> setStyleSheet("color: green");
        }
    }

    UstFile.endArray();


// Загрузка ЦМ2

    if (currentDeviceConfig.DMAS2_Enable)
    {
        tmp_data = UstFile.value("DMAS/DMAS2_Start", 0).toInt();
        tmp_data2 = UstFile.value("DMAS/DMAS2_Stop", 0).toInt();
        tmp_data3 = UstFile.value("DMAS/DMAS2_INCREMENT", 0).toInt();
        tmp_data4 = UstFile.value("DMAS/DMAS2_DELAY", 0).toInt();

        sl_AddLog(QString(tr("Загрузка ЦМ2")), true);

        if (LoadDMAS2(IMM_DMAS_ADDR, tmp_data, tmp_data2, tmp_data3, tmp_data4))
        {
            errorLoadUstavki(20, true, false, 10, false);
            return;
        }

        sl_LogResult(true);
        lDM2 -> setStyleSheet("color: green");
    }

// Загрузка ЦМ6

    if (currentDeviceConfig.DMAS6_Enable)
    {
        tmp_data = UstFile.value("DMAS/DMAS6_Start", 0).toInt();
        tmp_data2 = UstFile.value("DMAS/DMAS6_Stop", 0).toInt();
        tmp_data3 = UstFile.value("DMAS/DMAS6_INCREMENT", 0).toInt();
        tmp_data4 = UstFile.value("DMAS/DMAS6_DELAY", 0).toInt();

        sl_AddLog(QString(tr("Загрузка ЦМ6")), true);

        if (LoadDMAS6(IMM_DMAS_ADDR, tmp_data, tmp_data2, tmp_data3, tmp_data4))
        {
            errorLoadUstavki(20, false, true, 10, false);
            return;
        }

            sl_LogResult(true);
            lDM6 -> setStyleSheet("color: green");
     }

// Загрузка БЛК

    if (MAX_BLK != 0)
    {
        tmp_data = UstFile.beginReadArray("BLK");

        OnOffBLK(false, false, false, false);

        for (currentBLK = 0; currentBLK < MAX_BLK; currentBLK++)
        {
            UstFile.setArrayIndex(currentBLK);

            if (currentDeviceConfig.BLK_Enable[currentBLK])
            {
                sl_AddLog(QString(tr("Загрузка БЛК%1")).arg(currentBLK + 1), true);
                if (selectBLK(currentBLK))
                {
                    errorLoadUstavki(20, false, false, currentBLK, false);
                    return;
                }

                UstFile.beginReadArray("Sensor");

                    for (currentSensor = 0; currentSensor < 24; currentSensor++)
                    {
                        sl_AddLog(QString(tr("Загрузка датчика%1")).arg(currentSensor + 1), true);
                        UstFile.setArrayIndex(currentSensor);

                        tmp_data = UstFile.value("Freq", 0).toInt();
                        tmp_data2 = UstFile.value("Type", 0).toInt();
                        tmp_data3 = UstFile.value("Param_A", 0).toInt();
                        tmp_data4 = UstFile.value("Param_B", 0).toInt();
                        tmp_data5 = UstFile.value("Param_C", 0).toInt();

                        if (addSensorBLK(tmp_data, currentSensor, tmp_data2, tmp_data3, tmp_data4, tmp_data5))
                        {
                            errorLoadUstavki(20, false, false, currentBLK, false);
                            return;
                        }
                    }
                UstFile.endArray();
                BLK_label[currentBLK] -> setStyleSheet("color: green");
            }
        }

        UstFile.endArray();

        OnOffBLK(currentDeviceConfig.BLK_Enable[0], currentDeviceConfig.BLK_Enable[1],
                 currentDeviceConfig.BLK_Enable[2], currentDeviceConfig.BLK_Enable[3]);
    }

// Загрузка ПЗУ


    QMessageBox::information(this, tr("Загрузка уставок"), tr("Уставки загружены успешно"), QMessageBox::Ok);
    lLoading -> setText(tr("Уставки загружены"));
    pbLoadSettings -> setEnabled(true);
}

void MainWindow::sl_CheckKadr()
{

    waitLoadTimer = new QTimer;

   connect(waitLoadTimer, SIGNAL(timeout()), this, SLOT(sl_FPGALoadiang()));

   dCheckKadr   ChkKadrDialog;

   connect(this, SIGNAL(sFPGALoadOK()), &ChkKadrDialog, SLOT(sl_FPGALoadOk()));

#ifndef __DEBUG__
   if (SelectKadrType(currentDeviceConfig.KadrType, currentDeviceConfig.KadrInputType) != 0)
   {
       QMessageBox::critical(this, tr("Проверка кадра"), tr("Ошибка ТК170"),
                             QMessageBox::Ok);

       delete waitLoadTimer;
       return;
   }
#endif
    waitLoadTimer -> start(500);


   ChkKadrDialog.LoadUstavkiFile(currentUSTFile);

   qDebug() << Q_FUNC_INFO << currentUSTFile;

   ChkKadrDialog.exec();

   if (SelectKadrType(255, true) != 0)
   {
       QMessageBox::critical(this, tr("Проверка кадра"), tr("Ошибка ТК170"),
                             QMessageBox::Ok);
   }

   delete waitLoadTimer;

}

void MainWindow::sl_ViewUst()
{
    dViewUstavki vuDialog;

    vuDialog.LoadUstavkiFile(currentUSTFile);

    vuDialog.exec();

}

void MainWindow::sl_AddLog(QString text, bool WaitForResult)
{

    QTableWidgetItem *timeItem = new QTableWidgetItem(QTime::currentTime().toString());

    newItem = new QTableWidgetItem(text);

    twLog -> setRowCount(currentLogRow + 1);

    twLog -> setItem(currentLogRow, 0, timeItem);
    twLog -> setItem(currentLogRow, 1, newItem);

    if (WaitForResult)
    {
        LogRowForResult = currentLogRow;
    }

    currentLogRow++;
    QApplication::processEvents();
}

void MainWindow::sl_LogResult(bool result)
{
    QTableWidgetItem *resultItem;
    if (result)
        resultItem = new QTableWidgetItem(tr("Ok"));
    else
        resultItem = new QTableWidgetItem(tr("Ошибка"));

    twLog -> setItem(LogRowForResult, 2, resultItem);
}

void MainWindow::sl_clearLog()
{
    twLog -> clear();

    twLog -> setRowCount(0);

    QStringList header;

    header << tr("Время") << tr("Событие") << tr("Результат");

    twLog -> setColumnCount(3);
    twLog -> setHorizontalHeaderLabels(header);

    twLog -> setColumnWidth(1, 277);
    twLog -> setColumnWidth(2, 60);

    currentLogRow = 0;
}

void MainWindow::sl_setRegimeMAS(quint16 slave, quint16 regime)
{

    qDebug() << "Set MAS Regime: " << slave << regime;

    MBResult = 255;

    MBTcp -> WriteHoldingRegister(slave, REGIME_ADDR, regime);

    while (MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
    {
        QMessageBox::critical(this, tr("Ошибка ModBus"), tr("Ошибка протокола ModBus"), QMessageBox::Ok);
    }
}

void MainWindow::sl_LoadIMMMASData(quint16 slave, quint16 addr, quint8 data1, quint8 data2, quint8 data3, quint8 data4)
{

    qDebug() << Q_FUNC_INFO;

    MBResult = 255;

    quint16 tmp_data = addr & 0x00FF;

    MBTcp -> WriteHoldingRegister(slave, SF_ADDR_ADDR, tmp_data);

    while (MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
    {
        QMessageBox::critical(this, tr("Ошибка ТК170"), tr("Ошибка загрузки адреса имитатора"), QMessageBox::Ok);
        return;
    }

    tmp_data = (((quint16)data1 << 8) & 0xFF00) | ((quint16)data2 & 0x00FF);

    MBTcp -> WriteHoldingRegister(slave, SF_INF1_ADDR, tmp_data);

    while (MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
    {
        QMessageBox::critical(this, tr("Ошибка ТК170"), tr("Ошибка загрузки информации 1 имитатора"), QMessageBox::Ok);
        return;
    }

    tmp_data = (((quint16)data3 << 8) & 0xFF00) | ((quint16)data4 & 0x00FF);

    MBTcp -> WriteHoldingRegister(slave, SF_INF2_ADDR, tmp_data);

    while (MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
    {
        QMessageBox::critical(this, tr("Ошибка ТК170"), tr("Ошибка загрузки информации 2 имитатора"), QMessageBox::Ok);
        return;
    }
}

void MainWindow::sl_LoadDM2(int start, int stop, int increment, int delay)
{
    if (LoadDMAS2(IMM_DMAS_ADDR, start, stop, increment, delay) != 0)
    {
        QMessageBox::critical(this, tr("Управление ПК-ИЦМ"), tr("Ошибка загрузки уставок ЦМ2"), QMessageBox::Ok);
    }
}

void MainWindow::sl_LoadDM6(int start, int stop, int increment, int delay)
{
    if (LoadDMAS6(IMM_DMAS_ADDR, start, stop, increment, delay) != 0)
    {
        QMessageBox::critical(this, tr("Управление ПК-ИЦМ"), tr("Ошибка загрузки уставок ЦМ6"), QMessageBox::Ok);
    }
}

void MainWindow::sl_LoadSTMASData(quint16 slave, quint16 addr, quint8 data1, quint8 data2, quint8 data3, quint8 data4)
{
    QByteArray tmp_array; // Массив для временного хранения буфера
    qDebug() << Q_FUNC_INFO;

#define BUFF_SIZE 16

    quint8 ddd[4];

    ddd[0] = data1; ddd[1] = data2; ddd[2] = data3; ddd[3] = data4;

    tmp_array.resize(BUFF_SIZE);

//    for (int i = 0; i < 16; i++)
//        tmp_array[i] = i;
//   // tmp_array.fill(data, 24);

    for (int MASNumber = 0; MASNumber < 4; MASNumber++)
    {
        if (LoadMAS_LKNumber(slave, MASNumber, 0))
        {
            QMessageBox::critical(this, tr("Ошибка ТК170"), tr("Ошибка выбора МАС"), QMessageBox::Ok);
            return;
        }
        if (LoadLKAddr(slave, addr))
        {
            QMessageBox::critical(this, tr("Ошибка ТК170"), tr("Ошибка установки адреса ЛК"), QMessageBox::Ok);
            return;
        }

        if (LoadLKAdder(slave, 0))
        {
            QMessageBox::critical(this, tr("Ошибка ТК170"), tr("Ошибка установки сумматора"), QMessageBox::Ok);
            return;

        }
        if (LoadLKConst1(slave, 255, 0))
        {
            QMessageBox::critical(this, tr("Ошибка ТК170"), tr("Ошибка установки константы 1"), QMessageBox::Ok);
            return;

        }
        if (LoadLKConst2(slave, 255, 0))
        {
            QMessageBox::critical(this, tr("Ошибка ТК170"), tr("Ошибка установки константы 2"), QMessageBox::Ok);
            return;
        }

        tmp_array.fill(ddd[MASNumber], BUFF_SIZE);

        if (LoadLKBuffer(slave, tmp_array))
        {
            QMessageBox::critical(this, tr("Ошибка ТК170"), tr("Ошибка загрузки буфера ЛК"), QMessageBox::Ok);
            return;
        }
    }



}

void MainWindow::sl_ModBusOK()
{
    qDebug() << "ModBusOk";
    MBResult = 0;
}

void MainWindow::sl_ModBusError(int err)
{
    qDebug() << Q_FUNC_INFO << err;
    MBResult = err;
}

void MainWindow::sl_FPGALoadiang()
{

    MBResult = 255;

    quint16 data;


    MBTcp -> ReadInputRegisters(BU_ADDR, BU_KADR_TYPE_ADDR, 1, &data);

    while (MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
    {
        qDebug() << "Error Reading FPGA type";
        return;
    }

    if (data != 0)
    {
        waitLoadTimer -> stop();
        emit sFPGALoadOK();
    } else
      {
        qDebug() << "Error FPGA data ";
      }
}

void MainWindow::sl_LoadPZU(QByteArray data, quint16 type, quint16 pause, bool interliving)
{
    quint16 t_data[21];
    int tsize;
    int tPosition = 0;
    bool sendPack = false;

    while (tPosition < data.size())
    {

        if (interliving && sendPack)
        {
            sendPack = false;
            tsize = 16;
            for (int i = 1; i <= tsize; i++)
                t_data[i] = 0;

        } else
        {
            sendPack = true;
            emit sSetProgress(tPosition);
            qDebug() << "tPosition: " << tPosition;

            if (data.size() >= 32)
                tsize = 16;
            else
                tsize = data.size() / 2;

            qDebug() << "tsize: " << tsize;

            for (int i = 0; i < tsize; i++)
            {
                t_data[i + 1] = (((quint16)data.at((i + tPosition) * 2 + 1) << 8) & 0xFF00) | (((quint16)data.at((i + tPosition)*2)) & 0x00FF);

            }

        }


        t_data[0] = 0x0100;

        MBResult = 255;

        MBTcp -> WriteMultipleHoldingRegisters(IMM_PZU_ADDR, 0, tsize + 1, t_data);

        while(MBResult == 255)
        {
            QApplication::processEvents();
        }

        if (MBResult != 0)
        {
            qDebug() << "Error loading PZU";
            emit sPZULoadError();
            return;
        }

        if (sendPack == true)
        {
            tPosition += tsize * 2;
        }
        if (pause > 0)
            Sleep(pause);

    }

    emit sPZULoadOK();



}

void MainWindow::sl_StartPZU(bool status)
{
    quint16 data;
    MBResult = 255;

    if (status)
    {
        data = 0x0002;
    } else
    {
        data = 0x0001;
    }

    MBTcp -> WriteHoldingRegister(IMM_PZU_ADDR, 0, data);

    while(MBResult == 255)
    {
        QApplication::processEvents();
    }

    if (MBResult != 0)
    {
        qDebug() << "Error starting PZU";
        emit sPZULoadError();
        return;
    }

    emit sPZULoadOK();
}

void MainWindow::sl_OnOffPZU(bool status)
{
    quint16 data[2];
    MBResult = 255;

    if (status)
    {
        data[0] = data[1]= 0x0200;
    } else
    {
        data[0] = data[1] = 0x0100;
    }

    MBTcp -> WriteMultipleHoldingRegisters(IMM_PZU_ADDR, 0, 2, data);

    while(MBResult == 255)
    {
        QApplication::processEvents();
    }

    if (MBResult != 0)
    {
        qDebug() << "Error OnOff PZU";
        emit sPZULoadError();
        return;
    }

    emit sPZULoadOK();
}

void MainWindow::sl_ManualPK_ICM()
{

    dManualPK_ICM Dialog;

    connect(&Dialog, SIGNAL(setDM2(int,int,int,int)),
            this, SLOT(sl_LoadDM2(int,int,int,int)));
    connect(&Dialog, SIGNAL(setDM6(int,int,int,int)),
            this, SLOT(sl_LoadDM6(int,int,int,int)));

    Dialog.exec();
}

void MainWindow::sl_ManualImmMAS_485()
{
}

void MainWindow::sl_ManualImmMAS_Opt()
{
}

void MainWindow::sl_ManualImmBLK()
{
}

void MainWindow::sl_ManualImmPZU()
{
    dManualLoadPZU Dialog;

    connect(this, SIGNAL(sPZULoadOK()), &Dialog, SLOT(LoadingOK()));
    connect(this, SIGNAL(sPZULoadError()), &Dialog, SLOT(LoadingError()));

    connect(this, SIGNAL(sSetProgress(int)), &Dialog, SLOT(setProgress(int)));

    connect(&Dialog, SIGNAL(sWritePZU(QByteArray,quint16, quint16, bool)), this, SLOT(sl_LoadPZU(QByteArray,quint16, quint16, bool)));
    connect(&Dialog, SIGNAL(sStartPZU(bool)), this, SLOT(sl_StartPZU(bool)));
    connect(&Dialog, SIGNAL(sOnOffPZU(bool)), this, SLOT(sl_OnOffPZU(bool)));


    Dialog.fileDir = QApplication::applicationDirPath() + "//" + ADD_NUMBER("Dev", currentDeviceNum);

    Dialog.exec();
}

void MainWindow::sl_ManualReceiver()
{
    dManualReceiver mRcvDialog;
    quint16 data;


    MBResult = 255;

    MBTcp -> ReadInputRegisters(BU_ADDR, BU_KADR_TYPE_ADDR, 1, &data);

    while(MBResult == 255)
        QApplication::processEvents();

    if (MBResult != 0)
    {
        QMessageBox::critical(this, tr("Приемник кадра"), tr("Ошибка чтения текущеко типа кадра"), QMessageBox::Ok);
        return;
    }

    mRcvDialog.setKadrType(data);

    if (mRcvDialog.exec() == QDialog::Accepted)
    {

        data  = mRcvDialog.KadrType;

        MBResult = 255;

        MBTcp -> WriteHoldingRegister(BU_ADDR, BU_KADR_TYPE_ADDR, data);

        while (MBResult == 255)
          QApplication::processEvents();
          if (MBResult != 0)
          {
              QMessageBox::critical(this, tr("Приемник кадра"), tr("Ошибка установки типа кадра"), QMessageBox::Ok);
            return;
          }

         MBResult = 255;

         qDebug() << "InputType: " << mRcvDialog.InputType;

         if (mRcvDialog.InputType == 0)
         {
                MBTcp -> WriteHoldingRegister(IMM_DMAS_ADDR, INPUT_SWITCH_ADDR, 0xFF02);
         } else if (mRcvDialog.InputType == 1)
            {
                MBTcp -> WriteHoldingRegister(IMM_DMAS_ADDR, INPUT_SWITCH_ADDR, 0xFF01);
            } else {
                    MBTcp -> WriteHoldingRegister(IMM_DMAS_ADDR, INPUT_SWITCH_ADDR, 0xFF00);
            }


            while (MBResult == 255)
                QApplication::processEvents();
    }

}

void MainWindow::sl_SelfTest()
{
    dSelfTest dialog;

    connect(&dialog, SIGNAL(sSetRegime(quint16,quint16)),
            this, SLOT(sl_setRegimeMAS(quint16,quint16)));

    connect(&dialog, SIGNAL(sLoadIMMMASData(quint16,quint16,quint8,quint8,quint8,quint8)),
            this, SLOT(sl_LoadIMMMASData(quint16,quint16,quint8,quint8,quint8,quint8)));
    connect(&dialog, SIGNAL(sLoadSTMASData(quint16,quint16,quint8,quint8,quint8,quint8)),
           this, SLOT(sl_LoadSTMASData(quint16,quint16,quint8,quint8,quint8,quint8)));

    dialog.exec();

}
