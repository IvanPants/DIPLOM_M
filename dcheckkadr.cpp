#include "dcheckkadr.h"

dCheckKadr::dCheckKadr(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);


    InputButtonGroup = new QButtonGroup(this);

    progressBar -> setVisible(false);

    InputButtonGroup -> addButton(rbInputOff, 0);
    InputButtonGroup -> addButton(rbInput1, 1);
    InputButtonGroup -> addButton(rbInput2, 2);
    InputButtonGroup -> addButton(rbInput3, 3);
    InputButtonGroup -> addButton(rbInput4, 4);


// TODO: Поправить из настроек

    mas_input_array[0] = 0;   // Тут всеравно ничего нету
    mas_input_array[1] = 0;   // МАС1
    mas_input_array[2] = 0;   // ЦМ2
    mas_input_array[3] = 1;   // МАС3
    mas_input_array[4] = 2;   // МАС4
    mas_input_array[5] = 3;   // МАС5
    mas_input_array[6] = 1;   // ЦМ6
    mas_input_array[7] = 4;   // МАС71
    mas_input_array[8] = 5;   // МАС72
    mas_input_array[9] = 6;   // МАС81
    mas_input_array[10] = 7;   // МАС82
    mas_input_array[11] = 8;   // МАС9
    mas_input_array[12] = 9;   // МАС10



    //DualPS1[0] =

    send_socket = new QUdpSocket(this);
    recv_socket = new QUdpSocket(this);

    KadrReadTimer = new QTimer;

    connect(KadrReadTimer, SIGNAL(timeout()), this, SLOT(sl_KadrReadTimerTimeOut()));

    recv_socket -> bind(QHostAddress::Any, 3000);

    connect(pbCheckKadr, SIGNAL(clicked()), this, SLOT(sl_CheckKadr()));
    connect(pbSaveResult, SIGNAL(clicked()), this, SLOT(sl_SaveResult()));
    connect(pbClose, SIGNAL(clicked()), this, SLOT(sl_Close()));

    connect(InputButtonGroup, SIGNAL(buttonClicked(int)), SLOT(sl_startRecv(int)));


    setRsvChannel(0);
    qDebug() << Q_FUNC_INFO << "End constructor";

}

void dCheckKadr::UpdateEtalonRTSCM()
{
    int PZUPos = 0;         // Позиция в программе опроса
    int calcPZUPos = 0;     // Позиция в программе опроса с запарралеливания. или как там ее для 7.1 7.2

    bool faz_OK = false;

    QFile ff("kadr_et.txt");

    ff.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream ff_elalon(&ff);

    // Для начала нужно заполнить весь эталонный массив 255.
    // Это оснанеться для необработтаных позиций кадра

    for (int ssk = 0; ssk < 128; ssk++) // Цикл по ССК
    {
        for (int pos = 0; pos < 256; pos++) // Цикл по позиции в кадре
        {
            ImmKadr_PS1[ssk][pos] = 255;
            ImmKadr_PS2[ssk][pos] = 255;
        }
    }

    // Заполняем массив параметров ЛК

    for (int num_in = 1; num_in < 13; num_in++)
    {
        for (int lk = 0; lk < 4; lk++)
        {
            if (mas_input[num_in] == NULL)
                continue;
            lk_param[num_in][lk].Adder_en = mas_input[num_in] -> getAdd_en(lk);
            lk_param[num_in][lk].Addr = mas_input[num_in] -> getAddr(lk);
            lk_param[num_in][lk].Adder = mas_input[num_in] -> getAdder(lk);
            lk_param[num_in][lk].BufferSize = mas_input[num_in] -> getBufferSize(lk);
            lk_param[num_in][lk].current_Adder = 0;
            lk_param[num_in][lk].lk_channel = 0;
            lk_param[num_in][lk].clear_adder = false;
            lk_param[num_in][lk].mas_dm = mas_input[num_in] -> isMAS();

            lk_param[num_in][lk].const1Pos = mas_input[num_in] -> getConst1Pos(lk);
            lk_param[num_in][lk].const1 = mas_input[num_in] -> getConst1(lk);
            lk_param[num_in][lk].const2Pos = mas_input[num_in] -> getConst2Pos(lk);
            lk_param[num_in][lk].const2 = mas_input[num_in] -> getConst2(lk);

        }
    }

    // Проходим по программе опроса по всем ССК и заполняем эталонный массив используя lk_param
    // !!!!!!!!!!!!!!!!!!!!!!! ДЛЯ ПС1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    ff_elalon << QString(tr("=========================================================== ПС1 =========================================\r"));

    PZUPos = 0;
    int currenInput;   // Номер входя МАС
    int currentLK;     // Текущий ЛК в МАС
 //   int ssk_kadr;      // ССК кадра опроса (0..64)

    // TODO: Нужно что-то делать с ПС1 ПС2

    for (int ssk = 0; ssk < 128; ssk++)
    {
     //   ssk_kadr = ssk >> 1;

        for (int pos = 0; pos < 256; pos++)
        {


            calcPZUPos = calculatePZUPos(PZUPos, true, 0);

            ff_elalon << QString::number(ssk, 10) << "[" << QString::number(pos, 10) << "] - " << QString::number(PZUPos, 10) << "("
                      << QString::number(calcPZUPos, 10) << ") ";
            currenInput = PRG_PS1.data_str_m[calcPZUPos].num_in;

            ff_elalon << "Input: " << currenInput;

            er_param_PS1[ssk][pos].MSS_NUM = currenInput;
            er_param_PS1[ssk][pos].LK_ADDR = PRG_PS1.data_str_m[calcPZUPos].addr;

            if (mas_input[currenInput] == NULL)
            {
                er_param_PS1[ssk][pos].MSS_NUM = 255; // Не проверять такой вход
            } else
            {
                if (lk_param[currenInput][0].mas_dm) // Если опрашивается МАС
                {
                    if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][0].Addr)
                    {
                        currentLK = 0;
                    } else if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][1].Addr)
                    {
                        currentLK = 1;
                    } else if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][2].Addr)
                    {
                        currentLK = 2;
                    } else if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][3].Addr)
                    {
                        currentLK = 3;
                    } else
                    {
                        currentLK = 5;
                    }
                } else // Если опрашивается ЦМ
                {
                    currentLK = 0;
                }

                ff_elalon << " Addr: " << PRG_PS1.data_str_m[PZUPos].addr << "(" << currentLK << ") ";

                er_param_PS1[ssk][pos].LK_NUM = currentLK;
                er_param_PS1[ssk][pos].LK_Channel = lk_param[currenInput][currentLK].lk_channel;

                if (currentLK < 4)
                {

                    if (lk_param[currenInput][currentLK].const1Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS1[ssk][pos] = lk_param[currenInput][currentLK].const1;
                    else if (lk_param[currenInput][currentLK].const2Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS1[ssk][pos] = lk_param[currenInput][currentLK].const2;
                    else
                        ImmKadr_PS1[ssk][pos] = mas_input[currenInput] -> getLKData(currentLK,
                                                                      lk_param[currenInput][currentLK].lk_channel) + lk_param[currenInput][currentLK].current_Adder;

                    ff_elalon << "Data: " << ImmKadr_PS1[ssk][pos] << " (" << lk_param[currenInput][currentLK].lk_channel << ") ";

                    lk_param[currenInput][currentLK].lk_channel++;

                    if (PRG_PS1.data_str_m[calcPZUPos].pf == 1) // Если стои признак фазировки
                    {
//                        qDebug() << "LKI: " << currenInput << "gl: " << PRG_PS1.data_str[PZUPos].gl_faz << "PZU: " << PZUPos
//                             << PRG_PS1.data_str[PZUPos].addr;

                        ff_elalon << "FAZ(" << PRG_PS1.data_str_m[PZUPos].gl_faz << ") ";

                        if (((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 0) && (((ssk + 1) % 2) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 1) && (((ssk + 1) % 4) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 2) && (((ssk + 1) % 8) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 3) && (((ssk + 1) % 16) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 4) && (((ssk + 1) % 32) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 5) && (((ssk + 1) % 64) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 6) && (((ssk + 1) % 128) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 7) && (((ssk + 1) % 128) == 0)) ||
                        (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                            )
                        { // Если фазировка в текущем кадре
                            ff_elalon << "O ";
                            faz_OK = true;
                        } else
                        {
                            ff_elalon << "X ";
                            faz_OK = false;
                        }
                    } else
                    {
                        if (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                        { // Если дошли до конца буфера
                            ff_elalon << "Bend ";
                            faz_OK = true;
                        } else
                        {
                            faz_OK = false;
                        }
                    }

               //qDebug() << faz_OK;

                    if (faz_OK)
                    { // Фазируемся
                        faz_OK = false;
                        lk_param[currenInput][currentLK].lk_channel = 0;

                        lk_param[currenInput][currentLK].current_Adder += lk_param[currenInput][currentLK].Adder; // Прибавляем сумматор

                        if (currentLK == 3) // Фазировка в 4 ЛК. Сброс всех сумматоров
                        {
                            lk_param[currenInput][currentLK].current_Adder = 0; // Сбрасываем текущий суматор
                            lk_param[currenInput][0].clear_adder = true;        // Ставим признак сброса суматора в других каналах
                            lk_param[currenInput][1].clear_adder = true;
                            lk_param[currenInput][2].clear_adder = true;
                        } else
                        {
                            if (lk_param[currenInput][currentLK].clear_adder)        // Если есть признак сброса сумматора
                            {
                                lk_param[currenInput][currentLK].current_Adder = 0;
                                lk_param[currenInput][currentLK].clear_adder = false;
                            }
                        }
                    } else
                    {

                    }
                } else
                {
                    ff_elalon << "No LK";
                    ImmKadr_PS1[ssk][pos] = 255;
                }

            }
    // Преключаем на следующую позицию ПЗУ
        PZUPos++;
        if (PZUPos == 1024)
            PZUPos = 0;

        ff_elalon << "\r";
    }
    }

    qDebug() << PZUPos;
    // Проходим опять по программе опроса по всем ССК и повторно заполняем эталонный массив ничего не трогая

    for (int ssk = 0; ssk < 128; ssk++)
    {
     //   ssk_kadr = ssk >> 1;

        for (int pos = 0; pos < 256; pos++)
        {

            calcPZUPos = calculatePZUPos(PZUPos, true, 0);

            ff_elalon << QString::number(ssk, 10) << "[" << QString::number(pos, 10) << "] - " << QString::number(PZUPos, 10) << " ";
            currenInput = PRG_PS1.data_str_m[calcPZUPos].num_in;

            ff_elalon << "Input: " << currenInput;

            er_param_PS1[ssk][pos].MSS_NUM = currenInput;
            er_param_PS1[ssk][pos].LK_ADDR = PRG_PS1.data_str_m[calcPZUPos].addr;

            if (mas_input[currenInput] == NULL)
            {
                er_param_PS1[ssk][pos].MSS_NUM = 255;
            } else
            {

                if (lk_param[currenInput][0].mas_dm) // Если опрашивается МАС
                {
                    if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][0].Addr)
                    {
                        currentLK = 0;
                    } else if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][1].Addr)
                    {
                        currentLK = 1;
                    } else if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][2].Addr)
                    {
                        currentLK = 2;
                    } else if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][3].Addr)
                    {
                        currentLK = 3;
                    } else
                    {
                        currentLK = 5;
                    }
                } else // Если опрашивается ЦМ
                {
                    currentLK = 0;
                }

                ff_elalon << "Addr: " << PRG_PS1.data_str_m[PZUPos].addr << "(" << currentLK << ") ";

                er_param_PS1[ssk][pos].LK_NUM = currentLK;
                er_param_PS1[ssk][pos].LK_Channel = lk_param[currenInput][currentLK].lk_channel;

                if (currentLK < 4)
                {

                    if (lk_param[currenInput][currentLK].const1Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS1[ssk][pos] = lk_param[currenInput][currentLK].const1;
                    else if (lk_param[currenInput][currentLK].const2Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS1[ssk][pos] = lk_param[currenInput][currentLK].const2;
                    else
                        ImmKadr_PS1[ssk][pos] = mas_input[currenInput] -> getLKData(currentLK,
                                                                      lk_param[currenInput][currentLK].lk_channel) + lk_param[currenInput][currentLK].current_Adder;

                    ff_elalon << "Data: " << ImmKadr_PS1[ssk][pos];

                    lk_param[currenInput][currentLK].lk_channel++;

                    if (PRG_PS1.data_str_m[calcPZUPos].pf == 1) // Если стои признак фазировки
                    {
    //                    qDebug() << "LKI: " << currenInput << "gl: " << PRG_PS1.data_str[PZUPos].gl_faz << "PZU: " << PZUPos
    //                             << PRG_PS1.data_str[PZUPos].addr;

                        ff_elalon << "FAZ(" << PRG_PS1.data_str_m[PZUPos].gl_faz << ") ";

                        if (((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 0) && (((ssk + 1) % 2) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 1) && (((ssk + 1) % 4) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 2) && (((ssk + 1) % 8) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 3) && (((ssk + 1) % 16) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 4) && (((ssk + 1) % 32) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 5) && (((ssk + 1) % 64) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 6) && (((ssk + 1) % 128) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 7) && (((ssk + 1) % 128) == 0)) ||
                           (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                            )
                        { // Если фазировка в текущем кадре
                            ff_elalon << "O ";
                            faz_OK = true;
                        } else
                        {
                            ff_elalon << "X ";
                            faz_OK = false;
                        }
                    } else
                    {
                        if (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                        { // Если дошли до конца буфера
                            ff_elalon << "Bend ";
                            faz_OK = true;
                        } else
                        {
                            faz_OK = false;
                        }
                    }

               //qDebug() << faz_OK;

                    if (faz_OK)
                    { // Фазируемся
                        faz_OK = false;
                        lk_param[currenInput][currentLK].lk_channel = 0;

                        lk_param[currenInput][currentLK].current_Adder += lk_param[currenInput][currentLK].Adder; // Прибавляем сумматор

                        if (currentLK == 3) // Фазировка в 4 ЛК. Сброс всех сумматоров
                        {
                            lk_param[currenInput][currentLK].current_Adder = 0; // Сбрасываем текущий суматор
                            lk_param[currenInput][0].clear_adder = true;        // Ставим признак сброса суматора в других каналах
                            lk_param[currenInput][1].clear_adder = true;
                            lk_param[currenInput][2].clear_adder = true;
                        } else
                        {
                            if (lk_param[currenInput][currentLK].clear_adder)        // Если есть признак сброса сумматора
                            {
                                lk_param[currenInput][currentLK].current_Adder = 0;
                                lk_param[currenInput][currentLK].clear_adder = false;
                            }
                        }
                    } else
                    {

                    }
                } else
                {
                    ff_elalon << "No LK";
                    ImmKadr_PS1[ssk][pos] = 255;
                }

            }
    // Преключаем на следующую позицию ПЗУ
        PZUPos++;
        if (PZUPos == 1024)
            PZUPos = 0;

        ff_elalon << "\r";
    }
    }


    // Проходим по программе опроса по всем ССК и заполняем эталонный массив используя lk_param
    // !!!!!!!!!!!!!!!!!!!!!!! ДЛЯ ПС1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    ff_elalon << QString(tr("=========================================================== ПС2 =========================================\r"));

    PZUPos = 0;

    for (int ssk = 0; ssk < 128; ssk++)
    {
     //   ssk_kadr = ssk >> 1;

        for (int pos = 0; pos < 256; pos++)
        {

            calcPZUPos = calculatePZUPos(PZUPos, false, 0);

            ff_elalon << QString::number(ssk, 10) << "[" << QString::number(pos, 10) << "] - " << QString::number(PZUPos, 10) << " ";
            currenInput = PRG_PS2.data_str_m[calcPZUPos].num_in;

            ff_elalon << "Input: " << currenInput;

            er_param_PS2[ssk][pos].MSS_NUM = currenInput;
            er_param_PS2[ssk][pos].LK_ADDR = PRG_PS2.data_str_m[calcPZUPos].addr;

            if (mas_input[currenInput] == NULL)
            {
                er_param_PS2[ssk][pos].MSS_NUM = 255;
            } else
            {

                if (lk_param[currenInput][0].mas_dm) // Если опрашивается МАС
                {
                    if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][0].Addr)
                    {
                        currentLK = 0;
                    } else if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][1].Addr)
                    {
                        currentLK = 1;
                    } else if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][2].Addr)
                    {
                        currentLK = 2;
                    } else if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][3].Addr)
                    {
                        currentLK = 3;
                    } else
                    {
                        currentLK = 5;
                    }
                } else // Если опрашивается ЦМ
                {
                    currentLK = 0;
                }

                ff_elalon << " Addr: " << PRG_PS2.data_str_m[PZUPos].addr << "(" << currentLK << ") ";

                er_param_PS2[ssk][pos].LK_NUM = currentLK;
                er_param_PS2[ssk][pos].LK_Channel = lk_param[currenInput][currentLK].lk_channel;

                if (currentLK < 4)
                {

                    if (lk_param[currenInput][currentLK].const1Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS2[ssk][pos] = lk_param[currenInput][currentLK].const1;
                    else if (lk_param[currenInput][currentLK].const2Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS2[ssk][pos] = lk_param[currenInput][currentLK].const2;
                    else
                        ImmKadr_PS2[ssk][pos] = mas_input[currenInput] -> getLKData(currentLK,
                                                                      lk_param[currenInput][currentLK].lk_channel) + lk_param[currenInput][currentLK].current_Adder;

                    ff_elalon << "Data: " << ImmKadr_PS2[ssk][pos] << " (" << lk_param[currenInput][currentLK].lk_channel << ") ";

                    lk_param[currenInput][currentLK].lk_channel++;

                    if (PRG_PS2.data_str_m[calcPZUPos].pf == 1) // Если стои признак фазировки
                    {
//                        qDebug() << "LKI: " << currenInput << "gl: " << PRG_PS1.data_str[PZUPos].gl_faz << "PZU: " << PZUPos
//                             << PRG_PS1.data_str[PZUPos].addr;

                        ff_elalon << "FAZ(" << PRG_PS2.data_str_m[PZUPos].gl_faz << ") ";

                        if (((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 0) && (((ssk + 1) % 2) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 1) && (((ssk + 1) % 4) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 2) && (((ssk + 1) % 8) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 3) && (((ssk + 1) % 16) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 4) && (((ssk + 1) % 32) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 5) && (((ssk + 1) % 64) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 6) && (((ssk + 1) % 128) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 7) && (((ssk + 1) % 128) == 0)) ||
                        (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                            )
                        { // Если фазировка в текущем кадре
                            ff_elalon << "O ";
                            faz_OK = true;
                        } else
                        {
                            ff_elalon << "X ";
                            faz_OK = false;
                        }
                    } else
                    {
                        if (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                        { // Если дошли до конца буфера
                            ff_elalon << "Bend ";
                            faz_OK = true;
                        } else
                        {
                            faz_OK = false;
                        }
                    }

               //qDebug() << faz_OK;

                    if (faz_OK)
                    { // Фазируемся
                        faz_OK = false;
                        lk_param[currenInput][currentLK].lk_channel = 0;

                        lk_param[currenInput][currentLK].current_Adder += lk_param[currenInput][currentLK].Adder; // Прибавляем сумматор

                        if (currentLK == 3) // Фазировка в 4 ЛК. Сброс всех сумматоров
                        {
                            lk_param[currenInput][currentLK].current_Adder = 0; // Сбрасываем текущий суматор
                            lk_param[currenInput][0].clear_adder = true;        // Ставим признак сброса суматора в других каналах
                            lk_param[currenInput][1].clear_adder = true;
                            lk_param[currenInput][2].clear_adder = true;
                        } else
                        {
                            if (lk_param[currenInput][currentLK].clear_adder)        // Если есть признак сброса сумматора
                            {
                                lk_param[currenInput][currentLK].current_Adder = 0;
                                lk_param[currenInput][currentLK].clear_adder = false;
                            }
                        }
                    } else
                    {

                    }
                } else
                {
                    ff_elalon << "No LK";
                    ImmKadr_PS2[ssk][pos] = 255;
                }

            }
    // Преключаем на следующую позицию ПЗУ
        PZUPos++;
        if (PZUPos == 1024)
            PZUPos = 0;

        ff_elalon << "\r";
    }
    }

    qDebug() << PZUPos;
    // Проходим опять по программе опроса по всем ССК и повторно заполняем эталонный массив ничего не трогая

    for (int ssk = 0; ssk < 128; ssk++)
    {
     //   ssk_kadr = ssk >> 1;

        for (int pos = 0; pos < 256; pos++)
        {
            calcPZUPos = calculatePZUPos(PZUPos, false, 0);

            ff_elalon << QString::number(ssk, 10) << "[" << QString::number(pos, 10) << "] - " << QString::number(PZUPos, 10) << " ";
            currenInput = PRG_PS2.data_str_m[calcPZUPos].num_in;

            ff_elalon << "Input: " << currenInput;

            er_param_PS2[ssk][pos].MSS_NUM = currenInput;
            er_param_PS2[ssk][pos].LK_ADDR = PRG_PS2.data_str_m[calcPZUPos].addr;

            if (mas_input[currenInput] == NULL)
            {
                er_param_PS2[ssk][pos].MSS_NUM = 255;
            } else
            {


                if (lk_param[currenInput][0].mas_dm) // Если опрашивается МАС
                {
                    if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][0].Addr)
                    {
                        currentLK = 0;
                    } else if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][1].Addr)
                    {
                        currentLK = 1;
                    } else if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][2].Addr)
                    {
                        currentLK = 2;
                    } else if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][3].Addr)
                    {
                        currentLK = 3;
                    } else
                    {
                        currentLK = 5;
                    }
                } else // Если опрашивается ЦМ
                {
                    currentLK = 0;
                }

                ff_elalon << "Addr: " << PRG_PS2.data_str_m[PZUPos].addr << "(" << currentLK << ") ";

                er_param_PS2[ssk][pos].LK_NUM = currentLK;
                er_param_PS2[ssk][pos].LK_Channel = lk_param[currenInput][currentLK].lk_channel;

                if (currentLK < 4)
                {

                    if (lk_param[currenInput][currentLK].const1Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS2[ssk][pos] = lk_param[currenInput][currentLK].const1;
                    else if (lk_param[currenInput][currentLK].const2Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS2[ssk][pos] = lk_param[currenInput][currentLK].const2;
                    else
                        ImmKadr_PS2[ssk][pos] = mas_input[currenInput] -> getLKData(currentLK,
                                                                      lk_param[currenInput][currentLK].lk_channel) + lk_param[currenInput][currentLK].current_Adder;

                    ff_elalon << "Data: " << ImmKadr_PS2[ssk][pos];

                    lk_param[currenInput][currentLK].lk_channel++;

                    if (PRG_PS2.data_str_m[calcPZUPos].pf == 1) // Если стои признак фазировки
                    {
//                        qDebug() << "LKI: " << currenInput << "gl: " << PRG_PS1.data_str[PZUPos].gl_faz << "PZU: " << PZUPos
//                                 << PRG_PS1.data_str[PZUPos].addr;

                        ff_elalon << "FAZ(" << PRG_PS2.data_str_m[PZUPos].gl_faz << ") ";

                        if (((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 0) && (((ssk + 1) % 2) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 1) && (((ssk + 1) % 4) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 2) && (((ssk + 1) % 8) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 3) && (((ssk + 1) % 16) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 4) && (((ssk + 1) % 32) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 5) && (((ssk + 1) % 64) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 6) && (((ssk + 1) % 128) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 7) && (((ssk + 1) % 128) == 0)) ||
                           (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                            )
                        { // Если фазировка в текущем кадре
                            ff_elalon << "O ";
                            faz_OK = true;
                        } else
                        {
                            ff_elalon << "X ";
                            faz_OK = false;
                        }
                    } else
                    {
                        if (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                        { // Если дошли до конца буфера
                            ff_elalon << "Bend ";
                            faz_OK = true;
                        } else
                        {
                            faz_OK = false;
                        }
                    }

                   //qDebug() << faz_OK;

                    if (faz_OK)
                    { // Фазируемся
                        faz_OK = false;
                        lk_param[currenInput][currentLK].lk_channel = 0;

                        lk_param[currenInput][currentLK].current_Adder += lk_param[currenInput][currentLK].Adder; // Прибавляем сумматор

                        if (currentLK == 3) // Фазировка в 4 ЛК. Сброс всех сумматоров
                        {
                            lk_param[currenInput][currentLK].current_Adder = 0; // Сбрасываем текущий суматор
                            lk_param[currenInput][0].clear_adder = true;        // Ставим признак сброса суматора в других каналах
                            lk_param[currenInput][1].clear_adder = true;
                            lk_param[currenInput][2].clear_adder = true;
                        } else
                        {
                            if (lk_param[currenInput][currentLK].clear_adder)        // Если есть признак сброса сумматора
                            {
                                lk_param[currenInput][currentLK].current_Adder = 0;
                                lk_param[currenInput][currentLK].clear_adder = false;
                            }
                        }
                    } else
                    {

                    }
                } else
                {
                    ff_elalon << "No LK";
                    ImmKadr_PS2[ssk][pos] = 255;
                }

            }
    // Преключаем на следующую позицию ПЗУ
        PZUPos++;
        if (PZUPos == 1024)
            PZUPos = 0;

        ff_elalon << "\r";
    }
    }


}

