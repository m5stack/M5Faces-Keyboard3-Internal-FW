/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_scan_handle.h"
#include "user_sys.h"
#include "user_led.h"
#include "user_int.h"

/* ============================================================
 * 键值映射表  列: [0]=普通 [1]=aA [2]=SYM [3]=FN [4]=ALT
 * ============================================================ */
static const uint8_t KeyMap[35][5] = {
    /* 0  */ {'q', 'Q', '#', '~', 144},
    /* 1  */ {'w', 'W', '1', '^', 145},
    /* 2  */ {'e', 'E', '2', '&', 146},
    /* 3  */ {'r', 'R', '3', '`', 147},
    /* 4  */ {'t', 'T', '(', '<', 148},
    /* 5  */ {'y', 'Y', ')', '>', 149},
    /* 6  */ {'u', 'U', '_', '{', 150},
    /* 7  */ {'i', 'I', '-', '}', 151},
    /* 8  */ {'o', 'O', '+', '[', 152},
    /* 9  */ {'p', 'P', '@', ']', 153},
    /* 10 */ {'a', 'A', '*', '|', 154},
    /* 11 */ {'s', 'S', '4', '=', 155},
    /* 12 */ {'d', 'D', '5', '\\', 156},
    /* 13 */ {'f', 'F', '6', '%', 157},
    /* 14 */ {'g', 'G', '/', 180, 158},
    /* 15 */ {'h', 'H', ':', 181, 159},
    /* 16 */ {'j', 'J', ';', 182, 160},
    /* 17 */ {'k', 'K', '\'', 183, 161},
    /* 18 */ {'l', 'L', '"', 184, 162},
    /* 19 */ {8, 8, 127, 8, 163},
    /* 20 */ {255, 255, 255, 255, 255},
    /* 21 */ {'z', 'Z', '7', 186, 165},
    /* 22 */ {'x', 'X', '8', 187, 166},
    /* 23 */ {'c', 'C', '9', 188, 167},
    /* 24 */ {'v', 'V', '?', 189, 168},
    /* 25 */ {'b', 'B', '!', 190, 169},
    /* 26 */ {'n', 'N', ',', 191, 170},
    /* 27 */ {'m', 'M', '.', 192, 171},
    /* 28 */ {'$', '$', 255, 193, 172},
    /* 29 */ {13, 13, 13, 13, 13},  // 特殊按键按压两个字符0x0D、0x0A，有特殊处理的逻辑（enter）
    /* 30 */ {255, 255, 255, 255, 255},
    /* 31 */ {'0', '0', '0', '0', 175},
    /* 32 */ {' ', ' ', ' ', ' ', 176},
    /* 33 */ {255, 255, 255, 255, 255},
    /* 34 */ {255, 255, 255, 255, 255},
};

/* ============================================================
 * IDR 低10位 → 键索引查找表
 *
 * mode1: bit_pos → 0  + kGroupOffset[bit_pos]
 * mode2: bit_pos → 10 + kGroupOffset[bit_pos]
 * mode3: bit_pos → kMode3Key[bit_pos]
 *
 * 与原 switch-case 完全等价，包含多键同按返回 KEY_NONE 的行为。
 * ============================================================ */

/** mode1/mode2 组内偏移（bit_pos 0~9 → 组内键号） */
static const uint8_t kGroupOffset[10] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

/** mode3 绝对键索引（bit_pos 0~9 → 键号） */
static const uint8_t kMode3Key[10] = {32, 28, 27, 26, 25, 24, 23, 22, 21, 31};

#define KEYMAP_ROWS ((uint8_t)(sizeof(KeyMap) / sizeof(KeyMap[0])))

/* ============================================================
 * 共享变量（ISR + 主循环共用，必须 volatile）
 * ============================================================ */
volatile uint8_t send_data[SEND_DATA_LEN]             = {0};
volatile uint16_t old_output_data[OUTPUT_DATA_GROUPS] = {0};
volatile uint8_t operation_mode                       = FACES_OPERATION_MODE_NORMAL;
volatile uint8_t hadScaned                            = 0U;

/* ============================================================
 * ISR / 主循环共享的发送状态（volatile 必要）
 * ============================================================ */
static volatile uint8_t s_key        = 0U; /**< 待发键值第1字节 */
static volatile uint8_t s_key2       = 0U; /**< 待发键值第2字节 */
static volatile uint8_t s_hadPressed = 0U; /**< 有键待发标志    */
static volatile uint8_t s_twoBytes   = 0U; /**< 双字节待发标志  */

/* ============================================================
 * 仅在主循环访问的修饰键状态（无需 volatile）
 * ============================================================ */
