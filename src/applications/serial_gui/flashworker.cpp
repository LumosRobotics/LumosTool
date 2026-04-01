#include "flashworker.h"
#include "lumos_bootloader.h"

FlashWorker::FlashWorker(QObject* parent)
    : QThread(parent)
{}

void FlashWorker::setup(const QString& port, std::vector<uint8_t> firmware)
{
    m_port     = port;
    m_firmware = std::move(firmware);
}

void FlashWorker::run()
{
    SimpleSerial::LumosBootloader bl;

    const bool ok = bl.Flash(
        m_port.toStdString(),
        m_firmware,
        [this](int pct, const std::string& msg) {
            emit progressUpdated(pct, QString::fromStdString(msg));
        }
    );

    emit flashFinished(ok, ok ? QString() : QString::fromStdString(bl.GetLastError()));
}
