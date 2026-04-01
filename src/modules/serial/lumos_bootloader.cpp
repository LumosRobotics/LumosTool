#include "lumos_bootloader.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace SimpleSerial {

// ── CRC16-CCITT (XMODEM) table – identical to the MCU's crc16_table ────────

static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x5295, 0x42B4, 0x7297, 0x62B6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD39D, 0xC3BC, 0xF39F, 0xE3BE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64C6, 0x74E7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5CE, 0xF5EF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76F7, 0x66D6, 0x56B5, 0x4694,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A51, 0x0A70, 0x3A13, 0x2A32,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC31D, 0xD33C, 0xE35F, 0xF37E,
    0x0291, 0x12B0, 0x22D3, 0x32F2, 0x4235, 0x5254, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6677, 0x7656, 0x4635, 0x5614,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BD9, 0x9BF8, 0xAB9B, 0xBBBA,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

uint16_t LumosBootloader::Crc16(const uint8_t* data, uint32_t len)
{
    uint16_t crc = 0;
    for (uint32_t i = 0; i < len; i++) {
        uint8_t idx = static_cast<uint8_t>(crc >> 8) ^ data[i];
        crc = static_cast<uint16_t>(crc << 8) ^ crc16_table[idx];
    }
    return crc;
}

// ── Helpers ──────────────────────────────────────────────────────────────────

void LumosBootloader::SetError(const std::string& error)
{
    last_error_ = error;
}

void LumosBootloader::Report(const ProgressCallback& cb, int percent, const std::string& msg)
{
    if (cb) cb(percent, msg);
}

/** Read exactly one byte, return false on timeout or error. */
static bool ReadByte(Serial& serial, uint8_t& out)
{
    int n = serial.Read(&out, 1);
    return n == 1;
}

// ── Protocol steps ────────────────────────────────────────────────────────────

bool LumosBootloader::WaitBootloaderAck(Serial& serial)
{
    const uint8_t expected[3] = { BOOT_ACK_1, BOOT_ACK_2, BOOT_ACK_3 };
    for (int i = 0; i < 3; i++) {
        uint8_t b;
        if (!ReadByte(serial, b)) {
            SetError("Timeout waiting for bootloader ACK byte " + std::to_string(i + 1));
            return false;
        }
        if (b != expected[i]) {
            SetError("Bad bootloader ACK byte " + std::to_string(i + 1) +
                     ": expected 0x" + [](uint8_t v) {
                         char buf[8]; snprintf(buf, sizeof(buf), "%02X", v); return std::string(buf);
                     }(expected[i]) + " got 0x" + [](uint8_t v) {
                         char buf[8]; snprintf(buf, sizeof(buf), "%02X", v); return std::string(buf);
                     }(b));
            return false;
        }
    }
    return true;
}

bool LumosBootloader::WaitAck(Serial& serial)
{
    uint8_t b;
    if (!ReadByte(serial, b)) {
        SetError("Timeout waiting for ACK");
        return false;
    }
    if (b == RESP_NACK) {
        SetError("Received NACK from MCU");
        return false;
    }
    if (b != RESP_ACK) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Unexpected response byte: 0x%02X", b);
        SetError(buf);
        return false;
    }
    return true;
}

bool LumosBootloader::SendStartPacket(Serial& serial, uint32_t firmware_size)
{
    uint8_t buf[5];
    buf[0] = PKT_START;
    buf[1] = static_cast<uint8_t>(firmware_size >>  0);
    buf[2] = static_cast<uint8_t>(firmware_size >>  8);
    buf[3] = static_cast<uint8_t>(firmware_size >> 16);
    buf[4] = static_cast<uint8_t>(firmware_size >> 24);

    if (serial.Write(buf, 5) != 5) {
        SetError("Failed to write START packet");
        return false;
    }
    return WaitAck(serial);
}

bool LumosBootloader::SendDataPackets(Serial& serial,
                                       const std::vector<uint8_t>& firmware,
                                       const ProgressCallback& cb)
{
    const size_t total  = firmware.size();
    size_t       offset = 0;

    while (offset < total) {
        const auto chunk_size = static_cast<uint16_t>(
            std::min<size_t>(CHUNK_SIZE, total - offset));
        const uint8_t* chunk = firmware.data() + offset;
        const uint16_t crc   = Crc16(chunk, chunk_size);

        // Packet: type(1) | size_lo | size_hi | data(chunk_size) | crc_lo | crc_hi
        std::vector<uint8_t> pkt;
        pkt.reserve(1 + 2 + chunk_size + 2);
        pkt.push_back(PKT_DATA);
        pkt.push_back(static_cast<uint8_t>(chunk_size >> 0));
        pkt.push_back(static_cast<uint8_t>(chunk_size >> 8));
        pkt.insert(pkt.end(), chunk, chunk + chunk_size);
        pkt.push_back(static_cast<uint8_t>(crc >> 0));
        pkt.push_back(static_cast<uint8_t>(crc >> 8));

        if (serial.Write(pkt) != static_cast<int>(pkt.size())) {
            SetError("Failed to write DATA packet at offset " + std::to_string(offset));
            return false;
        }

        if (!WaitAck(serial)) {
            // WaitAck already set last_error_
            last_error_ += " (at offset " + std::to_string(offset) + ")";
            return false;
        }

        offset += chunk_size;

        // Progress mapped to 20 – 95 %
        const int pct = 20 + static_cast<int>((offset * 75) / total);
        char msg[64];
        snprintf(msg, sizeof(msg), "Uploading: %zu / %zu bytes",
                 offset, total);
        Report(cb, pct, msg);
    }

    return true;
}

