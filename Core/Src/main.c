#include "main.h"
#include "i2c.h"
#include "gpio.h"
#include "oled.h"
#include "bh1750.h"
#include "aht20.h"

void SystemClock_Config(void);
static void I2C_BusReset(void);
static void I2C_Reinit(void);

// 传感器设备
BH1750_HandleTypeDef bh1750;
AHT20_HandleTypeDef aht20;

// 传感器数据
float temperature = 0.0f;
float humidity = 0.0f;
float light = 0.0f;

// 传感器状态(0 = 正常,非 0 = 错误码)
uint8_t bh1750_status = 0xFF;
uint8_t aht20_status  = 0xFF;
uint8_t bh1750_present = 0;
uint8_t aht20_present  = 0;

// 探测结果(每个循环刷新)
uint8_t aht20_ready  = 0;
uint8_t bh1750_ready = 0;

// 状态行隐藏时间点(毫秒)
uint32_t status_hide_at = 0;

// 强光报警阈值(Lux)
#define LIGHT_ALARM_THRESHOLD  1000.0f

// 简洁的 I2C 设备探测(无屏幕动画)
static void I2C_Probe(void)
{
    bh1750_present = (HAL_I2C_IsDeviceReady(&hi2c1, BH1750_ADDR, 2, 5) == HAL_OK);
    aht20_present  = (HAL_I2C_IsDeviceReady(&hi2c1, AHT20_ADDR,  2, 5) == HAL_OK);
}

// 开机动画 — 标题淡入 + 进度条
static void BootAnimation(void)
{
    const char *title = "STM32 Sensor";
    const char *subtitle = "Monitor v1.0";

    /* 阶段 1:标题逐字显现 */
    OLED_Clear();
    OLED_Display();

    uint8_t title_len = 0;
    while (title[title_len]) title_len++;
    uint8_t title_x = (OLED_WIDTH - title_len * 6) / 2;

    char buf[16];
    for (uint8_t i = 1; i <= title_len; i++)
    {
        for (uint8_t k = 0; k < i; k++) buf[k] = title[k];
        buf[i] = 0;
        OLED_Clear();
        OLED_DrawString(title_x, 18, buf, 8);
        OLED_Display();
        HAL_Delay(40);
    }

    /* 阶段 2:副标题 + 下划线展开 */
    uint8_t sub_len = 0;
    while (subtitle[sub_len]) sub_len++;
    uint8_t sub_x = (OLED_WIDTH - sub_len * 6) / 2;
    OLED_DrawString(sub_x, 30, subtitle, 8);
    OLED_Display();
    HAL_Delay(150);

    for (uint8_t w = 0; w <= 100; w += 4)
    {
        uint8_t line_w = (uint8_t)((uint16_t)100 * w / 100);
        uint8_t lx = (OLED_WIDTH - 100) / 2;
        OLED_DrawHLine(lx, 42, line_w);
        OLED_Display();
        HAL_Delay(8);
    }
    HAL_Delay(120);

    /* 阶段 3:进度条加载 */
    OLED_Clear();
    OLED_DrawString(title_x, 14, title, 8);
    OLED_DrawString(sub_x, 26, subtitle, 8);
    OLED_DrawString(46, 42, "Loading", 8);
    OLED_Display();

    for (uint8_t p = 0; p <= 100; p += 5)
    {
        OLED_DrawProgressBar(14, 54, 100, 6, p);
        OLED_Display();
        HAL_Delay(15);
    }
    HAL_Delay(150);

    /* 阶段 4:水平擦除过渡 */
    for (int8_t x = 0; x < (int8_t)OLED_WIDTH; x += 4)
    {
        OLED_FillRect(x, 0, 4, OLED_HEIGHT);
        OLED_Display();
    }
    HAL_Delay(50);
}

