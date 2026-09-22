/**
 * @file ChangeLog.cpp
 * @brief Implementation of the View class to render Slackware's ChangeLogs.
 */

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>

#include <ChangeLog.hpp>
#include <RepoManager.hpp>

/**
 * @brief Constructs the ChangeLog view, initializes the layout and populates tabs with repository log files.
 * @param parent Pointer to the parent widget.
 */
ChangeLog::ChangeLog(QWidget *parent) : Dialog(parent, Qt::WindowModal) {
    this->setWindowTitle(tr("Slackware ChangeLogs"));
    this->setMinimumSize(672, 378);
    layout = new QVBoxLayout(this);

    if (QStringList logs = RepoManager::fetchChangeLogs(); logs.isEmpty()) {
        alertTriggered = true;
    } else {
        tabs = new QTabWidget(this);

        for (const QString &path: logs) {
            QFile file(path);
            QString content;
            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
                content = QTextStream(&file).readAll();

            QFileInfo fileInfo(path);
            QString repoName = fileInfo.dir().dirName();
            if (!repoName.isEmpty())
                repoName[0] = repoName[0].toUpper();

            auto *txtEdit = new QTextEdit(tabs); // NOLINT
            txtEdit->setReadOnly(true);
            txtEdit->setPlainText(content);
            tabs->addTab(txtEdit, repoName);
        }
        layout->addWidget(tabs);
    }
}

/**
 * @brief Triggers log retrieval and dialog configuration upon display.
 * @param event The show event triggering this action.
 */
void ChangeLog::showEvent(QShowEvent *event) {
    Dialog::showEvent(event);
    if (alertTriggered) {
        QMessageBox::warning(this, tr("Warning"), tr("ChangeLog not found."));
        QTimer::singleShot(0, this, &ChangeLog::close);
    }
}
