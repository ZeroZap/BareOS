#include "xy_hal_n32l40x_internal.h"

static uint16_t spi_prescaler(uint32_t speed_hz)
{
    (void)speed_hz;
    return XY_HAL_N32_SPI_DEFAULT_PRESCALER;
}

int xy_hal_spi_init(void *spi, const xy_hal_spi_config_t *config)
{
    SPI_Module *s = (SPI_Module *)spi;
    SPI_InitType init;
    if (!s || !config) return XY_HAL_INVALID_PARAM;
    if (config->io_mode == XY_HAL_IO_DMA && (!config->rx_dma || !config->tx_dma)) return XY_HAL_INVALID_PARAM;
    xy_hal_n32_enable_spi_clock(s);
    SPI_InitStruct(&init);
    init.DataDirection = SPI_DIR_DOUBLELINE_FULLDUPLEX;
    init.SpiMode = SPI_MODE_MASTER;
    init.DataLen = config->data_bits == 16u ? SPI_DATA_SIZE_16BITS : SPI_DATA_SIZE_8BITS;
    init.CLKPOL = (config->mode & 0x2u) ? SPI_CLKPOL_HIGH : SPI_CLKPOL_LOW;
    init.CLKPHA = (config->mode & 0x1u) ? SPI_CLKPHA_SECOND_EDGE : SPI_CLKPHA_FIRST_EDGE;
    init.NSS = SPI_NSS_SOFT;
    init.BaudRatePres = spi_prescaler(config->speed_hz);
    init.FirstBit = SPI_FB_MSB;
    init.CRCPoly = 7u;
    SPI_Init(s, &init);
    SPI_I2S_EnableDma(s, SPI_I2S_DMA_RX, config->rx_dma ? ENABLE : DISABLE);
    SPI_I2S_EnableDma(s, SPI_I2S_DMA_TX, config->tx_dma ? ENABLE : DISABLE);
    SPI_Enable(s, ENABLE);
    return XY_HAL_OK;
}

int xy_hal_spi_deinit(void *spi)
{
    if (!spi) return XY_HAL_INVALID_PARAM;
    SPI_Enable((SPI_Module *)spi, DISABLE);
    SPI_I2S_DeInit((SPI_Module *)spi);
    return XY_HAL_OK;
}

int xy_hal_spi_transfer(void *spi, const uint8_t *tx, uint8_t *rx, uint16_t len, uint32_t timeout_ms)
{
    SPI_Module *s = (SPI_Module *)spi;
    uint32_t start = xy_hal_time_ms();
    uint16_t n;
    if (!s || (!tx && !rx && len)) return XY_HAL_INVALID_PARAM;
    for (n = 0u; n < len; n++) {
        while (SPI_I2S_GetStatus(s, SPI_I2S_TE_FLAG) == RESET) if (xy_hal_n32_expired(start, timeout_ms)) return XY_HAL_TIMEOUT;
        SPI_I2S_TransmitData(s, tx ? tx[n] : 0xffu);
        while (SPI_I2S_GetStatus(s, SPI_I2S_RNE_FLAG) == RESET) if (xy_hal_n32_expired(start, timeout_ms)) return XY_HAL_TIMEOUT;
        if (rx) rx[n] = (uint8_t)SPI_I2S_ReceiveData(s);
        else (void)SPI_I2S_ReceiveData(s);
    }
    return (int)len;
}

int xy_hal_spi_read(void *spi, uint8_t *data, uint16_t len)
{
    return xy_hal_spi_transfer(spi, NULL, data, len, 100u);
}

int xy_hal_spi_write(void *spi, const uint8_t *data, uint16_t len)
{
    return xy_hal_spi_transfer(spi, data, NULL, len, 100u);
}

void xy_hal_spi_irq_handler(void *spi)
{
    (void)spi;
}
