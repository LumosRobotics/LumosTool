#include "usb.h"

USB::USB(PCD_TypeDef* usb_instance)
    : pcd_handle_{},
      rx_buffer_{},
      rx_head_(0),
      rx_tail_(0),
      initialized_(false),
      connected_(false)
{
    pcd_handle_.Instance = usb_instance;
}

bool USB::begin()
{
    // Configure USB OTG HS
    pcd_handle_.Init.dev_endpoints = 9;
    pcd_handle_.Init.speed = PCD_SPEED_HIGH;
    pcd_handle_.Init.dma_enable = DISABLE;
    pcd_handle_.Init.phy_itface = USB_OTG_EMBEDDED_PHY;
    pcd_handle_.Init.Sof_enable = DISABLE;
    pcd_handle_.Init.low_power_enable = DISABLE;
    pcd_handle_.Init.lpm_enable = DISABLE;
    pcd_handle_.Init.battery_charging_enable = DISABLE;
    pcd_handle_.Init.vbus_sensing_enable = DISABLE;
    pcd_handle_.Init.use_dedicated_ep1 = DISABLE;

    // Initialize USB peripheral
    if (HAL_PCD_Init(&pcd_handle_) != HAL_OK) {
        initialized_ = false;
        return false;
    }

    // Note: CDC device class initialization would be done here
    // This requires the USB Device middleware which is included in the toolchain

    initialized_ = true;
    rx_head_ = 0;
    rx_tail_ = 0;
    return true;
}

void USB::end()
{
    if (initialized_) {
        HAL_PCD_DeInit(&pcd_handle_);
        initialized_ = false;
        connected_ = false;
    }
}

bool USB::write(const uint8_t* data, uint16_t length, uint32_t timeout)
{
    if (!initialized_ || !connected_ || !data || length == 0) {
        return false;
    }

    // Note: Actual CDC transmission would use CDC_Transmit_HS() function
    // This is a placeholder that would need to call the USB CDC middleware
    // For now, we'll return true to indicate the interface is available

    // In a full implementation, this would be:
    // return (CDC_Transmit_HS((uint8_t*)data, length) == USBD_OK);

    return true;
}

bool USB::write(uint8_t byte)
{
    return write(&byte, 1);
}

bool USB::print(const char* str)
{
    if (!str) return false;
    return write((const uint8_t*)str, strlen(str));
}

bool USB::print(int value)
{
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", value);
    return print(buffer);
}

bool USB::print(float value, int decimals)
{
    char buffer[32];
    char format[8];
    snprintf(format, sizeof(format), "%%.%df", decimals);
    snprintf(buffer, sizeof(buffer), format, value);
    return print(buffer);
}

bool USB::println(const char* str)
{
    return print(str) && print("\r\n");
}

bool USB::println(int value)
{
    return print(value) && print("\r\n");
}

bool USB::println(float value, int decimals)
{
    return print(value, decimals) && print("\r\n");
}

bool USB::println()
{
    return print("\r\n");
}

uint16_t USB::available()
{
    if (!initialized_) {
        return 0;
    }

    if (rx_head_ >= rx_tail_) {
        return rx_head_ - rx_tail_;
    } else {
        return RX_BUFFER_SIZE - rx_tail_ + rx_head_;
    }
}

uint16_t USB::read(uint8_t* buffer, uint16_t length)
{
    if (!initialized_ || !buffer || length == 0) {
        return 0;
    }

    uint16_t bytes_read = 0;
    while (bytes_read < length && available() > 0) {
        buffer[bytes_read++] = rx_buffer_[rx_tail_];
        rx_tail_ = (rx_tail_ + 1) % RX_BUFFER_SIZE;
    }

    return bytes_read;
}

int USB::read()
{
    if (!initialized_ || available() == 0) {
        return -1;
    }

    uint8_t byte = rx_buffer_[rx_tail_];
    rx_tail_ = (rx_tail_ + 1) % RX_BUFFER_SIZE;
    return byte;
}

bool USB::isConnected()
{
    return initialized_ && connected_;
}

bool USB::isReady()
{
    return initialized_ && connected_;
}

void USB::flush()
{
    rx_head_ = 0;
    rx_tail_ = 0;
}

void USB::onDataReceived(uint8_t* data, uint32_t length)
{
    if (!data || length == 0) {
        return;
    }

    // Copy data to circular buffer
    for (uint32_t i = 0; i < length; i++) {
        uint16_t next_head = (rx_head_ + 1) % RX_BUFFER_SIZE;

        // Check if buffer is full
        if (next_head == rx_tail_) {
            // Buffer full, drop oldest data
            rx_tail_ = (rx_tail_ + 1) % RX_BUFFER_SIZE;
        }

        rx_buffer_[rx_head_] = data[i];
        rx_head_ = next_head;
    }
}

void USB::onConnect()
{
    connected_ = true;
}

void USB::onDisconnect()
{
    connected_ = false;
    flush();
}

uint16_t USB::getRxBufferAvailable()
{
    if (rx_head_ >= rx_tail_) {
        return RX_BUFFER_SIZE - (rx_head_ - rx_tail_);
    } else {
        return rx_tail_ - rx_head_;
    }
}
