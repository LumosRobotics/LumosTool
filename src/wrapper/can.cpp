#include "can.h"

CAN::CAN(FDCAN_GlobalTypeDef* fdcan_instance) : fdcan_handle_{}
{
    // Initialize FDCAN handle with default values
    fdcan_handle_.Instance = fdcan_instance;

    // Frame Format and Mode
    fdcan_handle_.Init.FrameFormat = FDCAN_FRAME_CLASSIC;  // Classic CAN (not FD)
    fdcan_handle_.Init.Mode = FDCAN_MODE_NORMAL;
    fdcan_handle_.Init.AutoRetransmission = ENABLE;
    fdcan_handle_.Init.TransmitPause = DISABLE;
    fdcan_handle_.Init.ProtocolException = ENABLE;

    // Nominal Bit Timing (500 kbps default, assuming 80 MHz kernel clock)
    // Bit time = (NominalPrescaler) * (NominalSyncJumpWidth + NominalTimeSeg1 + NominalTimeSeg2)
    // 500 kbps: 80 MHz / (10 * (1 + 13 + 2)) = 500 kbps
    fdcan_handle_.Init.NominalPrescaler = 10;
    fdcan_handle_.Init.NominalSyncJumpWidth = 1;
    fdcan_handle_.Init.NominalTimeSeg1 = 13;
    fdcan_handle_.Init.NominalTimeSeg2 = 2;

    // Data Bit Timing (not used in classic CAN)
    fdcan_handle_.Init.DataPrescaler = 1;
    fdcan_handle_.Init.DataSyncJumpWidth = 1;
    fdcan_handle_.Init.DataTimeSeg1 = 1;
    fdcan_handle_.Init.DataTimeSeg2 = 1;

    // Message RAM Configuration
    fdcan_handle_.Init.MessageRAMOffset = 0;
    fdcan_handle_.Init.StdFiltersNbr = 1;
    fdcan_handle_.Init.ExtFiltersNbr = 1;
    fdcan_handle_.Init.RxFifo0ElmtsNbr = 8;
    fdcan_handle_.Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
    fdcan_handle_.Init.RxFifo1ElmtsNbr = 0;
    fdcan_handle_.Init.RxBuffersNbr = 0;
    fdcan_handle_.Init.TxEventsNbr = 0;
    fdcan_handle_.Init.TxBuffersNbr = 0;
    fdcan_handle_.Init.TxFifoQueueElmtsNbr = 8;
    fdcan_handle_.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    fdcan_handle_.Init.TxElmtSize = FDCAN_DATA_BYTES_8;
}

void CAN::begin(const uint32_t bitrate)
{
    // Calculate prescaler for desired bitrate (assuming 80 MHz kernel clock)
    // Bitrate = ClockFreq / (Prescaler * (SyncJumpWidth + TimeSeg1 + TimeSeg2))
    // For simplicity, use fixed time segments and calculate prescaler
    uint32_t prescaler = 80000000 / (bitrate * 16);  // 16 time quanta
    fdcan_handle_.Init.NominalPrescaler = (prescaler > 0) ? prescaler : 1;

    // Initialize FDCAN peripheral
    if (HAL_FDCAN_Init(&fdcan_handle_) != HAL_OK)
    {
        // Error handling (could add error callback)
        return;
    }

    // Configure global filter to accept all messages by default
    setAcceptAll();

    // Start FDCAN module
    HAL_FDCAN_Start(&fdcan_handle_);
}

void CAN::end()
{
    HAL_FDCAN_Stop(&fdcan_handle_);
    HAL_FDCAN_DeInit(&fdcan_handle_);
}

bool CAN::send(uint32_t id, const uint8_t* data, uint8_t length, bool extended)
{
    if (length > 8)
        return false;

    FDCAN_TxHeaderTypeDef tx_header;
    tx_header.Identifier = id;
    tx_header.IdType = extended ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = length << 16;  // Convert to FDCAN_DLC format
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header.MessageMarker = 0;

    // Add message to TX FIFO
    if (HAL_FDCAN_AddMessageToTxFifoQ(&fdcan_handle_, &tx_header, (uint8_t*)data) != HAL_OK)
    {
        return false;
    }

    return true;
}

bool CAN::sendRemote(uint32_t id, bool extended)
{
    FDCAN_TxHeaderTypeDef tx_header;
    tx_header.Identifier = id;
    tx_header.IdType = extended ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    tx_header.TxFrameType = FDCAN_REMOTE_FRAME;
    tx_header.DataLength = 0;
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header.MessageMarker = 0;

    if (HAL_FDCAN_AddMessageToTxFifoQ(&fdcan_handle_, &tx_header, nullptr) != HAL_OK)
    {
        return false;
    }

    return true;
}

bool CAN::available()
{
    return (HAL_FDCAN_GetRxFifoFillLevel(&fdcan_handle_, FDCAN_RX_FIFO0) > 0);
}

bool CAN::read(uint32_t& id, uint8_t* data, uint8_t& length, bool& extended)
{
    if (!available())
        return false;

    FDCAN_RxHeaderTypeDef rx_header;

    // Get message from RX FIFO
    if (HAL_FDCAN_GetRxMessage(&fdcan_handle_, FDCAN_RX_FIFO0, &rx_header, data) != HAL_OK)
    {
        return false;
    }

    id = rx_header.Identifier;
    extended = (rx_header.IdType == FDCAN_EXTENDED_ID);
    length = rx_header.DataLength >> 16;  // Convert from FDCAN_DLC format

    return true;
}

void CAN::setFilter(uint32_t id, uint32_t mask, bool extended)
{
    FDCAN_FilterTypeDef filter;

    if (extended)
    {
        filter.IdType = FDCAN_EXTENDED_ID;
        filter.FilterIndex = 0;
        filter.FilterType = FDCAN_FILTER_MASK;
        filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
        filter.FilterID1 = id;
        filter.FilterID2 = mask;

        HAL_FDCAN_ConfigFilter(&fdcan_handle_, &filter);
    }
    else
    {
        filter.IdType = FDCAN_STANDARD_ID;
        filter.FilterIndex = 0;
        filter.FilterType = FDCAN_FILTER_MASK;
        filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
        filter.FilterID1 = id;
        filter.FilterID2 = mask;

        HAL_FDCAN_ConfigFilter(&fdcan_handle_, &filter);
    }
}

void CAN::setAcceptAll()
{
    // Configure global filter to accept all standard and extended IDs
    HAL_FDCAN_ConfigGlobalFilter(
        &fdcan_handle_,
        FDCAN_ACCEPT_IN_RX_FIFO0,  // Accept standard IDs
        FDCAN_ACCEPT_IN_RX_FIFO0,  // Accept extended IDs
        FDCAN_FILTER_REMOTE,        // Filter remote frames
        FDCAN_FILTER_REMOTE         // Filter remote frames
    );
}

uint32_t CAN::getErrorCount()
{
    FDCAN_ErrorCountersTypeDef counters;
    HAL_FDCAN_GetErrorCounters(&fdcan_handle_, &counters);

    // Return sum of TX and RX error counters
    return counters.TxErrorCnt + counters.RxErrorCnt;
}

bool CAN::isBusOff()
{
    FDCAN_ProtocolStatusTypeDef status;
    HAL_FDCAN_GetProtocolStatus(&fdcan_handle_, &status);

    return (status.BusOff == 1);
}