void dCheckKadr::UpdateEtalonRTSC()
{
    int PZUPos = 0;         // Позиция в программе опроса
    int calcPZUPos = 0;     // Позиция в программе опроса с запарралеливания. или как там ее для 7.1 7.2

    bool faz_OK = false;

    QFile ff("kadr_et.txt");

    ff.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream ff_elalon(&ff);

    // Для начала нужно заполнить весь эталонный массив 255.
    // Это оснанеться для необработтаных позиций кадра

    for (int ssk = 0; ssk < 128; ssk++) // Цикл по ССК
    {
        for (int pos = 0; pos < 512; pos++) // Цикл по позиции в кадре
        {
            ImmKadr_PS1[ssk][pos] = 255;
            ImmKadr_PS2[ssk][pos] = 255;
        }
    }

    // Заполняем массив параметров ЛК

    for (int num_in = 1; num_in < 13; num_in++)
    {
        if (mas_input[num_in] == NULL)
            continue;

        for (int lk = 0; lk < 4; lk++)
        {
            lk_param[num_in][lk].Adder_en = mas_input[num_in] -> getAdd_en(lk);
            lk_param[num_in][lk].Addr = mas_input[num_in] -> getAddr(lk);
            lk_param[num_in][lk].Adder = mas_input[num_in] -> getAdder(lk);
            lk_param[num_in][lk].BufferSize = mas_input[num_in] -> getBufferSize(lk);
            lk_param[num_in][lk].current_Adder = 0;
            lk_param[num_in][lk].lk_channel = 0;
            lk_param[num_in][lk].clear_adder = false;
            lk_param[num_in][lk].mas_dm = mas_input[num_in] -> isMAS();

            lk_param[num_in][lk].const1Pos = mas_input[num_in] -> getConst1Pos(lk);
            lk_param[num_in][lk].const1 = mas_input[num_in] -> getConst1(lk);
            lk_param[num_in][lk].const2Pos = mas_input[num_in] -> getConst2Pos(lk);
            lk_param[num_in][lk].const2 = mas_input[num_in] -> getConst2(lk);

        }
    }

    // Проходим по программе опроса по всем ССК и заполняем эталонный массив используя lk_param
    // !!!!!!!!!!!!!!!!!!!!!!! ДЛЯ ПС1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    ff_elalon << QString(tr("=========================================================== ПС1 =========================================\r"));

    PZUPos = 0;
    int currenInput = 0;   // Номер входя МАС
    int currentLK = 0;     // Текущий ЛК в МАС
    int currentLogicLK = 1; // Логический номер входа для выделения ЦМ


    for (int ssk = 0; ssk < 128; ssk++)
    {

        for (int pos = 0; pos < 512; pos++)
        {

            //qDebug() << "ssk: " << ssk << "pos: " << pos;
            ff_elalon << QString::number(ssk, 10) << "[" << QString::number(pos, 10) << "] - " << QString::number(PZUPos, 10);
            // << "(" << QString::number(calcPZUPos, 10) << ") ";


            if (PRG_PS1.data_str_r[PZUPos].zap_mac == 1)  // Если идет обращение к ЦМ
            {
                if (currentLogicLK == 2)
                {
                    // Идет ЦМ2
                    currenInput = 2;
                    currentLK = 0;
                    calcPZUPos = PZUPos;
                } else

                if (currentLogicLK == 6)
                {
                    // Идет ЦМ6
                    currenInput = 6;
                    currentLK = 0;
                    calcPZUPos = PZUPos;
                } else
                {
                    currenInput = 255;
                    currentLK = 255;
                    calcPZUPos = PZUPos;
                }

                ff_elalon << "(" << QString::number(calcPZUPos, 10) << ") ";
            } else
            {
                // Идет обращение к МАС

                 currenInput = mas_input_convert[PRG_PS1.data_str_r[PZUPos].num_in];

                calcPZUPos = calculatePZUPos(PZUPos, true, PRG_PS1.data_str_r[PZUPos].num_in);
                ff_elalon << "(" << QString::number(calcPZUPos, 10) << ") ";

                if ((currenInput != 255) && lk_param[currenInput][0].mas_dm) // Если опрашивается МАС
                {
                    if (PRG_PS1.data_str_r[calcPZUPos].addr == lk_param[currenInput][0].Addr)
                    {
                        currentLK = 0;
                    } else if (PRG_PS1.data_str_r[calcPZUPos].addr == lk_param[currenInput][1].Addr)
                    {
                        currentLK = 1;
                    } else if (PRG_PS1.data_str_r[calcPZUPos].addr == lk_param[currenInput][2].Addr)
                    {
                        currentLK = 2;
                    } else if (PRG_PS1.data_str_r[calcPZUPos].addr == lk_param[currenInput][3].Addr)
                    {
                        currentLK = 3;
                    } else
                    {
                        currentLK = 5;
                    }
                } else
                  {
                    currentLK = 5;
                  }

            }

            er_param_PS1[ssk][pos].LK_NUM = currentLK;

            er_param_PS1[ssk][pos].MSS_NUM = currenInput;
            er_param_PS1[ssk][pos].LK_ADDR = PRG_PS1.data_str_r[calcPZUPos].addr;

            if (currenInput != 255)
            {
                ff_elalon << "Input: " << mas_input[currenInput]->getMASName() << "(" << currenInput << ")";
                ff_elalon << " Addr: " << PRG_PS1.data_str_r[calcPZUPos].addr << "(" << currentLK << ") ";
                er_param_PS1[ssk][pos].LK_Channel = lk_param[currenInput][currentLK].lk_channel;
            }

            if (currentLK < 4)
            {

                if (lk_param[currenInput][currentLK].const1Pos == lk_param[currenInput][currentLK].lk_channel)
                    ImmKadr_PS1[ssk][pos] = lk_param[currenInput][currentLK].const1;
                else if (lk_param[currenInput][currentLK].const2Pos == lk_param[currenInput][currentLK].lk_channel)
                    ImmKadr_PS1[ssk][pos] = lk_param[currenInput][currentLK].const2;
                else
                    ImmKadr_PS1[ssk][pos] = mas_input[currenInput] -> getLKData(currentLK,
                                                                  lk_param[currenInput][currentLK].lk_channel) + lk_param[currenInput][currentLK].current_Adder;

                ff_elalon << "Data: " << ImmKadr_PS1[ssk][pos] << " (" << lk_param[currenInput][currentLK].lk_channel << ") ";

                lk_param[currenInput][currentLK].lk_channel++;

                if (PRG_PS1.data_str_r[calcPZUPos].pf == 1) // Если стои признак фазировки
                {
//                    qDebug() << "LKI: " << currenInput << "gl: " << PRG_PS1.data_str[PZUPos].gl_faz << "PZU: " << PZUPos
//                             << PRG_PS1.data_str[PZUPos].addr;

                    ff_elalon << "FAZ(" << PRG_PS1.data_str_r[calcPZUPos].gl_faz << ") ";

                    if ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 0) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 1) && (((ssk + 1) % 2) == 0)) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 2) && (((ssk + 1) % 4) == 0)) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 3) && (((ssk + 1) % 8) == 0)) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 4) && (((ssk + 1) % 16) == 0)) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 5) && (((ssk + 1) % 32) == 0)) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 6) && (((ssk + 1) % 64) == 0)) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 7) && (((ssk + 1) % 128) == 0)) ||
                       (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                        )
                    { // Если фазировка в текущем кадре
                        ff_elalon << "O ";
                        faz_OK = true;
                    } else
                    {
                        ff_elalon << "X ";
                        faz_OK = false;
                    }
                } else
                {
                    if (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                    { // Если дошли до конца буфера
                        ff_elalon << "Bend ";
                        faz_OK = true;
                    } else
                    {
                        faz_OK = false;
                    }
                }

               //qDebug() << faz_OK;

                if (faz_OK)
                { // Фазируемся
                    faz_OK = false;
                    lk_param[currenInput][currentLK].lk_channel = 0;

                    lk_param[currenInput][currentLK].current_Adder += lk_param[currenInput][currentLK].Adder; // Прибавляем сумматор

                    if (currentLK == 3) // Фазировка в 4 ЛК. Сброс всех сумматоров
                    {
                        lk_param[currenInput][currentLK].current_Adder = 0; // Сбрасываем текущий суматор
                        lk_param[currenInput][0].clear_adder = true;        // Ставим признак сброса суматора в других каналах
                        lk_param[currenInput][1].clear_adder = true;
                        lk_param[currenInput][2].clear_adder = true;
                    } else
                    {
                        if (lk_param[currenInput][currentLK].clear_adder)        // Если есть признак сброса сумматора
                        {
                            lk_param[currenInput][currentLK].current_Adder = 0;
                            lk_param[currenInput][currentLK].clear_adder = false;
                        }
                    }
                } else
                {

                }
            } else
            {
                ff_elalon << "No LK";
                ImmKadr_PS1[ssk][pos] = 255;
            }


    // Преключаем на следующую позицию ПЗУ

        //    qDebug() << "00000000";
        PZUPos++;
        if (PZUPos == 1024)
            PZUPos = 0;

        currentLogicLK++;
        if (currentLogicLK == 9)
            currentLogicLK = 1;
        ff_elalon << "\r";
        }
    }

    qDebug() << PZUPos;
    // Проходим опять по программе опроса по всем ССК и повторно заполняем эталонный массив ничего не трогая

    for (int ssk = 0; ssk < 128; ssk++)
    {

        for (int pos = 0; pos < 512; pos++)
        {


            ff_elalon << QString::number(ssk, 10) << "[" << QString::number(pos, 10) << "] - " << QString::number(PZUPos, 10);
            // << "(" << QString::number(calcPZUPos, 10) << ") ";


            if (PRG_PS1.data_str_r[PZUPos].zap_mac == 1)  // Если идет обращение к ЦМ
            {
                if (currentLogicLK == 2)
                {
                    // Идет ЦМ2
                    currenInput = 2;
                    currentLK = 0;
                    calcPZUPos = PZUPos;
                } else

                if (currentLogicLK == 6)
                {
                    // Идет ЦМ6
                    currenInput = 6;
                    currentLK = 0;
                    calcPZUPos = PZUPos;
                } else
                {
                    currenInput = 255;
                    currentLK = 255;
                    calcPZUPos = PZUPos;
                }

                ff_elalon << "(" << QString::number(calcPZUPos, 10) << ") ";
            } else
            {
                // Идет обращение к МАС

                currenInput = mas_input_convert[PRG_PS1.data_str_r[PZUPos].num_in];

                calcPZUPos = calculatePZUPos(PZUPos, true, PRG_PS1.data_str_r[PZUPos].num_in);
                ff_elalon << "(" << QString::number(calcPZUPos, 10) << ") ";

                if ((currenInput != 255) && lk_param[currenInput][0].mas_dm) // Если опрашивается МАС
                {
                    if (PRG_PS1.data_str_r[calcPZUPos].addr == lk_param[currenInput][0].Addr)
                    {
                        currentLK = 0;
                    } else if (PRG_PS1.data_str_r[calcPZUPos].addr == lk_param[currenInput][1].Addr)
                    {
                        currentLK = 1;
                    } else if (PRG_PS1.data_str_r[calcPZUPos].addr == lk_param[currenInput][2].Addr)
                    {
                        currentLK = 2;
                    } else if (PRG_PS1.data_str_r[calcPZUPos].addr == lk_param[currenInput][3].Addr)
                    {
                        currentLK = 3;
                    } else
                    {
                        currentLK = 5;
                    }
                } else
                  {
                    currentLK = 5;
                  }

            }

            er_param_PS1[ssk][pos].LK_NUM = currentLK;

            er_param_PS1[ssk][pos].MSS_NUM = currenInput;
            er_param_PS1[ssk][pos].LK_ADDR = PRG_PS1.data_str_r[calcPZUPos].addr;

            if (currenInput != 255)
            {
                ff_elalon << "Input: " << mas_input[currenInput]->getMASName() << "(" << currenInput << ")";
                ff_elalon << " Addr: " << PRG_PS1.data_str_r[calcPZUPos].addr << "(" << currentLK << ") ";
                er_param_PS1[ssk][pos].LK_Channel = lk_param[currenInput][currentLK].lk_channel;
            }

            if (currentLK < 4)
            {

                if (lk_param[currenInput][currentLK].const1Pos == lk_param[currenInput][currentLK].lk_channel)
                    ImmKadr_PS1[ssk][pos] = lk_param[currenInput][currentLK].const1;
                else if (lk_param[currenInput][currentLK].const2Pos == lk_param[currenInput][currentLK].lk_channel)
                    ImmKadr_PS1[ssk][pos] = lk_param[currenInput][currentLK].const2;
                else
                    ImmKadr_PS1[ssk][pos] = mas_input[currenInput] -> getLKData(currentLK,
                                                                  lk_param[currenInput][currentLK].lk_channel) + lk_param[currenInput][currentLK].current_Adder;

                ff_elalon << "Data: " << ImmKadr_PS1[ssk][pos] << " (" << lk_param[currenInput][currentLK].lk_channel << ") ";

                lk_param[currenInput][currentLK].lk_channel++;

                if (PRG_PS1.data_str_r[calcPZUPos].pf == 1) // Если стои признак фазировки
                {
//                    qDebug() << "LKI: " << currenInput << "gl: " << PRG_PS1.data_str[PZUPos].gl_faz << "PZU: " << PZUPos
//                             << PRG_PS1.data_str[PZUPos].addr;

                    ff_elalon << "FAZ(" << PRG_PS1.data_str_r[calcPZUPos].gl_faz << ") ";

                    if ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 0) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 1) && (((ssk + 1) % 2) == 0)) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 2) && (((ssk + 1) % 4) == 0)) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 3) && (((ssk + 1) % 8) == 0)) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 4) && (((ssk + 1) % 16) == 0)) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 5) && (((ssk + 1) % 32) == 0)) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 6) && (((ssk + 1) % 64) == 0)) ||
                       ((PRG_PS1.data_str_r[calcPZUPos].gl_faz == 7) && (((ssk + 1) % 128) == 0)) ||
                       (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                        )
                    { // Если фазировка в текущем кадре
                        ff_elalon << "O ";
                        faz_OK = true;
                    } else
                    {
                        ff_elalon << "X ";
                        faz_OK = false;
                    }
                } else
                {
                    if (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                    { // Если дошли до конца буфера
                        ff_elalon << "Bend ";
                        faz_OK = true;
                    } else
                    {
                        faz_OK = false;
                    }
                }

               //qDebug() << faz_OK;

                if (faz_OK)
                { // Фазируемся
                    faz_OK = false;
                    lk_param[currenInput][currentLK].lk_channel = 0;

                    lk_param[currenInput][currentLK].current_Adder += lk_param[currenInput][currentLK].Adder; // Прибавляем сумматор

                    if (currentLK == 3) // Фазировка в 4 ЛК. Сброс всех сумматоров
                    {
                        lk_param[currenInput][currentLK].current_Adder = 0; // Сбрасываем текущий суматор
                        lk_param[currenInput][0].clear_adder = true;        // Ставим признак сброса суматора в других каналах
                        lk_param[currenInput][1].clear_adder = true;
                        lk_param[currenInput][2].clear_adder = true;
                    } else
                    {
                        if (lk_param[currenInput][currentLK].clear_adder)        // Если есть признак сброса сумматора
                        {
                            lk_param[currenInput][currentLK].current_Adder = 0;
                            lk_param[currenInput][currentLK].clear_adder = false;
                        }
                    }
                } else
                {

                }
            } else
            {
                ff_elalon << "No LK";
                ImmKadr_PS1[ssk][pos] = 255;
            }


    // Преключаем на следующую позицию ПЗУ
        PZUPos++;
        if (PZUPos == 1024)
            PZUPos = 0;

        currentLogicLK++;
        if (currentLogicLK == 9)
            currentLogicLK = 1;
        ff_elalon << "\r";
    }
    }

    // Проходим по программе опроса по всем ССК и заполняем эталонный массив используя lk_param
    // !!!!!!!!!!!!!!!!!!!!!!! ДЛЯ ПС2 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    ff_elalon << QString(tr("=========================================================== ПС2 =========================================\r"));

    PZUPos = 0;
    currentLogicLK = 1; // Логический номер входа для выделения ЦМ


    for (int ssk = 0; ssk < 128; ssk++)
    {

        for (int pos = 0; pos < 512; pos++)
        {


            ff_elalon << QString::number(ssk, 10) << "[" << QString::number(pos, 10) << "] - " << QString::number(PZUPos, 10);
            // << "(" << QString::number(calcPZUPos, 10) << ") ";


            if (PRG_PS2.data_str_r[PZUPos].zap_mac == 1)  // Если идет обращение к ЦМ
            {
                if (currentLogicLK == 2)
                {
                    // Идет ЦМ2
                    currenInput = 2;
                    currentLK = 0;
                    calcPZUPos = PZUPos;
                } else

                if (currentLogicLK == 6)
                {
                    // Идет ЦМ6
                    currenInput = 6;
                    currentLK = 0;
                    calcPZUPos = PZUPos;
                } else
                {
                    currenInput = 255;
                    currentLK = 255;
                    calcPZUPos = PZUPos;
                }

                ff_elalon << "(" << QString::number(calcPZUPos, 10) << ") ";
            } else
            {
                // Идет обращение к МАС

                currenInput = mas_input_convert[PRG_PS2.data_str_r[PZUPos].num_in];

                calcPZUPos = calculatePZUPos(PZUPos, false, PRG_PS2.data_str_r[PZUPos].num_in);
                ff_elalon << "(" << QString::number(calcPZUPos, 10) << ") ";

                if ((currenInput != 255) && lk_param[currenInput][0].mas_dm) // Если опрашивается МАС
                {
                    if (PRG_PS2.data_str_r[calcPZUPos].addr == lk_param[currenInput][0].Addr)
                    {
                        currentLK = 0;
                    } else if (PRG_PS2.data_str_r[calcPZUPos].addr == lk_param[currenInput][1].Addr)
                    {
                        currentLK = 1;
                    } else if (PRG_PS2.data_str_r[calcPZUPos].addr == lk_param[currenInput][2].Addr)
                    {
                        currentLK = 2;
                    } else if (PRG_PS2.data_str_r[calcPZUPos].addr == lk_param[currenInput][3].Addr)
                    {
                        currentLK = 3;
                    } else
                    {
                        currentLK = 5;
                    }
                } else
                  {
                    currentLK = 5;
                  }

            }

            er_param_PS2[ssk][pos].LK_NUM = currentLK;

            er_param_PS2[ssk][pos].MSS_NUM = currenInput;
            er_param_PS2[ssk][pos].LK_ADDR = PRG_PS2.data_str_r[calcPZUPos].addr;

            if (currenInput != 255)
            {
                ff_elalon << "Input: " << mas_input[currenInput]->getMASName() << "(" << currenInput << ")";
                ff_elalon << " Addr: " << PRG_PS2.data_str_r[calcPZUPos].addr << "(" << currentLK << ") ";
                er_param_PS2[ssk][pos].LK_Channel = lk_param[currenInput][currentLK].lk_channel;
            }

            if (currentLK < 4)
            {

                if (lk_param[currenInput][currentLK].const1Pos == lk_param[currenInput][currentLK].lk_channel)
                    ImmKadr_PS2[ssk][pos] = lk_param[currenInput][currentLK].const1;
                else if (lk_param[currenInput][currentLK].const2Pos == lk_param[currenInput][currentLK].lk_channel)
                    ImmKadr_PS2[ssk][pos] = lk_param[currenInput][currentLK].const2;
                else
                    ImmKadr_PS2[ssk][pos] = mas_input[currenInput] -> getLKData(currentLK,
                                                                  lk_param[currenInput][currentLK].lk_channel) + lk_param[currenInput][currentLK].current_Adder;

                ff_elalon << "Data: " << ImmKadr_PS2[ssk][pos] << " (" << lk_param[currenInput][currentLK].lk_channel << ") ";

                lk_param[currenInput][currentLK].lk_channel++;

                if (PRG_PS2.data_str_r[calcPZUPos].pf == 1) // Если стои признак фазировки
                {
//                    qDebug() << "LKI: " << currenInput << "gl: " << PRG_PS1.data_str[PZUPos].gl_faz << "PZU: " << PZUPos
//                             << PRG_PS1.data_str[PZUPos].addr;

                    ff_elalon << "FAZ(" << PRG_PS2.data_str_r[calcPZUPos].gl_faz << ") ";

                    if ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 0) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 1) && (((ssk + 1) % 2) == 0)) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 2) && (((ssk + 1) % 4) == 0)) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 3) && (((ssk + 1) % 8) == 0)) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 4) && (((ssk + 1) % 16) == 0)) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 5) && (((ssk + 1) % 32) == 0)) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 6) && (((ssk + 1) % 64) == 0)) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 7) && (((ssk + 1) % 128) == 0)) ||
                       (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                        )
                    { // Если фазировка в текущем кадре
                        ff_elalon << "O ";
                        faz_OK = true;
                    } else
                    {
                        ff_elalon << "X ";
                        faz_OK = false;
                    }
                } else
                {
                    if (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                    { // Если дошли до конца буфера
                        ff_elalon << "Bend ";
                        faz_OK = true;
                    } else
                    {
                        faz_OK = false;
                    }
                }

               //qDebug() << faz_OK;

                if (faz_OK)
                { // Фазируемся
                    faz_OK = false;
                    lk_param[currenInput][currentLK].lk_channel = 0;

                    lk_param[currenInput][currentLK].current_Adder += lk_param[currenInput][currentLK].Adder; // Прибавляем сумматор

                    if (currentLK == 3) // Фазировка в 4 ЛК. Сброс всех сумматоров
                    {
                        lk_param[currenInput][currentLK].current_Adder = 0; // Сбрасываем текущий суматор
                        lk_param[currenInput][0].clear_adder = true;        // Ставим признак сброса суматора в других каналах
                        lk_param[currenInput][1].clear_adder = true;
                        lk_param[currenInput][2].clear_adder = true;
                    } else
                    {
                        if (lk_param[currenInput][currentLK].clear_adder)        // Если есть признак сброса сумматора
                        {
                            lk_param[currenInput][currentLK].current_Adder = 0;
                            lk_param[currenInput][currentLK].clear_adder = false;
                        }
                    }
                } else
                {

                }
            } else
            {
                ff_elalon << "No LK";
                ImmKadr_PS2[ssk][pos] = 255;
            }


    // Преключаем на следующую позицию ПЗУ
        PZUPos++;
        if (PZUPos == 512)
            PZUPos = 0;

        currentLogicLK++;
        if (currentLogicLK == 9)
            currentLogicLK = 1;
        ff_elalon << "\r";
    }
    }

    qDebug() << PZUPos;
    // Проходим опять по программе опроса по всем ССК и повторно заполняем эталонный массив ничего не трогая

    for (int ssk = 0; ssk < 128; ssk++)
    {

        for (int pos = 0; pos < 512; pos++)
        {


            ff_elalon << QString::number(ssk, 10) << "[" << QString::number(pos, 10) << "] - " << QString::number(PZUPos, 10);
            // << "(" << QString::number(calcPZUPos, 10) << ") ";


            if (PRG_PS2.data_str_r[PZUPos].zap_mac == 1)  // Если идет обращение к ЦМ
            {
                if (currentLogicLK == 2)
                {
                    // Идет ЦМ2
                    currenInput = 2;
                    currentLK = 0;
                    calcPZUPos = PZUPos;
                } else

                if (currentLogicLK == 6)
                {
                    // Идет ЦМ6
                    currenInput = 6;
                    currentLK = 0;
                    calcPZUPos = PZUPos;
                } else
                {
                    currenInput = 255;
                    currentLK = 255;
                    calcPZUPos = PZUPos;
                }
                    ff_elalon << "(" << QString::number(calcPZUPos, 10) << ") ";
            } else
            {
                // Идет обращение к МАС

                currenInput = mas_input_convert[PRG_PS2.data_str_r[PZUPos].num_in];

                calcPZUPos = calculatePZUPos(PZUPos, false, PRG_PS2.data_str_r[PZUPos].num_in);
                ff_elalon << "(" << QString::number(calcPZUPos, 10) << ") ";

                if ((currenInput != 255) && lk_param[currenInput][0].mas_dm) // Если опрашивается МАС
                {
                    if (PRG_PS2.data_str_r[calcPZUPos].addr == lk_param[currenInput][0].Addr)
                    {
                        currentLK = 0;
                    } else if (PRG_PS2.data_str_r[calcPZUPos].addr == lk_param[currenInput][1].Addr)
                    {
                        currentLK = 1;
                    } else if (PRG_PS2.data_str_r[calcPZUPos].addr == lk_param[currenInput][2].Addr)
                    {
                        currentLK = 2;
                    } else if (PRG_PS2.data_str_r[calcPZUPos].addr == lk_param[currenInput][3].Addr)
                    {
                        currentLK = 3;
                    } else
                    {
                        currentLK = 5;
                    }
                } else
                  {
                    currentLK = 5;
                  }

            }

            er_param_PS2[ssk][pos].LK_NUM = currentLK;

            er_param_PS2[ssk][pos].MSS_NUM = currenInput;
            er_param_PS2[ssk][pos].LK_ADDR = PRG_PS2.data_str_r[calcPZUPos].addr;

            if (currenInput != 255)
            {
                ff_elalon << "Input: " << mas_input[currenInput]->getMASName() << "(" << currenInput << ")";
                ff_elalon << " Addr: " << PRG_PS2.data_str_r[calcPZUPos].addr << "(" << currentLK << ") ";
                er_param_PS2[ssk][pos].LK_Channel = lk_param[currenInput][currentLK].lk_channel;
            }

            if (currentLK < 4)
            {

                if (lk_param[currenInput][currentLK].const1Pos == lk_param[currenInput][currentLK].lk_channel)
                    ImmKadr_PS2[ssk][pos] = lk_param[currenInput][currentLK].const1;
                else if (lk_param[currenInput][currentLK].const2Pos == lk_param[currenInput][currentLK].lk_channel)
                    ImmKadr_PS2[ssk][pos] = lk_param[currenInput][currentLK].const2;
                else
                    ImmKadr_PS2[ssk][pos] = mas_input[currenInput] -> getLKData(currentLK,
                                                                  lk_param[currenInput][currentLK].lk_channel) + lk_param[currenInput][currentLK].current_Adder;

                ff_elalon << "Data: " << ImmKadr_PS2[ssk][pos] << " (" << lk_param[currenInput][currentLK].lk_channel << ") ";

                lk_param[currenInput][currentLK].lk_channel++;

                if (PRG_PS2.data_str_r[calcPZUPos].pf == 1) // Если стои признак фазировки
                {
//                    qDebug() << "LKI: " << currenInput << "gl: " << PRG_PS1.data_str[PZUPos].gl_faz << "PZU: " << PZUPos
//                             << PRG_PS1.data_str[PZUPos].addr;

                    ff_elalon << "FAZ(" << PRG_PS2.data_str_r[calcPZUPos].gl_faz << ") ";

                    if ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 0) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 1) && (((ssk + 1) % 2) == 0)) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 2) && (((ssk + 1) % 4) == 0)) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 3) && (((ssk + 1) % 8) == 0)) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 4) && (((ssk + 1) % 16) == 0)) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 5) && (((ssk + 1) % 32) == 0)) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 6) && (((ssk + 1) % 64) == 0)) ||
                       ((PRG_PS2.data_str_r[calcPZUPos].gl_faz == 7) && (((ssk + 1) % 128) == 0)) ||
                       (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                        )
                    { // Если фазировка в текущем кадре
                        ff_elalon << "O ";
                        faz_OK = true;
                    } else
                    {
                        ff_elalon << "X ";
                        faz_OK = false;
                    }
                } else
                {
                    if (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                    { // Если дошли до конца буфера
                        ff_elalon << "Bend ";
                        faz_OK = true;
                    } else
                    {
                        faz_OK = false;
                    }
                }

               //qDebug() << faz_OK;

                if (faz_OK)
                { // Фазируемся
                    faz_OK = false;
                    lk_param[currenInput][currentLK].lk_channel = 0;

                    lk_param[currenInput][currentLK].current_Adder += lk_param[currenInput][currentLK].Adder; // Прибавляем сумматор

                    if (currentLK == 3) // Фазировка в 4 ЛК. Сброс всех сумматоров
                    {
                        lk_param[currenInput][currentLK].current_Adder = 0; // Сбрасываем текущий суматор
                        lk_param[currenInput][0].clear_adder = true;        // Ставим признак сброса суматора в других каналах
                        lk_param[currenInput][1].clear_adder = true;
                        lk_param[currenInput][2].clear_adder = true;
                    } else
                    {
                        if (lk_param[currenInput][currentLK].clear_adder)        // Если есть признак сброса сумматора
                        {
                            lk_param[currenInput][currentLK].current_Adder = 0;
                            lk_param[currenInput][currentLK].clear_adder = false;
                        }
                    }
                } else
                {

                }
            } else
            {
                ff_elalon << "No LK";
                ImmKadr_PS2[ssk][pos] = 255;
            }


    // Преключаем на следующую позицию ПЗУ
        PZUPos++;
        if (PZUPos == 512)
            PZUPos = 0;

        currentLogicLK++;
        if (currentLogicLK == 9)
            currentLogicLK = 1;
        ff_elalon << "\r";
    }
    }

}

