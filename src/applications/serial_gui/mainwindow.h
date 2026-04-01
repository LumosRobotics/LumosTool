#pragma once

#include <QMainWindow>
#include <QString>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class FlashWorker;
class SerialMonitor;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void refreshPorts();
    void openFirmware();
    void downloadFirmware();

    // FlashWorker callbacks
    void onProgress(int percent, const QString& message);
    void onFlashFinished(bool success, const QString& errorMessage);

    // Terminal callbacks
    void connectTerminal();
    void disconnectTerminal();
    void sendTerminalInput();
    void clearTerminal();
    void onSerialData(const QByteArray& data);
    void onConnectionLost(const QString& reason);

private:
    void setFlashUiEnabled(bool enabled);
    void updateTerminalButtons();
    void log(const QString& message);
    void terminalPrint(const QString& text);

    // ── Flash controls ────────────────────────────────────────────────────
    QComboBox*     m_portCombo;
    QPushButton*   m_refreshButton;
    QLineEdit*     m_firmwarePath;
    QPushButton*   m_openFirmwareButton;
    QPushButton*   m_downloadButton;

    // ── Flash status ──────────────────────────────────────────────────────
    QProgressBar*  m_progressBar;
    QLabel*        m_statusLabel;
    QPlainTextEdit* m_logView;

    // ── Terminal ──────────────────────────────────────────────────────────
    QPushButton*   m_connectButton;
    QPushButton*   m_disconnectButton;
    QPushButton*   m_clearButton;
    QComboBox*     m_baudCombo;
    QPlainTextEdit* m_terminal;
    QLineEdit*     m_inputLine;
    QPushButton*   m_sendButton;

    // ── State ─────────────────────────────────────────────────────────────
    QString        m_selectedFirmware;
    FlashWorker*   m_worker  = nullptr;
    SerialMonitor* m_monitor = nullptr;
};
