#pragma once

#include <QThread>
#include <QString>
#include <QByteArray>
#include <atomic>

/**
 * Background thread that keeps a serial port open and forwards any incoming
 * bytes to the GUI via the dataReceived() signal.
 *
 * Lifecycle:
 *   monitor->setup(port, baud);
 *   monitor->start();          // opens the port, begins reading
 *   monitor->stopMonitor();    // requests clean shutdown
 *   monitor->wait();           // blocks until thread exits
 */
class SerialMonitor : public QThread
{
    Q_OBJECT

public:
    explicit SerialMonitor(QObject* parent = nullptr);

    void setup(const QString& port, int baudRate = 115200);

    /** Thread-safe: sets the stop flag so run() exits on the next iteration. */
    void stopMonitor();

    bool isRunning() const;

signals:
    void dataReceived(const QByteArray& data);
    void connectionLost(const QString& reason);

protected:
    void run() override;

private:
    QString           m_port;
    int               m_baudRate = 115200;
    std::atomic<bool> m_stop { false };
};
