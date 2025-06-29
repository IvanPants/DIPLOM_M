#include "mainwindow.h"


#include <QApplication>

#include <iostream>

#include <QTextStream>
#include <QFile>
#include <QTextCodec>
#include <QDateTime>
#include <QSysInfo>

#include <JlCompress.h>


static QTextStream *logStream;
static QFile *logFile;

MainWindow *w;



static const char * msgType[] =
{
    "(II) ", // Info
    "(WW) ", // Warning
    "(EE) ", // Error
    "(FF) "  // Fatal error

};

const QString TextDescription = QObject::tr(
            "%1 %2\n"
            "Build on " __DATE__ " at " __TIME__ ".\n"
            "Based 0n Qt %3.\n")
        .arg(QLatin1String(APP_NAME), (QString::number(NVER1) + "." +
                                                    QString::number(NVER2) + "." +
                                                    QString::number(NVER3) + "." +
                                                    QString::number(NVER4)), QLatin1String(QT_VERSION_STR));

// Вывод лога в консоль
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
// Создание файла для логов
void installLog();
// Закрытие файла логов
void finishLog(int rmes);
// Информация об ОС
QString getOSInfo();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

   // QTextCodec::setCodecForTr(QTextCodec::codecForName("Windows-1251"));

    //a.setApplicationVersion("1.0.0");

   // QApplication::setApplicationVersion("1.0.0");



    w = new MainWindow;

    installLog();

    w  -> show();
    
    int mainReturn = a.exec();

    finishLog(mainReturn);

    return mainReturn;
}

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    std::cout << msgType[type] << msg.toStdString() << std::endl;

    if (logStream && logStream -> device())
    {
        *logStream << msgType[type] << QTime::currentTime().toString() << "(" << context.file << ":" << context.line << ")" << msg << endl;
    }
}

void installLog()
{
    logFile = new QFile("tk170.log");
    if (logFile -> open(QFile::WriteOnly | QIODevice::Unbuffered))
        logStream = new QTextStream(logFile);

#ifdef Q_WS_WIN
    logStream -> setCodec("Windows -1251");
#else
    logStream -> setCodec("utf-8");
#endif

    if (logStream && logStream -> device())
    {
        *logStream << endl << TextDescription << endl;
        *logStream << QString("Markers: (II) information, (WW) warning,") << endl;
        *logStream << QString("(EE) error, (FF) fatal error.") << endl;
        *logStream <<getOSInfo() << endl;
        *logStream << QString("Runned at %1.").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss")) << endl << endl;
    }

    qInstallMessageHandler(customMessageHandler);
    qDebug() << "Success opening log file";
}

void finishLog(int rmes)
{
    qDebug() << "Succes closing log file";




    delete logStream;
    logStream = 0;

    logFile -> close();
    delete logFile;
    logFile = 0;

    qInstallMessageHandler(0);

    if (rmes != 0)
    {

//        QString ArcFileName("%1/ErrorLogs//%2.zip").arg(QApplication::applicationFilePath())
//                .arg()

        qDebug() << "Archive log to: " << (QApplication::applicationDirPath() + "//ErrorLogs//" + QDateTime::currentDateTime().toString("dd_MM_yyyy-hh_mm_ss") + ".zip");
        JlCompress::compressFile((QApplication::applicationDirPath() + "//ErrorLogs//" + QDateTime::currentDateTime().toString("dd_MM_yyyy-hh_mm_ss") + ".zip"),
                                 "tk170.log");

    }

}

QString getOSInfo()
{
    QString infoStr("Current Operating Sysytem: %1:");

#ifdef Q_OS_WIN
    switch(QSysInfo::windowsVersion())
    {
    case QSysInfo::WV_NT: return infoStr.arg("Windows NT");
    case QSysInfo::WV_2000: return infoStr.arg("Windows 2000");
    case QSysInfo::WV_XP: return infoStr.arg("Windows XP");
    case QSysInfo::WV_VISTA: return infoStr.arg("Windows Vista");
    case QSysInfo::WV_WINDOWS7: return infoStr.arg("Windows 7");
    case QSysInfo::WV_WINDOWS8: return infoStr.arg("Windows 8");
    default: return infoStr.arg("Windows");
    }

#endif

#ifdef Q_OS_LINUX
#ifdef LINUX_OS_VERSION
    return infoStr.arg(LINUX_OS_VERSION);
#else
    return infoStr.arg("Linux");
#endif
#endif
}