static uint8_t s_alt = 0U; /**< ALT  状态 0/1      */
static uint8_t s_aa  = 0U; /**< aA   状态 0/1/2    */
static uint8_t s_sym = 0U; /**< SYM  状态 0/1/2    */
static uint8_t s_fn  = 0U; /**< FN   状态 0/1/2    */

static uint32_t s_last_aa_time  = 0U; /**< aA  双击计时基准   */
static uint32_t s_last_fn_time  = 0U; /**< FN  双击计时基准   */
static uint32_t s_last_sym_time = 0U; /**< SYM 双击计时基准   */

static uint8_t clear_buffer[10] = {0};

/**
 * @brief GPIOA IDR 低10位转换为键索引。
 *
 * @param idr   GPIOA->IDR & 0x3FF
 * @param mode  输出模式 1/2/3
 * @return      键索引（0~34），KEY_NONE 表示无效
 *
 * 单键校验：inv 必须是2的幂（恰好1个bit为低），
 * 多键同按时原 switch-case 无匹配，此处同样返回 KEY_NONE。
 */
static uint8_t idr_to_key(uint16_t idr, uint8_t mode)
{
    uint16_t inv = (uint16_t)((~idr) & 0x3FFU);

    /* 无键或多键：与原 switch-case 无匹配的行为等价 */
    if ((inv == 0U) || ((inv & (inv - 1U)) != 0U)) {
        return KEY_NONE;
    }

    /* 计算唯一被拉低的 bit 位置（inv 已确认是2的幂） */
    uint8_t bit_pos = 0U;
    uint16_t tmp    = inv;
    while ((tmp & 1U) == 0U) {
        tmp >>= 1U;
        bit_pos++;
    }
    /* bit_pos 范围 0~9，越界不可能（inv & 0x3FF 保证），但做防护 */
    if (bit_pos >= 10U) return KEY_NONE;

    switch (mode) {
        case 1U:
            return (uint8_t)(0U + kGroupOffset[bit_pos]);
        case 2U:
            return (uint8_t)(10U + kGroupOffset[bit_pos]);
        case 3U:
            return kMode3Key[bit_pos];
        default:
            return KEY_NONE;
    }
}

/* ============================================================
 * 修饰键通用处理（AA / FN / SYM 共性逻辑）
 *
 * @return 1 = 本帧触发瞬间（lock 0→1），调用方在此时执行互斥清除
 * ============================================================ */
static uint8_t modifier_key_update(uint8_t pressed,        /**< 当前键电平（0=按下） */
                                   uint8_t *p_state,       /**< 修饰键状态 0/1/2     */
                                   uint32_t *p_start_tick, /**< 消抖起始时刻         */
                                   uint8_t *p_lock,        /**< 触发锁               */
                                   uint32_t *p_last_time,  /**< 上次触发时刻         */
                                   uint8_t led_single,     /**< 单击 led_mode 值     */
                                   uint8_t led_lock,       /**< 锁定 led_mode 值     */
                                   uint32_t now)
{
    if (pressed == 0U) {
        if (*p_start_tick == 0U) {
            *p_start_tick = now;
        }
        if ((*p_lock == 0U) && ((now - *p_start_tick) >= BTN_DEBOUNCE_MS)) {
            uint32_t diff = now - *p_last_time;
            if (diff < DOUBLE_CLICK_THRESHOLD_MS) {
                /* 双击：state 1→2（锁定） 或 其他→0（关闭） */
                if (*p_state == 1U) {
                    *p_state = 2U;
                    led_mode = led_lock;
                } else {
                    *p_state = 0U;
                    led_mode = 0U;
                }
            } else {
                /* 单击：state 0→1（激活） 或 其他→0（关闭） */
                if (*p_state == 0U) {
                    *p_state = 1U;
                    led_mode = led_single;
                } else {
                    *p_state = 0U;
                    led_mode = 0U;
                }
            }
            *p_last_time = now;
            *p_lock      = 1U;
            return 1U; /* 触发瞬间 */
        }
    } else {
        *p_lock       = 0U;
        *p_start_tick = 0U;
    }
    return 0U;
}

/* ============================================================
 * 键盘扫描主函数
 * 在 while(1) 中每帧调用，HAL_GetTick() 精度 1ms
 * ============================================================ */
