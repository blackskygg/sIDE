#ifndef CONSOLEWORKER_H
#define CONSOLEWORKER_H

#include <QObject>
#include <QByteArray>
#include <QStringList>
#include <QString>
#include <QProcess>

class ConsoleWorker : public QObject
{
    Q_OBJECT
public:
    explicit ConsoleWorker(QObject *parent = 0);
    ~ConsoleWorker();

signals:
    // Notifies receivers to fetch data.
    void readyRead(QByteArray data);
    // Execution completed.
    void finished(int status);
    // Process started;
    void started();

public slots:
    //Fork a process and execute a command.
    //Use pipe to communicate. Emits readReady when outputs are ready.
    void execute(const QString program, const QStringList args);
    void stop();

    //Write data to process's stdin.
    void write(const QByteArray data);

    void on_process_readyRead();
    void on_process_finished(int status);
    void on_process_started();

private:
    QProcess *process;
};

#endif // CONSOLEWORKER_H