void dCheckKadr::UpdateEtalonRTSCM1()
{
    int PZUPos = 0;         // Позиция в программе опроса
    int calcPZUPos = 0;     // Позиция в программе опроса с запарралеливания. или как там ее для 7.1 7.2

    bool faz_OK = false;

    QFile ff("kadr_et.txt");

    ff.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream ff_elalon(&ff);

    // Для начала нужно заполнить весь эталонный массив 255.
    // Это оснанеться для необработтаных позиций кадра

    for (int ssk = 0; ssk < 128; ssk++) // Цикл по ССК
    {
        for (int pos = 0; pos < 256; pos++) // Цикл по позиции в кадре
        {
            ImmKadr_PS1[ssk][pos] = 255;
            ImmKadr_PS2[ssk][pos] = 255;
        }
    }

    // Заполняем массив параметров ЛК

    for (int num_in = 1; num_in < 13; num_in++)
    {
        for (int lk = 0; lk < 4; lk++)
        {
            if (mas_input[num_in] == NULL)
                continue;
            lk_param[num_in][lk].Adder_en = mas_input[num_in] -> getAdd_en(lk);
            lk_param[num_in][lk].Addr = mas_input[num_in] -> getAddr(lk);
            lk_param[num_in][lk].Adder = mas_input[num_in] -> getAdder(lk);
            lk_param[num_in][lk].BufferSize = mas_input[num_in] -> getBufferSize(lk);
            lk_param[num_in][lk].current_Adder = 0;
            lk_param[num_in][lk].lk_channel = 0;
            lk_param[num_in][lk].clear_adder = false;
            lk_param[num_in][lk].mas_dm = mas_input[num_in] -> isMAS();

            lk_param[num_in][lk].const1Pos = mas_input[num_in] -> getConst1Pos(lk);
            lk_param[num_in][lk].const1 = mas_input[num_in] -> getConst1(lk);
            lk_param[num_in][lk].const2Pos = mas_input[num_in] -> getConst2Pos(lk);
            lk_param[num_in][lk].const2 = mas_input[num_in] -> getConst2(lk);

        }
    }

    // Проходим по программе опроса по всем ССК и заполняем эталонный массив используя lk_param
    // !!!!!!!!!!!!!!!!!!!!!!! ДЛЯ ПС1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    ff_elalon << QString(tr("=========================================================== ПС1 =========================================\r"));

    PZUPos = 0;
    int currenInput;   // Номер входя МАС
    int currentLK;     // Текущий ЛК в МАС
 //   int ssk_kadr;      // ССК кадра опроса (0..64)

    // TODO: Нужно что-то делать с ПС1 ПС2

    for (int ssk = 0; ssk < 128; ssk++)
    {
     //   ssk_kadr = ssk >> 1;

        for (int pos = 0; pos < 256; pos++)
        {


            calcPZUPos = calculatePZUPos(PZUPos, true, 0);

            ff_elalon << QString::number(ssk, 10) << "[" << QString::number(pos, 10) << "] - " << QString::number(PZUPos, 10) << "("
                      << QString::number(calcPZUPos, 10) << ") ";
            currenInput = PRG_PS1.data_str_m[calcPZUPos].num_in;

            ff_elalon << "Input: " << currenInput;

            er_param_PS1[ssk][pos].MSS_NUM = currenInput;
            er_param_PS1[ssk][pos].LK_ADDR = PRG_PS1.data_str_m[calcPZUPos].addr;

            if (mas_input[currenInput] == NULL)
            {
                er_param_PS1[ssk][pos].MSS_NUM = 255; // Не проверять такой вход
            } else
            {
                if (lk_param[currenInput][0].mas_dm) // Если опрашивается МАС
                {
                    if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][0].Addr)
                    {
                        currentLK = 0;
                    } else if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][1].Addr)
                    {
                        currentLK = 1;
                    } else if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][2].Addr)
                    {
                        currentLK = 2;
                    } else if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][3].Addr)
                    {
                        currentLK = 3;
                    } else
                    {
                        currentLK = 5;
                    }
                } else // Если опрашивается ЦМ
                {
                    currentLK = 0;
                }

                ff_elalon << " Addr: " << PRG_PS1.data_str_m[PZUPos].addr << "(" << currentLK << ") ";

                er_param_PS1[ssk][pos].LK_NUM = currentLK;
                er_param_PS1[ssk][pos].LK_Channel = lk_param[currenInput][currentLK].lk_channel;

                if (currentLK < 4)
                {

                    if (lk_param[currenInput][currentLK].const1Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS1[ssk][pos] = lk_param[currenInput][currentLK].const1;
                    else if (lk_param[currenInput][currentLK].const2Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS1[ssk][pos] = lk_param[currenInput][currentLK].const2;
                    else
                        ImmKadr_PS1[ssk][pos] = mas_input[currenInput] -> getLKData(currentLK,
                                                                      lk_param[currenInput][currentLK].lk_channel) + lk_param[currenInput][currentLK].current_Adder;

                    ff_elalon << "Data: " << ImmKadr_PS1[ssk][pos] << " (" << lk_param[currenInput][currentLK].lk_channel << ") ";

                    lk_param[currenInput][currentLK].lk_channel++;

                    if (PRG_PS1.data_str_m[calcPZUPos].pf == 1) // Если стои признак фазировки
                    {
//                        qDebug() << "LKI: " << currenInput << "gl: " << PRG_PS1.data_str[PZUPos].gl_faz << "PZU: " << PZUPos
//                             << PRG_PS1.data_str[PZUPos].addr;

                        ff_elalon << "FAZ(" << PRG_PS1.data_str_m[PZUPos].gl_faz << ") ";

                        if (((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 0) && (((ssk + 1) % 2) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 1) && (((ssk + 1) % 4) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 2) && (((ssk + 1) % 8) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 3) && (((ssk + 1) % 16) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 4) && (((ssk + 1) % 32) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 5) && (((ssk + 1) % 64) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 6) && (((ssk + 1) % 128) == 0)) ||
                            ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 7) && (((ssk + 1) % 128) == 0)) ||
                        (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                            )
                        { // Если фазировка в текущем кадре
                            ff_elalon << "O ";
                            faz_OK = true;
                        } else
                        {
                            ff_elalon << "X ";
                            faz_OK = false;
                        }
                    } else
                    {
                        if (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                        { // Если дошли до конца буфера
                            ff_elalon << "Bend ";
                            faz_OK = true;
                        } else
                        {
                            faz_OK = false;
                        }
                    }

               //qDebug() << faz_OK;

                    if (faz_OK)
                    { // Фазируемся
                        faz_OK = false;
                        lk_param[currenInput][currentLK].lk_channel = 0;

                        lk_param[currenInput][currentLK].current_Adder += lk_param[currenInput][currentLK].Adder; // Прибавляем сумматор

                        if (currentLK == 3) // Фазировка в 4 ЛК. Сброс всех сумматоров
                        {
                            lk_param[currenInput][currentLK].current_Adder = 0; // Сбрасываем текущий суматор
                            lk_param[currenInput][0].clear_adder = true;        // Ставим признак сброса суматора в других каналах
                            lk_param[currenInput][1].clear_adder = true;
                            lk_param[currenInput][2].clear_adder = true;
                        } else
                        {
                            if (lk_param[currenInput][currentLK].clear_adder)        // Если есть признак сброса сумматора
                            {
                                lk_param[currenInput][currentLK].current_Adder = 0;
                                lk_param[currenInput][currentLK].clear_adder = false;
                            }
                        }
                    } else
                    {

                    }
                } else
                {
                    ff_elalon << "No LK";
                    ImmKadr_PS1[ssk][pos] = 255;
                }

            }
    // Преключаем на следующую позицию ПЗУ
        PZUPos++;
        if (PZUPos == 1024)
            PZUPos = 0;

        ff_elalon << "\r";
    }
    }

    qDebug() << PZUPos;
    // Проходим опять по программе опроса по всем ССК и повторно заполняем эталонный массив ничего не трогая

    for (int ssk = 0; ssk < 128; ssk++)
    {
     //   ssk_kadr = ssk >> 1;

        for (int pos = 0; pos < 256; pos++)
        {

            calcPZUPos = calculatePZUPos(PZUPos, true, 0);

            ff_elalon << QString::number(ssk, 10) << "[" << QString::number(pos, 10) << "] - " << QString::number(PZUPos, 10) << " ";
            currenInput = PRG_PS1.data_str_m[calcPZUPos].num_in;

            ff_elalon << "Input: " << currenInput;

            er_param_PS1[ssk][pos].MSS_NUM = currenInput;
            er_param_PS1[ssk][pos].LK_ADDR = PRG_PS1.data_str_m[calcPZUPos].addr;

            if (mas_input[currenInput] == NULL)
            {
                er_param_PS1[ssk][pos].MSS_NUM = 255;
            } else
            {

                if (lk_param[currenInput][0].mas_dm) // Если опрашивается МАС
                {
                    if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][0].Addr)
                    {
                        currentLK = 0;
                    } else if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][1].Addr)
                    {
                        currentLK = 1;
                    } else if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][2].Addr)
                    {
                        currentLK = 2;
                    } else if (PRG_PS1.data_str_m[calcPZUPos].addr == lk_param[currenInput][3].Addr)
                    {
                        currentLK = 3;
                    } else
                    {
                        currentLK = 5;
                    }
                } else // Если опрашивается ЦМ
                {
                    currentLK = 0;
                }

                ff_elalon << "Addr: " << PRG_PS1.data_str_m[PZUPos].addr << "(" << currentLK << ") ";

                er_param_PS1[ssk][pos].LK_NUM = currentLK;
                er_param_PS1[ssk][pos].LK_Channel = lk_param[currenInput][currentLK].lk_channel;

                if (currentLK < 4)
                {

                    if (lk_param[currenInput][currentLK].const1Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS1[ssk][pos] = lk_param[currenInput][currentLK].const1;
                    else if (lk_param[currenInput][currentLK].const2Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS1[ssk][pos] = lk_param[currenInput][currentLK].const2;
                    else
                        ImmKadr_PS1[ssk][pos] = mas_input[currenInput] -> getLKData(currentLK,
                                                                      lk_param[currenInput][currentLK].lk_channel) + lk_param[currenInput][currentLK].current_Adder;

                    ff_elalon << "Data: " << ImmKadr_PS1[ssk][pos];

                    lk_param[currenInput][currentLK].lk_channel++;

                    if (PRG_PS1.data_str_m[calcPZUPos].pf == 1) // Если стои признак фазировки
                    {
    //                    qDebug() << "LKI: " << currenInput << "gl: " << PRG_PS1.data_str[PZUPos].gl_faz << "PZU: " << PZUPos
    //                             << PRG_PS1.data_str[PZUPos].addr;

                        ff_elalon << "FAZ(" << PRG_PS1.data_str_m[PZUPos].gl_faz << ") ";

                        if (((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 0) && (((ssk + 1) % 2) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 1) && (((ssk + 1) % 4) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 2) && (((ssk + 1) % 8) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 3) && (((ssk + 1) % 16) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 4) && (((ssk + 1) % 32) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 5) && (((ssk + 1) % 64) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 6) && (((ssk + 1) % 128) == 0)) ||
                           ((PRG_PS1.data_str_m[calcPZUPos].gl_faz == 7) && (((ssk + 1) % 128) == 0)) ||
                           (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                            )
                        { // Если фазировка в текущем кадре
                            ff_elalon << "O ";
                            faz_OK = true;
                        } else
                        {
                            ff_elalon << "X ";
                            faz_OK = false;
                        }
                    } else
                    {
                        if (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                        { // Если дошли до конца буфера
                            ff_elalon << "Bend ";
                            faz_OK = true;
                        } else
                        {
                            faz_OK = false;
                        }
                    }

               //qDebug() << faz_OK;

                    if (faz_OK)
                    { // Фазируемся
                        faz_OK = false;
                        lk_param[currenInput][currentLK].lk_channel = 0;

                        lk_param[currenInput][currentLK].current_Adder += lk_param[currenInput][currentLK].Adder; // Прибавляем сумматор

                        if (currentLK == 3) // Фазировка в 4 ЛК. Сброс всех сумматоров
                        {
                            lk_param[currenInput][currentLK].current_Adder = 0; // Сбрасываем текущий суматор
                            lk_param[currenInput][0].clear_adder = true;        // Ставим признак сброса суматора в других каналах
                            lk_param[currenInput][1].clear_adder = true;
                            lk_param[currenInput][2].clear_adder = true;
                        } else
                        {
                            if (lk_param[currenInput][currentLK].clear_adder)        // Если есть признак сброса сумматора
                            {
                                lk_param[currenInput][currentLK].current_Adder = 0;
                                lk_param[currenInput][currentLK].clear_adder = false;
                            }
                        }
                    } else
                    {

                    }
                } else
                {
                    ff_elalon << "No LK";
                    ImmKadr_PS1[ssk][pos] = 255;
                }

            }
    // Преключаем на следующую позицию ПЗУ
        PZUPos++;
        if (PZUPos == 1024)
            PZUPos = 0;

        ff_elalon << "\r";
    }
    }


    // Проходим по программе опроса по всем ССК и заполняем эталонный массив используя lk_param
    // !!!!!!!!!!!!!!!!!!!!!!! ДЛЯ ПС1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    ff_elalon << QString(tr("=========================================================== ПС2 =========================================\r"));

    PZUPos = 0;

    for (int ssk = 0; ssk < 128; ssk++)
    {
     //   ssk_kadr = ssk >> 1;

        for (int pos = 0; pos < 256; pos++)
        {

            calcPZUPos = calculatePZUPos(PZUPos, false, 0);

            ff_elalon << QString::number(ssk, 10) << "[" << QString::number(pos, 10) << "] - " << QString::number(PZUPos, 10) << " ";
            currenInput = PRG_PS2.data_str_m[calcPZUPos].num_in;

            ff_elalon << "Input: " << currenInput;

            er_param_PS2[ssk][pos].MSS_NUM = currenInput;
            er_param_PS2[ssk][pos].LK_ADDR = PRG_PS2.data_str_m[calcPZUPos].addr;

            if (mas_input[currenInput] == NULL)
            {
                er_param_PS2[ssk][pos].MSS_NUM = 255;
            } else
            {

                if (lk_param[currenInput][0].mas_dm) // Если опрашивается МАС
                {
                    if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][0].Addr)
                    {
                        currentLK = 0;
                    } else if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][1].Addr)
                    {
                        currentLK = 1;
                    } else if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][2].Addr)
                    {
                        currentLK = 2;
                    } else if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][3].Addr)
                    {
                        currentLK = 3;
                    } else
                    {
                        currentLK = 5;
                    }
                } else // Если опрашивается ЦМ
                {
                    currentLK = 0;
                }

                ff_elalon << " Addr: " << PRG_PS2.data_str_m[PZUPos].addr << "(" << currentLK << ") ";

                er_param_PS2[ssk][pos].LK_NUM = currentLK;
                er_param_PS2[ssk][pos].LK_Channel = lk_param[currenInput][currentLK].lk_channel;

                if (currentLK < 4)
                {

                    if (lk_param[currenInput][currentLK].const1Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS2[ssk][pos] = lk_param[currenInput][currentLK].const1;
                    else if (lk_param[currenInput][currentLK].const2Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS2[ssk][pos] = lk_param[currenInput][currentLK].const2;
                    else
                        ImmKadr_PS2[ssk][pos] = mas_input[currenInput] -> getLKData(currentLK,
                                                                      lk_param[currenInput][currentLK].lk_channel) + lk_param[currenInput][currentLK].current_Adder;

                    ff_elalon << "Data: " << ImmKadr_PS2[ssk][pos] << " (" << lk_param[currenInput][currentLK].lk_channel << ") ";

                    lk_param[currenInput][currentLK].lk_channel++;

                    if (PRG_PS2.data_str_m[calcPZUPos].pf == 1) // Если стои признак фазировки
                    {
//                        qDebug() << "LKI: " << currenInput << "gl: " << PRG_PS1.data_str[PZUPos].gl_faz << "PZU: " << PZUPos
//                             << PRG_PS1.data_str[PZUPos].addr;

                        ff_elalon << "FAZ(" << PRG_PS2.data_str_m[PZUPos].gl_faz << ") ";

                        if (((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 0) && (((ssk + 1) % 2) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 1) && (((ssk + 1) % 4) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 2) && (((ssk + 1) % 8) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 3) && (((ssk + 1) % 16) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 4) && (((ssk + 1) % 32) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 5) && (((ssk + 1) % 64) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 6) && (((ssk + 1) % 128) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 7) && (((ssk + 1) % 128) == 0)) ||
                        (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                            )
                        { // Если фазировка в текущем кадре
                            ff_elalon << "O ";
                            faz_OK = true;
                        } else
                        {
                            ff_elalon << "X ";
                            faz_OK = false;
                        }
                    } else
                    {
                        if (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                        { // Если дошли до конца буфера
                            ff_elalon << "Bend ";
                            faz_OK = true;
                        } else
                        {
                            faz_OK = false;
                        }
                    }

               //qDebug() << faz_OK;

                    if (faz_OK)
                    { // Фазируемся
                        faz_OK = false;
                        lk_param[currenInput][currentLK].lk_channel = 0;

                        lk_param[currenInput][currentLK].current_Adder += lk_param[currenInput][currentLK].Adder; // Прибавляем сумматор

                        if (currentLK == 3) // Фазировка в 4 ЛК. Сброс всех сумматоров
                        {
                            lk_param[currenInput][currentLK].current_Adder = 0; // Сбрасываем текущий суматор
                            lk_param[currenInput][0].clear_adder = true;        // Ставим признак сброса суматора в других каналах
                            lk_param[currenInput][1].clear_adder = true;
                            lk_param[currenInput][2].clear_adder = true;
                        } else
                        {
                            if (lk_param[currenInput][currentLK].clear_adder)        // Если есть признак сброса сумматора
                            {
                                lk_param[currenInput][currentLK].current_Adder = 0;
                                lk_param[currenInput][currentLK].clear_adder = false;
                            }
                        }
                    } else
                    {

                    }
                } else
                {
                    ff_elalon << "No LK";
                    ImmKadr_PS2[ssk][pos] = 255;
                }

            }
    // Преключаем на следующую позицию ПЗУ
        PZUPos++;
        if (PZUPos == 1024)
            PZUPos = 0;

        ff_elalon << "\r";
    }
    }

    qDebug() << PZUPos;
    // Проходим опять по программе опроса по всем ССК и повторно заполняем эталонный массив ничего не трогая

    for (int ssk = 0; ssk < 128; ssk++)
    {
     //   ssk_kadr = ssk >> 1;

        for (int pos = 0; pos < 256; pos++)
        {
            calcPZUPos = calculatePZUPos(PZUPos, false, 0);

            ff_elalon << QString::number(ssk, 10) << "[" << QString::number(pos, 10) << "] - " << QString::number(PZUPos, 10) << " ";
            currenInput = PRG_PS2.data_str_m[calcPZUPos].num_in;

            ff_elalon << "Input: " << currenInput;

            er_param_PS2[ssk][pos].MSS_NUM = currenInput;
            er_param_PS2[ssk][pos].LK_ADDR = PRG_PS2.data_str_m[calcPZUPos].addr;

            if (mas_input[currenInput] == NULL)
            {
                er_param_PS2[ssk][pos].MSS_NUM = 255;
            } else
            {


                if (lk_param[currenInput][0].mas_dm) // Если опрашивается МАС
                {
                    if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][0].Addr)
                    {
                        currentLK = 0;
                    } else if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][1].Addr)
                    {
                        currentLK = 1;
                    } else if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][2].Addr)
                    {
                        currentLK = 2;
                    } else if (PRG_PS2.data_str_m[calcPZUPos].addr == lk_param[currenInput][3].Addr)
                    {
                        currentLK = 3;
                    } else
                    {
                        currentLK = 5;
                    }
                } else // Если опрашивается ЦМ
                {
                    currentLK = 0;
                }

                ff_elalon << "Addr: " << PRG_PS2.data_str_m[PZUPos].addr << "(" << currentLK << ") ";

                er_param_PS2[ssk][pos].LK_NUM = currentLK;
                er_param_PS2[ssk][pos].LK_Channel = lk_param[currenInput][currentLK].lk_channel;

                if (currentLK < 4)
                {

                    if (lk_param[currenInput][currentLK].const1Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS2[ssk][pos] = lk_param[currenInput][currentLK].const1;
                    else if (lk_param[currenInput][currentLK].const2Pos == lk_param[currenInput][currentLK].lk_channel)
                        ImmKadr_PS2[ssk][pos] = lk_param[currenInput][currentLK].const2;
                    else
                        ImmKadr_PS2[ssk][pos] = mas_input[currenInput] -> getLKData(currentLK,
                                                                      lk_param[currenInput][currentLK].lk_channel) + lk_param[currenInput][currentLK].current_Adder;

                    ff_elalon << "Data: " << ImmKadr_PS2[ssk][pos];

                    lk_param[currenInput][currentLK].lk_channel++;

                    if (PRG_PS2.data_str_m[calcPZUPos].pf == 1) // Если стои признак фазировки
                    {
//                        qDebug() << "LKI: " << currenInput << "gl: " << PRG_PS1.data_str[PZUPos].gl_faz << "PZU: " << PZUPos
//                                 << PRG_PS1.data_str[PZUPos].addr;

                        ff_elalon << "FAZ(" << PRG_PS2.data_str_m[PZUPos].gl_faz << ") ";

                        if (((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 0) && (((ssk + 1) % 2) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 1) && (((ssk + 1) % 4) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 2) && (((ssk + 1) % 8) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 3) && (((ssk + 1) % 16) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 4) && (((ssk + 1) % 32) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 5) && (((ssk + 1) % 64) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 6) && (((ssk + 1) % 128) == 0)) ||
                           ((PRG_PS2.data_str_m[calcPZUPos].gl_faz == 7) && (((ssk + 1) % 128) == 0)) ||
                           (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                            )
                        { // Если фазировка в текущем кадре
                            ff_elalon << "O ";
                            faz_OK = true;
                        } else
                        {
                            ff_elalon << "X ";
                            faz_OK = false;
                        }
                    } else
                    {
                        if (lk_param[currenInput][currentLK].lk_channel == lk_param[currenInput][currentLK].BufferSize)
                        { // Если дошли до конца буфера
                            ff_elalon << "Bend ";
                            faz_OK = true;
                        } else
                        {
                            faz_OK = false;
                        }
                    }

                   //qDebug() << faz_OK;

                    if (faz_OK)
                    { // Фазируемся
                        faz_OK = false;
                        lk_param[currenInput][currentLK].lk_channel = 0;

                        lk_param[currenInput][currentLK].current_Adder += lk_param[currenInput][currentLK].Adder; // Прибавляем сумматор

                        if (currentLK == 3) // Фазировка в 4 ЛК. Сброс всех сумматоров
                        {
                            lk_param[currenInput][currentLK].current_Adder = 0; // Сбрасываем текущий суматор
                            lk_param[currenInput][0].clear_adder = true;        // Ставим признак сброса суматора в других каналах
                            lk_param[currenInput][1].clear_adder = true;
                            lk_param[currenInput][2].clear_adder = true;
                        } else
                        {
                            if (lk_param[currenInput][currentLK].clear_adder)        // Если есть признак сброса сумматора
                            {
                                lk_param[currenInput][currentLK].current_Adder = 0;
                                lk_param[currenInput][currentLK].clear_adder = false;
                            }
                        }
                    } else
                    {

                    }
                } else
                {
                    ff_elalon << "No LK";
                    ImmKadr_PS2[ssk][pos] = 255;
                }

            }
    // Преключаем на следующую позицию ПЗУ
        PZUPos++;
        if (PZUPos == 1024)
            PZUPos = 0;

        ff_elalon << "\r";
    }
    }


}


