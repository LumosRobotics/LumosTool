#include "gpio.h"

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

// GPIO Class Implementation

GPIO::GPIO(GPIO_TypeDef* port, uint16_t pin)
    : port_(port),
      pin_(pin),
      initialized_(false)
{
    // Enable GPIO port clock
    enableGPIOClock(port_);
}

void GPIO::mode(uint32_t mode, uint32_t pull, uint32_t speed)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin_;
    GPIO_InitStruct.Mode = mode;
    GPIO_InitStruct.Pull = pull;
    GPIO_InitStruct.Speed = speed;

    HAL_GPIO_Init(port_, &GPIO_InitStruct);

    initialized_ = true;
}

void GPIO::setAlternateFunction(uint32_t alternate)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin_;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = alternate;

    HAL_GPIO_Init(port_, &GPIO_InitStruct);

    initialized_ = true;
}

void GPIO::write(bool value)
{
    HAL_GPIO_WritePin(port_, pin_, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool GPIO::read() const
{
    return HAL_GPIO_ReadPin(port_, pin_) == GPIO_PIN_SET;
}

void GPIO::toggle()
{
    HAL_GPIO_TogglePin(port_, pin_);
}

void GPIO::high()
{
    write(true);
}

void GPIO::low()
{
    write(false);
}

// Helper Functions Implementation

void pinMode(GPIO_TypeDef* port, uint16_t pin, uint32_t mode, uint32_t pull)
{
    // Enable GPIO port clock
    enableGPIOClock(port);

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = mode;
    GPIO_InitStruct.Pull = pull;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

void digitalWrite(GPIO_TypeDef* port, uint16_t pin, bool value)
{
    HAL_GPIO_WritePin(port, pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool digitalRead(GPIO_TypeDef* port, uint16_t pin)
{
    return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET;
}

void togglePin(GPIO_TypeDef* port, uint16_t pin)
{
    HAL_GPIO_TogglePin(port, pin);
}
