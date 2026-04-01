#include "mainwindow.h"
#include "flashworker.h"
#include "serialmonitor.h"
#include "serial.h"

#include <QComboBox>
#include <QDateTime>
#include <QSettings>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QSerialPortInfo>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

// ── Construction ──────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Lumos Firmware Downloader");
    setMinimumSize(580, 700);

    QWidget*     central = new QWidget(this);
    QVBoxLayout* root    = new QVBoxLayout(central);
    root->setSpacing(10);
    root->setContentsMargins(16, 16, 16, 16);
    setCentralWidget(central);

    // ── Top half: config + flash ──────────────────────────────────────────
    QWidget*     topWidget = new QWidget(central);
    QVBoxLayout* topLayout = new QVBoxLayout(topWidget);
    topLayout->setSpacing(10);
    topLayout->setContentsMargins(0, 0, 0, 0);

    // Serial port group
    {
        QGroupBox*   g = new QGroupBox("Serial Device", topWidget);
        QHBoxLayout* h = new QHBoxLayout(g);

        m_portCombo = new QComboBox(g);
        m_portCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_refreshButton = new QPushButton("Refresh", g);
        m_refreshButton->setFixedWidth(80);

        h->addWidget(m_portCombo);
        h->addWidget(m_refreshButton);
        topLayout->addWidget(g);
    }

    // Firmware file group
    {
        QGroupBox*   g = new QGroupBox("Firmware Binary", topWidget);
        QHBoxLayout* h = new QHBoxLayout(g);

        m_firmwarePath = new QLineEdit(g);
        m_firmwarePath->setPlaceholderText("No file selected…");
        m_firmwarePath->setReadOnly(true);

        m_openFirmwareButton = new QPushButton("Open Firmware…", g);
        m_openFirmwareButton->setFixedWidth(120);

        h->addWidget(m_firmwarePath);
        h->addWidget(m_openFirmwareButton);
        topLayout->addWidget(g);
    }

    // Status group
    {
        QGroupBox*   g = new QGroupBox("Status", topWidget);
        QVBoxLayout* v = new QVBoxLayout(g);

        m_progressBar = new QProgressBar(g);
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);

        m_statusLabel = new QLabel("Ready.", g);
        m_statusLabel->setWordWrap(true);

        m_logView = new QPlainTextEdit(g);
        m_logView->setReadOnly(true);
        m_logView->setMaximumHeight(100);
        m_logView->setPlaceholderText("Flash log…");
        QFont monoLog;
        monoLog.setFamily("Menlo");
        monoLog.setStyleHint(QFont::Monospace);
        monoLog.setPointSize(10);
        m_logView->setFont(monoLog);

        v->addWidget(m_progressBar);
        v->addWidget(m_statusLabel);
        v->addWidget(m_logView);
        topLayout->addWidget(g);
    }

    // Download button
    m_downloadButton = new QPushButton("Download", topWidget);
    m_downloadButton->setFixedHeight(38);
    QFont bold = m_downloadButton->font();
    bold.setBold(true);
    m_downloadButton->setFont(bold);
    topLayout->addWidget(m_downloadButton);

    // ── Bottom half: terminal ─────────────────────────────────────────────
    QGroupBox*   termGroup  = new QGroupBox("Terminal", central);
    QVBoxLayout* termLayout = new QVBoxLayout(termGroup);
    termLayout->setSpacing(6);

    // Toolbar row: Connect | Disconnect | Clear | Baud label + combo
    {
        QHBoxLayout* bar = new QHBoxLayout;

        m_connectButton    = new QPushButton("Connect",    termGroup);
        m_disconnectButton = new QPushButton("Disconnect", termGroup);
        m_clearButton      = new QPushButton("Clear",      termGroup);

        m_connectButton->setFixedWidth(90);
        m_disconnectButton->setFixedWidth(90);
        m_clearButton->setFixedWidth(60);

        m_baudCombo = new QComboBox(termGroup);
        for (int baud : {9600, 19200, 38400, 57600, 115200, 230400})
            m_baudCombo->addItem(QString::number(baud), baud);
        m_baudCombo->setCurrentText("115200");
        m_baudCombo->setFixedWidth(90);

        bar->addWidget(m_connectButton);
        bar->addWidget(m_disconnectButton);
        bar->addWidget(m_clearButton);
        bar->addStretch();
        bar->addWidget(new QLabel("Baud:", termGroup));
        bar->addWidget(m_baudCombo);

        termLayout->addLayout(bar);
    }

    // Terminal display
    m_terminal = new QPlainTextEdit(termGroup);
    m_terminal->setReadOnly(true);
    m_terminal->setMinimumHeight(160);
    m_terminal->setPlaceholderText("Serial output will appear here…");
    m_terminal->setMaximumBlockCount(2000);   // keep last ~2000 lines

    QFont monoTerm;
    monoTerm.setFamily("Menlo");
    monoTerm.setStyleHint(QFont::Monospace);
    monoTerm.setPointSize(11);
    m_terminal->setFont(monoTerm);

    // Dark terminal palette
    QPalette pal = m_terminal->palette();
    pal.setColor(QPalette::Base,  QColor(0x1e, 0x1e, 0x1e));
    pal.setColor(QPalette::Text,  QColor(0xd4, 0xd4, 0xd4));
    m_terminal->setPalette(pal);

    termLayout->addWidget(m_terminal);

    // Input row
    {
        QHBoxLayout* inputRow = new QHBoxLayout;

        m_inputLine = new QLineEdit(termGroup);
        m_inputLine->setPlaceholderText("Send data…");
        m_inputLine->setEnabled(false);

        m_sendButton = new QPushButton("Send", termGroup);
        m_sendButton->setFixedWidth(60);
        m_sendButton->setEnabled(false);

        inputRow->addWidget(m_inputLine);
        inputRow->addWidget(m_sendButton);
        termLayout->addLayout(inputRow);
    }

    // ── Splitter: top config / bottom terminal ────────────────────────────
    QSplitter* splitter = new QSplitter(Qt::Vertical, central);
    splitter->addWidget(topWidget);
    splitter->addWidget(termGroup);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    root->addWidget(splitter);

    // ── Signal connections ────────────────────────────────────────────────
    connect(m_refreshButton,      &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(m_openFirmwareButton, &QPushButton::clicked, this, &MainWindow::openFirmware);
    connect(m_downloadButton,     &QPushButton::clicked, this, &MainWindow::downloadFirmware);
    connect(m_connectButton,      &QPushButton::clicked, this, &MainWindow::connectTerminal);
    connect(m_disconnectButton,   &QPushButton::clicked, this, &MainWindow::disconnectTerminal);
    connect(m_clearButton,        &QPushButton::clicked, this, &MainWindow::clearTerminal);
    connect(m_sendButton,         &QPushButton::clicked, this, &MainWindow::sendTerminalInput);
    connect(m_inputLine,          &QLineEdit::returnPressed, this, &MainWindow::sendTerminalInput);

    refreshPorts();
    updateTerminalButtons();

    QSettings settings;
    const QString savedFirmware = settings.value("firmware/lastPath").toString();
    if (!savedFirmware.isEmpty() && QFile::exists(savedFirmware)) {
        m_selectedFirmware = savedFirmware;
        m_firmwarePath->setText(savedFirmware);
    }
}