void dCheckKadr::LoadUstavkiFile(QString ustFileName)
{
    QString devFileName;
    QString pzuFileName;
    QString VAARpzuFileName;

    QFileInfo ustFileInfo(ustFileName);

    devFileName = ustFileInfo.path() + "/Device.ini";

    DevFileName = devFileName;
    UstFileName = ustFileName;

    qDebug() << devFileName;

    QSettings devFile(devFileName, QSettings::IniFormat, this);
    devFile.setIniCodec("Windows-1251");

    devFile.beginReadArray("MAS");

    for (int i = 0; i < 13; i++)
    {

        switch(i)
        {
            case 0: // Пустой элемент для выравнивания

                mas_input[i] = NULL;

                break;

            case 1:     // Входы МАС
            case 3:
            case 4:
            case 5:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:

            qDebug() << "mas_input_array: "  << mas_input_array[i];

            devFile.setArrayIndex(mas_input_array[i]);
            if (!devFile.value("Enable", false).toBool())
            {
    //            if (mas[i] != NULL)
    //                delete mas[i];
                mas_input_convert[mas_input_array[i]] = 255;
                mas_input[i] = NULL;
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ТУТ ПРОВЕРИТЬ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            } else
            {
                mas_input_convert[mas_input_array[i]] = i;
                mas_input[i] = new MAS_Data(devFile.value("Name", "").toString());
                mas_input[i] -> LoadData(ustFileName, mas_input_array[i], true);
            }
                break;

            case 2: // Вход ЦМ2
            case 6: // Вход ЦМ6

            mas_input[i] = new MAS_Data(QString("ЦМ%1").arg(i));
            mas_input[i] -> LoadData(ustFileName, mas_input_array[i], false);

                break;

        }




    }

    devFile.endArray();

    KadrType = devFile.value("KadrType", 0).toInt();

    switch (KadrType)
    {
    case KADR_RTSC:
            lKadrType -> setText(tr("РТСЦ"));
            connect(recv_socket, SIGNAL(readyRead()), this, SLOT(sl_Recv_PackageR()));
            break;
    case KADR_RTSCM:
            lKadrType -> setText(tr("РТСЦ-М"));
            connect(recv_socket, SIGNAL(readyRead()), this, SLOT(sl_Recv_PackageM()));
            break;
    case KADR_VAAR:
            lKadrType -> setText(tr("ВААР"));
            connect(recv_socket, SIGNAL(readyRead()), this, SLOT(sl_Recv_PackageV()));
            break;

    case KADR_RTSCM1:
            lKadrType -> setText(tr("РТСЦ-М1"));
            connect(recv_socket, SIGNAL(readyRead()), this, SLOT(sl_Recv_PackageM()));
            break;

    default: lKadrType -> setText(tr("Неизвестный"));
    }

    qDebug() << Q_FUNC_INFO << "1";

    devFile.beginReadArray("Kadr");

    for (int i = 0; i < 4; i++)
    {
        devFile.setArrayIndex(i);

        Potok[i] = devFile.value("Potok", 2).toInt();

        if (Potok[i] == POTOK_OFF)
            InputButtonGroup -> button(i + 1) -> setEnabled(false);
        else
            InputButtonGroup -> button(i + 1) -> setEnabled(true);

        InputButtonGroup -> button(i + 1) -> setText(devFile.value("Name", tr("Нет входа")).toString());
    }

    devFile.endArray();

    QSettings ustFile(ustFileName, QSettings::IniFormat);
    ustFile.setIniCodec("Windows-1251");

    ustFile.beginGroup("PZU");
    pzuFileName = ustFile.value("file_name", "").toString();
    VAARpzuFileName = ustFile.value("vaar_pzu", "").toString();

    qDebug() << "PZUFileName: " << pzuFileName;
    qDebug() << "VAAR_PZU: " << VAARpzuFileName;

    if (pzuFileName == "")
    {
        qDebug() << "No PZU file";
        return;
    }


    ustFile.beginReadArray("Dual");

        for (int i = 0; i < 8; i++)
        {
            ustFile.setArrayIndex(i);
            DualPS1[i] = ustFile.value("PS1", false).toBool();
            DualPS2[i] = ustFile.value("PS2", false).toBool();
        }

    ustFile.endArray();

    ustFile.beginReadArray("ZapretCheck");

        for (int i = 0; i < 8; i++)
        {
            ustFile.setArrayIndex(i);
            ZapretPS1[i] = ustFile.value("PS1", false).toBool();
            ZapretPS2[i] = ustFile.value("PS2", false).toBool();
        }

    ustFile.endArray();

    qDebug() << Q_FUNC_INFO << "5";

    QFile PZUFile(ustFileInfo.path() + "/" + pzuFileName);

    qDebug() << "PZUFile: " << PZUFile.fileName();

    if (!PZUFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Error open PZU file";
        return;
    }

    QFile kk("kadr.txt");

    kk.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream ff(&kk);

    switch (KadrType)
    {

    case KADR_RTSC: // Кадр РТСЦ

        PZUFile.read((char *)PRG_PS1.buff, 2048);
        PZUFile.read((char *)PRG_PS2.buff, 1024);
        ff << " +++++++++++++++++++++++++++++ PS1 +++++++++++++++++++++++++++++++++++++++++++++\r";

        for (int i = 0; i < 1024; i++)
        {
            ff << "[" << i << "]  " << PRG_PS1.data_str_r[i].addr << " " << PRG_PS1.data_str_r[i].pf << " "
               << PRG_PS1.data_str_r[i].num_in << " " <<  PRG_PS1.data_str_r[i].zap_mac
               << " ========= " << QString::number(PRG_PS1.word[i], 16) << "\r";
        }

        ff << " +++++++++++++++++++++++++++++ PS2 +++++++++++++++++++++++++++++++++++++++++++++\r";

        for (int i = 0; i < 512; i++)
        {
            ff << "[" << i << "]  " << PRG_PS2.data_str_r[i].addr << " " << PRG_PS2.data_str_r[i].pf << " "
               << PRG_PS2.data_str_r[i].num_in << " " <<  PRG_PS2.data_str_r[i].zap_mac
               << " ========= " << QString::number(PRG_PS2.word[i], 16) << "\r";
        }
        break;

    case KADR_RTSCM:
    case KADR_RTSCM1:
        qDebug() << "PRG1: " << PZUFile.read((char *)PRG_PS1.buff, 2048);
        PZUFile.read((char *)PRG_PS2.buff, 2048);

        for (int i = 0; i < 1024; i++)
        {
            ff << PRG_PS1.data_str_m[i].addr << " " << PRG_PS1.data_str_m[i].pf << " " <<
                  PRG_PS1.data_str_m[i].num_in << " " << PRG_PS1.data_str_m[i].gl_faz
               << "=======" << QString::number(PRG_PS1.buff[i * 2], 16) << " " << QString::number(PRG_PS1.buff[i * 2 + 1], 16) << " " <<
               QString::number(PRG_PS1.word[i], 16) << "\r";
        }
        break;

    case KADR_VAAR:
            qDebug() << "PRG1: " << PZUFile.read((char *)PRG_PS1.buff, 2048);
            PZUFile.read((char *)PRG_PS2.buff, 2048);

            for (int i = 0; i < 1024; i++)
            {
                ff << PRG_PS1.data_str_m[i].addr << " " << PRG_PS1.data_str_m[i].pf << " " <<
                      PRG_PS1.data_str_m[i].num_in << " " << PRG_PS1.data_str_m[i].gl_faz
                   << "=======" << QString::number(PRG_PS1.buff[i * 2], 16) << " " << QString::number(PRG_PS1.buff[i * 2 + 1], 16) << " " <<
                   QString::number(PRG_PS1.word[i], 16) << "\r";
            }

            if (VAARpzuFileName == "")
            {
                qDebug() << "No VAAR pzu file name";
                return;
            }

            QFile VAARFile(ustFileInfo.path() + "/" + VAARpzuFileName);

            qDebug() << "VAAR_PZUFile: " << VAARFile.fileName();

            if (!VAARFile.open(QIODevice::ReadOnly))
            {
                qDebug() << "Error open VAAR PZU file";
                return;
            }
            VAARFile.read((char *)PRG_VAAR, 2048);

            VAARFile.close();

            break;


    }

// ---------------------------------------------------------------------------------------------------------------


    kk.close();

    ustFile.endGroup();
// ---------------------------------------------------------------------------------------------------------------


    ustFile.beginGroup("KADR");

        KadrReadTime = ustFile.value("rsv_time", 0).toInt();
        lCheckTime -> setText(QString::number(KadrReadTime) + tr(" с."));


        switch(KadrType)
        {
        case KADR_RTSCM:
        case KADR_RTSCM1:

            cbCheckBMP -> setChecked(ustFile.value("CheckBMP", false).toBool());
            cbCheckDelay -> setChecked(ustFile.value("CheckDelay", false).toBool());
            cbCheckCRC -> setChecked(ustFile.value("CheckCRC", false).toBool());
            cbCheckRS -> setChecked(ustFile.value("CheckRS", false).toBool());

                break;

        case KADR_VAAR:

            cbCheckBMP -> setChecked(false);
            cbCheckBMP -> setEnabled(false);
            cbCheckDelay -> setChecked(false);
            cbCheckDelay -> setEnabled(false);
            cbCheckCRC -> setChecked(ustFile.value("CheckCRC", false).toBool());
            cbCheckRS -> setChecked(ustFile.value("CheckRS", false).toBool());
            break;


        case KADR_RTSC:

            cbCheckBMP -> setChecked(false);
            cbCheckBMP -> setEnabled(false);
            cbCheckDelay -> setChecked(false);
            cbCheckDelay -> setEnabled(false);
            cbCheckCRC -> setChecked(false);
            cbCheckCRC -> setEnabled(false);
            cbCheckRS -> setChecked(false);
            cbCheckRS -> setEnabled(false);

                break;
        }

        cbCheckMMP -> setChecked(ustFile.value("CheckMMP", false).toBool());
        cbCheckSSK -> setChecked(ustFile.value("CheckSSK", false).toBool());
        cbCheckM1M2 -> setChecked(ustFile.value("CheckM1M2", false).toBool());
        cbCheckKadrTime -> setChecked(ustFile.value("CheckTime", false).toBool());

    ustFile.endGroup();

    lUstavkiName -> setText(ustFile.fileName());
    lPZUFile -> setText(pzuFileName);


    if ((KadrType == KADR_RTSCM) || (KadrType == KADR_RTSCM1))
    {
        ustFile.beginReadArray("BLK");

        for (int blk = 0; blk < 4; blk++)
        {
            ustFile.setArrayIndex(blk);

            ustFile.beginReadArray("Sensor");

            for (int sens = 0; sens < 24; sens++)
            {
                ustFile.setArrayIndex(sens);

                BLK_param[blk][sens].Freq = ustFile.value("Freq", 0).toInt();
                BLK_param[blk][sens].Type = ustFile.value("Type", 0).toInt();
                BLK_param[blk][sens].paramA = ustFile.value("Param_A", 0).toInt();
                BLK_param[blk][sens].paramB = ustFile.value("Param_B", 0).toInt();
                BLK_param[blk][sens].paramC = ustFile.value("Param_C", 0).toInt();
                BLK_param[blk][sens].PotokTM = ustFile.value("PotokTM", 0).toInt();
                BLK_param[blk][sens].currentData = 0;
                BLK_param[blk][sens].isTreugolUp = true;
                BLK_param[blk][sens].waitForFirst = true;

            }

            ustFile.endArray();
        }

        ustFile.endArray();
    }

    switch (KadrType)
    {

       case KADR_RTSC: // Кадр РТСЦ

            UpdateEtalonRTSC();
            break;

       case KADR_RTSCM: // Кадр РТСЦ-М

            UpdateEtalonRTSCM();
            break;

       case KADR_VAAR: // Кадр ВААР
            UpdateEtalonVAAR();
            break;

       case KADR_RTSCM1:

            UpdateEtalonRTSCM1();
            break;

    }

    qDebug() << Q_FUNC_INFO << "end";

}

