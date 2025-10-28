#include "usbd_cdc_if.h"

// CDC receive buffer
#define APP_RX_DATA_SIZE  512
#define APP_TX_DATA_SIZE  512

uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

// External USB device handle
extern USBD_HandleTypeDef hUsbDeviceFS;

// Private function prototypes
static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len);

// CDC interface callbacks
USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
    CDC_Init_FS,
    CDC_DeInit_FS,
    CDC_Control_FS,
    CDC_Receive_FS
};

/**
 * @brief Initialize the CDC media low layer
 * @retval USBD_OK if success, USBD_FAIL otherwise
 */
static int8_t CDC_Init_FS(void)
{
    // Set Application Buffers
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
    return (USBD_OK);
}

/**
 * @brief De-initialize the CDC media low layer
 * @retval USBD_OK if success, USBD_FAIL otherwise
 */
static int8_t CDC_DeInit_FS(void)
{
    return (USBD_OK);
}

/**
 * @brief Manage the CDC class requests
 * @param cmd: Command code
 * @param pbuf: Buffer containing command data (request parameters)
 * @param length: Number of data to be sent (in bytes)
 * @retval Result of the operation: USBD_OK if success
 */
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
    switch(cmd)
    {
        case CDC_SEND_ENCAPSULATED_COMMAND:
            break;

        case CDC_GET_ENCAPSULATED_RESPONSE:
            break;

        case CDC_SET_COMM_FEATURE:
            break;

        case CDC_GET_COMM_FEATURE:
            break;

        case CDC_CLEAR_COMM_FEATURE:
            break;

        /*******************************************************************************/
        /* Line Coding Structure                                                       */
        /*-----------------------------------------------------------------------------*/
        /* Offset | Field       | Size | Value  | Description                          */
        /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
        /* 4      | bCharFormat |   1  | Number | Stop bits                            */
        /*                                        0 - 1 Stop bit                       */
        /*                                        1 - 1.5 Stop bits                    */
        /*                                        2 - 2 Stop bits                      */
        /* 5      | bParityType |  1   | Number | Parity                               */
        /*                                        0 - None                             */
        /*                                        1 - Odd                              */
        /*                                        2 - Even                             */
        /*                                        3 - Mark                             */
        /*                                        4 - Space                            */
        /* 6      | bDataBits   |   1  | Number Data bits (5, 6, 7, 8 or 16).          */
        /*******************************************************************************/
        case CDC_SET_LINE_CODING:
            // Handle line coding (baud rate, stop bits, parity, data bits)
            // For this example, we just accept any configuration
            break;

        case CDC_GET_LINE_CODING:
            // Return current line coding
            // For this example, return default values (115200, 8N1)
            if (length >= 7)
            {
                pbuf[0] = (uint8_t)(115200);       // Baud rate: 115200
                pbuf[1] = (uint8_t)(115200 >> 8);
                pbuf[2] = (uint8_t)(115200 >> 16);
                pbuf[3] = (uint8_t)(115200 >> 24);
                pbuf[4] = 0;  // Stop bits: 1
                pbuf[5] = 0;  // Parity: None
                pbuf[6] = 8;  // Data bits: 8
            }
            break;

        case CDC_SET_CONTROL_LINE_STATE:
            // Handle DTR and RTS signals
            break;

        case CDC_SEND_BREAK:
            break;

        default:
            break;
    }

    return (USBD_OK);
}

/**
 * @brief Data received over USB OUT endpoint
 * @param Buf: Buffer of data to be received
 * @param Len: Number of data received (in bytes)
 * @retval Result of the operation: USBD_OK if success
 */
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
    // This function is called when data is received from the host
    // You can process the received data here

    // Echo back the received data (loopback)
    CDC_Transmit_FS(Buf, *Len);

    // Prepare for next reception
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    return (USBD_OK);
}

/**
 * @brief Transmit data over USB IN endpoint
 * @param Buf: Buffer of data to be sent
 * @param Len: Number of data to be sent (in bytes)
 * @retval USBD_OK if success, USBD_BUSY otherwise
 */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
    uint8_t result = USBD_OK;
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;

    if (hcdc == NULL)
    {
        return USBD_FAIL;
    }

    if (hcdc->TxState != 0)
    {
        return USBD_BUSY;
    }

    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, Buf, Len);
    result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);

    return result;
}