static uint8_t get_input(void)
{
    uint32_t now = HAL_GetTick();

    /* 修饰键消抖状态 */
    static uint32_t s_aa_start = 0U, s_fn_start = 0U, s_sym_start = 0U;
    static uint8_t s_aa_lock = 0U, s_fn_lock = 0U, s_sym_lock = 0U;
    static uint8_t s_enter_lock = 0U;

    /* 矩阵状态机（语义清晰，无借用） */
    static MatrixState s_state    = MATRIX_IDLE;
    static uint8_t s_debounce_key = KEY_NONE;
    static uint32_t s_press_tick  = 0U; /**< 按下消抖计时 */
    static uint32_t s_rel_tick    = 0U; /**< 松开消抖计时（独立，不复用） */
    static uint8_t s_rel_timing   = 0U; /**< 是否正在进行松开计时 */

    /* ----------------------------------------------------------
     * 1. 修饰键
     * ---------------------------------------------------------- */
    if (modifier_key_update((uint8_t)btn_read(BTN_AA), &s_aa, &s_aa_start, &s_aa_lock, &s_last_aa_time, 1U, 2U, now)) {
        s_fn  = 0U;
        s_sym = 0U;
        s_alt = 0U;
    }
    if (modifier_key_update((uint8_t)btn_read(BTN_FN), &s_fn, &s_fn_start, &s_fn_lock, &s_last_fn_time, 4U, 5U, now)) {
        s_aa  = 0U;
        s_sym = 0U;
        s_alt = 0U;
    }
    if (modifier_key_update((uint8_t)btn_read(BTN_SYM), &s_sym, &s_sym_start, &s_sym_lock, &s_last_sym_time, 7U, 6U,
                            now)) {
        s_aa  = 0U;
        s_fn  = 0U;
        s_alt = 0U;
    }
    if (btn_read(BTN_ALT) == GPIO_PIN_RESET) {
        s_fn     = 0U;
        s_sym    = 0U;
        s_aa     = 0U;
        s_alt    = 1U;
        led_mode = 3U;
    } else {
        s_alt = 0U;
    }
    if (btn_read(BTN_ENTER) == GPIO_PIN_RESET) {
        if (s_enter_lock == 0U) {
            s_enter_lock = 1U;
            return 29U;
        }
    } else {
        s_enter_lock = 0U;
    }

    /* ----------------------------------------------------------
     * 2. 矩阵全量扫描（修复跨组多键盲区）
     *
     * 改动：扫描全部三组，不再 break，
     *       收集所有组的按键状态后再综合判断。
     *
     * active_groups : 有按键活动的组数
     * valid_key     : 有效单键值（只有 active_groups==1 且该组单键时有效）
     * ---------------------------------------------------------- */
    uint8_t active_groups = 0U;
    uint8_t valid_key     = KEY_NONE;

    for (uint8_t mode = 1U; mode <= 3U; mode++) {
        output_mode_set(mode);
        HAL_Delay(DIRECT_SETTLE_MS);
        uint16_t idr = (uint16_t)(GPIOA->IDR & 0x3FFU);
        if (idr != 0x3FFU) {
            active_groups++;
            if (active_groups == 1U) {
                /* 暂存第一个有效组的键值 */
                valid_key = idr_to_key(idr, mode);
            } else {
                /* 第二个组也有键 → 跨组多键，清除键值 */
                valid_key = KEY_NONE;
            }
        }
    }

    /*
     * 综合判断：
     *   active_groups == 0             → 无任何键
     *   active_groups == 1, valid_key有效 → 单键
     *   active_groups == 1, valid_key无效 → 同组多键
     *   active_groups >= 2             → 跨组多键
     *
     * any_key_pressed : 物理上是否有任何键（用于松开检测）
     * current_raw_key : 唯一有效单键（否则 KEY_NONE）
     */
    uint8_t any_key_pressed = (active_groups > 0U) ? 1U : 0U;
    uint8_t current_raw_key = (active_groups == 1U) ? valid_key : KEY_NONE;

    /* 多组同时有键 → 确认多键状态 */
    uint8_t multi_key = (active_groups > 1U) ? 1U : 0U;

    /* ----------------------------------------------------------
     * 3. 四状态机（松开计时使用独立变量，逻辑清晰）
     * ---------------------------------------------------------- */
    switch (s_state) {
        /* ======================================================
         * IDLE：等待新输入
         * ====================================================== */
        case MATRIX_IDLE:
            s_rel_timing = 0U; /* 确保松开计时清零 */

            if (multi_key || (any_key_pressed && current_raw_key == KEY_NONE)) {
                /* 多键或同组多键 → 直接封锁 */
                s_state    = MATRIX_LOCKOUT;
                s_rel_tick = now;
            } else if (current_raw_key != KEY_NONE) {
                /* 有效单键 → 开始消抖 */
                s_state        = MATRIX_DEBOUNCING;
                s_debounce_key = current_raw_key;
                s_press_tick   = now;
            }
            break;

        /* ======================================================
         * DEBOUNCING：单键消抖中
         * ====================================================== */
        case MATRIX_DEBOUNCING:
            if (!any_key_pressed) {
                /* 键松开（抖动）→ 回 IDLE */
                s_state        = MATRIX_IDLE;
                s_debounce_key = KEY_NONE;

            } else if (multi_key || current_raw_key == KEY_NONE) {
                /* 多键介入 → 封锁 */
                s_state    = MATRIX_LOCKOUT;
                s_rel_tick = now;

            } else if (current_raw_key != s_debounce_key) {
                /* 键值变化 → 重新消抖 */
                s_debounce_key = current_raw_key;
                s_press_tick   = now;

            } else if ((now - s_press_tick) >= MATRIX_DEBOUNCE_MS) {
                /* 稳定超过消抖时间 → 触发 */
                s_state      = MATRIX_TRIGGERED;
                s_rel_timing = 0U;
                return s_debounce_key;
            }
            break;

        /* ======================================================
         * TRIGGERED：已触发，等待完全松开
         *
         * 核心原则：
         *   无论此时检测到何种键（同键/换键/多键），
         *   全部忽略，不产生任何新触发。
         *   只有物理上完全无键，且稳定 MATRIX_DEBOUNCE_MS，
         *   才回到 IDLE。
         * ====================================================== */
        case MATRIX_TRIGGERED:
            if (!any_key_pressed) {
                /* 物理无键：启动/推进松开计时 */
                if (s_rel_timing == 0U) {
                    s_rel_timing = 1U;
                    s_rel_tick   = now; /* 仅首次记录，不重复刷新 */
                }
                if ((now - s_rel_tick) >= MATRIX_DEBOUNCE_MS) {
                    /* 松开稳定 → 回 IDLE */
                    s_state        = MATRIX_IDLE;
                    s_debounce_key = KEY_NONE;
                    s_rel_timing   = 0U;
                }
            } else {
                /*
                 * 物理上仍有键（任意键）：
                 *   重置松开计时，继续等待。
                 *   绝对不触发任何新键。
                 */
                s_rel_timing = 0U; /* 取消松开计时，需重新从无键开始计 */
            }
            break;

        /* ======================================================
         * LOCKOUT：多键封锁
         *
         * 所有键完全松开并稳定后才恢复 IDLE。
         * ====================================================== */
        case MATRIX_LOCKOUT:
            if (!any_key_pressed) {
                /* 无键时计时（s_rel_tick 在进入时已设置，此处不刷新） */
                if ((now - s_rel_tick) >= MATRIX_DEBOUNCE_MS) {
                    s_state        = MATRIX_IDLE;
                    s_debounce_key = KEY_NONE;
                    s_rel_timing   = 0U;
                }
            } else {
                /* 还有键：重置等待计时 */
                s_rel_tick = now;
            }
            break;

        default:
            s_state        = MATRIX_IDLE;
            s_debounce_key = KEY_NONE;
            s_rel_timing   = 0U;
            break;
    }

    return KEY_NONE;
}
/* ============================================================
 * 直连模式扫描
 * 注意：get_output_data 内含 HAL_Delay(2)，
 *       三次调用共阻塞约 6ms，在 while 循环中属于正常开销。
 * ============================================================ */