void dCheckKadr::UpdateErrorData()
{



    lChkKadrNumber -> setText(QString::number(KadrCheckNumber));

    if (cbCheckMMP -> isChecked())
        lChkMMPError -> setText(QString::number(MMPError));
    else
        lChkMMPError -> setText("-");

    if (cbCheckBMP -> isChecked())
        lChkBMPError -> setText(QString::number(BMPError));
    else
        lChkBMPError -> setText("-");

    if (cbCheckSSK -> isChecked())
        lChkSSKError -> setText(QString::number(SSKError));
    else
        lChkSSKError -> setText("-");

    if (cbCheckM1M2 -> isChecked())
    {
        lChkM1Error -> setText(QString::number(M1Error));
        lChkM2Error -> setText(QString::number(M2Error));
    } else
    {
        lChkM1Error -> setText("-");
        lChkM2Error -> setText("-");
    }

    if (cbCheckKadrTime -> isChecked())
        lChkKadrTimeError -> setText(QString::number(TimeError));
    else
        lChkKadrTimeError -> setText("-");

    if (cbCheckDelay -> isChecked())
        lChkDelayError -> setText(QString::number(DelayError));
    else
        lChkDelayError -> setText("-");

    if (cbCheckCRC -> isChecked())
        lChkCRCError -> setText(QString::number(CRCError));
    else
        lChkCRCError -> setText("-");

    if (cbCheckRS -> isChecked())
        lChkRSError -> setText(QString::number(RSError));
    else
        lChkRSError -> setText("-");

    QApplication::processEvents();


}

void dCheckKadr::ViewError()
{

    if(QMessageBox::critical(this,
                             tr("Проверка кадра"),
                             tr("В результате проверки обнаружены ошибки"),
                             QMessageBox::Ok | QMessageBox::Open) == QMessageBox::Open)
    {

        dViewError errorLog;

        errorLog.LoadTextFile("error.txt");

        errorLog.exec();

    }
}

void dCheckKadr::setRsvChannel(int num)
{
    QByteArray datagram;

    datagram.resize(8);

    datagram[0] = num;

    if (num > 0)
    {
        RecvKadr = 0;
        kadrTmpFile = new QTemporaryFile(this);

        if (kadrTmpFile -> isOpen())
        {
            qDebug() << "tmp file closing";

            kadrTmpFile -> close();
        }

        if (kadrTmpFile -> open())
        {
            qDebug() << kadrTmpFile -> fileName();
        }
    } else
    {

    }

    send_socket -> writeDatagram(datagram, QHostAddress::Broadcast, 3001);

    qDebug() << "Set input to " << num;
}

quint16 dCheckKadr::crc16(quint8 *buff, quint16 len)
{
    quint16 crc = 0xFFFF;

    while (len--)
    {
       crc = (crc << 8) ^ Crc16Table[(crc >> 8) ^ *buff++];
    }

    return crc;
}


quint8 dCheckKadr::getBLKData(quint8 blk_num, quint8 sensor_num)
{
    quint8 tmp_data;

    if (BLK_param[blk_num][sensor_num].Freq == 0)
    {
        return 0;
    }

    switch (BLK_param[blk_num][sensor_num].Type)
    {
   case 0:

        tmp_data = BLK_param[blk_num][sensor_num].paramA;
        BLK_param[blk_num][sensor_num].currentData = tmp_data;
        BLK_param[blk_num][sensor_num].waitForFirst = false;
        return tmp_data;

        break;

    case 1:

        if (BLK_param[blk_num][sensor_num].waitForFirst)
        {
            tmp_data = BLK_param[blk_num][sensor_num].currentData;
            BLK_param[blk_num][sensor_num].waitForFirst = false;
        } else
        {

            if (BLK_param[blk_num][sensor_num].currentData >= BLK_param[blk_num][sensor_num].paramB)
            {
                tmp_data = BLK_param[blk_num][sensor_num].paramA;
            }
            else
            {
                tmp_data = BLK_param[blk_num][sensor_num].currentData + BLK_param[blk_num][sensor_num].paramC;
                if (tmp_data > BLK_param[blk_num][sensor_num].paramB)
                {
                    tmp_data = BLK_param[blk_num][sensor_num].paramA;
                }
            }

        }

        BLK_param[blk_num][sensor_num].currentData = tmp_data;
        return tmp_data;

        break;

    case 2:

        if (BLK_param[blk_num][sensor_num].waitForFirst)
        {
            tmp_data = BLK_param[blk_num][sensor_num].currentData;
            BLK_param[blk_num][sensor_num].waitForFirst = false;
        } else
        {
            tmp_data = BLK_param[blk_num][sensor_num].currentData - BLK_param[blk_num][sensor_num].paramC;
            if (tmp_data < BLK_param[blk_num][sensor_num].paramB)
            {
                tmp_data = BLK_param[blk_num][sensor_num].paramA;
            }

        }

        BLK_param[blk_num][sensor_num].currentData = tmp_data;
        return tmp_data;

        break;

    case 3:

        if (BLK_param[blk_num][sensor_num].waitForFirst)
        {
            tmp_data = BLK_param[blk_num][sensor_num].currentData;
            BLK_param[blk_num][sensor_num].isTreugolUp = true;
            BLK_param[blk_num][sensor_num].waitForFirst = false;

        } else
        {
            if (BLK_param[blk_num][sensor_num].isTreugolUp)
            {
                tmp_data = BLK_param[blk_num][sensor_num].currentData + BLK_param[blk_num][sensor_num].paramC;
                if (tmp_data > BLK_param[blk_num][sensor_num].paramB)
                {
                    tmp_data = BLK_param[blk_num][sensor_num].currentData - BLK_param[blk_num][sensor_num].paramC;
                    BLK_param[blk_num][sensor_num].isTreugolUp = false;
                }
            } else
            {

                tmp_data = BLK_param[blk_num][sensor_num].currentData - BLK_param[blk_num][sensor_num].paramC;
                if (tmp_data < BLK_param[blk_num][sensor_num].paramB)
                {
                    tmp_data = BLK_param[blk_num][sensor_num].currentData + BLK_param[blk_num][sensor_num].paramC;
                    BLK_param[blk_num][sensor_num].isTreugolUp = true;
                }
            }

        }

        BLK_param[blk_num][sensor_num].currentData = tmp_data;
        return tmp_data;


        break;

    case 4:

        break;

    case 5:

        break;

    case 6:

        break;

    default:

        return 0;
    }

    return 0;
}

// Проверка счетчика пакетов БЛК на монотонность и времени в пакетах
// return:
// true  - Нужно дальше проверять текущий пакет
// false - Текущий пакет проверять не нужно

bool dCheckKadr::CheckBLKPackageCounter(quint8 blk_num, quint8 sensor_num, quint8 curr_counter, quint64 packageTime)
{
    if (BLK_param[blk_num][sensor_num].waitForFirst)
    {
        BLK_param[blk_num][sensor_num].lastPackageNumber = curr_counter;
        qDebug() << "WaitForFirst for BLK" << blk_num << " sensor " << sensor_num;

        BLK_param[blk_num][sensor_num].lastPackageTime = packageTime;
        // Добавить сохранение времени

        return true;
    } else
    {
        if (BLK_param[blk_num][sensor_num].lastPackageNumber == 255) // Если переход через 0
        {
            if (curr_counter == 0) // Если нормально перешли.
            {
                // Подсчет и проверка времени

                BLK_param[blk_num][sensor_num].lastPackageTime = packageTime;
                BLK_param[blk_num][sensor_num].lastPackageNumber = curr_counter;
                return true;
            } else
            {
                if (curr_counter < 20) // Типа считаем, что максимум потерять можем около 20 пакетов
                {
                    // Потеря пакета
// Добавить вывод дельты в отчет
                    error_stream << tr("        ") <<
                                    tr("Потеря пакета. Предыдущий пакет: ") << QString::number(BLK_param[blk_num][sensor_num].lastPackageNumber) << "\r\n";

                    BLK_param[blk_num][sensor_num].waitForFirst = true; // Типа первый пакет ищем и все с начала
                    BLK_param[blk_num][sensor_num].lastPackageNumber = curr_counter;
                    BLK_param[blk_num][sensor_num].lastPackageTime = packageTime;
                    BLK_Error[blk_num][sensor_num].lossPackage++;
                    ADD_BMP_ERROR();
                    return true;
                } else // Считаем, что идет повторный пакет, его не проверяем и игнорирурем
                {
                    error_stream << tr("        ") <<
                                    tr("Повтор пакета. Предыдущий пакет: ") << QString::number(BLK_param[blk_num][sensor_num].lastPackageNumber) << "\r\n";
                    BLK_Error[blk_num][sensor_num].RepeatPackage++;
                    return false;
                }
            }
        } else // Нет перехода через ноль, обычное больше - меньше.
               // Больше чем на единицу, потеря пакетов, меньше или равно - повтор
        {
            if (curr_counter == (BLK_param[blk_num][sensor_num].lastPackageNumber + 1)) // Нормально течение событий
            {
                // Подсчет и проверка времени
                BLK_param[blk_num][sensor_num].lastPackageTime = packageTime;
                BLK_param[blk_num][sensor_num].lastPackageNumber = curr_counter;
                return true;
            } else
            {
                if (curr_counter > (BLK_param[blk_num][sensor_num].lastPackageNumber + 1)) // Потеря пакетов
                {
                    // Добавить вывод дельты в отчет
                                        error_stream << tr("        ") <<
                                                        tr("Потеря пакета. Предыдущий пакет: ") << QString::number(BLK_param[blk_num][sensor_num].lastPackageNumber) << "\r\n";

                                        BLK_param[blk_num][sensor_num].waitForFirst = true; // Типа первый пакет ищем и все с начала
                                        BLK_param[blk_num][sensor_num].lastPackageNumber = curr_counter;
                                        BLK_param[blk_num][sensor_num].lastPackageTime = packageTime;
                                        BLK_Error[blk_num][sensor_num].lossPackage++;
                                        ADD_BMP_ERROR();
                                        return true;

                } else // Повтор пакетов
                {
                    error_stream << tr("        ") <<
                                    tr("Повтор пакета. Предыдущий пакет: ") << QString::number(BLK_param[blk_num][sensor_num].lastPackageNumber) << "\r\n";
                    BLK_Error[blk_num][sensor_num].RepeatPackage++;
                    return false;

                }
            }
        }
    }
}

