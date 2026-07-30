/********************************** (C) COPYRIGHT *******************************
* File Name          : lmx2572.c
* Description        : LMX2572 SPI register programming interface.
*******************************************************************************/

#include "lmx2572.h"
#include "debug.h"

#define LMX2572_CSB_PORT          GPIOA
#define LMX2572_CSB_PIN           GPIO_Pin_4
#define LMX2572_SCK_PIN           GPIO_Pin_5
#define LMX2572_MOSI_PIN          GPIO_Pin_7
#define LMX2572_MUXOUT_PORT       GPIOB
#define LMX2572_MUXOUT_PIN        GPIO_Pin_1

#define LMX2572_CSB_HIGH()        GPIO_SetBits(LMX2572_CSB_PORT, LMX2572_CSB_PIN)
#define LMX2572_CSB_LOW()         GPIO_ResetBits(LMX2572_CSB_PORT, LMX2572_CSB_PIN)

#define LMX2572_FPD_MHZ           100UL
#define LMX2572_PLL_DEN           10UL
#define LMX2572_SWEEP_START_MHZ   1230UL
#define LMX2572_SWEEP_STOP_MHZ    6000UL
#define LMX2572_SWEEP_STEP_MHZ    500UL
#define LMX2572_MIN_FREQ_MHZ      13UL
#define LMX2572_FIXED_FREQ_MHZ    45UL

#define LMX2572_REG_R0            0x00211C
#define LMX2572_R36_PLL_N         0x240000
#define LMX2572_R38_PLL_DEN_MSB   0x260000
#define LMX2572_R39_PLL_DEN_LSB   0x270000
#define LMX2572_R42_PLL_NUM_MSB   0x2A0000
#define LMX2572_R43_PLL_NUM_LSB   0x2B0000
#define LMX2572_R37_PFD_DLY_2     0x250205
#define LMX2572_R37_PFD_DLY_3     0x250305
#define LMX2572_R44_MASH_INT      0x2C01A3
#define LMX2572_R44_MASH_FRAC     0x2C0AA3
#define LMX2572_R45_OUTA_MUX_MASK 0x000800
#define LMX2572_R75_CHDIV_MASK    0x0007C0
#define LMX2572_OUTA_MUX_VCO      LMX2572_R45_OUTA_MUX_MASK
#define LMX2572_OUTA_MUX_CHDIV    0x000000

static const uint32_t lmx2572_reg_values[] = {
    0x7D2288, 0x7C0000, 0x7B0000, 0x7A0000, 0x790000, 0x780000, 0x770000, 0x760000,
    0x750000, 0x740000, 0x730000, 0x727802, 0x710000, 0x700000, 0x6F0000, 0x6E0000,
    0x6D0000, 0x6C0000, 0x6B0000, 0x6A0007, 0x690000, 0x682710, 0x670000, 0x660000,
    0x650000, 0x642710, 0x638698, 0x620004, 0x610000, 0x600000, 0x5F0000, 0x5E0000,
    0x5D0000, 0x5C0000, 0x5B0000, 0x5A0000, 0x590000, 0x580000, 0x570000, 0x560000,
    0x551400, 0x540000, 0x530000, 0x526400, 0x510000, 0x50CCCC, 0x4F004C, 0x4E0001,
    0x4D0000, 0x4C000C, 0x4B0B00, 0x4A0000, 0x49003F, 0x480000, 0x470041, 0x46C350,
    0x450000, 0x4403E8, 0x430000, 0x4201F4, 0x410000, 0x401388, 0x3F0000, 0x3E00AF,
    0x3D00A8, 0x3C03E8, 0x3B0001, 0x3A9001, 0x390020, 0x380000, 0x370000, 0x360000,
    0x350000, 0x340421, 0x330080, 0x320080, 0x314180, 0x3003E0, 0x2F0300, 0x2E07F0,
    0x2DC60F, 0x2C0FA3, 0x2B0003, 0x2A0000, 0x290000, 0x280000, 0x270005, 0x260000,
    0x250305, 0x240039, 0x230004, 0x220010, 0x211E01, 0x2005BF, 0x1FC3E6, 0x1E18A6,
    0x1D0000, 0x1C0488, 0x1B0002, 0x1A0808, 0x190624, 0x18071A, 0x17007C, 0x160001,
    0x150409, 0x144848, 0x1327B7, 0x120064, 0x110096, 0x100080, 0x0F060E, 0x0E1820,
    0x0D4000, 0x0C5001, 0x0BB018, 0x0A1278, 0x090004, 0x082000, 0x0740B2, 0x06C802,
    0x0530C8, 0x040A43, 0x030782, 0x020500, 0x010808, 0x00211C,
};

