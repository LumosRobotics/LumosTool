#pragma once

#include <QThread>
#include <QString>
#include <vector>
#include <cstdint>

/**
 * Background thread that runs the LumosBootloader flash sequence and
 * emits progress/completion signals back to the GUI thread.
 */
class FlashWorker : public QThread
{
    Q_OBJECT

public:
    explicit FlashWorker(QObject* parent = nullptr);

    /** Call before start(). */
    void setup(const QString& port, std::vector<uint8_t> firmware);

signals:
    /** Emitted periodically during flashing (0-100, human-readable message). */
    void progressUpdated(int percent, const QString& message);

    /** Emitted once when the operation completes or fails. */
    void flashFinished(bool success, const QString& errorMessage);

protected:
    void run() override;

private:
    QString              m_port;
    std::vector<uint8_t> m_firmware;
};