// 显示主界面
void ShowMainUI(void)
{
    OLED_Clear();

    // 标题
    OLED_DrawString(16, 0, "Sensor Monitor", 8);
    OLED_DrawHLine(0, 10, 128);

    // 温度
    if (aht20_status == 0 || aht20_status == 4)
    {
        int16_t t_int  = (int16_t)temperature;
        int16_t t_frac = (int16_t)((temperature - t_int) * 100);
        if (t_frac < 0) t_frac = -t_frac;
        OLED_Printf(0, 16, 8, "T:%d.%02dC", t_int, t_frac);
    }
    else
        OLED_Printf(0, 16, 8, "T:ERR%d", aht20_status);

    // 湿度
    if (aht20_status == 0 || aht20_status == 4)
    {
        int16_t h_int  = (int16_t)humidity;
        int16_t h_frac = (int16_t)((humidity - h_int) * 100);
        if (h_frac < 0) h_frac = -h_frac;
        OLED_Printf(0, 28, 8, "H:%d.%02d%%", h_int, h_frac);
    }
    else
        OLED_Printf(0, 28, 8, "H:ERR%d", aht20_status);

    // 进度条 — 文字最长 9 字符 = 54px,从 x=60 开始,留 64px 给进度条
    if (aht20_status == 0 || aht20_status == 4)
    {
        uint8_t pct = (uint8_t)(temperature * 2);
        if (temperature < 0) pct = 0;
        if (pct > 100) pct = 100;
        OLED_DrawProgressBar(60, 17, 66, 6, pct);

        uint8_t hpct = (uint8_t)humidity;
        if (humidity < 0) hpct = 0;
        if (hpct > 100) hpct = 100;
        OLED_DrawProgressBar(60, 29, 66, 6, hpct);
    }

    // 分隔线
    OLED_DrawHLine(0, 40, 128);

    // 状态行是否还在显示
    uint8_t status_visible = (HAL_GetTick() < status_hide_at);

    // 光照位置:状态行可见时在 y=44(原位),隐藏后下移到 y=54 占据空白
    uint8_t light_y = status_visible ? 44 : 54;

    // 强光报警(阈值以上,每 500ms 闪烁一次)
    uint8_t alarm = (bh1750_status == 0) && (light >= LIGHT_ALARM_THRESHOLD);
    uint8_t alarm_blink = alarm && ((HAL_GetTick() / 500) & 1);

    // 光照行
    if (bh1750_status == 0)
    {
        if (alarm)
        {
            /* 报警:文字加 "ALARM" 标记,500ms 周期反白闪烁 */
            OLED_Printf(0, light_y, 8, "Light:%dLux ALARM", (int)light);
            if (alarm_blink)
                OLED_InvertPageRect(0, light_y, 128, 8);
        }
        else
        {
            OLED_Printf(0, light_y, 8, "Light: %d Lux", (int)light);
        }
    }
    else
    {
        OLED_Printf(0, light_y, 8, "Light: ERR %d", bh1750_status);
    }

    // 状态行 — 启动 3 秒内显示,之后自动隐藏
    if (status_visible)
    {
        OLED_Printf(0, 56, 8, "AHT:%c%02X BH:%c%02X",
                    aht20_ready ? 'Y' : 'N', aht20_status,
                    bh1750_ready ? 'Y' : 'N', bh1750_status);
    }

    OLED_Display();
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    // 在初始化 I2C 外设前,先把可能卡住的总线复位
    I2C_BusReset();

    MX_I2C1_Init();
    OLED_Init();

    // 开机动画
    BootAnimation();

    // 静默探测 I2C 设备
    I2C_Probe();

    // 不管探测结果如何都强制初始化两个传感器
    BH1750_Init(&bh1750, &hi2c1, BH1750_ADDR);
    AHT20_Init(&aht20, &hi2c1);
    bh1750_present = 1;
    aht20_present  = 1;

    // 状态行从此刻起 3 秒后隐藏
    status_hide_at = HAL_GetTick() + 3000;

    // 主循环 — 用 HAL_GetTick(),传感器读取带超时保护
    uint32_t last_read = 0;
    uint32_t last_update = 0;
    uint32_t last_blink = 0;

    while (1)
    {
        uint32_t now = HAL_GetTick();

        // LED 心跳:每 500ms 翻转,证明主循环没卡死
        if (now - last_blink >= 500)
        {
            HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
            last_blink = now;
        }

        // 每 1.5 秒读取传感器
        if (now - last_read >= 1500)
        {
            // 实时探测设备是否在 I2C 上响应
            aht20_ready  = (HAL_I2C_IsDeviceReady(&hi2c1, AHT20_ADDR, 2, 5) == HAL_OK);
            bh1750_ready = (HAL_I2C_IsDeviceReady(&hi2c1, BH1750_ADDR, 2, 5) == HAL_OK);

            if (bh1750_present)
                bh1750_status = BH1750_ReadLight(&bh1750, &light);
            else
                bh1750_status = 0xEE;

            if (aht20_present)
                aht20_status = AHT20_Read(&aht20, &temperature, &humidity);
            else
                aht20_status = 0xEE;

            // 如果 I2C 卡死,尝试恢复
            if (hi2c1.ErrorCode != HAL_I2C_ERROR_NONE)
                I2C_Reinit();

            last_read = now;
        }

        // 每 200ms 刷屏
        if (now - last_update >= 200)
        {
            ShowMainUI();
            last_update = now;
        }

        HAL_Delay(5);
    }
}

/* ------------------------------------------------------------------------ */
/* I2C 总线手动复位:模拟 9 个时钟把卡死的从机解锁                          */
/* ------------------------------------------------------------------------ */
static void I2C_BusReset(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PB6 = SCL, PB7 = SDA 改为开漏推挽输出 */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(2);

    /* 拨 9 个时钟 */
    for (int i = 0; i < 9; i++)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_Delay(1);
    }

    /* 产生 STOP:SDA 在 SCL 高时由低变高 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_Delay(1);
}

/* I2C 出错后重新初始化外设 */
static void I2C_Reinit(void)
{
    HAL_I2C_DeInit(&hi2c1);
    I2C_BusReset();
    MX_I2C1_Init();
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
    RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
        Error_Handler();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}