bool LumosBootloader::SendEndPacket(Serial& serial,
                                     const std::vector<uint8_t>& firmware)
{
    const uint16_t crc = Crc16(firmware.data(),
                                static_cast<uint32_t>(firmware.size()));

    // MCU receives 4 bytes but only uses bytes_received == app_size check;
    // send CRC16 zero-padded to 4 bytes for protocol completeness.
    uint8_t buf[5];
    buf[0] = PKT_END;
    buf[1] = static_cast<uint8_t>(crc >> 0);
    buf[2] = static_cast<uint8_t>(crc >> 8);
    buf[3] = 0x00;
    buf[4] = 0x00;

    if (serial.Write(buf, 5) != 5) {
        SetError("Failed to write END packet");
        return false;
    }
    return WaitAck(serial);
}

// ── Public Flash entry point ─────────────────────────────────────────────────

bool LumosBootloader::Flash(const std::string& port_name,
                             const std::vector<uint8_t>& firmware,
                             ProgressCallback cb)
{
    last_error_.clear();

    if (firmware.empty()) {
        SetError("Firmware is empty");
        return false;
    }

    // Use a 5-second read timeout to comfortably cover the flash erase step
    // (~1-2 s for 96 KB on STM32G0).  Normal ACK bytes arrive in <100 ms.
    SerialConfig cfg;
    cfg.baud_rate  = 115200;
    cfg.data_bits  = 8;
    cfg.stop_bits  = 1;
    cfg.parity     = 'N';
    cfg.timeout_ms = 5000;

    Serial serial;
    Report(cb, 0, "Opening " + port_name + "...");

    if (!serial.Open(port_name, cfg)) {
        SetError("Cannot open port: " + serial.GetLastError());
        return false;
    }

    serial.Flush();

    // ── Step 1: Reset MCU via DTR and send magic bytes ──────────────────────
    Report(cb, 2, "Resetting MCU via DTR...");

    // Active-low DTR reset: pull low for 50 ms then release
    serial.PulseDTR(50, true);

    // Brief pause for the MCU to come out of reset and reach bootloader_start()
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    serial.Flush();   // discard any startup noise from the app firmware

    Report(cb, 4, "Sending magic sequence...");
    const uint8_t magic[3] = { MAGIC_1, MAGIC_2, MAGIC_3 };
    if (serial.Write(magic, 3) != 3) {
        SetError("Failed to send magic bytes");
        serial.Close();
        return false;
    }

    // ── Step 2: Wait for bootloader handshake ────────────────────────────────
    Report(cb, 6, "Waiting for bootloader ACK...");
    if (!WaitBootloaderAck(serial)) {
        serial.Close();
        return false;
    }

    // ── Step 3: Wait for READY byte (bootloader entered) ────────────────────
    Report(cb, 8, "Bootloader detected, waiting for ready...");
    if (!WaitAck(serial)) {
        last_error_ = "No READY byte from bootloader: " + last_error_;
        serial.Close();
        return false;
    }

    // ── Step 4: Wait for ERASE_DONE byte ────────────────────────────────────
    Report(cb, 10, "Erasing flash (this takes ~1-2 s)...");
    if (!WaitAck(serial)) {
        last_error_ = "Flash erase failed or timed out: " + last_error_;
        serial.Close();
        return false;
    }

    // ── Step 5: START packet ─────────────────────────────────────────────────
    Report(cb, 15, "Sending firmware size...");
    if (!SendStartPacket(serial, static_cast<uint32_t>(firmware.size()))) {
        serial.Close();
        return false;
    }

    // ── Step 6: DATA packets ─────────────────────────────────────────────────
    Report(cb, 20, "Uploading firmware...");
    if (!SendDataPackets(serial, firmware, cb)) {
        serial.Close();
        return false;
    }

    // ── Step 7: END packet ───────────────────────────────────────────────────
    Report(cb, 97, "Finalising transfer...");
    if (!SendEndPacket(serial, firmware)) {
        serial.Close();
        return false;
    }

    Report(cb, 100, "Firmware downloaded successfully! MCU is booting...");
    serial.Close();
    return true;
}

} // namespace SimpleSerial