MainWindow::~MainWindow()
{
    if (m_monitor) {
        m_monitor->stopMonitor();
        m_monitor->wait(2000);
    }
}

// ── Port list ─────────────────────────────────────────────────────────────────

void MainWindow::refreshPorts()
{
    m_portCombo->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    if (ports.isEmpty()) {
        m_portCombo->addItem("No devices found");
        m_portCombo->setEnabled(false);
    } else {
        m_portCombo->setEnabled(true);
        for (const QSerialPortInfo& info : ports) {
            QString label = info.portName();
            if (!info.description().isEmpty())
                label += "  –  " + info.description();
            m_portCombo->addItem(label, info.systemLocation());
        }
    }
}

// ── Firmware selection ────────────────────────────────────────────────────────

void MainWindow::openFirmware()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Select Firmware Binary", QString(),
        "Binary files (*.bin);;ELF files (*.elf);;All files (*)");
    if (path.isEmpty()) return;

    m_selectedFirmware = path;
    m_firmwarePath->setText(path);
    QSettings().setValue("firmware/lastPath", path);
    log("Firmware selected: " + path);
}

// ── Download / flash ──────────────────────────────────────────────────────────

void MainWindow::downloadFirmware()
{
    const QString portPath = m_portCombo->currentData().toString();
    if (portPath.isEmpty()) {
        QMessageBox::warning(this, "No Device", "Please select a serial device.");
        return;
    }
    if (m_selectedFirmware.isEmpty()) {
        QMessageBox::warning(this, "No Firmware", "Please select a firmware file first.");
        return;
    }

    QFile file(m_selectedFirmware);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "File Error",
            "Cannot open firmware file:\n" + m_selectedFirmware);
        return;
    }
    const QByteArray raw = file.readAll();
    file.close();
    if (raw.isEmpty()) {
        QMessageBox::critical(this, "File Error", "Firmware file is empty.");
        return;
    }

    // Disconnect terminal so the port is free for flashing
    if (m_monitor && m_monitor->isRunning()) {
        terminalPrint("\n[Terminal disconnected for flashing]\n");
        disconnectTerminal();
    }

    std::vector<uint8_t> firmware(
        reinterpret_cast<const uint8_t*>(raw.constData()),
        reinterpret_cast<const uint8_t*>(raw.constData()) + raw.size());

    log(QString("Starting download: %1 bytes → %2").arg(raw.size()).arg(portPath));

    setFlashUiEnabled(false);
    m_progressBar->setValue(0);
    m_statusLabel->setText("Connecting…");

    if (m_worker) { m_worker->wait(); delete m_worker; }

    m_worker = new FlashWorker(this);
    m_worker->setup(portPath, std::move(firmware));
    connect(m_worker, &FlashWorker::progressUpdated,
            this,     &MainWindow::onProgress,     Qt::QueuedConnection);
    connect(m_worker, &FlashWorker::flashFinished,
            this,     &MainWindow::onFlashFinished, Qt::QueuedConnection);
    m_worker->start();
}

