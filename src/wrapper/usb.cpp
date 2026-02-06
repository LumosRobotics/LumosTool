#include "usb.h"

// Helper function to enable GPIO port clock
static void enableGPIOClock(GPIO_TypeDef* port)
{
    if (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
#ifdef GPIOG
    else if (port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
#endif
#ifdef GPIOH
    else if (port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
#endif
}

USB::USB(PCD_TypeDef* usb_instance,
         GPIO_TypeDef* dp_port, uint16_t dp_pin,
         GPIO_TypeDef* dm_port, uint16_t dm_pin,
         uint32_t alternate_function)
    : pcd_handle_{},
      rx_buffer_{},
      rx_head_(0),
      rx_tail_(0),
      initialized_(false),
      connected_(false),
      dp_port_(dp_port),
      dp_pin_(dp_pin),
      dm_port_(dm_port),
      dm_pin_(dm_pin),
      alternate_function_(alternate_function)
{
    pcd_handle_.Instance = usb_instance;
}

bool USB::begin()
{
    // Enable GPIO port clocks
    enableGPIOClock(dp_port_);
    enableGPIOClock(dm_port_);

    // Configure GPIO pins for USB
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure DP pin (D+)
    GPIO_InitStruct.Pin = dp_pin_;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = alternate_function_;
    HAL_GPIO_Init(dp_port_, &GPIO_InitStruct);

    // Configure DM pin (D-)
    GPIO_InitStruct.Pin = dm_pin_;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = alternate_function_;
    HAL_GPIO_Init(dm_port_, &GPIO_InitStruct);

    // Configure USB - platform specific
#if defined(STM32H7)
    // USB OTG HS on STM32H7
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
#elif defined(STM32G0) || defined(STM32G4)
    // USB DRD (Dual Role Device) on STM32G0/G4
    pcd_handle_.Init.dev_endpoints = 8;
    pcd_handle_.Init.speed = PCD_SPEED_FULL;
    pcd_handle_.Init.phy_itface = PCD_PHY_EMBEDDED;
    pcd_handle_.Init.Sof_enable = DISABLE;
    pcd_handle_.Init.low_power_enable = DISABLE;
    pcd_handle_.Init.lpm_enable = DISABLE;
    pcd_handle_.Init.battery_charging_enable = DISABLE;
#elif defined(STM32F4)
    // USB OTG FS on STM32F4
    pcd_handle_.Init.dev_endpoints = 4;
    pcd_handle_.Init.speed = PCD_SPEED_FULL;
    pcd_handle_.Init.dma_enable = DISABLE;
    pcd_handle_.Init.phy_itface = PCD_PHY_EMBEDDED;
    pcd_handle_.Init.Sof_enable = DISABLE;
    pcd_handle_.Init.low_power_enable = DISABLE;
    pcd_handle_.Init.lpm_enable = DISABLE;
    pcd_handle_.Init.vbus_sensing_enable = DISABLE;
    pcd_handle_.Init.use_dedicated_ep1 = DISABLE;
#elif defined(STM32H5)
    // USB OTG on STM32H5
    pcd_handle_.Init.dev_endpoints = 9;
    pcd_handle_.Init.speed = PCD_SPEED_FULL;
    pcd_handle_.Init.phy_itface = PCD_PHY_EMBEDDED;
    pcd_handle_.Init.Sof_enable = DISABLE;
    pcd_handle_.Init.low_power_enable = DISABLE;
    pcd_handle_.Init.lpm_enable = DISABLE;
    pcd_handle_.Init.battery_charging_enable = DISABLE;
    pcd_handle_.Init.vbus_sensing_enable = DISABLE;
#endif

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

        // Deinitialize GPIO pins
        HAL_GPIO_DeInit(dp_port_, dp_pin_);
        HAL_GPIO_DeInit(dm_port_, dm_pin_);

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
