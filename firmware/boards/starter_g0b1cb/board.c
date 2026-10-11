#include "board.h"
#include "stm32g0xx_hal.h"
#include "tusb.h"
#include <string.h>

static FDCAN_HandleTypeDef can;
static ADC_HandleTypeDef adc;
void board_fault(void) { __disable_irq(); for (;;) { } }
static void check(HAL_StatusTypeDef result) { if (result != HAL_OK) board_fault(); }
void board_led(bool on) { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, on ? GPIO_PIN_SET : GPIO_PIN_RESET); }
static bool closed(GPIO_TypeDef *port, uint16_t pin) { return HAL_GPIO_ReadPin(port,pin)==GPIO_PIN_RESET; }
profile_sample_t board_read_inputs(void) {
    profile_sample_t s={.button=closed(GPIOA,GPIO_PIN_0), .toggle=closed(GPIOB,GPIO_PIN_1),
        .encoder_push=closed(GPIOA,GPIO_PIN_15), .selector_low=closed(GPIOB,GPIO_PIN_2),
        .selector_high=closed(GPIOB,GPIO_PIN_3)};
    s.hat_contacts=(closed(GPIOA,GPIO_PIN_2)?1:0) | (closed(GPIOA,GPIO_PIN_3)?2:0) |
        (closed(GPIOA,GPIO_PIN_4)?4:0) | (closed(GPIOA,GPIO_PIN_6)?8:0);
    s.encoder_ab=(closed(GPIOA,GPIO_PIN_7)?0:1) | (closed(GPIOA,GPIO_PIN_8)?0:2);
    if(__HAL_ADC_GET_FLAG(&adc,ADC_FLAG_EOC)) {
        s.axis_raw=(uint16_t)HAL_ADC_GetValue(&adc); s.axis_fresh=true;
    }
    return s;
}
bool board_usb_present(void) { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET; }
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
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_SYSCFG_CLK_ENABLE();
    /* Prevent USB-C UCPD dead-battery pull-downs conflicting with external Rd. */
    HAL_SYSCFG_DisableRemap(SYSCFG_REMAP_PA11 | SYSCFG_REMAP_PA12);
    HAL_SYSCFG_StrobeDBattpinsConfig(SYSCFG_UCPD1_STROBE | SYSCFG_UCPD2_STROBE);
    GPIO_InitTypeDef gpio = {0};
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET); /* transceiver standby */
    gpio.Pin = GPIO_PIN_5; gpio.Mode = GPIO_MODE_OUTPUT_PP; gpio.Pull = GPIO_NOPULL; gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio); HAL_GPIO_Init(GPIOB, &gpio); board_led(false);
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_15;
    gpio.Mode = GPIO_MODE_INPUT; gpio.Pull = GPIO_PULLUP; HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3; HAL_GPIO_Init(GPIOB,&gpio);
    gpio.Pin = GPIO_PIN_1; gpio.Mode = GPIO_MODE_ANALOG; gpio.Pull = GPIO_NOPULL; HAL_GPIO_Init(GPIOA,&gpio);
    gpio.Pin = GPIO_PIN_0; gpio.Mode = GPIO_MODE_INPUT; gpio.Pull = GPIO_NOPULL; HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9; gpio.Mode = GPIO_MODE_AF_PP; gpio.Alternate = GPIO_AF3_FDCAN1;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH; HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12; gpio.Mode = GPIO_MODE_INPUT; gpio.Pull = GPIO_NOPULL; HAL_GPIO_Init(GPIOA, &gpio);
    __HAL_RCC_USB_CLK_ENABLE(); HAL_PWREx_EnableVddUSB();
    NVIC_SetPriority(USB_UCPD1_2_IRQn, 1); NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
    __HAL_RCC_ADC_CLK_ENABLE();
    adc.Instance=ADC1;
    adc.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4; adc.Init.Resolution=ADC_RESOLUTION_12B;
    adc.Init.DataAlign=ADC_DATAALIGN_RIGHT; adc.Init.ScanConvMode=ADC_SCAN_SEQ_FIXED;
    adc.Init.EOCSelection=ADC_EOC_SINGLE_CONV; adc.Init.LowPowerAutoWait=ENABLE;
    adc.Init.LowPowerAutoPowerOff=DISABLE; adc.Init.ContinuousConvMode=ENABLE;
    adc.Init.NbrOfConversion=1; adc.Init.DiscontinuousConvMode=DISABLE;
    adc.Init.ExternalTrigConv=ADC_SOFTWARE_START; adc.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc.Init.DMAContinuousRequests=DISABLE; adc.Init.Overrun=ADC_OVR_DATA_OVERWRITTEN;
    adc.Init.SamplingTimeCommon1=ADC_SAMPLETIME_160CYCLES_5;
    adc.Init.SamplingTimeCommon2=ADC_SAMPLETIME_160CYCLES_5;
    adc.Init.OversamplingMode=DISABLE; adc.Init.TriggerFrequencyMode=ADC_TRIGGER_FREQ_LOW;
    check(HAL_ADC_Init(&adc));
    ADC_ChannelConfTypeDef channel={.Channel=ADC_CHANNEL_1,.Rank=ADC_RANK_CHANNEL_NUMBER,
        .SamplingTime=ADC_SAMPLINGTIME_COMMON_1};
    check(HAL_ADC_ConfigChannel(&adc,&channel)); check(HAL_ADCEx_Calibration_Start(&adc));
    check(HAL_ADC_Start(&adc));
    __HAL_RCC_FDCAN_CLK_ENABLE();
    can.Instance = FDCAN1;
    can.Init.ClockDivider = FDCAN_CLOCK_DIV1; can.Init.FrameFormat = FDCAN_FRAME_FD_BRS;
    can.Init.Mode = FDCAN_MODE_INTERNAL_LOOPBACK; can.Init.AutoRetransmission = DISABLE;
    can.Init.TransmitPause = DISABLE; can.Init.ProtocolException = DISABLE;
    /* 64 MHz: 500 kbit/s nominal, 2 Mbit/s data. Internal loopback only. */
    can.Init.NominalPrescaler = 8; can.Init.NominalSyncJumpWidth = 2;
    can.Init.NominalTimeSeg1 = 13; can.Init.NominalTimeSeg2 = 2;
    can.Init.DataPrescaler = 2; can.Init.DataSyncJumpWidth = 2;
    can.Init.DataTimeSeg1 = 13; can.Init.DataTimeSeg2 = 2;
    can.Init.StdFiltersNbr = 0; can.Init.ExtFiltersNbr = 0; can.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    check(HAL_FDCAN_Init(&can));
    check(HAL_FDCAN_ConfigGlobalFilter(&can, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO0,
                                      FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE));
    check(HAL_FDCAN_Start(&can));
}
bool board_can_loopback(void) {
    uint8_t rx[64]; FDCAN_RxHeaderTypeDef received;
    /* Drain old results so stale data cannot satisfy a new test. */
    while (HAL_FDCAN_GetRxFifoFillLevel(&can, FDCAN_RX_FIFO0))
        if (HAL_FDCAN_GetRxMessage(&can, FDCAN_RX_FIFO0, &received, rx) != HAL_OK) return false;
    const uint8_t payload[8] = {'O','S','G','H','E','L','L','O'};
    FDCAN_TxHeaderTypeDef tx = {.Identifier = 0x123, .IdType = FDCAN_STANDARD_ID,
        .TxFrameType = FDCAN_DATA_FRAME, .DataLength = FDCAN_DLC_BYTES_8,
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE, .BitRateSwitch = FDCAN_BRS_ON,
        .FDFormat = FDCAN_FD_CAN, .TxEventFifoControl = FDCAN_NO_TX_EVENTS};
    if (HAL_FDCAN_AddMessageToTxFifoQ(&can, &tx, payload) != HAL_OK) return false;
    uint32_t start = HAL_GetTick();
    while (!HAL_FDCAN_GetRxFifoFillLevel(&can, FDCAN_RX_FIFO0))
        if ((uint32_t)(HAL_GetTick() - start) >= 20) return false;
    return HAL_FDCAN_GetRxMessage(&can, FDCAN_RX_FIFO0, &received, rx) == HAL_OK &&
        received.Identifier == tx.Identifier && received.DataLength == FDCAN_DLC_BYTES_8 &&
        received.FDFormat == FDCAN_FD_CAN && memcmp(rx, payload, sizeof payload) == 0;
}
void SysTick_Handler(void) { HAL_IncTick(); }
void USB_UCPD1_2_IRQHandler(void) { tusb_int_handler(0, true); }
uint32_t tusb_time_millis_api(void) { return HAL_GetTick(); }
void _init(void) { }