// ── Flash worker callbacks ────────────────────────────────────────────────────

void MainWindow::onProgress(int percent, const QString& message)
{
    m_progressBar->setValue(percent);
    m_statusLabel->setText(message);
    log(message);
}

void MainWindow::onFlashFinished(bool success, const QString& errorMessage)
{
    setFlashUiEnabled(true);

    if (success) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("Done! Firmware downloaded successfully.");
        log("✓ Flash complete.");

        // Auto-connect terminal so we can see the newly flashed firmware's output
        const QString portPath = m_portCombo->currentData().toString();
        if (!portPath.isEmpty()) {
            terminalPrint("\n[Auto-connecting terminal after successful flash…]\n");
            connectTerminal();
        }
    } else {
        m_progressBar->setValue(0);
        m_statusLabel->setText("Error: " + errorMessage);
        log("✗ Error: " + errorMessage);
        QMessageBox::critical(this, "Flash Failed",
            "Firmware download failed:\n\n" + errorMessage);
    }
}

// ── Terminal: connect / disconnect ────────────────────────────────────────────

void MainWindow::connectTerminal()
{
    const QString portPath = m_portCombo->currentData().toString();
    if (portPath.isEmpty()) {
        terminalPrint("[Error: no port selected]\n");
        return;
    }

    // Stop any existing monitor first
    if (m_monitor) {
        m_monitor->stopMonitor();
        m_monitor->wait(2000);
        delete m_monitor;
        m_monitor = nullptr;
    }

    const int baud = m_baudCombo->currentData().toInt();

    m_monitor = new SerialMonitor(this);
    m_monitor->setup(portPath, baud);
    connect(m_monitor, &SerialMonitor::dataReceived,
            this,      &MainWindow::onSerialData,    Qt::QueuedConnection);
    connect(m_monitor, &SerialMonitor::connectionLost,
            this,      &MainWindow::onConnectionLost, Qt::QueuedConnection);
    m_monitor->start();

    terminalPrint(QString("[Connected to %1 @ %2 baud]\n").arg(portPath).arg(baud));
    updateTerminalButtons();
}

