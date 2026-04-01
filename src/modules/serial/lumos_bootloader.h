#pragma once

#include "serial.h"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace SimpleSerial {

/**
 * @brief PC-side implementation of the custom Lumos bootloader protocol.
 *
 * Matches the MCU bootloader in CanModuleV2Bootloader/Core/Src/bootloader.c.
 *
 * Protocol flow:
 *   1. Pulse DTR to reset MCU
 *   2. Send magic bytes (0x7E 0x5B 0x9C) within 300 ms window
 *   3. Receive bootloader ACK (0xAC 0xCE 0x55)
 *   4. Receive READY byte (0xAA) – bootloader entered
 *   5. Receive ERASE_DONE byte (0xAA) – flash erased (~1-2 s)
 *   6. Send START packet: 0x01 + uint32 firmware_size (LE)
 *   7. Send DATA packets: 0x02 + uint16 size (LE) + <size> bytes + CRC16 (LE)
 *   8. Send END packet:   0x03 + uint32 firmware_crc16 (LE, zero-padded)
 *   Each step waits for an ACK byte (0xAA) before continuing.
 */
class LumosBootloader {
public:
    /**
     * @brief Progress/status callback.
     * @param percent  0-100
     * @param message  Human-readable description of the current step
     */
    using ProgressCallback = std::function<void(int percent, const std::string& message)>;

    LumosBootloader() = default;

    /**
     * @brief Flash firmware to the MCU via the custom Lumos bootloader protocol.
     *
     * Opens the serial port, resets the MCU via DTR, performs the full
     * download sequence, then closes the port.
     *
     * @param port_name  Serial port (e.g. "/dev/cu.usbserial-XXXX")
     * @param firmware   Raw binary firmware bytes
     * @param progress   Optional callback invoked at each protocol step
     * @return true on success, false otherwise (check GetLastError())
     */
    bool Flash(const std::string& port_name,
               const std::vector<uint8_t>& firmware,
               ProgressCallback progress = nullptr);

    std::string GetLastError() const { return last_error_; }

    /** CRC16-CCITT (XMODEM) – same table used on the MCU side */
    static uint16_t Crc16(const uint8_t* data, uint32_t len);

private:
    bool WaitBootloaderAck(Serial& serial);
    bool WaitAck(Serial& serial);
    bool SendStartPacket(Serial& serial, uint32_t firmware_size);
    bool SendDataPackets(Serial& serial, const std::vector<uint8_t>& firmware,
                         const ProgressCallback& cb);
    bool SendEndPacket(Serial& serial, const std::vector<uint8_t>& firmware);

    void Report(const ProgressCallback& cb, int percent, const std::string& msg);
    void SetError(const std::string& error);

    std::string last_error_;

    // Protocol constants (mirror bootloader.h)
    static constexpr uint8_t  MAGIC_1     = 0x7E;
    static constexpr uint8_t  MAGIC_2     = 0x5B;
    static constexpr uint8_t  MAGIC_3     = 0x9C;
    static constexpr uint8_t  BOOT_ACK_1  = 0xAC;
    static constexpr uint8_t  BOOT_ACK_2  = 0xCE;
    static constexpr uint8_t  BOOT_ACK_3  = 0x55;
    static constexpr uint8_t  PKT_START   = 0x01;
    static constexpr uint8_t  PKT_DATA    = 0x02;
    static constexpr uint8_t  PKT_END     = 0x03;
    static constexpr uint8_t  RESP_ACK    = 0xAA;
    static constexpr uint8_t  RESP_NACK   = 0x55;
    static constexpr uint16_t CHUNK_SIZE  = 256;
};

} // namespace SimpleSerial
