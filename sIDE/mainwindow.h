#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QByteArray>
#include <QCloseEvent>
#include <QTime>
#include "consoleworker.h"
#include "syntaxhighlighter.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_textEdit_cursorPositionChanged();
    void on_textEdit_modificationChanged(bool);
    void on_assembler_readyRead(QByteArray data);
    void on_assembler_finished(int status);
    void on_run_triggered();
    void on_runThread_started();
    void on_runThread_readRead(QByteArray data);
    void on_runThread_finished(int status);
    void on_stop_triggered();
    void on_open_triggered();
    void on_new_triggered();
    bool on_save_triggered();
    bool on_saveas_triggered();
    void on_assemble_triggered();
    void on_showOutput_toggled(bool);
    void on_showToolbar_toggled(bool);
    void on_about_triggered();

    void on_text_undoAvailable(bool);
    void on_text_redoAvailable(bool);
    void on_lineInput_returnPressed();

private:
    bool maybeSave();
    void loadFile(const QString &name);
    bool saveFile(const QString &name);
    void setCurrFile(const QString &name);
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    SyntaxHighlighter *highlighter;
    QThread *assembleThread;
    QThread *runThread;
    ConsoleWorker *assembleWorker;
    ConsoleWorker *runWorker;
    QString currFile;
    QTime time;
};

#endif // MAINWINDOW_H