quint16 dCheckKadr::calculatePZUPos(quint16 currPos, bool isPS1, quint8 currentInput)
{
    quint16 tmpPos;
    quint16 currLK = currPos % 8;  // Остаток от деления как раз номер локального входа
   // qint16 subAddr = 1;

  //  bool NashMAS;



    if (KadrType == KADR_RTSC)
    {


        if ((isPS1 && ZapretPS1[currLK]) || (!isPS1 && ZapretPS2[currLK]))
        {
            tmpPos = currPos;
        } else
        {
            if (isPS1)
            {
                if (currPos == 0)
                {
                    tmpPos = 1023;
                } else
                    tmpPos = currPos - 1;

                while (!((PRG_PS1.data_str_r[tmpPos].num_in == currentInput) &&
                       (PRG_PS1.data_str_r[tmpPos].zap_mac == 0)))
                {
                    if (tmpPos == 0)
                        tmpPos = 1023;
                    else
                        tmpPos--;
                }
            } else
            {
                if (currPos == 0)
                {
                    tmpPos = 511;
                } else
                    tmpPos = currPos - 1;

                while (!((PRG_PS2.data_str_r[tmpPos].num_in == currentInput) &&
                         (PRG_PS2.data_str_r[tmpPos].zap_mac == 0)))
                {
                    if (tmpPos == 0)
                        tmpPos = 511;
                    else
                        tmpPos--;

                }
            }
        }

        /*
         *
         *
        if ((isPS1 && DualPS1[currLK]) || (!isPS1 && DualPS2[currLK]))
        {
            if ((isPS1 && ZapretPS1[currLK]) || (!isPS1 && ZapretPS2[currLK]))
            {
                subAddr = 4;

                if (currPos > 4)
                {
                    if (isPS1)
                    {
                        if (PRG_PS1.data_str_r[currPos - 4].zap_mac == 1)
                        {
                            subAddr = 8;
                        }
                    } else
                    {
                        if(PRG_PS2.data_str_r[currPos - 4].zap_mac == 1)
                        {
                            subAddr = 8;
                        }
                    }
                } else
                {
                    if (isPS1)
                    {
                        if (PRG_PS1.data_str_r[currPos + 1024 - 4].zap_mac == 1)
                        {
                            subAddr = 8;
                        }
                    } else
                    {
                        if(PRG_PS2.data_str_r[currPos + 512 - 4].zap_mac == 1)
                        {
                            subAddr = 8;
                        }
                    }
                }
            }
            else
                subAddr = 16;
        } else
            subAddr = 8;

        if (currPos >= subAddr)
        {
            tmpPos = currPos - subAddr;

        } else
        {
            if (isPS1)
                tmpPos = currPos + 1024 - subAddr;
            else
                tmpPos = currPos + 512 - subAddr;
        } */

        return tmpPos;
    }
    if (KadrType == KADR_RTSCM)
    {
        if ((isPS1 && DualPS1[currLK]) || (!isPS1 && DualPS2[currLK]))
        {
            if (currPos >= 16)
            {
                tmpPos = currPos - 16;
            } else
            {
                tmpPos = currPos + 1024 - 16;
            }

        } else
        {
            tmpPos = currPos;
        }

        return tmpPos;
    }

    if ((KadrType == KADR_RTSCM1) || (KadrType == KADR_VAAR))
    {
        if ((isPS1 && DualPS1[currLK]) || (!isPS1 && DualPS2[currLK]))
        {
            if (currPos >= 16)
            {
                tmpPos = currPos - 16;
            } else
            {
                tmpPos = currPos + 1024 - 16;
            }

        } else if ((isPS1 == false) && ((currLK == 0) || (currLK == 4)))
        {

            tmpPos = currPos + 8;
            if (tmpPos >= 1024)
            {
                tmpPos = tmpPos - 1024;
            }
        } else
        {
            tmpPos = currPos;
        }
        return tmpPos;
    }
}

void dCheckKadr::sl_CheckKadr()
{

    UDP_PACKAGE package;

    quint8 ssk;                 // Текущий ССК
    quint8 min;                 // Текущие минуты из кадра
    quint8 sec;                 // Текущие секунды из кадра
    quint16 msec;               // Текущие миллисекунды из кадра

    quint8 kss;                 // Текущий КСС
    quint8 sec_kadr, min_kadr;  // Байт с секундами(минутами) из кадра, вместе с метками

    quint8 M1Sec, M2Sec;        // Метки из байта секунд
    quint8 M1Min, M2Min;        // Метки из байта минут

    quint16 MinKadrSec;         // Минимально число кадров в секунду для проверки времени
    quint16 MaxKadrSec;         // Максимальное число кадров в секунду для проверки времени
    quint32 KadrTime;

    quint32 KadrCounter;        // Счетчик кадров
    quint32 old_KadrCounter;    // Предыдущее значение счетчика, для проверки потерь в сети

    quint8 blk_number;
    quint8 blk_sensor_number;

    quint64 BLKTime_ms;
    quint64 KadrTime_ms;
    quint64 delay;

    bool isPS1;                 // Если текущая программа опроса ПС1

    qDebug() << " ====================== " << sizeof(UDP_PACKAGE);

    if (KadrType == KADR_VAAR)
        isPS1 = false;

    CLR_ERROR();

    for (int blk = 0; blk < 4; blk++)
    {
        for (int sens = 0; sens < 24; sens++)
        {
            BLK_param[blk][sens].waitForFirst = true;
            BLK_param[blk][sens].isTreugolUp = true;
            BLK_param[blk][sens].currentData = 0;

            BLK_Error[blk][sens].All_package = 0;
            BLK_Error[blk][sens].OK_package = 0;
            BLK_Error[blk][sens].Err_package = 0;
            BLK_Error[blk][sens].lossPackage = 0;
            BLK_Error[blk][sens].RepeatPackage = 0;
        }

    }

    UpdateErrorData();

    setRsvChannel(0);            // Выключаем прием кадров

    // ----------------------------------- Запись файла ----------------------------------------


    fWriteKadr wrDialog;

    wrDialog.hide();

    RecvKadr = 0;
    currentReadTime = 0;

    wrDialog.setStartData(KadrReadTime);
    connect(this, SIGNAL(UpdateReadingInfo(quint32, quint32)), &wrDialog, SLOT(sl_update(quint32, quint32)));
    connect(this, SIGNAL(StopReading()), &wrDialog, SLOT(sl_Stop()));



//    if (kadrTmpFile.isOpen())
//    {
//        kadrTmpFile.close();
//    }

//    if (kadrTmpFile.open())
//    {
//        qDebug() << kadrTmpFile.fileName();
//    }

    wrDialog.show();

    KadrReading = true;

    int channelNum = InputButtonGroup -> checkedId();

    if ((channelNum == -1) || (channelNum == 0))
    {
        QMessageBox::information(this, tr("Проверка кадра"), tr("Выберите вход для проверки"), QMessageBox::Ok);
        return;

    }

    setRsvChannel(channelNum);            // Поправить номер входа


    KadrReadTimer -> start(1000);

    //wrDialog.setWindowModality(Qt::WindowActive);

    while (wrDialog.getModalResult() == fWriteKadr::None){
        QApplication::processEvents();
    }

    setRsvChannel(0);

    if (wrDialog.getModalResult() == fWriteKadr::Cancel)
    {
        qDebug() << "User stop";
        return;
    }

    disconnect(this, SIGNAL(UpdateReadingInfo(quint32,quint32)), &wrDialog, SLOT(sl_update(quint32,quint32)));
    disconnect(this, SIGNAL(StopReading()), &wrDialog, SLOT(sl_Stop()));

    wrDialog.close();

    if (RecvKadr == 0)
    {
        QMessageBox::critical(this, tr("Ошибка проверки кадра"), tr("Не принято ни одного кадра"), QMessageBox::Ok);
        return;
    }

    QFile ErrorFile("error.txt");
    QFile BLKDataFile("blk.txt");



    if (!ErrorFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Error open report error file";
        return;
    }

    if (!BLKDataFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Error open blk data file";
        return;
    }



   error_stream.setDevice(&ErrorFile);
   QTextStream BLKDatastream(&BLKDataFile);

    CheckKadrFile = new QFile(kadrTmpFile -> fileName());

    if (!CheckKadrFile -> open(QIODevice::ReadOnly))
    {
        qDebug() << "Error opening kadr file";
        return;
    }


    progressBar ->setMaximum(CheckKadrFile -> size() / sizeof(UDP_PACKAGE));
    progressBar -> setValue(0);
    progressBar -> setVisible(true);
    // Читаем несколько первых пакетов

    CheckKadrFile -> read((char *)&package, sizeof(UDP_PACKAGE));
    GET_KADR_COUNTER(old_KadrCounter, package);

    //qDebug() << QString::number(package.counter);

    progressBar -> setValue(KadrCheckNumber++);
    CheckKadrFile -> read((char *)&package, sizeof(UDP_PACKAGE));
    GET_KADR_COUNTER(old_KadrCounter, package);

    //qDebug() << QString::number(package.counter);

    switch (KadrType)
    {
    case KADR_RTSCM:
    case KADR_RTSCM1:

        old_ssk = package.KD.M_PACKAGE.SSK;
        old_sec = package.KD.M_PACKAGE.SEK & 0x3F;
        old_min = package.KD.M_PACKAGE.MIN & 0x3F;
        break;
    case KADR_VAAR:

        old_ssk = package.KD.V_PACKAGE.pCount & 0x3F;
        old_sec = package.KD.V_PACKAGE.SEK & 0x3F;
        old_min = package.KD.V_PACKAGE.MIN & 0x3F;
        old_msec = ((quint16)package.KD.V_PACKAGE.MSEK_H << 8) | (quint16)package.KD.V_PACKAGE.MSEK_L;
        break;


    case KADR_RTSC:
        old_ssk = package.KD.R_PACKAGE.KADR[RSSK_POSITION];
        old_sec = (package.KD.R_PACKAGE.KADR[RSEK_POSITION] >> 2) & 0x3F;
        old_min = (package.KD.R_PACKAGE.KADR[RMIN_POSITION] >> 2) & 0x3F;



    }
    kadrTimeNumber = 0;
    timeChecking = false;


    while (!CheckKadrFile -> atEnd())   // Тут пока не нашли конец файла
    {

       // Читаем пакет
        progressBar -> setValue(KadrCheckNumber++);
        CheckKadrFile -> read((char *)&package, sizeof(UDP_PACKAGE));
        GET_KADR_COUNTER(KadrCounter, package);

        GET_SSK(ssk, package);
        GET_KSS(kss, package);
        GET_SEK(sec_kadr, package);
        GET_MIN(min_kadr, package);

        if (KadrCounter != old_KadrCounter + 1)
        {
 // Произошла потеря пакетов по UDP

            qDebug() << "[" << KadrCheckNumber << "] " << "Loss package: " << old_KadrCounter << " - " << KadrCounter;

            error_stream << "[" << KadrCheckNumber << "] " << "Loss package: " << old_KadrCounter << " - " << KadrCounter << "\r\n";


            if (KadrType == KADR_VAAR)
            {
                if (ssk == 1)
                {
                    old_ssk = 63;
                } else
                {
                    old_ssk = ssk - 2;
                }
            } else
            {
                if (ssk == 0)
                {
                    old_ssk = 127;
                } else
                {
                    old_ssk = ssk - 1;
                }
            }

            timeChecking = false;

            switch (KadrType)
            {
            case KADR_RTSCM:
            case KADR_RTSCM1:

                old_sec = package.KD.M_PACKAGE.SEK & 0x3F;
                old_min = package.KD.M_PACKAGE.MIN & 0x3F;
                break;

            case KADR_VAAR:
                old_sec = package.KD.V_PACKAGE.SEK & 0x3F;
                old_min = package.KD.V_PACKAGE.MIN & 0x3F;
                old_msec = ((quint16)package.KD.V_PACKAGE.MSEK_H << 8) | (quint16)package.KD.V_PACKAGE.MSEK_L;
                break;

            case KADR_RTSC:
                old_sec = (package.KD.R_PACKAGE.KADR[RSEK_POSITION] >> 2) & 0x3F;
                old_min = (package.KD.R_PACKAGE.KADR[RMIN_POSITION] >> 2) & 0x3F;



            }

            for (int blk = 0; blk < 4; blk++)
            {
                for (int sens = 0; sens < 24; sens++)
                {
                    BLK_param[blk][sens].waitForFirst = true;
                    BLK_param[blk][sens].isTreugolUp = true;
                    BLK_param[blk][sens].currentData = 0;
                }

            }

        }


        old_KadrCounter = KadrCounter;
        //qDebug() << QString::number(package.counter);

        switch (KadrType)
        {
        case KADR_RTSCM:
        case KADR_RTSCM1:

            if (kss & 0x10) isPS1 = false; else isPS1 = true;

            sec = sec_kadr & 0x3F;
            min = min_kadr & 0x3F;

            M1Sec = (sec_kadr >> 6) & 0x01;
            M2Sec = (sec_kadr >> 7) & 0x01;

            M1Min = (min_kadr >> 6) & 0x01;
            M2Min = (min_kadr >> 7) & 0x01;

            MinKadrSec = 198;
            MaxKadrSec = 202;
            KadrTime = 5;
            break;

        case KADR_VAAR:


            sec = sec_kadr & 0x3F;
            min = min_kadr & 0x3F;

            M1Sec = (sec_kadr >> 6) & 0x01;
            M2Sec = (sec_kadr >> 7) & 0x01;

            M1Min = (min_kadr >> 6) & 0x01;
            M2Min = (min_kadr >> 7) & 0x01;

            MinKadrSec = 198;
            MaxKadrSec = 202;
            KadrTime = 5;
            break;


        case   KADR_RTSC:

            if (kss & 0x08) isPS1 = false; else isPS1 = true;

            sec = (sec_kadr >> 2) & 0x3F;
            min = (min_kadr >> 2) & 0x3F;

            M1Sec = sec_kadr & 0x01;
            M2Sec = (sec_kadr >> 1) & 0x01;

            M1Min = min_kadr & 0x01;
            M2Min = (min_kadr >> 1) & 0x01;

            MinKadrSec = 98;
            MaxKadrSec = 102;
            KadrTime = 10;
            break;
        }


    // Проверка ССК на соответствие

        if (cbCheckSSK -> isChecked())
        {
            if (KadrType == KADR_VAAR)
            {
                if (old_ssk == 63)
                {
                    if (ssk != 1)
                    {
                        ADD_SSK_ERROR();
                        error_stream << tr("[") << KadrCheckNumber << tr("]")
                                     << tr("Ошибка ССК. Ожидаемое значение: 1")
                                     << tr(" полученное значение: ") << QString::number(ssk) << "\r\n";
                    }
                } else
                {
                    if (ssk != (old_ssk + 2))
                    {
                        ADD_SSK_ERROR();
                        error_stream << tr("[") << KadrCheckNumber << tr("]")
                                     << tr("Ошибка ССК. Ожидаемое значение: ") << QString::number(old_ssk + 2)
                                     << tr(" полученное значение: ") << QString::number(ssk) << "\r\n";
                    }
                }


            } else
            {
                if (old_ssk == 127)
                {
                    if (ssk != 0)
                    {
                        ADD_SSK_ERROR();
                        error_stream << tr("[") << KadrCheckNumber << tr("]")
                                     << tr("Ошибка ССК. Ожидаемое значение: 0")
                                     << tr(" полученное значение: ") << QString::number(ssk) << "\r\n";
                    }
                } else
                {
                    if (ssk != (old_ssk + 1))
                    {
                        ADD_SSK_ERROR();
                        error_stream << tr("[") << KadrCheckNumber << tr("]")
                                     << tr("Ошибка ССК. Ожидаемое значение: ") << QString::number(old_ssk + 1)
                                     << tr(" полученное значение: ") << QString::number(ssk) << "\r\n";
                    }
                }
            }

            old_ssk = ssk;

            UpdateErrorData();
        }

    // Проверка времени на соответствие

        if (cbCheckKadrTime ->isChecked())
        {
            if (KadrType == KADR_VAAR)
            {

            } else
            {
                if (old_sec != sec)
            {
                if (timeChecking)
                {

                    if ((kadrTimeNumber < MinKadrSec) || (kadrTimeNumber > MaxKadrSec))
                    {
                        ADD_TIME_ERROR();
                        error_stream << tr("[") << KadrCheckNumber << tr("]")
                                     << tr("Ошибка длительности секундного интервала: ")
                                     << QString::number(old_sec) <<  tr("c -> ") << QString::number(sec) << tr("c ")
                                     << tr(" -- ") << QString::number((kadrTimeNumber + 1) * KadrTime) << tr("мс") << "\r\n";
                    }

                    if (old_sec == 59)
                    {
                        if (sec != 0)
                        {
                            ADD_TIME_ERROR();
                            error_stream << tr("[") << KadrCheckNumber << tr("]")
                                         << tr("Ошибка счетчика секунд. Ожидается: 0с, получено: ")
                                         << QString::number(sec) << tr("c ") << "\r\n";

                            old_sec = sec;


                        } else
                        {
                            if (old_min == 59)
                            {
                                if (min != 0)
                                {
                                    ADD_TIME_ERROR();
                                    error_stream << tr("[") << KadrCheckNumber << tr("]")
                                                 << tr("Ошибка счетчика минут. Ожидается: 0 мин, получено: ")
                                                 << QString::number(min) << tr("мин  ")
                                                 << QString::number(sec) << tr("sec") << "\r\n";
                                } else
                                {
                                    old_min = 0;
                                }
                            } else
                            {
                                if (min != (old_min + 1))
                                {

                                    ADD_TIME_ERROR();
                                    error_stream << tr("[") << KadrCheckNumber << tr("]")
                                                 << tr("Ошибка счетчика минут. Ожидается: ")
                                                 << QString::number(old_min + 1) << tr(" мин, получено: ")
                                                 << QString::number(min) << tr("мин  ")
                                                 << QString::number(sec) << tr("sec") << "\r\n";
                                    old_min = min;

                                } else
                                {
                                    old_min = min;
                                }
                            }
                        }
                    } else
                    {
                        if (sec != old_sec + 1)
                        {
                            ADD_TIME_ERROR();
                            error_stream << tr("[") << KadrCheckNumber << tr("]")
                                         << tr("Ошибка счетчика секунд. Ожидается: ")
                                         << QString::number(old_sec + 1) << tr(" c, получено: ")
                                         << QString::number(sec) << tr("c") << "\r\n";
                            old_sec = sec;

                        } else
                        {
                            old_sec = sec;
                        }
                    }


                } else
                {

                    timeChecking = true;
                }

                kadrTimeNumber = 0;
                old_sec = sec;
                old_min = min;

                } else
            {
                kadrTimeNumber++;
                if (old_min != min)
                {
                    ADD_TIME_ERROR();
                    error_stream << tr("[") << KadrCheckNumber << tr("]")
                                 << tr("Ошибка счетчика минут. Ожидается: ")
                                 << QString::number(old_min) << tr(" мин, получено: ")
                                 << QString::number(min) << tr("мин  ")
                                 << QString::number(sec) << tr("sec") << "\r\n";
                }

            }

            }
        }

        UpdateErrorData();

    // Проверка М1 и М2


        if (cbCheckM1M2 -> isChecked())
        {
            if ((sec & 0x01) != M1Sec)
            {
                ADD_M1_ERROR();
                error_stream << tr("[") << KadrCheckNumber << tr("]")
                             << tr("Ошибка метки М1 в байте секунд. Ожидается: ")
                             << QString::number(sec & 0x01) << tr(" получено: ")
                             << QString::number(M1Sec) << tr(" время: ") << QString::number(sec) << tr("с.") << "\r\n";

            }

            if ((sec & 0x01) != M1Min)
            {
                ADD_M1_ERROR();
                error_stream << tr("[") << KadrCheckNumber << tr("]")
                             << tr("Ошибка метки М1 в байте минут. Ожидается: ")
                             << QString::number(sec & 0x01) << tr(" получено: ")
                             << QString::number(M1Min) << tr(" время: ") << QString::number(sec) << tr("с.") << "\r\n";

            }

            quint8 DecSec = sec / 10;

            if ((DecSec & 0x01) != M2Sec)
            {
                ADD_M2_ERROR();
                error_stream << tr("[") << KadrCheckNumber << tr("]")
                             << tr("Ошибка метки М2 в байте секунд. Ожидается: ")
                             << QString::number(DecSec & 0x01) << tr(" получено: ")
                             << QString::number(M2Sec) << tr(" время: ") << QString::number(sec) << tr("с.") << "\r\n";

            }

            if ((DecSec & 0x01) != M2Min)
            {
                ADD_M2_ERROR();
                error_stream << tr("[") << KadrCheckNumber << tr("]")
                             << tr("Ошибка метки М2 в байте минут. Ожидается: ")
                             << QString::number(DecSec & 0x01) << tr(" получено: ")
                             << QString::number(M2Min) << tr(" время: ") << QString::number(sec) << tr("с.") << "\r\n";

            }

            UpdateErrorData();
        }


    // Проверка CRC

        if (cbCheckCRC -> isChecked())
        {
            quint16 calc_crc;
            quint16 read_crc;

            if (KadrType == KADR_VAAR)
            {
                calc_crc = crc16(package.KD.V_PACKAGE.MMP, 512);
                read_crc = (((quint16)package.KD.V_PACKAGE.crc[0] << 8) & 0xFF00) | ((quint16)package.KD.V_PACKAGE.crc[1] & 0x00FF);
            } else
            {

                calc_crc = crc16(package.KD.M_PACKAGE.MMP, 256);
                read_crc = (((quint16)package.KD.M_PACKAGE.MMP[256] << 8) & 0xFF00) | ((quint16)package.KD.M_PACKAGE.MMP[257] & 0x00FF);
            }

            if (calc_crc != read_crc)
            {
                ADD_CRC_ERROR();
                error_stream << tr("[") << KadrCheckNumber << tr("]")
                             << tr("Ошибка CRC. Расчетное CRC: ")
                             << QString::number(calc_crc, 16) << tr(" принятое: ")
                             << QString::number(read_crc, 16) << "\r\n";

            }

            UpdateErrorData();

        }

    // Проверка кода Рида-Соломона

        if (cbCheckRS -> isChecked())
        {

            if(KadrType == KADR_VAAR)
            {

            } else
            {
                quint8 buff_data[SIZE_BLOCK_DATA];
                quint8 *frame = (quint8 *)&package;

                for (quint8 i = 0; i < 3; i++)
                {
                    if (i == 2)
                    {
                        memset(buff_data, 0, SIZE_BLOCK_DATA);

                        memcpy(buff_data, &frame[SIZE_BLOCK_DATA * i], (SIZE_OF_FRAME -
                                                                        (SIZE_BLOCK_RS + SYNC_MARKER_SIZE) -
                                                                        (SIZE_BLOCK_DATA * 2)));
                    } else
                    {
                        memcpy(buff_data, &frame[SIZE_BLOCK_DATA * i], SIZE_BLOCK_DATA);
                    }

                    if (RSObject.check_rs(buff_data, &frame[(SIZE_OF_FRAME -
                                                         (SIZE_BLOCK_RS + SYNC_MARKER_SIZE)) + i * NPAR]) != 0)
                    {
                        ADD_RS_ERROR();
                    error_stream << tr("[") << KadrCheckNumber << tr("]") <<
                                    tr("Ошибка кода Рида-Соломона в блоке ") << QString::number(i + 1) << "\r\n";
                    }
                }
            }

            UpdateErrorData();

        }

    // Проверка кадра ММП

        if (cbCheckMMP -> isChecked())
        {

            switch (KadrType)
            {
                case KADR_RTSCM:

                    if (isPS1)
                    {
                        for (int kadrPos = 0; kadrPos < 256; kadrPos++)
                        {
                            if (er_param_PS1[ssk][kadrPos].MSS_NUM == 255)
                                continue;

                            if (package.KD.M_PACKAGE.MMP[kadrPos] != ImmKadr_PS1[ssk][kadrPos])
                            {
#ifdef TEST

                                if (er_param_PS1[ssk][kadrPos].MSS_NUM != 4)
                                {

#endif
                                    ADD_MMP_ERROR();

                                    error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС1) ") <<
                                                    tr("Ошибка в кадре ССК:") <<  ssk <<
                                                    tr(" позиция: ") << kadrPos <<
                                                    tr(" Ожидаемое значение: ") << ImmKadr_PS1[ssk][kadrPos] <<
                                                    tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";
                                    error_stream << tr("    Номер входа МАС: ") << er_param_PS1[ssk][kadrPos].MSS_NUM <<
                                                    tr(" номер ЛК: ") << er_param_PS1[ssk][kadrPos].LK_NUM << "\r";
                                    error_stream << tr("    Адрес ЛК: ") << er_param_PS1[ssk][kadrPos].LK_ADDR <<
                                                    tr(" канал ЛК: ") << er_param_PS1[ssk][kadrPos].LK_Channel << "\r\n";

                    //qDebug() << "Error is pos " << kadrPos << "from SSK " << ssk << " imm" << ImmKadr[ssk][kadrPos] << " kadr " << package.MMP[kadrPos];

#ifdef TEST
                                } else
                                {
                                    error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС1) ") <<
                                                    tr("Ошибка в кадре ССК:") <<  ssk <<
                                                    tr(" позиция: ") << kadrPos <<
                                                    tr(" Ожидаемое значение: ") << ImmKadr_PS1[ssk][kadrPos] <<
                                                    tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";
                                    error_stream << tr("    Номер входа МАС: ") << er_param_PS1[ssk][kadrPos].MSS_NUM <<
                                                    tr(" номер ЛК: ") << er_param_PS1[ssk][kadrPos].LK_NUM << "\r";
                                    error_stream << tr("    Адрес ЛК: ") << er_param_PS1[ssk][kadrPos].LK_ADDR <<
                                                    tr(" канал ЛК: ") << er_param_PS1[ssk][kadrPos].LK_Channel << "\r\n";
                                }
#endif
                            }

                        }
                    } else
                    {
                        for (int kadrPos = 0; kadrPos < 256; kadrPos++)
                        {
                            if (er_param_PS2[ssk][kadrPos].MSS_NUM == 255)
                                continue;
                          if (package.KD.M_PACKAGE.MMP[kadrPos] != ImmKadr_PS2[ssk][kadrPos])
                          {
                               ADD_MMP_ERROR();

                              error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС2) ") <<
                                              tr("Ошибка в кадре ССК:") <<  ssk <<
                                              tr(" позиция: ") << kadrPos <<
                                              tr(" Ожидаемое значение: ") << ImmKadr_PS2[ssk][kadrPos] <<
                                              tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";
                              error_stream << tr("    Номер входа МАС: ") << er_param_PS2[ssk][kadrPos].MSS_NUM <<
                                              tr(" номер ЛК: ") << er_param_PS2[ssk][kadrPos].LK_NUM << "\r";
                              error_stream << tr("    Адрес ЛК: ") << er_param_PS2[ssk][kadrPos].LK_ADDR <<
                                              tr(" канал ЛК: ") << er_param_PS2[ssk][kadrPos].LK_Channel << "\r\n";

                    //qDebug() << "Error is pos " << kadrPos << "from SSK " << ssk << " imm" << ImmKadr[ssk][kadrPos] << " kadr " << package.MMP[kadrPos];
                          }

                        }

                    }
                    break;

            case KADR_VAAR:

                quint16 pos_m1;
                quint8 ssk_m1;

                if (package.KD.V_PACKAGE.IKP1 != 0x2A)
                {
                    AllError++;
                    error_stream << tr("[") << KadrCheckNumber << tr("]") <<
                                    tr("Ошибка в загаловке кадра ССК: ") <<  ssk <<
                                    tr(" 1-й байт главного заголовка ") <<
                                    tr(" Ожидаемое значение: 0x2A") <<
                                    tr(" полученное значение: ") << QString::number(package.KD.V_PACKAGE.IKP1, 16) << "\r";

                }

                if (package.KD.V_PACKAGE.IKP2 != 0xA0)
                {
                    AllError++;
                    error_stream << tr("[") << KadrCheckNumber << tr("]") <<
                                    tr("Ошибка в загаловке кадра ССК: ") <<  ssk <<
                                    tr(" 2-й байт главного заголовка ") <<
                                    tr(" Ожидаемое значение: 0xA0") <<
                                    tr(" полученное значение: ") << QString::number(package.KD.V_PACKAGE.IKP2, 16) << "\r";

                }


                for (int kadrPos = 0; kadrPos < 512; kadrPos++)
                {

                   pos_m1 = PRG_VAAR[512 + kadrPos];


//                    if (pos_m1 < 256)
//                    {
//                        ssk_m1 = ssk - 1;
//                    } else
//                    {
//                        pos_m1 = pos_m1 - 256;
//                    }

                    if (kadrPos < 256)
                    {
                        ssk_m1 = ssk - 1;
                    } else
                    {
                        ssk_m1 = ssk;
                    }


                    if ((kadrPos == VSEK1_POSITION) || (kadrPos == VSEK2_POSITION))
                    {
                        continue;
                    }
                    if ((kadrPos == VKSS1_POSITION) || (kadrPos == VKSS2_POSITION))
                    {
                        continue;
                    }
                    if ((kadrPos == VSSK1_POSITION) || (kadrPos == VSSK2_POSITION))
                    {
                        continue;
                    }

                    if (package.KD.V_PACKAGE.MMP[kadrPos] != ImmKadr_PS1[ssk_m1][pos_m1])
                    {
                         ADD_MMP_ERROR();

                        error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС2) ") <<
                                        tr("Ошибка в кадре ССК:") <<  ssk <<
                                        tr(" позиция: ") << kadrPos <<
                                        tr(" Ожидаемое значение: ") << ImmKadr_PS2[ssk_m1][pos_m1] <<
                                        tr(" полученное значение: ") << package.KD.V_PACKAGE.MMP[kadrPos] << "\r";
//                        error_stream << tr("    Номер входа МАС: ") << er_param_PS2[ssk_m1][pos_m1].MSS_NUM <<
//                                        tr(" номер ЛК: ") << er_param_PS2[ssk_m1][pos_m1].LK_NUM << "\r";
//                        error_stream << tr("    Адрес ЛК: ") << er_param_PS2[ssk_m1][pos_m1].LK_ADDR <<
//                                        tr(" канал ЛК: ") << er_param_PS2[ssk_m1][pos_m1].LK_Channel << "\r\n";

              //qDebug() << "Error is pos " << kadrPos << "from SSK " << ssk << " imm" << ImmKadr[ssk][kadrPos] << " kadr " << package.MMP[kadrPos];
                    }


                }

                break;

            case KADR_RTSCM1:

                if (isPS1)
                {
                    for (int kadrPos = 0; kadrPos < 256; kadrPos++)
                    {
                        if (((ssk & 0x01) == M1KSS_SSK_PS1) && (kadrPos == M1KSS_POSITION_PS1))
                        {
                            if (package.KD.M_PACKAGE.MMP[kadrPos] != package.KD.M_PACKAGE.KSS)
                            {
                                ADD_MMP_ERROR();
                                error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС1) ") <<
                                                tr("Ошибка КСС в кадре ССК:") <<  ssk <<
                                                tr(" позиция: ") << kadrPos <<
                                                tr(" Ожидаемое значение: ") << package.KD.M_PACKAGE.KSS <<
                                                tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";

                            }
                            continue;
                        }

                        if (((ssk & 0x01) == M1SSK_SSK_PS1) && (kadrPos == M1SSK_POSITION_PS1))
                        {
                            if (package.KD.M_PACKAGE.MMP[kadrPos] != package.KD.M_PACKAGE.SSK)
                            {
                                 ADD_MMP_ERROR();
                                 error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС1) ") <<
                                                 tr("Ошибка ССK в кадре ССК:") <<  ssk <<
                                                 tr(" позиция: ") << kadrPos <<
                                                 tr(" Ожидаемое значение: ") << package.KD.M_PACKAGE.SSK <<
                                                 tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";

                             }
                            continue;
                         }

                        if (((ssk & 0x01) == M1SEK_SSK_PS1) && (kadrPos == M1SEK_POSITION_PS1))
                        {
                              if (package.KD.M_PACKAGE.MMP[kadrPos] != package.KD.M_PACKAGE.SEK)
                              {
                                   ADD_MMP_ERROR();
                                   error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС1) ") <<
                                                   tr("Ошибка СЕК в кадре ССК:") <<  ssk <<
                                                   tr(" позиция: ") << kadrPos <<
                                                   tr(" Ожидаемое значение: ") << package.KD.M_PACKAGE.SEK <<
                                                   tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";
                              }

                              continue;
                        }

                        if (((ssk & 0x01) == M1MIN_SSK_PS1) && (kadrPos == M1MIN_POSITION_PS1))
                        {
                            if (package.KD.M_PACKAGE.MMP[kadrPos] != package.KD.M_PACKAGE.MIN)
                            {
                                ADD_MMP_ERROR();
                                error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС1) ") <<
                                                tr("Ошибка МИН в кадре ССК:") <<  ssk <<
                                                tr(" позиция: ") << kadrPos <<
                                                tr(" Ожидаемое значение: ") << package.KD.M_PACKAGE.MIN <<
                                                tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";

                            }
                            continue;
                        }

                        if (((ssk & 0x01) == M1KADR_SSK_PS1) && (kadrPos == M1KADR_POSITION_PS1))
                        {
                            if (package.KD.M_PACKAGE.MMP[kadrPos] != 0xFF)
                            {
                                ADD_MMP_ERROR();
                                error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС1) ") <<
                                                tr("Ошибка КАДР в кадре ССК:") <<  ssk <<
                                                tr(" позиция: ") << kadrPos <<
                                                tr(" Ожидаемое значение: 255") <<
                                                tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";
                            }

                            continue;
                        }

                        if (er_param_PS1[ssk][kadrPos].MSS_NUM == 255)
                           continue;

                        if (package.KD.M_PACKAGE.MMP[kadrPos] != ImmKadr_PS1[ssk][kadrPos])
                        {
#ifdef TEST

                            if (er_param_PS1[ssk][kadrPos].MSS_NUM != 4)
                            {

#endif
                                ADD_MMP_ERROR();
                                error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС1) ") <<
                                                tr("Ошибка в кадре ССК:") <<  ssk <<
                                                tr(" позиция: ") << kadrPos <<
                                                tr(" Ожидаемое значение: ") << ImmKadr_PS1[ssk][kadrPos] <<
                                                tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";
                                error_stream << tr("    Номер входа МАС: ") << er_param_PS1[ssk][kadrPos].MSS_NUM <<
                                                tr(" номер ЛК: ") << er_param_PS1[ssk][kadrPos].LK_NUM << "\r";
                                error_stream << tr("    Адрес ЛК: ") << er_param_PS1[ssk][kadrPos].LK_ADDR <<
                                                tr(" канал ЛК: ") << er_param_PS1[ssk][kadrPos].LK_Channel << "\r\n";

                //qDebug() << "Error is pos " << kadrPos << "from SSK " << ssk << " imm" << ImmKadr[ssk][kadrPos] << " kadr " << package.MMP[kadrPos];

#ifdef TEST
                            } else
                            {
                                error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС1) ") <<
                                                tr("Ошибка в кадре ССК:") <<  ssk <<
                                                tr(" позиция: ") << kadrPos <<
                                                tr(" Ожидаемое значение: ") << ImmKadr_PS1[ssk][kadrPos] <<
                                                tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";
                                error_stream << tr("    Номер входа МАС: ") << er_param_PS1[ssk][kadrPos].MSS_NUM <<
                                                tr(" номер ЛК: ") << er_param_PS1[ssk][kadrPos].LK_NUM << "\r";
                                error_stream << tr("    Адрес ЛК: ") << er_param_PS1[ssk][kadrPos].LK_ADDR <<
                                                tr(" канал ЛК: ") << er_param_PS1[ssk][kadrPos].LK_Channel << "\r\n";
                            }
