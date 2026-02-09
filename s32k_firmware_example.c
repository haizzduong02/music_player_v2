#include "sdk_project_config.h"

#include <string.h>

#include <stdio.h>

#include <stdbool.h>



/* --- CẤU HÌNH CHÂN NÚT BẤM --- */

#define BTN_SW2_PORT PTC /* SW2 = Next */

#define BTN_SW2_PIN 12

#define BTN_SW3_PORT PTC /* SW3 = Toggle Play/Pause */

#define BTN_SW3_PIN 13



/* --- CẤU HÌNH CHÂN LED (ĐỂ TẮT NÓ ĐI) --- */

#define LED_PORT PTD

#define LED_RED_PIN 15

#define LED_GREEN_PIN 16



/* ADC */

#define ADC_INSTANCE INST_ADC_CONFIG_1

#define ADC_CHN 12



/* UART */

#define UART_INSTANCE INST_LPUART_1

#define TIMEOUT_MS 500



/* Logic nút bấm (Pull-up) */

#define BTN_PRESSED 0

#define BTN_RELEASED 1



volatile int exit_code = 0;



void my_delay(volatile int cycles) {

while(cycles > 0) cycles--;

}



int main(void)

{

/* 1. KHỞI TẠO */

CLOCK_SYS_Init(g_clockManConfigsArr, CLOCK_MANAGER_CONFIG_CNT,

g_clockManCallbacksArr, CLOCK_MANAGER_CALLBACK_CNT);

CLOCK_SYS_UpdateConfiguration(0U, CLOCK_MANAGER_POLICY_AGREEMENT);

PINS_DRV_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);



/* --- [MỚI] TẮT HẾT ĐÈN D11 NGAY LẬP TỨC --- */

/* Ghi mức 1 vào chân LED để tắt (Vì LED tích cực mức thấp) */

PINS_DRV_WritePin(LED_PORT, LED_RED_PIN, 1); // Tắt đỏ

PINS_DRV_WritePin(LED_PORT, LED_GREEN_PIN, 1); // Tắt xanh



/* Chống nhiễu khi vừa bật mạch */

my_delay(1000000);



LPUART_DRV_Init(UART_INSTANCE, &lpUartState1, &lpuart_1_InitConfig0);

ADC_DRV_ConfigConverter(ADC_INSTANCE, &adc_config_1_ConvConfig0);



/* Gửi lời chào */

char *msg = "READY: LED OFF. SW3 First Press = PAUSE\r\n";

LPUART_DRV_SendDataBlocking(UART_INSTANCE, (uint8_t *)msg, strlen(msg), TIMEOUT_MS);



uint32_t sw2_prev = (PINS_DRV_ReadPins(BTN_SW2_PORT) >> BTN_SW2_PIN) & 1;

uint32_t sw3_prev = (PINS_DRV_ReadPins(BTN_SW3_PORT) >> BTN_SW3_PIN) & 1;



/* Mặc định board hiểu nhạc đang chạy -> Bấm cái đầu là Pause */

bool is_playing = true;



adc_chan_config_t chnConfig;

chnConfig.interruptEnable = false;

chnConfig.channel = ADC_CHN;



/* 2. VÒNG LẶP CHÍNH */

while (1)

{

/* --- ĐẢM BẢO LED LUÔN TẮT (Cho chắc ăn) --- */

/* Nếu muốn tiết kiệm CPU thì bỏ 2 dòng này đi cũng được, vì ở trên đã tắt rồi */

// PINS_DRV_WritePin(LED_PORT, LED_RED_PIN, 1);

// PINS_DRV_WritePin(LED_PORT, LED_GREEN_PIN, 1);



uint32_t sw2_curr = (PINS_DRV_ReadPins(BTN_SW2_PORT) >> BTN_SW2_PIN) & 1;

uint32_t sw3_curr = (PINS_DRV_ReadPins(BTN_SW3_PORT) >> BTN_SW3_PIN) & 1;



/* --- NÚT NEXT (SW2) --- */

if (sw2_prev == BTN_RELEASED && sw2_curr == BTN_PRESSED)

{

char *cmd = "cmd:next\r\n";

LPUART_DRV_SendDataBlocking(UART_INSTANCE, (uint8_t *)cmd, strlen(cmd), TIMEOUT_MS);

}



/* --- NÚT TOGGLE (SW3) --- */

if (sw3_prev == BTN_RELEASED && sw3_curr == BTN_PRESSED)

{

if (is_playing == true)

{

char *cmd = "cmd:pause\r\n";

LPUART_DRV_SendDataBlocking(UART_INSTANCE, (uint8_t *)cmd, strlen(cmd), TIMEOUT_MS);

is_playing = false;

}

else

{

char *cmd = "cmd:play\r\n";

LPUART_DRV_SendDataBlocking(UART_INSTANCE, (uint8_t *)cmd, strlen(cmd), TIMEOUT_MS);

is_playing = true;

}

}



sw2_prev = sw2_curr;

sw3_prev = sw3_curr;



/* --- ADC --- */

ADC_DRV_ConfigChan(ADC_INSTANCE, 0, &chnConfig);

ADC_DRV_WaitConvDone(ADC_INSTANCE);

uint16_t adcValue;

ADC_DRV_GetChanResult(ADC_INSTANCE, 0, &adcValue);



char adc_buffer[30];

sprintf(adc_buffer, "VR: %d\r\n", adcValue);

LPUART_DRV_SendDataBlocking(UART_INSTANCE, (uint8_t *)adc_buffer, strlen(adc_buffer), TIMEOUT_MS);



my_delay(1000000);

}



return exit_code;

}