#include "serialmonitor.h"
#include "serial.h"

SerialMonitor::SerialMonitor(QObject* parent)
    : QThread(parent)
{}

void SerialMonitor::setup(const QString& port, int baudRate)
{
    m_port     = port;
    m_baudRate = baudRate;
    m_stop     = false;
}

void SerialMonitor::stopMonitor()
{
    m_stop = true;
}

bool SerialMonitor::isRunning() const
{
    return QThread::isRunning();
}

void SerialMonitor::run()
{
    SimpleSerial::SerialConfig cfg;
    cfg.baud_rate  = m_baudRate;
    cfg.data_bits  = 8;
    cfg.stop_bits  = 1;
    cfg.parity     = 'N';
    cfg.timeout_ms = 100;   // short timeout so we can check m_stop frequently

    SimpleSerial::Serial serial;

    if (!serial.Open(m_port.toStdString(), cfg)) {
        emit connectionLost("Cannot open port: " +
                            QString::fromStdString(serial.GetLastError()));
        return;
    }

    uint8_t buf[256];

    while (!m_stop) {
        const int n = serial.Read(buf, sizeof(buf));

        if (n > 0) {
            emit dataReceived(QByteArray(reinterpret_cast<const char*>(buf), n));
        } else if (n < 0) {
            emit connectionLost("Read error: " +
                                QString::fromStdString(serial.GetLastError()));
            break;
        }
        // n == 0  →  timeout, loop again
    }

    serial.Close();
}
