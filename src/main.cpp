/**
 * Project: Klass Package Manager
 * * Description:
 * This application is a specialized package management utility developed
 * specifically for the Slackware Linux distribution. Its primary purpose
 * is to provide an intuitive interface for users to browse, search, and
 * manage their installed software.
 * * Key Features:
 * - Advanced package categorization to streamline the Slackware experience.
 * - Integration with official repositories and custom local mirrors.
 * - System tray support for minimized operation and background notifications.
 * - Unified interface for database updates and dependency management.
 * * Purpose:
 * Klass aims to simplify Slackware administration by transforming complex
 * terminal-based operations into a clean, categorized, and user-friendly
 * graphical experience.
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QLibraryInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QProcess>
#include <QTranslator>

#include <SingleApplication>

#include <DefaultPath.hpp>
#include <Helper.hpp>
#include <Klass.hpp>
#include <MessageReceiver.hpp>
#include <SettingsManager.hpp>
#include <StatusIcons.hpp>
#include <SysTray.hpp>
#include <Theme.hpp>
#include <UpdateWorker.hpp>
#include <Utils.hpp>
#include <Version.hpp>

inline const auto TRAY_IPC_SERVER = QStringLiteral("klass_tray_ipc");

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--helper") == 0) {
            qputenv("QT_QPA_PLATFORM", "minimal");
            qputenv("QT_NODBUS", "1");
            qputenv("QT_NO_GLIB", "1");
            qputenv("QT_LOGGING_RULES", "*=false");

            QCoreApplication app(argc, argv);
            Helper h;

            Utils::releaseFreeMemory();
            return QCoreApplication::exec();
        }
    }

    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
    QApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents, true);

    SingleApplication app(argc, argv, true, SingleApplication::Mode::SecondaryNotification);

    QApplication::setApplicationName("Klass");
    QApplication::setOrganizationName(QApplication::applicationName());
    QApplication::setApplicationDisplayName(QApplication::applicationName());
    QApplication::setApplicationVersion(VERSION);

    Debug::msg("Program version", "Main", {QApplication::applicationVersion()});
    Debug::msg("Program pid", "Main", {QString::number(QApplication::applicationPid())});

    const QCommandLineOption autoStartOption("autostart", QObject::tr("Start the application in system tray."));
    const QCommandLineOption packageOption(QStringList() << "p" << "package",
                                           QObject::tr("Install package in system."), "package");

    QCommandLineParser parser;
    parser.addOption(autoStartOption);
    parser.addOption(packageOption);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale::system(), QStringLiteral("qtbase"), QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        SingleApplication::installTranslator(&qtTranslator);
    }

    QTranslator translator;
    const auto lang = QLocale::system().name();
    if (auto trFile = DefaultPath::defaultPath(QString("lang/klass_%1.qm").arg(lang));
        translator.load(trFile)) {
        Debug::msg("Loading translation", "Translator", {trFile, DColor::LightGreen});
        SingleApplication::installTranslator(&translator);
    } else {
        Debug::msg("Translation not available", "Translator", {DColor::LightRed});
    }

    if (app.isSecondary()) {
        Debug::msg("Secondary instance: Opening Klass GUI directly", "Main");
        Theme::applyTheme();
        StatusIcons::init();
        Klass klass;

        QLocalSocket ipcSocket;
        ipcSocket.connectToServer(TRAY_IPC_SERVER);

        QObject::connect(&ipcSocket, &QLocalSocket::readyRead, [&] {
            if (const QByteArray msg = ipcSocket.readAll(); msg.contains("UPDATE"))
                klass.onUpdateAvailable();
        });

        QObject::connect(&klass, &Klass::databaseUpdateFinished, [&] {
            if (ipcSocket.state() == QLocalSocket::ConnectedState)
                ipcSocket.write("UPDATE_DONE\n");
        });

        if (parser.isSet(packageOption)) {
            const auto pkg = parser.value(packageOption);
            Debug::msg("Processing package from command line", "Main", {pkg});
            klass.handlePackage(pkg);
        }

        if (const SettingsManager sm; sm.startMaximized() || sm.windowMaximize())
            klass.showMaximized();
        else
            klass.show();

        klass.raise();
        klass.activateWindow();

        return SingleApplication::exec();
    }

    SysTray tray(&app);
    const SettingsManager sm;

    bool hasUpdate = false;
    QProcess guiProcess;
    QPointer<QLocalSocket> guiSocket;

    QLocalServer ipcServer(&app);
    QLocalServer::removeServer(TRAY_IPC_SERVER);
    ipcServer.listen(TRAY_IPC_SERVER);

    QObject::connect(&ipcServer, &QLocalServer::newConnection, [&] {
        guiSocket = ipcServer.nextPendingConnection();

        if (hasUpdate && guiSocket)
            guiSocket->write("UPDATE\n");

        QObject::connect(guiSocket, &QLocalSocket::readyRead, [&] {
            if (guiSocket && guiSocket->readAll().contains("UPDATE_DONE")) {
                hasUpdate = false;
                tray.setUpdateAvailable(false);
            }
        });
    });

    auto toggleGui = [&](const QStringList &args = {}) {
        if (guiProcess.state() == QProcess::NotRunning) {
            QStringList runArgs;
            if (parser.isSet(packageOption) && args.isEmpty())
                runArgs << "-p" << parser.value(packageOption);
            else
                runArgs << args;

            guiProcess.start(QCoreApplication::applicationFilePath(), runArgs);
        } else {
            guiProcess.terminate();
        }
    };

    QObject::connect(&tray, &SysTray::openRequested, [&] {
        toggleGui();
    });

    QObject::connect(&tray, &SysTray::quitRequested, [&] {
        if (guiProcess.state() != QProcess::NotRunning) {
            guiProcess.terminate();
            guiProcess.waitForFinished(1000);
        }
        QCoreApplication::quit();
    });

    const MessageReceiver msg;
    QObject::connect(&app, &SingleApplication::receivedMessage, &msg, &MessageReceiver::receivedMessage);
    QObject::connect(&msg, &MessageReceiver::arg, [&](const QString &arg) {
        if (arg == "__OPEN__") {
            if (guiProcess.state() == QProcess::NotRunning)
                toggleGui();
        } else if (!arg.isEmpty()) {
            if (guiProcess.state() != QProcess::NotRunning) {
                guiProcess.terminate();
                guiProcess.waitForFinished(500);
            }
            toggleGui({"-p", arg});
        }
    });

    UpdateWorker checker(&app);

    QObject::connect(&checker, &UpdateWorker::updateAvailable, &app, [&] {
        hasUpdate = true;
        tray.setUpdateAvailable(true);

        if (guiSocket && guiSocket->state() == QLocalSocket::ConnectedState)
            guiSocket->write("UPDATE\n");
    });

    checker.start();
    Utils::releaseFreeMemory();

    const bool isBackground = parser.isSet(autoStartOption) || sm.startMinimized();
    if (parser.isSet(packageOption)) {
        toggleGui({"-p", parser.value(packageOption)});
    } else if (!isBackground) {
        toggleGui();
    }

    const int ret = SingleApplication::exec();

    if (guiProcess.state() != QProcess::NotRunning) {
        guiProcess.terminate();
        guiProcess.waitForFinished(1000);
    }

    return ret;
}