#endif
                        }

                    } // end for
                } else
                {
                    for (int kadrPos = 0; kadrPos < 256; kadrPos++)
                    {

                        if (((ssk & 0x01) == M1KSS_SSK_PS2) && (kadrPos == M1KSS_POSITION_PS2))
                        {
                            if (package.KD.M_PACKAGE.MMP[kadrPos] != package.KD.M_PACKAGE.KSS)
                            {
                                ADD_MMP_ERROR();
                                error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС2) ") <<
                                                tr("Ошибка КСС в кадре ССК:") <<  ssk <<
                                                tr(" позиция: ") << kadrPos <<
                                                tr(" Ожидаемое значение: ") << package.KD.M_PACKAGE.KSS <<
                                                tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";

                            }
                            continue;
                        }

                        if (((ssk & 0x01) == M1SSK_SSK_PS2) && ((kadrPos == M1SSK_POSITION_PS2) || (kadrPos == M1SSK2_POSITION_PS2)))
                        {
                            if (package.KD.M_PACKAGE.MMP[kadrPos] != package.KD.M_PACKAGE.SSK)
                            {
                                ADD_MMP_ERROR();
                                error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС2) ") <<
                                                tr("Ошибка ССK в кадре ССК:") <<  ssk <<
                                                tr(" позиция: ") << kadrPos <<
                                                tr(" Ожидаемое значение: ") << package.KD.M_PACKAGE.SSK <<
                                                tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";

                            }
                            continue;
                        }

                        if (((ssk & 0x01) == M1SEK_SSK_PS2) && (kadrPos == M1SEK_POSITION_PS2))
                        {
                            if (package.KD.M_PACKAGE.MMP[kadrPos] != package.KD.M_PACKAGE.SEK)
                            {
                                ADD_MMP_ERROR();
                                error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС2) ") <<
                                                tr("Ошибка СЕК в кадре ССК:") <<  ssk <<
                                                tr(" позиция: ") << kadrPos <<
                                                tr(" Ожидаемое значение: ") << package.KD.M_PACKAGE.SEK <<
                                                tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";

                            }
                            continue;
                        }

                        if (((ssk & 0x01) == M1MIN_SSK_PS2) && (kadrPos == M1MIN_POSITION_PS2))
                        {
                            if (package.KD.M_PACKAGE.MMP[kadrPos] != package.KD.M_PACKAGE.MIN)
                            {
                                ADD_MMP_ERROR();
                                error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС2) ") <<
                                                tr("Ошибка МИН в кадре ССК:") <<  ssk <<
                                                tr(" позиция: ") << kadrPos <<
                                                tr(" Ожидаемое значение: ") << package.KD.M_PACKAGE.MIN <<
                                                tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";

                            }
                            continue;
                        }

                        if (((ssk & 0x01) == M1KADR_SSK_PS2) && (kadrPos == M1KADR_POSITION_PS2))
                        {
                            if (package.KD.M_PACKAGE.MMP[kadrPos] != 0xFF)
                            {
                                ADD_MMP_ERROR();
                                error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС2) ") <<
                                                tr("Ошибка КАДР в кадре ССК:") <<  ssk <<
                                                tr(" позиция: ") << kadrPos <<
                                                tr(" Ожидаемое значение: 255") <<
                                                tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";

                            }
                            continue;
                        }


                        if (er_param_PS2[ssk][kadrPos].MSS_NUM == 255)
                            continue;
                      if (package.KD.M_PACKAGE.MMP[kadrPos] != ImmKadr_PS2[ssk][kadrPos])
                      {
                           ADD_MMP_ERROR();

                          error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС2) ") <<
                                          tr("Ошибка в кадре ССК:") <<  ssk <<
                                          tr(" позиция: ") << kadrPos <<
                                          tr(" Ожидаемое значение: ") << ImmKadr_PS2[ssk][kadrPos] <<
                                          tr(" полученное значение: ") << package.KD.M_PACKAGE.MMP[kadrPos] << "\r";
                          error_stream << tr("    Номер входа МАС: ") << er_param_PS2[ssk][kadrPos].MSS_NUM <<
                                          tr(" номер ЛК: ") << er_param_PS2[ssk][kadrPos].LK_NUM << "\r";
                          error_stream << tr("    Адрес ЛК: ") << er_param_PS2[ssk][kadrPos].LK_ADDR <<
                                          tr(" канал ЛК: ") << er_param_PS2[ssk][kadrPos].LK_Channel << "\r\n";

                //qDebug() << "Error is pos " << kadrPos << "from SSK " << ssk << " imm" << ImmKadr[ssk][kadrPos] << " kadr " << package.MMP[kadrPos];
                      }

                    }

                }
                break;


            case KADR_RTSC:

                if (isPS1)
                {
                    for (int kadrPos = 0; kadrPos < 512; kadrPos++)
                    {
                        if ((kadrPos != RKSS_POSITION) &&
                                (kadrPos != RSEK_POSITION) &&
                                (kadrPos != RMIN_POSITION) &&
                                (kadrPos != RSSK_POSITION))
                        {
                            if (ZapretPS1[kadrPos % 8] || (er_param_PS1[ssk][kadrPos].MSS_NUM == 255))
                                continue;

                            if (package.KD.R_PACKAGE.KADR[kadrPos] != ImmKadr_PS1[ssk][kadrPos])
                            {
                                ADD_MMP_ERROR();

                                error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС1) ") <<
                                                tr("Ошибка в кадре ССК:") <<  ssk <<
                                                tr(" позиция: ") << kadrPos <<
                                                tr(" Ожидаемое значение: ") << ImmKadr_PS1[ssk][kadrPos] <<
                                                tr(" полученное значение: ") << package.KD.R_PACKAGE.KADR[kadrPos] << "\r";
                                error_stream << tr("    МАС: ") << mas_input[er_param_PS1[ssk][kadrPos].MSS_NUM]->getMASName() << "(" << er_param_PS1[ssk][kadrPos].MSS_NUM << ") " <<
                                                tr(" номер ЛК: ") << er_param_PS1[ssk][kadrPos].LK_NUM << "\r";
                                error_stream << tr("    Адрес ЛК: ") << er_param_PS1[ssk][kadrPos].LK_ADDR <<
                                                tr(" канал ЛК: ") << er_param_PS1[ssk][kadrPos].LK_Channel << "\r\n";
                            }
                        }
                    }

                } else
                {
                    for (int kadrPos = 0; kadrPos < 512; kadrPos++)
                    {
                        if ((kadrPos != RKSS_POSITION) &&
                                (kadrPos != RSEK_POSITION) &&
                                (kadrPos != RMIN_POSITION) &&
                                (kadrPos != RSSK_POSITION))
                        {
                            if (ZapretPS2[kadrPos % 8] || (er_param_PS2[ssk][kadrPos].MSS_NUM == 255))
                                continue;


                            if (package.KD.R_PACKAGE.KADR[kadrPos] != ImmKadr_PS2[ssk][kadrPos])
                            {
                                ADD_MMP_ERROR();

                                error_stream << tr("[") << KadrCheckNumber << tr("]") << tr(" (ПС2) ") <<
                                                tr("Ошибка в кадре ССК:") <<  ssk <<
                                                tr(" позиция: ") << kadrPos <<
                                                tr(" Ожидаемое значение: ") << ImmKadr_PS2[ssk][kadrPos] <<
                                                tr(" полученное значение: ") << package.KD.R_PACKAGE.KADR[kadrPos] << "\r";
                                error_stream << tr("    МАС: ") << mas_input[er_param_PS2[ssk][kadrPos].MSS_NUM]->getMASName() << "(" << er_param_PS2[ssk][kadrPos].MSS_NUM << ") " <<
                                                 tr(" номер ЛК: ") << er_param_PS2[ssk][kadrPos].LK_NUM << "\r";
                                error_stream << tr("    Адрес ЛК: ") << er_param_PS2[ssk][kadrPos].LK_ADDR <<
                                                tr(" канал ЛК: ") << er_param_PS2[ssk][kadrPos].LK_Channel << "\r\n";
                            }
                        }
                    }

                }
                break;
            }


            UpdateErrorData();
        }

     // Проверка БМП

        if (cbCheckBMP -> isChecked())
        {

            bool package_good = true;
            bool package_repeat = false;

            blk_package_good = false;   // Пока не проверили пакет недостоверный

            if (package.KD.M_PACKAGE.BMP[0] > 0) // Если в кадре есть пакет БЛК
            {
                blk_number = package.KD.M_PACKAGE.BMP[0] - 1;
                blk_sensor_number = package.KD.M_PACKAGE.BMP[1] - 1;

                if ((blk_number > 3) || (blk_sensor_number > 23)) // Неправельный номер БЛК или датчика
                {
                    ADD_BMP_ERROR();
                    error_stream << tr("[") << KadrCheckNumber << tr("]") <<
                                    tr("Ошибка в заголовке пакета БЛК. ") <<
                                    tr("БЛК") << QString::number(blk_number + 1) <<
                                    tr(" датчик: ") << QString::number(blk_sensor_number + 1) << "\r\n";

                } else
                {

            // Просто в лог информация о пакете
                    BLKTime_ms =  (((quint64)package.KD.M_PACKAGE.BMP[2] * 60) + (quint64)package.KD.M_PACKAGE.BMP[3]) * 1000 +
                            (quint64)((((quint16)package.KD.M_PACKAGE.BMP[4] << 8) & 0xFF00) | ((quint16)package.KD.M_PACKAGE.BMP[5] & 0x00FF));

                    if ((BLKTime_ms >= BLK_param[blk_number][blk_sensor_number].lastPackageTime)
                            || (BLK_param[blk_number][blk_sensor_number].waitForFirst == true))
                    { // Если интервал правильный и мы не ждем первый пакет
//                        error_stream << tr("[") << KadrCheckNumber << tr("]") <<
//                                        tr(" БЛК") << QString::number(blk_number + 1) <<
//                                        tr(" датчик: ") << QString::number(blk_sensor_number + 1) <<
//                                        tr(" номер пакета: ") << QString::number(package.KD.M_PACKAGE.BMP[6]) <<
//                                        tr("  время: ") <<
//                                        QString::number(package.KD.M_PACKAGE.BMP[2], 16) << ":" << QString::number(package.KD.M_PACKAGE.BMP[3], 16) << "." <<
//                                        QString::number(package.KD.M_PACKAGE.BMP[4], 16) << QString::number(package.KD.M_PACKAGE.BMP[5], 16) <<
//                                        tr(" Интервал между пакетами: ") << QString::number(BLKTime_ms - BLK_param[blk_number][blk_sensor_number].lastPackageTime) << tr(" мс.\r\n");
                        // Сохраняем принятый пакет в файле БЛК
                        BLKDatastream << KadrCheckNumber << ";" <<
                                         QString::number(blk_number + 1) << ";" <<
                                         QString::number(blk_sensor_number + 1) << ";" <<
                                         QString::number(package.KD.M_PACKAGE.BMP[6]) << ";" <<
                                         QString::number(BLKTime_ms) << ";" <<
                                         QString::number(BLKTime_ms - BLK_param[blk_number][blk_sensor_number].lastPackageTime) << ";" <<
                                                                                         QString::number(package.KD.M_PACKAGE.BMP[2], 16) << ";" <<
                                         QString::number(package.KD.M_PACKAGE.BMP[3], 16) << ";" <<
                                         QString::number(package.KD.M_PACKAGE.BMP[4], 16) << ";" <<
                                         QString::number(package.KD.M_PACKAGE.BMP[5], 16) << ";";

                        for (int i = 7; i < 326; i++)
                            BLKDatastream << QString::number(package.KD.M_PACKAGE.BMP[i]) << ";";
                        BLKDatastream << "\r\n";
                        blk_package_good = true; // Время в пакете вроде нормальное

                    }
                    else
                    {  // Какая-то хрень с интервалом между пакетами
                        error_stream << tr("[") << KadrCheckNumber << tr("]") <<
                                        tr(" БЛК") << QString::number(blk_number + 1) <<
                                        tr(" датчик: ") << QString::number(blk_sensor_number + 1) <<
                                        tr(" номер пакета: ") << QString::number(package.KD.M_PACKAGE.BMP[6]) <<
                                        tr("  время: ") <<
                                        QString::number(package.KD.M_PACKAGE.BMP[2], 16) << ":" << QString::number(package.KD.M_PACKAGE.BMP[3], 16) << "." <<
                                        QString::number(package.KD.M_PACKAGE.BMP[4], 16) << QString::number(package.KD.M_PACKAGE.BMP[5], 16) <<
                                        tr(" Интервал между пакетами не верный: ") << QString::number(BLKTime_ms - BLK_param[blk_number][blk_sensor_number].lastPackageTime) << tr(" мс.\r\n");
                        BLKDatastream << KadrCheckNumber << ";" <<
                                         QString::number(blk_number + 1) << ";" <<
                                         QString::number(blk_sensor_number + 1) << ";" <<
                                         QString::number(package.KD.M_PACKAGE.BMP[6]) << ";" <<
                                         QString::number(BLKTime_ms) << ";" <<
                                         QString::number(-1) << ";" <<
                                                                                         QString::number(package.KD.M_PACKAGE.BMP[2], 16) << ";" <<
                                         QString::number(package.KD.M_PACKAGE.BMP[3], 16) << ";" <<
                                         QString::number(package.KD.M_PACKAGE.BMP[4], 16) << ";" <<
                                         QString::number(package.KD.M_PACKAGE.BMP[5], 16) << ";";
                        for (int i = 7; i < 326; i++)
                            BLKDatastream << QString::number(package.KD.M_PACKAGE.BMP[i]) << ";";
                        BLKDatastream  << "\r\n";

                    }


                // Проверка, что нужный пакет в нужном потоке

                    if (BLK_param[blk_number][blk_sensor_number].PotokTM != Potok[channelNum - 1])
                    {
                        ADD_BMP_ERROR();
                        package_good = false;
                        error_stream << tr("        ") <<
                                        tr("Принятый пакет БЛК не должен находиться в этом потоке. ") <<
                                        "\r\n";
                        blk_package_good = false;

                    } else {
                        if (BLK_param[blk_number][blk_sensor_number].Freq == 0)
                        {
                            ADD_BMP_ERROR();
                            package_good = false;
                            error_stream << tr("        ")<<
                                            tr("Принятый пакет БЛК не формируется имитатором. ") << "\r\n";

                            blk_package_good = false;

                        } else
                        {
                            if (CheckBLKPackageCounter(blk_number, blk_sensor_number, package.KD.M_PACKAGE.BMP[6], BLKTime_ms))
                            {

                                quint8 blk_data;

                                if (BLK_param[blk_number][blk_sensor_number].waitForFirst)
                                { // Если это первый принятый пакет с этого датчика

                                    BLK_param[blk_number][blk_sensor_number].currentData = package.KD.M_PACKAGE.BMP[7]; // Откуда пляшем, это правильно, так-как на патере пакетов тоже сюда должны попасть
                                    BLK_param[blk_number][blk_sensor_number].lastPackageNumber = package.KD.M_PACKAGE.BMP[6];
                                    BLK_param[blk_number][blk_sensor_number].lastPackageTime = BLKTime_ms;
                                    blk_package_good = false;

                                } else
                                {

                                }




                                for (int i = 7; i < 326; i++)
                                {
                                    blk_data = getBLKData(blk_number, blk_sensor_number);
                                    if (package.KD.M_PACKAGE.BMP[i] != blk_data)
                                    {
                                        ADD_BMP_ERROR();
                                        package_good = false;
                                        error_stream << tr("        ") <<
                                                        tr("Ошибка в пакете. ") <<
                                                        tr(" В позиции: ") << QString::number(i - 5) <<
                                                        tr(" ожидаемое значение: ") << QString::number(blk_data) <<
                                                        tr(" полученное значение: ") << QString::number(package.KD.M_PACKAGE.BMP[i]) << "\r\n";


                                    }
                                }

                                } else
                                {
                                    package_repeat = true;
                                }
                            }
                        }

                    if (package_repeat == false)
                    {
                        if (package_good)
                            BLK_Error[blk_number][blk_sensor_number].OK_package++;
                        else
                            BLK_Error[blk_number][blk_sensor_number].Err_package++;

                        BLK_Error[blk_number][blk_sensor_number].All_package++;
                    }
                }
            } else
            {
                for (int i = 0; i < 326; i++)
                {
                    if (package.KD.M_PACKAGE.BMP[i] != 0)
                    {
                        error_stream << tr("[") << KadrCheckNumber << tr("]") <<
                                        tr("Не нулевое значение в позиции БМП. Позиция: ") <<
                                        QString::number(i + 1) << tr(", значение: ") << QString::number(package.KD.M_PACKAGE.BMP[i]) << "\r\n";
                       ADD_BMP_ERROR();
                    }
                }
            }


        }


        // Проверка задержки

        if (cbCheckDelay -> isChecked() && (currentPotokType == POTOK_ZAD) && (blk_package_good == true))
                {

                    if ((package.KD.M_PACKAGE.BMP[0] > 0) && (timeChecking))
                    {
                        BLKTime_ms =  (((quint64)package.KD.M_PACKAGE.BMP[2] * 60) + (quint64)package.KD.M_PACKAGE.BMP[3]) * 1000 +
                                (quint64)((((quint16)package.KD.M_PACKAGE.BMP[4] << 8) & 0xFF00) | ((quint16)package.KD.M_PACKAGE.BMP[5] & 0x000FF));

                        BLKTime_ms += blk_package_time[BLK_param[package.KD.M_PACKAGE.BMP[0]][package.KD.M_PACKAGE.BMP[1]].Freq];

                        if (BLKTime_ms >= 3600000)
                            BLKTime_ms -= 3600000;


                      //  if (BLKTime_ms < 3000) BLKTime_ms += 3600000;


                        KadrTime_ms = ((quint64)(package.KD.M_PACKAGE.MIN & 0x3F) * 60 + (quint64)(package.KD.M_PACKAGE.SEK & 0x3F)) * 1000 +
                                (quint64)(kadrTimeNumber * 5);

                        if (KadrTime_ms >= BLKTime_ms) BLKTime_ms += 3600000;

                        delay = BLKTime_ms - KadrTime_ms;

                        if (delay < DELAY_MIN || delay > DELAY_MAX)
                        {
                            ADD_DELAY_ERROR();
                            error_stream << tr("[") << KadrCheckNumber << tr("]")
                                         << tr("Время БЛК: ") << QString::number(BLKTime_ms) << tr("мс ")
                                         << QString::number(package.KD.M_PACKAGE.BMP[2]) << ":" << QString::number(package.KD.M_PACKAGE.BMP[3])
                                         << tr("  Время кадр: ") << QString::number(KadrTime_ms) << tr("мс ")
                                         << QString::number(package.KD.M_PACKAGE.MIN & 0x3F) << ":" << QString::number(package.KD.M_PACKAGE.SEK & 0x3F)

                                         << tr(" задержка: ") << QString::number((qint64)BLKTime_ms - (qint64)KadrTime_ms) << tr("мс ")
                                         << tr(" Задержка не верная\r\n");


                        }

                        UpdateErrorData();
                    }


                }

        UpdateErrorData();


    }

    for (int blk = 0; blk < 4; blk++)
    {
        error_stream << tr("БЛК") << QString::number(blk + 1) << "   ==========================" << "\r\n";

        for(int sens = 0; sens < 24; sens++)
        {
            if (BLK_param[blk][sens].Freq != 0)
            {
                error_stream << tr("Датчик") << QString::number(sens + 1) << ": "
                             << QString::number(BLK_Error[blk][sens].Err_package) << tr(" сбойных пакетов, ")
                             << QString::number(BLK_Error[blk][sens].lossPackage) << tr(" потеряно пакетов, ")
                             << QString::number(BLK_Error[blk][sens].RepeatPackage) << tr(" повторов пакетов из ")
                             << QString::number(BLK_Error[blk][sens].All_package) << "\r\n";
            }
        }
     }

    ErrorFile.close();
    BLKDataFile.close();
    CheckKadrFile -> close();
    delete kadrTmpFile;

    progressBar -> setVisible(false);
    qDebug() << "Check end";

    if (AllError > 0)
    {
        qDebug() << "Error in kadr: " << AllError;
        ViewError();


    } else
    {
        QMessageBox::information(this, tr("Проверка кадра"), tr("Ошибок не обнаружено"), QMessageBox::Ok);
    }


}

