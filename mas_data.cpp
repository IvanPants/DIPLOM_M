#include "mas_data.h"

MAS_Data::MAS_Data(QString Name, QObject *parent) :
    QObject(parent),
    Loading(false),
    MASName(Name)
{

}

int MAS_Data::LoadData(QString FileName, quint8 MASNumber, bool mas_dm)
{
    QSettings UstavkiFile(FileName, QSettings::IniFormat, this);
    UstavkiFile.setIniCodec("Windows-1251");



    MAS_DM = mas_dm;

    if (mas_dm)
    {

        int MaxMASNum = UstavkiFile.beginReadArray("MAS");

        if (MASNumber > MaxMASNum)
        {
            qDebug() << "Error MASNumber";
            return -1;
        }

        isDualPS1 = UstavkiFile.value("DualPS1", false).toBool();
        isDualPS2 = UstavkiFile.value("DualPS2", false).toBool();

        UstavkiFile.setArrayIndex(MASNumber);

        int MaxLKNumber = UstavkiFile.beginReadArray("LK");

        for (int i = 0; i < MaxLKNumber; i++)
        {
            UstavkiFile.setArrayIndex(i);


            Addr[i] = (quint8)UstavkiFile.value("Addr", 0).toInt();
            Adder[i] = (quint8)UstavkiFile.value("Adder", 0).toInt();
            Const1Pos[i] = (quint16)UstavkiFile.value("Const1Pos", 255).toInt();
            Const1[i] = (quint8)UstavkiFile.value("Const1", 0).toInt();
            Const2Pos[i] = (quint16)UstavkiFile.value("Const2Pos", 255).toInt();
            Const2[i] = (quint8)UstavkiFile.value("Const2", 0).toInt();
            BufferSize[i] = (quint16)UstavkiFile.value("BufferSize", 0).toInt();

            UstavkiFile.beginReadArray("Buffer");

            for (int BuffPos = 0; BuffPos < BufferSize[i]; BuffPos++)
            {
                UstavkiFile.setArrayIndex(BuffPos);

                LK_DATA[i][BuffPos] = (quint8)UstavkiFile.value("buf", 0).toInt();
            }

            UstavkiFile.endArray(); // Buffer
        }

        UstavkiFile.endArray(); // LK

    } else {

        quint8 dm_START;
        quint8 dm_INCREMENT;
        quint8 dm_STOP;

        MasNum = MASNumber;
        Adder[0] = Adder[1] = Adder[2] = Adder[3] = 0;
        BufferSize[0] = BufferSize[1] = BufferSize[2] = BufferSize[3] = 64;
        Const1Pos[0] = Const1Pos[1] = Const1Pos[2] = Const1Pos[3] = 300;
        Const2Pos[0] = Const2Pos[1] = Const2Pos[2] = Const2Pos[3] = 300;


        UstavkiFile.beginGroup("DMAS");

        if (MASNumber == 0) // ЦМ2
        {

            dm_START = UstavkiFile.value("DMAS2_START", 0).toInt();
            dm_INCREMENT = UstavkiFile.value("DMAS2_INCREMENT", 0).toInt();
            dm_STOP = UstavkiFile.value("DMAS2_STOP", 0).toInt();

        } else
        {
            dm_START = UstavkiFile.value("DMAS6_START", 0).toInt();
            dm_INCREMENT = UstavkiFile.value("DMAS6_INCREMENT", 0).toInt();
            dm_STOP = UstavkiFile.value("DMAS6_STOP", 0).toInt();

        }

        LK_DATA[0][0] = LK_DATA[1][0] = LK_DATA[2][0] = LK_DATA[3][0] = dm_START;

        for (int i = 1; i < 64; i++)
        {
            LK_DATA[0][i] = LK_DATA[0][i - 1] + dm_INCREMENT;
            if (LK_DATA[0][i] > dm_STOP)
            {
                LK_DATA[0][i] = dm_START;
            }
        }



    }

return 0; //

}