void MainWindow::disconnectTerminal()
{
    if (!m_monitor) return;

    m_monitor->stopMonitor();
    m_monitor->wait(2000);
    delete m_monitor;
    m_monitor = nullptr;

    terminalPrint("[Disconnected]\n");
    updateTerminalButtons();
}

// ── Terminal: data and input ──────────────────────────────────────────────────

void MainWindow::onSerialData(const QByteArray& data)
{
    // Append raw bytes as text; replace \r to avoid double newlines on some
    // terminals but keep \n for line breaks.
    QString text = QString::fromLatin1(data);
    text.remove('\r');
    terminalPrint(text);
}

void MainWindow::onConnectionLost(const QString& reason)
{
    terminalPrint("\n[Connection lost: " + reason + "]\n");
    if (m_monitor) {
        m_monitor->wait(2000);
        delete m_monitor;
        m_monitor = nullptr;
    }
    updateTerminalButtons();
}

void MainWindow::sendTerminalInput()
{
    if (!m_monitor || !m_monitor->isRunning()) return;

    // We need a direct serial write here; open the port briefly and write.
    // For simplicity we share the port only for the send, then the monitor
    // re-reads.  A production implementation would hand a shared Serial
    // object to both, but this is sufficient for a command-response workflow.
    const QString text = m_inputLine->text();
    if (text.isEmpty()) return;

    const QString portPath = m_portCombo->currentData().toString();

    SimpleSerial::SerialConfig cfg;
    cfg.baud_rate  = m_baudCombo->currentData().toInt();
    cfg.timeout_ms = 500;

    SimpleSerial::Serial serial;
    if (serial.Open(portPath.toStdString(), cfg)) {
        const std::string line = text.toStdString() + "\r\n";
        serial.Write(reinterpret_cast<const uint8_t*>(line.data()), line.size());
        serial.Close();
    }

    terminalPrint("\x1b[90m> " + text + "\x1b[0m\n");  // dimmed echo
    m_inputLine->clear();
}

void MainWindow::clearTerminal()
{
    m_terminal->clear();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MainWindow::setFlashUiEnabled(bool enabled)
{
    m_portCombo->setEnabled(enabled);
    m_refreshButton->setEnabled(enabled);
    m_openFirmwareButton->setEnabled(enabled);
    m_downloadButton->setEnabled(enabled);
    m_downloadButton->setText(enabled ? "Download" : "Downloading…");
}

void MainWindow::updateTerminalButtons()
{
    const bool connected = m_monitor && m_monitor->isRunning();
    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);
    m_baudCombo->setEnabled(!connected);
    m_inputLine->setEnabled(connected);
    m_sendButton->setEnabled(connected);
}

void MainWindow::log(const QString& message)
{
    const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_logView->appendPlainText("[" + ts + "] " + message);
}

void MainWindow::terminalPrint(const QString& text)
{
    // Move cursor to end and insert without adding extra newlines
    QTextCursor cursor = m_terminal->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
    m_terminal->setTextCursor(cursor);
    m_terminal->verticalScrollBar()->setValue(
        m_terminal->verticalScrollBar()->maximum());
}