void dCheckKadr::sl_SaveResult()
{
    qDebug() << "Save Result";

    QStringList fileToCompress;
    fileToCompress << "kadr.txt" << "kadr_et.txt" << "error.txt" << "blk.txt";
    fileToCompress << "pult.log";
    fileToCompress << UstFileName;
    fileToCompress << DevFileName;

    qDebug() << "File to archive: " << fileToCompress;

    JlCompress::compressFiles((QApplication::applicationDirPath() +
                               "//ErrorCheck//" +
                               QDateTime::currentDateTime().toString("dd_MM_yyyy-hh_mm_ss") +
                               ".zip"),
                              fileToCompress);

}

void dCheckKadr::sl_Close()
{
    this -> close();
}

void dCheckKadr::sl_Recv_PackageM()
{
    while(recv_socket -> hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(recv_socket -> pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        quint16 msek;

        recv_socket -> readDatagram(datagram.data(), datagram.size(),
                                    &sender, &senderPort);

// ======================  Отображение текущей информации =======================================



        lKadrNumber -> setText(QString::number(RecvKadr));

        QString ps_type;

        if (datagram[KSS_POSITION] & 0x10)

            ps_type = tr("ПС2");
        else
            ps_type = tr("ПС1");


        lKSS -> setText(QString("%1(%2)").arg(QString::number(datagram[KSS_POSITION], 2)).arg(ps_type));

        lKadrTime -> setText(QString("%1:%2").arg(QString::number(datagram[MIN_POSITION] & 0x3F)).arg(QString::number(datagram[SEK_POSITION] & 0x3F)));

        msek = (((quint16)datagram[MSEK_H_BLK_POSITION] << 5) & 0x1FE0) | (((quint16)datagram[MSEK_L_BLK_POSITION] >> 3) & 0x001F);

        if (datagram.at(NBLK_POSITION) > 0)
        {
            lBLKTime -> setText(QString("%1:%2.%3").arg(QString::number(datagram[MIN_BLK_POSITION]))
                                               .arg(QString::number(datagram[SEK_BLK_POSITION]))
                                               .arg(QString::number(msek)));
        }


        if (!cbManualChannelSelect -> isChecked())
            kadrTmpFile -> write(datagram);
        RecvKadr++;
    }
}

void dCheckKadr::sl_Recv_PackageV()
{
    while(recv_socket -> hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(recv_socket -> pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        quint16 msek;

        recv_socket -> readDatagram(datagram.data(), datagram.size(),
                                    &sender, &senderPort);

// ======================  Отображение текущей информации =======================================



       // ps_type = tr("ПС2");
        lKadrNumber -> setText(QString::number(RecvKadr));
        QString ps_type;

        if (datagram[VKSS1_POSITION + 20] & 0x10)

            ps_type = tr("ПС2");
        else
            ps_type = tr("ПС1");


        lKSS -> setText(QString("%1(%2)").arg(QString::number(datagram[VKSS1_POSITION + 20], 2)).arg(ps_type));

        lKadrTime -> setText(QString("%1:%2").arg(QString::number(datagram[VMIN_POSITION] & 0x3F)).arg(QString::number(datagram[VSEK_POSITION] & 0x3F)));

//        msek = (((quint16)datagram[MSEK_H_BLK_POSITION] << 5) & 0x1FE0) | (((quint16)datagram[MSEK_L_BLK_POSITION] >> 3) & 0x001F);

//        if (datagram.at(NBLK_POSITION) > 0)
//        {
//            lBLKTime -> setText(QString("%1:%2.%3").arg(QString::number(datagram[MIN_BLK_POSITION]))
//                                               .arg(QString::number(datagram[SEK_BLK_POSITION]))
//                                               .arg(QString::number(msek)));
//        }

        lSSK -> setText(QString::number(datagram[13]));

        if (!cbManualChannelSelect -> isChecked())
            kadrTmpFile -> write(datagram);
        RecvKadr++;
    }
}

void dCheckKadr::sl_Recv_PackageR()
{
    lBLKTime -> setText("-");
    while(recv_socket -> hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(recv_socket -> pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

       // quint16 msek;

        recv_socket -> readDatagram(datagram.data(), datagram.size(),
                                    &sender, &senderPort);

// ======================  Отображение текущей информации =======================================



        lKadrNumber -> setText(QString::number(RecvKadr));

        QString ps_type;

        if (datagram[RKSS_POSITION] & 0x08)

            ps_type = tr("ПС2");
        else
            ps_type = tr("ПС1");


        lKSS -> setText(QString("%1(%2)").arg(QString::number(datagram[RKSS_POSITION], 2)).arg(ps_type));

        lKadrTime -> setText(QString("%1:%2").arg(QString::number((datagram[RMIN_POSITION] >> 2) & 0x3F)).arg(QString::number((datagram[RSEK_POSITION] >> 2) & 0x3F)));


        if (!cbManualChannelSelect -> isChecked())
            kadrTmpFile -> write(datagram);
        RecvKadr++;
    }

}

void dCheckKadr::sl_startRecv(int num)
{

    if (num == 0)
    {
        lPotokType -> setText(tr("Отключено"));
        currentPotokType = POTOK_OFF;
    }
    else
    {
        switch (Potok[num - 1]) {

        case POTOK_OSN:
            lPotokType -> setText(tr("ТМ1 (Основной)"));
            currentPotokType = POTOK_OSN;
            break;
        case POTOK_ZAD:
            lPotokType -> setText(tr("ТМ2 (Задерж."));
            currentPotokType = POTOK_ZAD;
            break;
        default:
            break;
        }
    }

    if (cbManualChannelSelect -> isChecked())
    {
        setRsvChannel(num);
    }
}

void dCheckKadr::sl_KadrReadTimerTimeOut()
{
    currentReadTime++;

    if ((currentReadTime == 10) && (RecvKadr < 10))
    {
        KadrReading = false;
        KadrReadTimer -> stop();
        kadrTmpFile -> close();
        emit StopReading();
        return;
    }
    if (currentReadTime > KadrReadTime)
    {
        KadrReading = false;
        KadrReadTimer -> stop();
        kadrTmpFile -> close();
        emit StopReading();
    } else
    {
        emit UpdateReadingInfo(currentReadTime, RecvKadr);
    }

}

void dCheckKadr::sl_FPGALoadOk()
{
    pbCheckKadr -> setEnabled(true);

}