static void LMX2572_GPIOConfig(void)
{
    GPIO_InitTypeDef gpio = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO, ENABLE);

    LMX2572_CSB_HIGH();

    gpio.GPIO_Pin = LMX2572_CSB_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(LMX2572_CSB_PORT, &gpio);

    gpio.GPIO_Pin = LMX2572_SCK_PIN | LMX2572_MOSI_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = LMX2572_MUXOUT_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(LMX2572_MUXOUT_PORT, &gpio);
}

static void LMX2572_SPIConfig(void)
{
    SPI_InitTypeDef spi = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    SPI_I2S_DeInit(SPI1);
    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_1Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &spi);
    SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Set);
    SPI_Cmd(SPI1, ENABLE);
}

static void LMX2572_SPIWriteByte(uint8_t data)
{
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
    {
    }

    SPI_I2S_SendData(SPI1, data);

    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)
    {
    }

    (void)SPI_I2S_ReceiveData(SPI1);
}

static void LMX2572_WriteRegister(uint32_t value)
{
    LMX2572_CSB_LOW();
    LMX2572_SPIWriteByte((uint8_t)(value >> 16));
    LMX2572_SPIWriteByte((uint8_t)(value >> 8));
    LMX2572_SPIWriteByte((uint8_t)value);

    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) != RESET)
    {
    }

    LMX2572_CSB_HIGH();
    Delay_Us(10);
}

static uint32_t LMX2572_BuildRegister(uint8_t address, uint16_t data)
{
    return (((uint32_t)address) << 16) | data;
}

static uint8_t LMX2572_GetChannelDividerCode(uint32_t divider)
{
    switch(divider)
    {
        case 2:
            return 0;
        case 4:
            return 1;
        case 8:
            return 3;
        case 16:
            return 5;
        case 32:
            return 7;
        case 64:
            return 9;
        case 128:
            return 12;
        case 256:
            return 14;
        default:
            return 0;
    }
}

static uint32_t LMX2572_GetOutputDivider(uint32_t frequency_mhz)
{
    if(frequency_mhz >= 3200)
    {
        return 1;
    }
    else if(frequency_mhz >= 1600)
    {
        return 2;
    }
    else if(frequency_mhz >= 800)
    {
        return 4;
    }
    else if(frequency_mhz >= 400)
    {
        return 8;
    }
    else if(frequency_mhz >= 200)
    {
        return 16;
    }
    else if(frequency_mhz >= 100)
    {
        return 32;
    }
    else if(frequency_mhz >= 50)
    {
        return 64;
    }

    return 128;
}

