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
#include <QLoggingCategory>
#include <QThread>
#include <QTranslator>

#include <SingleApplication>

#include <DefaultPath.hpp>
#include <Helper.hpp>
#include <Klass.hpp>
#include <MessageReceiver.hpp>
#include <SettingsManager.hpp>
#include <StatusIcons.hpp>
#include <Theme.hpp>
#include <UpdateWorker.hpp>
#include <Version.hpp>

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]) == "--helper") {
            qputenv("QT_QPA_PLATFORM", "minimal");
            QCoreApplication app(argc, argv);
            Helper h;
            return QCoreApplication::exec();
        }
    }

    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
    QApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents, true);

    SingleApplication app(argc, argv, true, SingleApplication::Mode::SecondaryNotification);
    const MessageReceiver msg;
    const Debug::Debug debug;
    bool autostart = false;

    QApplication::setApplicationName("Klass");
    QApplication::setOrganizationName(QApplication::applicationName());
    QApplication::setApplicationDisplayName(QApplication::applicationName());
    QApplication::setApplicationVersion(VERSION);
    Theme::applyTheme();

    debug.msg("Program version", "Main", {QApplication::applicationVersion()});
    debug.msg("Program pid", "Main", {QString::number(QApplication::applicationPid())});

    const QCommandLineOption autoStartOption("autostart", QObject::tr("Start the application in system tray."));
    const QCommandLineOption packageOption(QStringList() << "p" << "package",
                                           QObject::tr("Install package in system."), "package");

    QCommandLineParser parser;
    parser.addOption(autoStartOption);
    parser.addOption(packageOption);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    if (app.isSecondary()) {
        debug.msg("New instance, passing the arguments", "Main");
        if (parser.isSet(packageOption)) {
            QString pkg = parser.value(packageOption);
            debug.msg("Processing package from secondary instance", "Main", {pkg});
            app.sendMessage(pkg.toUtf8());
        }
        return 0;
    }

    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale::system(), QStringLiteral("qtbase"), QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        SingleApplication::installTranslator(&qtTranslator);
    }

    QTranslator translator;
    const auto lang = QLocale::system().name();

    if (auto trFile = DefaultPath().defaultPath(QString("lang/klass_%1.qm").arg(lang));
        translator.load(trFile)) {
        debug.msg("Loading translation", "Translator", {trFile, Debug::LightGreen});
        SingleApplication::installTranslator(&translator);
    } else {
        debug.msg("Translation not available", "Translator", {Debug::LightRed});
    }

    StatusIcons::init(); // Antes do Klass, senão dá ruim.
    Klass klass;
    QThread thread;
    auto *checker = new UpdateWorker();
    checker->moveToThread(&thread);

    if (parser.isSet(packageOption)) {
        autostart = true;
        const auto pkg = parser.value(packageOption);
        debug.msg("Processing package from command line", "Main", {pkg});
        klass.handlePackage(pkg);
    }

    QObject::connect(&app, &SingleApplication::receivedMessage, &msg, &MessageReceiver::receivedMessage);
    QObject::connect(&msg, &MessageReceiver::arg, &klass, &Klass::handlePackage);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &thread, &QThread::quit);
    QObject::connect(&thread, &QThread::finished, checker, &QObject::deleteLater);
    QObject::connect(&thread, &QThread::started, checker, &UpdateWorker::start);
    QObject::connect(checker, &UpdateWorker::updateAvailable, &klass, &Klass::onUpdateAvailable);
    thread.start();

    if (!autostart && !parser.isSet(autoStartOption)) {
        if (SettingsManager().windowMaximize())
            klass.showMaximized();
        else
            klass.show();
    } else {
        klass.hide();
    }

    const int ret = SingleApplication::exec();

    thread.quit();
    thread.wait();
    return ret;
}
