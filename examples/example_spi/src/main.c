#include "spi_config.h"

// W25Q Flash Commands
#define W25Q_CMD_READ_JEDEC_ID      0x9F
#define W25Q_CMD_READ_STATUS1       0x05
#define W25Q_CMD_WRITE_ENABLE       0x06
#define W25Q_CMD_CHIP_ERASE         0xC7
#define W25Q_CMD_READ_DATA          0x03
#define W25Q_CMD_PAGE_PROGRAM       0x02

// Status register bits
#define W25Q_STATUS_BUSY            0x01

/**
 * @brief Read JEDEC ID from flash
 * @param manufacturer Pointer to store manufacturer ID
 * @param device_type Pointer to store device type
 * @param capacity Pointer to store capacity
 */
void Flash_ReadJEDEC_ID(uint8_t* manufacturer, uint8_t* device_type, uint8_t* capacity)
{
    uint8_t tx_data[4] = {W25Q_CMD_READ_JEDEC_ID, 0xFF, 0xFF, 0xFF};
    uint8_t rx_data[4];

    SPI_CS_LOW();
    SPI_TransmitReceive(tx_data, rx_data, 4);
    SPI_CS_HIGH();

    *manufacturer = rx_data[1];
    *device_type = rx_data[2];
    *capacity = rx_data[3];
}

/**
 * @brief Read status register
 * @return Status register value
 */
uint8_t Flash_ReadStatus(void)
{
    uint8_t tx_data[2] = {W25Q_CMD_READ_STATUS1, 0xFF};
    uint8_t rx_data[2];

    SPI_CS_LOW();
    SPI_TransmitReceive(tx_data, rx_data, 2);
    SPI_CS_HIGH();

    return rx_data[1];
}

/**
 * @brief Wait until flash is not busy
 */
void Flash_WaitReady(void)
{
    while (Flash_ReadStatus() & W25Q_STATUS_BUSY)
    {
        HAL_Delay(1);
    }
}

/**
 * @brief Enable write operations
 */
void Flash_WriteEnable(void)
{
    uint8_t cmd = W25Q_CMD_WRITE_ENABLE;

    SPI_CS_LOW();
    SPI_Transmit(&cmd, 1);
    SPI_CS_HIGH();
}

/**
 * @brief Read data from flash
 * @param address 24-bit address to read from
 * @param buffer Pointer to store read data
 * @param size Number of bytes to read
 */
void Flash_ReadData(uint32_t address, uint8_t* buffer, uint16_t size)
{
    uint8_t cmd[4];
    cmd[0] = W25Q_CMD_READ_DATA;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;

    SPI_CS_LOW();
    SPI_Transmit(cmd, 4);
    SPI_Receive(buffer, size);
    SPI_CS_HIGH();
}

int main(void)
{
    uint8_t manufacturer, device_type, capacity;
    uint8_t status;

    // Initialize HAL Library
    HAL_Init();

    // Configure system clock to 550 MHz
    SystemClock_Config();

    // Initialize SPI
    SPI_Init();

    // Give SPI devices time to power up
    HAL_Delay(100);

    // Main loop
    while (1)
    {
        // Read JEDEC ID from flash (if connected)
        Flash_ReadJEDEC_ID(&manufacturer, &device_type, &capacity);

        // Check if we got valid data
        // Common manufacturers: Winbond=0xEF, Micron=0x20, ISSI=0x9D
        if (manufacturer == 0xEF || manufacturer == 0x20 || manufacturer == 0x9D)
        {
            // Valid flash detected
            // manufacturer: 0xEF = Winbond
            // device_type: 0x40 = W25Q series
            // capacity: 0x14 = 8Mbit (1MB), 0x15 = 16Mbit (2MB), etc.

            // Read status register
            status = Flash_ReadStatus();

            // You can add more flash operations here:
            // 1. Read data from specific address
            // 2. Write data to flash
            // 3. Erase sectors
            // 4. Check write protection status

            // Example: Read first 16 bytes from flash
            uint8_t read_buffer[16];
            Flash_ReadData(0x000000, read_buffer, 16);

            // Data is now in read_buffer
            // In a real application, you might output this to UART
            (void)read_buffer;  // Suppress unused variable warning
        }
        else
        {
            // No valid flash detected or SPI not connected
            // You could toggle an LED or output a message here
        }

        // Wait 1 second before next operation
        HAL_Delay(1000);
    }

    return 0;
}

// SysTick interrupt handler (required by HAL_Delay)
void SysTick_Handler(void)
{
    HAL_IncTick();
}
