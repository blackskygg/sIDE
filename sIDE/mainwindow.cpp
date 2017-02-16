#include <syscalls.h>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <QCloseEvent>
#include <QSurfaceFormat>
#include <QStringList>
#include <QTime>
#include <QScrollBar>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "consoleworker.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    highlighter = new SyntaxHighlighter(ui->textEdit->document());
    showMaximized();

    // Add actions;
    ui->mainToolBar->addAction(ui->actionNew);
    ui->mainToolBar->addAction(ui->actionOpen);
    ui->mainToolBar->addAction(ui->actionSave);
    ui->mainToolBar->addAction(ui->actionSave_As);
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(ui->actionCopy);
    ui->mainToolBar->addAction(ui->actionPaste);
    ui->mainToolBar->addAction(ui->actionUndo);
    ui->mainToolBar->addAction(ui->actionRedo);
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(ui->actionAssemble);
    ui->mainToolBar->addAction(ui->actionRun);
    ui->mainToolBar->addAction(ui->actionKill);
    QAction *actionDockToggleView = ui->dockWidget->toggleViewAction();
    actionDockToggleView->setText("Show Output Window");
    ui->menuWindow->addAction(actionDockToggleView);

    // connect Actions.
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(on_open_triggered()));
    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(on_new_triggered()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(on_save_triggered()));
    connect(ui->actionSave_As, SIGNAL(triggered()), this, SLOT(on_saveas_triggered()));
    connect(ui->actionAssemble, SIGNAL(triggered()), this, SLOT(on_assemble_triggered()));
    connect(ui->textEdit->document(), SIGNAL(modificationChanged(bool)),
            this, SLOT(on_textEdit_modificationChanged(bool)));
    connect(ui->actionRun, SIGNAL(triggered()), this, SLOT(on_run_triggered()));
    connect(ui->actionKill, SIGNAL(triggered()), this, SLOT(on_stop_triggered()));
    connect(ui->actionShow_Output, SIGNAL(toggled(bool)),
            this, SLOT(on_showOuput_toggled(bool)));
    connect(ui->actionShow_Toolbar, SIGNAL(toggled(bool)),
            this, SLOT(on_showToolbar_toggled(bool)));
    connect(ui->actionCopy, SIGNAL(triggered()), ui->textEdit, SLOT(copy()));
    connect(ui->actionCut, SIGNAL(triggered()), ui->textEdit, SLOT(cut()));
    connect(ui->actionPaste, SIGNAL(triggered()), ui->textEdit, SLOT(paste()));
    connect(ui->actionUndo, SIGNAL(triggered()), ui->textEdit, SLOT(undo()));
    connect(ui->actionRedo, SIGNAL(triggered()), ui->textEdit, SLOT(redo()));
    connect(ui->textEdit, SIGNAL(undoAvailable(bool)), this, SLOT(on_text_undoAvailable(bool)));
    connect(ui->textEdit, SIGNAL(redoAvailable(bool)), this, SLOT(on_text_redoAvailable(bool)));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(on_about_triggered()));

    // Setup assemble thread, and kick off to standby mode.
    assembleThread = new QThread(this);
    runThread = new QThread(this);
    assembleWorker = new ConsoleWorker(assembleThread);
    runWorker = new ConsoleWorker(runThread);

    connect(assembleWorker, SIGNAL(readyRead(QByteArray)),
            this, SLOT(on_assembler_readyRead(QByteArray)));
    connect(assembleWorker, SIGNAL(finished(int)),
            this, SLOT(on_assembler_finished(int)));
    connect(runWorker, SIGNAL(started()), this, SLOT(on_runThread_started()));
    connect(runWorker, SIGNAL(readyRead(QByteArray)),
            this, SLOT(on_runThread_readRead(QByteArray)));
    connect(runWorker, SIGNAL(finished(int)), this, SLOT(on_runThread_finished(int)));
    assembleThread->start();
    runThread->start();

    // Initialize some status.
    on_textEdit_cursorPositionChanged();
    setCurrFile(QString());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        assembleThread->quit();
        runThread->quit();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::setCurrFile(const QString &name) {
    QFileInfo file_info(name);

    currFile = name;
    if (currFile.isEmpty())
        setWindowTitle(QString("sIDE - Untitled"));
    else
        setWindowTitle(QString("sIDE - %1").arg(file_info.fileName()));
}

void MainWindow::loadFile(const QString &name) {
    QFile file(name);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("sIDE"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(name),
                                  file.errorString()));
        return;
    }

    QTextStream in(&file);
    ui->textEdit->setPlainText(in.readAll());
    setCurrFile(name);
    statusBar()->showMessage(tr("File loaded"), 0);
}

bool MainWindow::maybeSave() {
    if (!ui->textEdit->document()->isModified())
        return true;
    QMessageBox::StandardButton ret
        = QMessageBox::warning(this, tr("sIDE"),
                               tr("The file has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Save:
        return on_save_triggered();
    case QMessageBox::Cancel:
        return false;
    case QMessageBox::Discard:
        return true;
    default:
        break;
    }
    return true;
}

void MainWindow::on_open_triggered()
{
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this);
        if (!fileName.isEmpty())
            loadFile(fileName);
    }
}

void MainWindow::on_new_triggered()
{
    if (maybeSave()) {
        ui->textEdit->clear();
        setCurrFile(QString());
    }
}

