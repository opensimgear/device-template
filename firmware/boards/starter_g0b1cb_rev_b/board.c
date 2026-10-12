#include "board.h"
#include "stm32g0xx_hal.h"
#include "tusb.h"
#include "opensimgear/tca9555.h"
#include <string.h>

static ADC_HandleTypeDef adc;
static DMA_HandleTypeDef adc_dma;
static I2C_HandleTypeDef i2c;
static osg_tca9555_t expanders;
static uint16_t dma_samples[16];
static volatile uint16_t axis_pending[8];
static volatile bool axis_fresh;
static uint8_t adc_count, adc_indexes[8];
static volatile bool i2c_done, i2c_error;
static volatile bool i2c_recovery_needed;
static uint32_t i2c_bank_started;
typedef struct { GPIO_TypeDef *port; uint16_t pin; } board_pin_t;
static const board_pin_t digital_pins[24]={
    {GPIOA,GPIO_PIN_8},{GPIOA,GPIO_PIN_9},{GPIOA,GPIO_PIN_10},{GPIOA,GPIO_PIN_15},
    {GPIOB,GPIO_PIN_2},{GPIOB,GPIO_PIN_3},{GPIOB,GPIO_PIN_4},{GPIOB,GPIO_PIN_10},
    {GPIOB,GPIO_PIN_11},{GPIOB,GPIO_PIN_12},{GPIOB,GPIO_PIN_13},{GPIOB,GPIO_PIN_14},
    {GPIOB,GPIO_PIN_15},{GPIOC,GPIO_PIN_6},{GPIOC,GPIO_PIN_7},{GPIOC,GPIO_PIN_13},
    {GPIOC,GPIO_PIN_14},{GPIOC,GPIO_PIN_15},{GPIOD,GPIO_PIN_0},{GPIOD,GPIO_PIN_1},
    {GPIOD,GPIO_PIN_2},{GPIOD,GPIO_PIN_3},{GPIOF,GPIO_PIN_0},{GPIOF,GPIO_PIN_1}
};
static bool i2c_wait(HAL_StatusTypeDef started) {
    if(started!=HAL_OK) { i2c_recovery_needed=started==HAL_BUSY; return false; }
    uint32_t began=HAL_GetTick();
    while(!i2c_done && !i2c_error) {
        /* Two tick boundaries guarantee at least 1 ms wall time even when
         * a transfer starts just before the next millisecond tick. */
        if((uint32_t)(HAL_GetTick()-began)>=2) {
            /* Stop IRQ/DMA access to callback stack buffers before returning. */
            HAL_NVIC_DisableIRQ(I2C1_IRQn); __HAL_I2C_DISABLE(&i2c);
            expanders.available=0; i2c_recovery_needed=true; return false;
        }
    }
    return i2c_done && !i2c_error;
}
static bool i2c_allowed(void) {
    if(i2c_recovery_needed || (uint32_t)(HAL_GetTick()-i2c_bank_started)>=2) return false;
    i2c_done=false; i2c_error=false; return true;
}
static bool i2c_write(void *context,uint8_t address,uint8_t reg,const uint8_t *data,uint8_t count) {
    (void)context; if(!i2c_allowed()) return false;
    return i2c_wait(HAL_I2C_Mem_Write_IT(&i2c,(uint16_t)(address<<1),reg,I2C_MEMADD_SIZE_8BIT,(uint8_t *)data,count));
}
static bool i2c_read(void *context,uint8_t address,uint8_t reg,uint8_t *data,uint8_t count) {
    (void)context; if(!i2c_allowed()) return false;
    return i2c_wait(HAL_I2C_Mem_Read_IT(&i2c,(uint16_t)(address<<1),reg,I2C_MEMADD_SIZE_8BIT,data,count));
}
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *handle) { if(handle==&i2c) i2c_done=true; }
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *handle) { if(handle==&i2c) i2c_done=true; }
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *handle) {
    if(handle==&i2c) { i2c_error=true; i2c_recovery_needed=__HAL_I2C_GET_FLAG(&i2c,I2C_FLAG_BUSY)==SET; }
}
static void i2c_short_delay(void) { for(volatile unsigned i=0;i<96;i++) __NOP(); }
static void i2c_recover(void) {
    /* Single-master local bus clear. Fixed nine clocks; a held line never
     * introduces a wait loop. Recheck on later ticks rather than freezing USB. */
    HAL_NVIC_DisableIRQ(I2C1_IRQn); __HAL_I2C_DISABLE(&i2c);
    GPIO_InitTypeDef gpio={.Pin=GPIO_PIN_6|GPIO_PIN_7,.Mode=GPIO_MODE_OUTPUT_OD,
        .Pull=GPIO_NOPULL,.Speed=GPIO_SPEED_FREQ_HIGH};
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6|GPIO_PIN_7,GPIO_PIN_SET); HAL_GPIO_Init(GPIOB,&gpio);
    for(unsigned pulse=0;pulse<9;pulse++) {
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET); i2c_short_delay();
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET); i2c_short_delay();
    }
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET); i2c_short_delay();
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET); i2c_short_delay();
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET); i2c_short_delay();
    gpio.Mode=GPIO_MODE_AF_OD; gpio.Alternate=GPIO_AF6_I2C1; HAL_GPIO_Init(GPIOB,&gpio);
    __HAL_RCC_I2C1_FORCE_RESET(); __HAL_RCC_I2C1_RELEASE_RESET();
    i2c.State=HAL_I2C_STATE_RESET; i2c.Lock=HAL_UNLOCKED;
    bool restored=HAL_I2C_Init(&i2c)==HAL_OK && HAL_I2CEx_ConfigAnalogFilter(&i2c,I2C_ANALOGFILTER_ENABLE)==HAL_OK &&
        HAL_I2CEx_ConfigDigitalFilter(&i2c,0)==HAL_OK;
    HAL_NVIC_ClearPendingIRQ(I2C1_IRQn); HAL_NVIC_EnableIRQ(I2C1_IRQn);
    i2c_recovery_needed=!restored;
    expanders.configured=0; expanders.available=0;
}
void board_fault(void) { HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_SET); __disable_irq(); for (;;) { } }
static void check(HAL_StatusTypeDef result) { if (result != HAL_OK) board_fault(); }
void board_led(bool on) { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, on ? GPIO_PIN_SET : GPIO_PIN_RESET); }
osg_profile_sample_t board_read_inputs(void) {
    osg_profile_sample_t sample={0};
    uint32_t used=device_report_layout()->digital_mask;
    for(unsigned i=0;i<24;i++) if((used&(1UL<<i)) && HAL_GPIO_ReadPin(digital_pins[i].port,digital_pins[i].pin)==GPIO_PIN_RESET)
        sample.digital_closed|=1UL<<i;
    uint32_t irq_state=__get_PRIMASK(); __disable_irq();
    if(axis_fresh) {
        for(unsigned i=0;i<8;i++) sample.axes[i]=axis_pending[i];
        sample.axes_fresh=device_report_layout()->adc_mask; axis_fresh=false;
    }
    __set_PRIMASK(irq_state);
    if(device_profile()->expander_mask) {
        if(i2c_recovery_needed) i2c_recover();
        i2c_bank_started=HAL_GetTick();
        osg_tca9555_poll(&expanders,device_profile()->expander_mask,i2c_bank_started);
        sample.expansion_available=osg_tca9555_inputs(&expanders,HAL_GetTick(),sample.expansion_closed);
    }
    return sample;
}
static void capture_axes(unsigned offset) {
    for(unsigned i=0;i<adc_count;i++) axis_pending[adc_indexes[i]]=dma_samples[offset+i];
    axis_fresh=true;
}
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *handle) { if(handle==&adc) capture_axes(0); }
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *handle) { if(handle==&adc) capture_axes(adc_count); }
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *handle) { if(handle==&adc) axis_fresh=false; }
bool board_usb_present(void) { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET; }
void board_module_identity(uint8_t out[16]) {
    memcpy(out,"OSGD",4); memcpy(out+4,(const void *)UID_BASE,12);
}
void board_uid_hex(char out[25]) {
    static const char hex[] = "0123456789ABCDEF";
    const volatile uint8_t *uid = (const volatile uint8_t *)UID_BASE;
    for (unsigned i = 0; i < 12; ++i) { out[i * 2] = hex[uid[i] >> 4]; out[i * 2 + 1] = hex[uid[i] & 15]; }
    out[24] = '\0';
}
void board_init(void) {
    HAL_Init();
    __HAL_RCC_PWR_CLK_ENABLE();
    check(HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1));
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48;
    osc.HSIState = RCC_HSI_ON; osc.HSIDiv = RCC_HSI_DIV1;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT; osc.HSI48State = RCC_HSI48_ON;
    osc.PLL.PLLState = RCC_PLL_ON; osc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM = RCC_PLLM_DIV1; osc.PLL.PLLN = 8;
    osc.PLL.PLLP = RCC_PLLP_DIV2; osc.PLL.PLLQ = RCC_PLLQ_DIV2; osc.PLL.PLLR = RCC_PLLR_DIV2;
    check(HAL_RCC_OscConfig(&osc));
    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; clk.AHBCLKDivider = RCC_SYSCLK_DIV1; clk.APB1CLKDivider = RCC_HCLK_DIV1;
    check(HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2));
    RCC_PeriphCLKInitTypeDef peripheral = {0};
    peripheral.PeriphClockSelection = RCC_PERIPHCLK_USB | RCC_PERIPHCLK_FDCAN;
    peripheral.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    peripheral.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;
    check(HAL_RCCEx_PeriphCLKConfig(&peripheral));
    __HAL_RCC_CRS_CLK_ENABLE();
    RCC_CRSInitTypeDef crs = {0};
    crs.Prescaler = RCC_CRS_SYNC_DIV1; crs.Source = RCC_CRS_SYNC_SOURCE_USB;
    crs.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
    crs.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000, 1000);
    crs.ErrorLimitValue = 34; crs.HSI48CalibrationValue = 32;
    HAL_RCCEx_CRSConfig(&crs);
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE(); __HAL_RCC_GPIOF_CLK_ENABLE(); __HAL_RCC_SYSCFG_CLK_ENABLE();
    /* Prevent USB-C UCPD dead-battery pull-downs conflicting with external Rd. */
    HAL_SYSCFG_DisableRemap(SYSCFG_REMAP_PA11 | SYSCFG_REMAP_PA12);
    HAL_SYSCFG_StrobeDBattpinsConfig(SYSCFG_UCPD1_STROBE | SYSCFG_UCPD2_STROBE);
    GPIO_InitTypeDef gpio = {0};
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET); /* transceiver standby */
    gpio.Pin = GPIO_PIN_5; gpio.Mode = GPIO_MODE_OUTPUT_PP; gpio.Pull = GPIO_NOPULL; gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio); HAL_GPIO_Init(GPIOB, &gpio); board_led(false);
    for(unsigned i=0;i<24;i++) {
        gpio.Pin=digital_pins[i].pin;
        bool used=(device_report_layout()->digital_mask&(1UL<<i))!=0;
        gpio.Mode=used?GPIO_MODE_INPUT:GPIO_MODE_ANALOG; gpio.Pull=used?GPIO_PULLUP:GPIO_NOPULL;
        HAL_GPIO_Init(digital_pins[i].port,&gpio);
    }
    gpio.Pin=GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_7;
    gpio.Mode=GPIO_MODE_ANALOG; gpio.Pull=GPIO_NOPULL; HAL_GPIO_Init(GPIOA,&gpio);
    gpio.Pin=GPIO_PIN_1; HAL_GPIO_Init(GPIOB,&gpio);
    gpio.Pin = GPIO_PIN_0; gpio.Mode = GPIO_MODE_INPUT; gpio.Pull = GPIO_NOPULL; HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9; gpio.Mode = GPIO_MODE_AF_PP; gpio.Alternate = GPIO_AF3_FDCAN1;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH; HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12; gpio.Mode = GPIO_MODE_INPUT; gpio.Pull = GPIO_NOPULL; HAL_GPIO_Init(GPIOA, &gpio);
    __HAL_RCC_USB_CLK_ENABLE(); HAL_PWREx_EnableVddUSB();
    NVIC_SetPriority(USB_UCPD1_2_IRQn, 1); NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
    if(device_report_layout()->adc_mask) {
    __HAL_RCC_ADC_CLK_ENABLE();
    adc.Instance=ADC1;
    adc.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4; adc.Init.Resolution=ADC_RESOLUTION_12B;
    adc.Init.DataAlign=ADC_DATAALIGN_RIGHT; adc.Init.ScanConvMode=ADC_SCAN_SEQ_FIXED;
    adc.Init.EOCSelection=ADC_EOC_SEQ_CONV; adc.Init.LowPowerAutoWait=DISABLE;
    adc.Init.LowPowerAutoPowerOff=DISABLE; adc.Init.ContinuousConvMode=ENABLE;
    adc.Init.NbrOfConversion=8; /* ignored by the fixed ascending sequencer */ adc.Init.DiscontinuousConvMode=DISABLE;
    adc.Init.ExternalTrigConv=ADC_SOFTWARE_START; adc.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc.Init.DMAContinuousRequests=ENABLE; adc.Init.Overrun=ADC_OVR_DATA_OVERWRITTEN;
    adc.Init.SamplingTimeCommon1=ADC_SAMPLETIME_160CYCLES_5;
    adc.Init.SamplingTimeCommon2=ADC_SAMPLETIME_160CYCLES_5;
    adc.Init.OversamplingMode=DISABLE; adc.Init.TriggerFrequencyMode=ADC_TRIGGER_FREQ_LOW;
    check(HAL_ADC_Init(&adc));
    __HAL_RCC_DMA1_CLK_ENABLE();
    adc_dma.Instance=DMA1_Channel1; adc_dma.Init.Request=DMA_REQUEST_ADC1;
    adc_dma.Init.Direction=DMA_PERIPH_TO_MEMORY; adc_dma.Init.PeriphInc=DMA_PINC_DISABLE;
    adc_dma.Init.MemInc=DMA_MINC_ENABLE; adc_dma.Init.PeriphDataAlignment=DMA_PDATAALIGN_HALFWORD;
    adc_dma.Init.MemDataAlignment=DMA_MDATAALIGN_HALFWORD; adc_dma.Init.Mode=DMA_CIRCULAR;
    adc_dma.Init.Priority=DMA_PRIORITY_LOW; check(HAL_DMA_Init(&adc_dma));
    __HAL_LINKDMA(&adc,DMA_Handle,adc_dma);
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn,2,0); HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    const uint32_t channels[8]={ADC_CHANNEL_0,ADC_CHANNEL_1,ADC_CHANNEL_2,ADC_CHANNEL_3,
        ADC_CHANNEL_4,ADC_CHANNEL_6,ADC_CHANNEL_7,ADC_CHANNEL_9};
    for(unsigned i=0;i<8;i++) if(device_report_layout()->adc_mask&(1U<<i)) {
        adc_indexes[adc_count++]=(uint8_t)i;
        ADC_ChannelConfTypeDef channel={.Channel=channels[i],.Rank=ADC_RANK_CHANNEL_NUMBER,
            .SamplingTime=ADC_SAMPLINGTIME_COMMON_1};
        check(HAL_ADC_ConfigChannel(&adc,&channel));
    }
    check(HAL_ADCEx_Calibration_Start(&adc));
    check(HAL_ADC_Start_DMA(&adc,(uint32_t *)dma_samples,2*adc_count));
    }
    if(device_profile()->expander_mask) {
        gpio.Pin=GPIO_PIN_6|GPIO_PIN_7; gpio.Mode=GPIO_MODE_AF_OD; gpio.Pull=GPIO_NOPULL;
        gpio.Speed=GPIO_SPEED_FREQ_HIGH; gpio.Alternate=GPIO_AF6_I2C1; HAL_GPIO_Init(GPIOB,&gpio);
        __HAL_RCC_I2C1_CLK_ENABLE();
        i2c.Instance=I2C1;
        /* Initial 400 kHz timing target at 64 MHz PCLK; verify rise/fall times
         * on the assembled, capacitance-qualified local expansion bus. */
        i2c.Init.Timing=0x30520B13; i2c.Init.OwnAddress1=0; i2c.Init.AddressingMode=I2C_ADDRESSINGMODE_7BIT;
        i2c.Init.DualAddressMode=I2C_DUALADDRESS_DISABLE; i2c.Init.OwnAddress2=0;
        i2c.Init.OwnAddress2Masks=I2C_OA2_NOMASK; i2c.Init.GeneralCallMode=I2C_GENERALCALL_DISABLE;
        i2c.Init.NoStretchMode=I2C_NOSTRETCH_DISABLE; check(HAL_I2C_Init(&i2c));
        check(HAL_I2CEx_ConfigAnalogFilter(&i2c,I2C_ANALOGFILTER_ENABLE));
        check(HAL_I2CEx_ConfigDigitalFilter(&i2c,0));
        HAL_NVIC_SetPriority(I2C1_IRQn,2,0); HAL_NVIC_EnableIRQ(I2C1_IRQn);
        osg_tca9555_init(&expanders,(osg_i2c_t){.write=i2c_write,.read=i2c_read});
    }
    board_can_init();
}
void I2C1_IRQHandler(void) { HAL_I2C_EV_IRQHandler(&i2c); HAL_I2C_ER_IRQHandler(&i2c); }
void DMA1_Channel1_IRQHandler(void) { HAL_DMA_IRQHandler(&adc_dma); }
void SysTick_Handler(void) { HAL_IncTick(); }
void USB_UCPD1_2_IRQHandler(void) { tusb_int_handler(0, true); }
uint32_t tusb_time_millis_api(void) { return HAL_GetTick(); }
void _init(void) { }