static uint16_t get_output_data(uint8_t mode)
{
    output_mode_set(mode);
    HAL_Delay(DIRECT_SETTLE_MS);
    return (uint16_t)(GPIOA->IDR & 0x3FFU);
}

static uint8_t direct_key_scan(void)
{
    uint8_t diff = 0U;
    uint8_t *p   = (uint8_t *)send_data;

    /* 采集四组原始数据 */
    uint16_t output_data[OUTPUT_DATA_GROUPS];
    output_data[0] = (uint16_t)((0U << 12) | get_output_data(1U));
    output_data[1] = (uint16_t)((1U << 12) | get_output_data(2U));
    output_data[2] = (uint16_t)((2U << 12) | get_output_data(3U));
    output_data[3] = (uint16_t)((3U << 12) | ((uint16_t)btn_read(BTN_AA) << 0U) | ((uint16_t)btn_read(BTN_ALT) << 1U) |
                                ((uint16_t)btn_read(BTN_ENTER) << 2U) | ((uint16_t)btn_read(BTN_SYM) << 3U) |
                                ((uint16_t)btn_read(BTN_FN) << 4U));

    *p++ = 0x00U; /* 长度占位，后面回填 */

    for (uint8_t i = 0U; i < OUTPUT_DATA_GROUPS; i++) {
        uint8_t ldiff = 0x00U;
        if (old_output_data[i] != output_data[i]) {
            old_output_data[i] = output_data[i];
            ldiff              = 0x80U;
            diff |= ldiff;
        }
        uint16_t packed = output_data[i] | (uint16_t)(ldiff << 8U);
        *p++            = (uint8_t)(packed >> 8U);
        *p++            = (uint8_t)(packed);
    }

    /* 回填长度（含校验字节） */
    uint8_t len  = (uint8_t)(p - (uint8_t *)send_data);
    send_data[0] = len + 1U;

    /* 追加补码校验字节，使所有字节之和为 0 */
    uint8_t sum = 0U;
    for (uint8_t i = 0U; i < len; i++) {
        sum += send_data[i];
    }
    *p = (uint8_t)(-(int8_t)sum);

    return diff;
}