bool MainWindow::saveFile(const QString &name)
{
    QFile file(name);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("sIDE"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(currFile),
                                  file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out << ui->textEdit->toPlainText();
    ui->textEdit->document()->setModified(false);
    statusBar()->showMessage(tr("File saved"), 0);
    return true;
}

bool MainWindow::on_save_triggered()
{
    if (currFile.isEmpty())
        return on_saveas_triggered();
    else
        return saveFile(currFile);
    return true;
}

bool MainWindow::on_saveas_triggered()
{
    QFileDialog dialog(this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (dialog.exec() != QDialog::Accepted)
        return false;
    if (!saveFile(dialog.selectedFiles().first()))
        return false;
    setCurrFile(dialog.selectedFiles().first());
    return true;

}

void MainWindow::on_assembler_readyRead(QByteArray data)
{
    ui->textAssembler->insertPlainText(QString(data));
    ui->textAssembler->verticalScrollBar()->setSliderPosition(
                ui->textAssembler->verticalScrollBar()->maximum());

    // Check for error msg and highlight the troublesome line
    QString s(data);
    QRegExp errPattern("^Line: (\\d+)");
    if(-1 != errPattern.indexIn(s)) {
        // Move to line
        int linum = errPattern.capturedTexts().at(1).toInt() - 1;
        ui->textEdit->moveCursor(QTextCursor::End);
        QTextCursor cursor(ui->textEdit->document()->findBlockByNumber(linum));
        ui->textEdit->setTextCursor(cursor);

        // Highlight the line
        QList<QTextEdit::ExtraSelection> extraSelections;
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::red);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = ui->textEdit->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
        ui->textEdit->setExtraSelections(extraSelections);
    }
}

void MainWindow::on_assembler_finished(int status)
{
    // Enable assemble actions.
    ui->actionAssemble->setEnabled(true);
    ui->actionAssemble_Run->setEnabled(true);
    ui->textAssembler->append(QString("\n(%1): Assembler returned %2.")
                              .arg(time.currentTime().toString("hh:mm:ss"))
                              .arg(status));

    if (0 == status) {
        QFile file("list.txt");
        file.open(QFile::ReadOnly|QFile::Text);
        QTextStream in(&file);
        ui->textLog->setPlainText(in.readAll());
    }
}

void MainWindow::on_assemble_triggered()
{
    if (!on_save_triggered())
        return;

    // Execute the assembler.
    QFileInfo file_info(currFile);
    QStringList args;
    args.append(currFile);
    args.append(tr("%1/%2.data").arg(file_info.path())
                .arg(file_info.completeBaseName()));

    ui->textAssembler->clear();
    ui->actionAssemble->setEnabled(false);
    ui->actionAssemble_Run->setEnabled(false);

    ui->tabWidget->setCurrentWidget(ui->tabAssembler);
    assembleWorker->execute(tr("sas.exe"), args);
}

void MainWindow::on_runThread_started()
{
    ui->tabWidget->setCurrentWidget(ui->tabConsole);
    ui->textConsole->clear();
    ui->lineInput->clear();
    ui->lineInput->setEnabled(true);
}

void MainWindow::on_runThread_readRead(QByteArray data)
{
    ui->textConsole->insertPlainText(QString(data));
    ui->textConsole->verticalScrollBar()->setSliderPosition(
                ui->textConsole->verticalScrollBar()->maximum());
}

void MainWindow::on_runThread_finished(int status)
{
    ui->lineInput->setEnabled(false);
    ui->textConsole->append(QString("\n(%1): Program returned %2.")
                              .arg(time.currentTime().toString("hh:mm:ss"))
                              .arg(status));
    ui->actionKill->setEnabled(false);
    ui->actionRun->setEnabled(true);
}

void MainWindow::on_run_triggered()
{
    // Execute the assembler.
    QFileInfo file_info(currFile);
    QStringList args;
    args.append(tr("%1/%2.data").arg(file_info.path())
                .arg(file_info.completeBaseName()));

    ui->actionRun->setEnabled(false);
    ui->actionKill->setEnabled(true);
    runWorker->execute(tr("ssim.exe"), args);
}

void MainWindow::on_stop_triggered()
{
    runWorker->stop();
}

void MainWindow::on_textEdit_modificationChanged(bool state) {
    if (state) {
        QString name(currFile);
        if (name.isEmpty())
            name = "Untitled";
        else
            name = QFileInfo(name).fileName();

        setWindowTitle(tr("sIDE - %1*").arg(name));
    } else {
        setCurrFile(currFile);
    }
}

void MainWindow::on_textEdit_cursorPositionChanged()
{
    QString s = "Col: %1, Line: %2";
    s = s.arg(ui->textEdit->textCursor().columnNumber())
            .arg(ui->textEdit->textCursor().blockNumber());
    ui->statusBar->showMessage(s, 0);
}

void MainWindow::on_lineInput_returnPressed()
{
    QByteArray line = ui->lineInput->text().toLocal8Bit();
    line.append('\n');
    ui->textConsole->append(line);
    runWorker->write(line);
}

void MainWindow::on_showOutput_toggled(bool checked)
{
    if (checked)
        ui->dockWidget->show();
    else
        ui->dockWidget->hide();
}

void MainWindow::on_showToolbar_toggled(bool checked)
{
    if (checked)
        ui->mainToolBar->setVisible(true);
    else
        ui->mainToolBar->setVisible(false);
}

void MainWindow::on_text_undoAvailable(bool available)
{
    if (available)
        ui->actionUndo->setEnabled(true);
    else
        ui->actionUndo->setEnabled(false);
}

void MainWindow::on_text_redoAvailable(bool available)
{
    if (available)
        ui->actionRedo->setEnabled(true);
    else
        ui->actionRedo->setEnabled(false);
}

void MainWindow::on_about_triggered()
{
    QMessageBox::about(this, "About sIDE", "This Program is written as a curriculum design project in HUST.\n\n"
                       "Author: Zhongze Liu\n\n"
                       "Email: blackskygg@gmail.com\n");
}