void LMX2572_SetFrequencyMHz(uint32_t frequency_mhz)
{
    uint32_t divider;
    uint32_t vco_mhz;
    uint32_t pll_n;
    uint32_t pll_num;
    uint32_t pll_den;
    uint16_t r45_data;
    uint16_t r75_data;

    if(frequency_mhz < LMX2572_MIN_FREQ_MHZ)
    {
        frequency_mhz = LMX2572_MIN_FREQ_MHZ;
    }
    else if(frequency_mhz > LMX2572_SWEEP_STOP_MHZ)
    {
        frequency_mhz = LMX2572_SWEEP_STOP_MHZ;
    }

    divider = LMX2572_GetOutputDivider(frequency_mhz);
    vco_mhz = frequency_mhz * divider;
    pll_n = vco_mhz / LMX2572_FPD_MHZ;
    pll_num = ((vco_mhz % LMX2572_FPD_MHZ) * LMX2572_PLL_DEN) / LMX2572_FPD_MHZ;
    pll_den = LMX2572_PLL_DEN;

    r45_data = (uint16_t)(0xC60F & ~LMX2572_R45_OUTA_MUX_MASK);
    if(divider == 1)
    {
        r45_data |= LMX2572_OUTA_MUX_VCO;
    }
    else
    {
        r45_data |= LMX2572_OUTA_MUX_CHDIV;
    }

    r75_data = (uint16_t)((0x0800 & ~LMX2572_R75_CHDIV_MASK) |
                          (((uint16_t)LMX2572_GetChannelDividerCode(divider)) << 6));

    LMX2572_WriteRegister(LMX2572_BuildRegister(75, r75_data));
    LMX2572_WriteRegister(LMX2572_BuildRegister(45, r45_data));
    LMX2572_WriteRegister(LMX2572_BuildRegister(38, (uint16_t)(pll_den >> 16)));
    LMX2572_WriteRegister(LMX2572_BuildRegister(39, (uint16_t)pll_den));
    LMX2572_WriteRegister(LMX2572_BuildRegister(42, (uint16_t)(pll_num >> 16)));
    LMX2572_WriteRegister(LMX2572_BuildRegister(43, (uint16_t)pll_num));
    if(vco_mhz >= 4900)
    {
        LMX2572_WriteRegister(LMX2572_R37_PFD_DLY_3);
    }
    else
    {
        LMX2572_WriteRegister(LMX2572_R37_PFD_DLY_2);
    }
    if(pll_num == 0)
    {
        LMX2572_WriteRegister(LMX2572_R44_MASH_INT);
    }
    else
    {
        LMX2572_WriteRegister(LMX2572_R44_MASH_FRAC);
    }
    LMX2572_WriteRegister(LMX2572_BuildRegister(36, (uint16_t)pll_n));
    LMX2572_WriteRegister(LMX2572_REG_R0);
    printf("LMX2572 Calc RF:%d MHz VCO:%d MHz CHDIV:%d N:%d NUM:%d DEN:%d\r\n",
           (int)frequency_mhz, (int)vco_mhz, (int)divider, (int)pll_n,
           (int)pll_num, (int)pll_den);
    Delay_Ms(20);
}

uint32_t LMX2572_GetNextSweepFrequencyMHz(uint32_t current_frequency_mhz)
{
    if(current_frequency_mhz >= LMX2572_SWEEP_STOP_MHZ)
    {
        return LMX2572_SWEEP_START_MHZ;
    }

    if((current_frequency_mhz + LMX2572_SWEEP_STEP_MHZ) > LMX2572_SWEEP_STOP_MHZ)
    {
        return LMX2572_SWEEP_STOP_MHZ;
    }

    return current_frequency_mhz + LMX2572_SWEEP_STEP_MHZ;
}

void LMX2572_WriteAllRegisters(void)
{
    uint32_t i;

    for(i = 0; i < (sizeof(lmx2572_reg_values) / sizeof(lmx2572_reg_values[0])); i++)
    {
        LMX2572_WriteRegister(lmx2572_reg_values[i]);
    }
}

uint8_t LMX2572_GetLockDetect(void)
{
    return GPIO_ReadInputDataBit(LMX2572_MUXOUT_PORT, LMX2572_MUXOUT_PIN);
}

void LMX2572_Init(void)
{
    LMX2572_GPIOConfig();
    LMX2572_SPIConfig();
    Delay_Ms(10);
    LMX2572_WriteAllRegisters();
    /* Fixed 45 MHz uses the exact TICS Pro register export above. */
}
