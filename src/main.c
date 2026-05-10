/*
 * =====================================================
 *  STM32L432KC Digital Stopwatch
 *  Display  : ST7735S 0.96" TFT LCD (SPI1)
 *  Button 1 : PB3 — Start / Stop
 *  Button 2 : PB4 — Reset
 *  LED Green: PA9 — ON when running
 *  LED Red  : PB1 — ON when stopped
 *
 *  Wiring:
 *   LCD: VCC->3.3V  GND->GND  SCK->PA5  SDA->PA7
 *        CS->PA4    DC->PA8   RST->PB0
 *   BTN1: PB3 -> button -> GND  (internal pull-up)
 *   BTN2: PB4 -> button -> GND  (internal pull-up)
 *   LED1: PA9 -> 220ohm -> Green LED -> GND
 *   LED2: PB1 -> 220ohm -> Red LED   -> GND
 * =====================================================
 */

#include "stm32l4xx_hal.h"
#include "st7735.h"
#include "stopwatch.h"
#include <stdio.h>
#include <string.h>

/* ── Handles ─────────────────────────────────────── */
SPI_HandleTypeDef  hspi1;
TIM_HandleTypeDef  htim6;

/* ── Pin defines ─────────────────────────────────── */
#define BTN_START_PIN   GPIO_PIN_3   /* PB3 — Start/Stop */
#define BTN_START_PORT  GPIOB
#define BTN_RESET_PIN   GPIO_PIN_4   /* PB4 — Reset      */
#define BTN_RESET_PORT  GPIOB
#define LED_RUN_PIN     GPIO_PIN_9   /* PA9 — Running    */
#define LED_RUN_PORT    GPIOA
#define LED_STOP_PIN    GPIO_PIN_1   /* PB1 — Stopped    */
#define LED_STOP_PORT   GPIOB

/* ── Debounce timing (ms) ────────────────────────── */
#define DEBOUNCE_MS     50

/* ── Forward declarations ────────────────────────── */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void SPI1_Init(void);
static void TIM6_Init(void);
static void UI_DrawBackground(void);
static void UI_UpdateTime(StopwatchTime t, StopwatchState s);
static void Handle_Buttons(void);
static void Update_LEDs(StopwatchState s);

/* ── Button debounce state ───────────────────────── */
static uint32_t btn1_last_time = 0;
static uint32_t btn2_last_time = 0;
static uint8_t  btn1_last_state = 1;   /* 1 = not pressed (pull-up) */
static uint8_t  btn2_last_state = 1;

/* ── Previous display values (to avoid redraw) ───── */
static StopwatchTime  prev_time  = {999, 99, 99};
static StopwatchState prev_state = (StopwatchState)255;

/* ═════════════════════════════════════════════════
 *  MAIN
 * ═════════════════════════════════════════════════ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    GPIO_Init();
    SPI1_Init();
    TIM6_Init();

    /* Start TIM6 — fires interrupt every 1ms */
    HAL_TIM_Base_Start_IT(&htim6);

    ST7735_Init(&hspi1);
    Stopwatch_Init();
    UI_DrawBackground();

    /* Initial LED state — stopped = red LED on */
    HAL_GPIO_WritePin(LED_STOP_PORT, LED_STOP_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_RUN_PORT,  LED_RUN_PIN,  GPIO_PIN_RESET);

    while (1)
    {
        Handle_Buttons();

        StopwatchTime  t = Stopwatch_GetTime();
        StopwatchState s = Stopwatch_GetState();

        /* Only redraw if something changed */
        if (t.milliseconds != prev_time.milliseconds ||
            t.seconds      != prev_time.seconds      ||
            t.minutes      != prev_time.minutes      ||
            s              != prev_state)
        {
            UI_UpdateTime(t, s);
            Update_LEDs(s);
            prev_time  = t;
            prev_state = s;
        }

        HAL_Delay(10);   /* check every 10ms — fast enough for smooth display */
    }
}

/* ═════════════════════════════════════════════════
 *  BUTTON HANDLING WITH DEBOUNCE
 * ═════════════════════════════════════════════════ */
static void Handle_Buttons(void)
{
    uint32_t now = HAL_GetTick();

    /* ── Button 1: Start / Stop ── */
    uint8_t btn1 = HAL_GPIO_ReadPin(BTN_START_PORT, BTN_START_PIN);

    if (btn1 == 0 && btn1_last_state == 1)
    {
        /* Falling edge detected — button just pressed */
        if ((now - btn1_last_time) > DEBOUNCE_MS)
        {
            btn1_last_time = now;
            /* Toggle running state */
            if (Stopwatch_GetState() == SW_RUNNING)
                Stopwatch_Stop();
            else
                Stopwatch_Start();
        }
    }
    btn1_last_state = btn1;

    /* ── Button 2: Reset ── */
    uint8_t btn2 = HAL_GPIO_ReadPin(BTN_RESET_PORT, BTN_RESET_PIN);

    if (btn2 == 0 && btn2_last_state == 1)
    {
        if ((now - btn2_last_time) > DEBOUNCE_MS)
        {
            btn2_last_time = now;
            Stopwatch_Reset();
        }
    }
    btn2_last_state = btn2;
}

/* ═════════════════════════════════════════════════
 *  LED CONTROL
 * ═════════════════════════════════════════════════ */
