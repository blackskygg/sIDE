#include <QMessageBox>
#include "syscalls.h"
#include "consoleworker.h"

ConsoleWorker::ConsoleWorker(QObject *parent) : QObject(parent)
{
    process = new QProcess(this);
    connect(process, SIGNAL(started()), this, SLOT(on_process_started()));
    connect(process, SIGNAL(finished(int)), this, SLOT(on_process_finished(int)));
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(on_process_readyRead()));
}

ConsoleWorker::~ConsoleWorker() {
    delete process;
}

void ConsoleWorker::write(const QByteArray data)
{
    process->write(data);
}

void ConsoleWorker::on_process_finished(int status)
{
    emit finished(status);
}

void ConsoleWorker::on_process_readyRead()
{
    QByteArray line;

    //emit readyRead(process->readAllStandardOutput());
    while (true) {
        line = process->readLine();
        if (line.isEmpty()) break;
        emit readyRead(line);
    };
}

void ConsoleWorker::on_process_started()
{
    emit started();
}

void ConsoleWorker::execute(const QString program, const QStringList args)
{
    process->start(program, args);
}

void ConsoleWorker::stop()
{
    process->kill();
}