/* ============================================================
 * 对外接口
 * ============================================================ */

/**
 * @brief 将直连数据推入 I2C 发送缓冲（仅内部使用）。
 */
static void set_direct_data(void)
{
    i2c2_set_send_data((uint8_t *)send_data, SEND_DATA_LEN);
}

/**
 * @brief I2C 请求中断回调。
 *        按优先级：直连数据 > 键值第1字节 > 键值第2字节，
 *        发完后拉高 IRQ 线。
 */
void request_event(void)
{
    if (hadScaned != 0U) {
        set_direct_data();
        hadScaned = 0U;
    } else if (s_hadPressed != 0U) {
        i2c2_set_send_data((uint8_t *)&s_key, 1U);
        s_hadPressed = 0U;
        if (s_twoBytes != 0U) {
            return;
        }
    } else if (s_twoBytes != 0U) {
        i2c2_set_send_data((uint8_t *)&s_key2, 1U);
        s_twoBytes = 0U;
    } else {
        i2c2_set_send_data(clear_buffer, 10U);
    }
    IRQ_CLR;
    return;
}

/**
 * @brief 键盘主更新，在 while(1) 中每帧调用。
 */
void keyboard_update(void)
{
    if (operation_mode == FACES_OPERATION_MODE_NORMAL) {
        uint8_t kk = get_input();

        if ((s_aa == 0U) && (s_alt == 0U) && (s_fn == 0U) && (s_sym == 0U)) {
            led_mode = 0U;
        }

        if (kk != KEY_NONE) {
            /*
             * 保护：上一个键事件还未被主机读走时，不覆盖 s_key 也不重复拉低 IRQ。
             * 防止：
             *   1. s_key 被新值覆盖导致旧键丢失
             *   2. IRQ 已为低时再次调用 IRQ_SET 触发主机多余读取
             */
            if (s_hadPressed != 0U || s_twoBytes != 0U) {
                return;
            }

            if (kk == 29U) {
                s_key      = 0x0DU;
                s_key2     = 0x0AU;
                s_twoBytes = 1U;
            } else if (kk < KEYMAP_ROWS) {
                if (s_alt != 0U) {
                    s_key = KeyMap[kk][4];
                } else if (s_sym != 0U) {
                    if (s_sym == 1U) s_sym = 0U;
                    s_key = KeyMap[kk][2];
                } else if (s_fn != 0U) {
                    if (s_fn == 1U) s_fn = 0U;
                    s_key = KeyMap[kk][3];
                } else if (s_aa != 0U) {
                    if (s_aa == 1U) s_aa = 0U;
                    s_key = KeyMap[kk][1];
                } else {
                    s_key = KeyMap[kk][0];
                }
            }
            s_hadPressed = 1U;
            IRQ_SET;
        }
    } else if (operation_mode == FACES_OPERATION_MODE_DIRECT) {
        if (direct_key_scan() != 0U) {
            hadScaned = 1U;
            IRQ_SET;
        }
    }
}

void direct_mode_sync_baseline(void)
{
    old_output_data[0] = (uint16_t)((0U << 12) | get_output_data(1U));
    old_output_data[1] = (uint16_t)((1U << 12) | get_output_data(2U));
    old_output_data[2] = (uint16_t)((2U << 12) | get_output_data(3U));
    old_output_data[3] = (uint16_t)((3U << 12) | ((uint16_t)btn_read(BTN_AA) << 0U) |
                                    ((uint16_t)btn_read(BTN_ALT) << 1U) | ((uint16_t)btn_read(BTN_ENTER) << 2U) |
                                    ((uint16_t)btn_read(BTN_SYM) << 3U) | ((uint16_t)btn_read(BTN_FN) << 4U));
}