static void UI_DrawBackground(void)
{
    ST7735_FillScreen(BLACK);

    /* Title bar */
    ST7735_FillRect(0, 0, LCD_WIDTH, 18, DARKGRAY);
    ST7735_DrawString(14, 5, "STOPWATCH", WHITE, DARKGRAY, 1);
    ST7735_DrawHLine(0, 18, LCD_WIDTH, CYAN);

    /* Labels */
    ST7735_DrawString(4, 100, "MIN  SEC   MS", GRAY, BLACK, 1);
    ST7735_DrawHLine(0, 110, LCD_WIDTH, DARKGRAY);

    /* Button hints */
    ST7735_DrawString(2, 116, "PB3:Start/Stop", GRAY, BLACK, 1);
    ST7735_DrawString(2, 126, "PB4:Reset",      GRAY, BLACK, 1);

    /* Outer border */
    ST7735_DrawRect(0, 0, LCD_WIDTH, LCD_HEIGHT, CYAN, 1);
}

static void UI_UpdateTime(StopwatchTime t, StopwatchState s)
{
    char buf[16];
    uint16_t col = (s == SW_RUNNING) ? GREEN : RED;

    /* ── MM:SS — scale 2, fits 80px easily ── */
    snprintf(buf, sizeof(buf), "%02lu:%02lu", t.minutes, t.seconds);
    ST7735_FillRect(0, 24, LCD_WIDTH, 20, BLACK);
    ST7735_DrawString(8, 25, buf, col, BLACK, 2);

    /* ── Milliseconds — scale 2, same size ── */
    snprintf(buf, sizeof(buf), "  .%03lu", t.milliseconds);
    ST7735_FillRect(0, 48, LCD_WIDTH, 20, BLACK);
    ST7735_DrawString(8, 49, buf, col, BLACK, 2);

    /* ── Divider ── */
    ST7735_DrawHLine(0, 72, LCD_WIDTH, DARKGRAY);

    /* ── Full time small at bottom ── */
    ST7735_FillRect(0, 138, LCD_WIDTH, 10, BLACK);
    snprintf(buf, sizeof(buf), "%02lu:%02lu.%03lu",
             t.minutes, t.seconds, t.milliseconds);
    ST7735_DrawString(2, 139, buf, DARKGRAY, BLACK, 1);

    /* ── Status bar ── */
    ST7735_FillRect(0, 150, LCD_WIDTH, 10, BLACK);
    if (s == SW_RUNNING) {
        ST7735_DrawString(10, 151, ">> RUNNING <<", GREEN, BLACK, 1);
    } else if (t.minutes == 0 && t.seconds == 0 && t.milliseconds == 0) {
        ST7735_DrawString(16, 151, "-- READY --",  CYAN,  BLACK, 1);
    } else {
        ST7735_DrawString(13, 151, "-- PAUSED --", RED,   BLACK, 1);
    }
}

static void Update_LEDs(StopwatchState s)
{
    if (s == SW_RUNNING) {
        HAL_GPIO_WritePin(LED_RUN_PORT,  LED_RUN_PIN,  GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_STOP_PORT, LED_STOP_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(LED_RUN_PORT,  LED_RUN_PIN,  GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_STOP_PORT, LED_STOP_PIN, GPIO_PIN_SET);
    }
}

/* ═════════════════════════════════════════════════
 *  TIMER 6 INTERRUPT — fires every 1ms
 *  This is what drives the stopwatch counting
 * ═════════════════════════════════════════════════ */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        Stopwatch_Tick();   /* increment ms counter if running */
    }
}

/* ═════════════════════════════════════════════════
 *  PERIPHERAL CONFIGURATION
 * ═════════════════════════════════════════════════ */

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState            = RCC_MSI_ON;
    osc.MSICalibrationValue = 0;
    osc.MSIClockRange       = RCC_MSIRANGE_6;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_MSI;
    osc.PLL.PLLM            = 1;
    osc.PLL.PLLN            = 40;
    osc.PLL.PLLP            = RCC_PLLP_DIV7;
    osc.PLL.PLLQ            = RCC_PLLQ_DIV2;
    osc.PLL.PLLR            = RCC_PLLR_DIV2;   /* 80 MHz */
    HAL_RCC_OscConfig(&osc);

    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                       | RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4);
}

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    /* ── LCD control outputs: PA4 (CS), PA8 (DC) ── */
    g.Pin   = GPIO_PIN_4 | GPIO_PIN_8;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);

    /* ── LCD RST: PB0, LED Stop: PB1 ── */
    g.Pin   = GPIO_PIN_0 | GPIO_PIN_1;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);

    /* ── LED Run: PA9 ── */
    g.Pin   = GPIO_PIN_9;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);

    /* ── Buttons: PB3, PB4 — input with internal pull-up ── */
    g.Pin   = GPIO_PIN_3 | GPIO_PIN_4;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_PULLUP;   /* internal pull-up — no resistor needed */
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);

    /* ── Default output states ── */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);    /* CS high  */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);    /* RST high */
    HAL_GPIO_WritePin(LED_RUN_PORT,  LED_RUN_PIN,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_STOP_PORT, LED_STOP_PIN, GPIO_PIN_RESET);
}

static void SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_5 | GPIO_PIN_7;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &g);

    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi1);
}

static void TIM6_Init(void)
{
    /*
     *  TIM6 is a basic timer on STM32L432KC.
     *  We configure it to fire an interrupt every 1ms.
     *
     *  PCLK1 = 80 MHz
     *  Prescaler = 7999  → timer clock = 80MHz / (7999+1) = 10 kHz
     *  Period    = 9     → interrupt every (9+1) ticks = 1ms
     *
     *  1ms = 1/(10000 Hz) ✓
     */
    __HAL_RCC_TIM6_CLK_ENABLE();

    htim6.Instance               = TIM6;
    htim6.Init.Prescaler         = 7999;
    htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim6.Init.Period            = 9;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim6);

    /* Enable TIM6 interrupt in NVIC */
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

/* ── Required HAL callbacks ── */
void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

void SysTick_Handler(void)    { HAL_IncTick(); }

/* TIM6 IRQ handler — HAL routes this to HAL_TIM_PeriodElapsedCallback */
void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim6);
}