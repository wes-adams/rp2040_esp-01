#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "tusb.h"

#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "pico/time.h"

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "hardware/watchdog.h"

#define ESP_UART  (uart0)
#define BAUD_RATE (115200)
#define LED_PIN   (25)
#define BUF_MAX   (CFG_TUD_CDC_TX_BUFSIZE)
//--------------------------------------------------------------------+
enum dest {
    TO_ESP,
    TO_RP2040,
};

struct commands_t {
    char *command;
    char *description;
    bool (*f_ptr)(void *data);
};

struct buffer_t {
    uint8_t data[BUF_MAX];
    uint16_t head_ptr;
};

struct buffers_t {
    struct buffer_t rp2040;
    struct buffer_t to_esp;
    struct buffer_t from_esp;
};

//--------------------------------------------------------------------+
// static vars
//--------------------------------------------------------------------+
static struct buffers_t buffers;

static const char CRNL[] = {'\r', '\n'};
static enum dest direction = TO_RP2040;
static bool already_printed_dest;
static bool ready_for_proc;
static bool g_timeout = false;
static alarm_id_t alarm_id;
static bool echo;
//--------------------------------------------------------------------+
// static function prototypes
//--------------------------------------------------------------------+
static void local_led_init(void);
static void local_uart_init(void);
static void local_wdog_init(void);
static void cdc_task(void);
static void process_rp2040_msg(void);
static void process_esp_msg(void);
static bool print(const char *fmt, ...);
static void print_dest(void);
static void uart0_irq(void);
static bool send_and_catch_resp(const char *at_cmd, uint32_t timeout);
static bool catch_response(void);

static bool a(void *data);
static bool b(void *data);
static bool c(void *data);
static bool d(void *data);
static bool e(void *data);
static bool f(void *data);
static bool g(void *data);
static bool h(void *data);
static bool usage(void *data);
static bool reset(void *data);
//--------------------------------------------------------------------+
static struct commands_t commands[] = {
    {"a", "LED on", &a},
    {"b", "LED off", &b},
    {"c", "Switch interface", &c},
    {"d", "AT", &d},
    {"e", "AT+CWLAP", &e},
    {"f", "AT+CIFSR", &f},
    {"g", "AT+CIPMUX=1", &g},
    {"h", "AT+CIPSERVER=1,333", &h},
    {"r", "reset RP2040", &reset},
    {"?", "usage", &usage},
};
//--------------------------------------------------------------------+
int64_t alarm_cb(alarm_id_t id, void *user_data)
{
    print("alarm fired\r\n");
    g_timeout = true;
}

static void local_led_init(void)
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}

void core1_task(void)
{
    while (!tusb_inited());
    
    local_wdog_init();
    local_uart_init();
    local_led_init();
    
    //TODO:add command to report reset status
    if (watchdog_caused_reboot()) {
        print("Rebooted by Watchdog!\r\n");
    } else {
        print("Clean boot\r\n");
    }

    while (1) {
        print_dest();
        cdc_task();
        watchdog_update();
    }
    return;
}

int main(void)
{
    tusb_init();
    while (!tusb_inited());

    multicore_launch_core1(core1_task);

    while (1) {
        tud_task();
    }
    return 0;
}

static void uart0_irq(void) {
    static uint8_t count = 0;
    if (uart_is_readable(ESP_UART)) {
        uint8_t ch = uart_getc(ESP_UART);
        print("%c", ch);
        /* ignore \r */
        if (ch == 0x0D)
            return;
        buffers.from_esp.data[buffers.from_esp.head_ptr++] = ch;
        if (ch == 0x0A) {
            ready_for_proc = true;
            return;
        }
    }
}

void local_uart_init(void)
{
    uart_init(ESP_UART, BAUD_RATE);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
    irq_set_exclusive_handler(UART0_IRQ, uart0_irq);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(ESP_UART, true, false);
}

static void local_wdog_init(void)
{
    watchdog_enable(0x7fffff, 0);
}

static bool check_commands(void)
{
    bool ret;
    uint8_t *buf;

    buf = ((direction == TO_RP2040) ? buffers.rp2040.data : buffers.to_esp.data);

    for (unsigned i = 0; i < TU_ARRAY_SIZE(commands); i++) {
        if (strcmp(buf, commands[i].command) == 0) {
            print("executing %s\r\n", commands[i].description);
            /* pass CLI args here */
            ret = commands[i].f_ptr(NULL);
            return true;
        }
    }
    return false;
}


static void process_rp2040_msg(void)
{
    check_commands();
    memset(buffers.rp2040.data, 0, sizeof buffers.rp2040.data);
    buffers.rp2040.head_ptr = 0;
}

static void process_esp_msg(void)
{
    print("process_esp_msg\r\n");
    if (!check_commands()) {
        /* add '\r' ,'\n' and '\0' to satisfy AT set */
        buffers.to_esp.data[buffers.to_esp.head_ptr - 1] = 0x0D;
        buffers.to_esp.data[buffers.to_esp.head_ptr++] = 0x0A;
        buffers.to_esp.data[buffers.to_esp.head_ptr++] = 0;
        uart_puts(ESP_UART, buffers.to_esp.data);
    }
    memset(buffers.to_esp.data, 0, sizeof buffers.to_esp.data);
    buffers.to_esp.head_ptr = 0;
}

static void print_dest(void)
{
    if (!already_printed_dest) {
        print("[%s]: ", direction == TO_RP2040 ? "RP2040" : "ESP");
        already_printed_dest = true;
    }
}

static bool print(const char *fmt, ...) {
    unsigned count = 0;
    char ascii[BUF_MAX];
    va_list va;

    va_start(va, fmt);
    vsnprintf(ascii, BUF_MAX, fmt, va);
    va_end(va);

    tud_cdc_write(ascii, strlen(ascii));
    tud_cdc_write_flush();
}

static bool a(void *data)
{
    gpio_put(LED_PIN, true);
}

static bool b(void *data)
{
    gpio_put(LED_PIN, false);
}

static bool c(void *data)
{
    direction ^= 1;
}

static bool d(void *data)
{
    bool ret;

    ret = send_and_catch_resp("AT\r\n", 10000);
    print("%s :: %d :: %b\r\n", __func__, __LINE__, ret);
    return ret;
}

static bool e(void *data)
{
    bool ret;

    ret = send_and_catch_resp("AT+CWLAP\r\n", 10000);
    print("%s :: %d :: %b\r\n", __func__, __LINE__, ret);
    return ret;
}

static bool f(void *data)
{
    bool ret;

    ret = send_and_catch_resp("AT+CIFSR\r\n", 10000);
    print("%s :: %d :: %b\r\n", __func__, __LINE__, ret);
    return ret;
}

static bool g(void *data)
{
    bool ret;

    ret = send_and_catch_resp("AT+CIPMUX=1\r\n", 10000);
    print("%s :: %d :: %b\r\n", __func__, __LINE__, ret);
    return ret;
}

static bool h(void *data)
{
    bool ret;

    ret = send_and_catch_resp("AT+CIPSERVER=1,333\r\n", 10000);
    print("%s :: %d :: %b\r\n", __func__, __LINE__, ret);
    return ret;
}

static bool send_and_catch_resp(const char *at_cmd, uint32_t timeout)
{
    alarm_id = add_alarm_in_ms(timeout, alarm_cb, NULL, true);
    uart_puts(ESP_UART, at_cmd);
    return catch_response();
}

static bool catch_response(void)
{
    char *ok_ptr;
    char *err_ptr;
    bool ret = false;
    /* target = "\n, \r, O, K, \n, \a, \0" */
    const char ok[] = {0x0A, 0x4F, 0x4B, 0x0A, 0};
    const char error[] = {0x0A, 0x45, 0x52, 0x52, 0x4F, 0x52, 0x0A, 0};
   
    while (true) {
        watchdog_update();
        if (ready_for_proc) {
            ready_for_proc = false;
            ok_ptr = strstr(buffers.from_esp.data, ok);
            if (ok_ptr) {
                ret = true;
                cancel_alarm(alarm_id);
                break;
            }
            err_ptr = strstr(buffers.from_esp.data, error);
            if (err_ptr) {
                cancel_alarm(alarm_id);
                break;
            }
        }
        if (g_timeout) {
            g_timeout = false;
            print("FAILURE\r\n");
            break;
        }
        sleep_ms(1);
    }
    memset(buffers.from_esp.data, 0, sizeof buffers.from_esp.data);
    buffers.from_esp.head_ptr = 0;
    return ret;
}
            
static bool usage(void *data)
{
    /* 
     * split usage into chunks because CDCACM driver supports
     * 64 bytes max
     */
    char *usage[7] = {" a - LED on\r\n",
                      " b - LED off\r\n",
                      " c - Switch interface RP2040/ESP-1\r\n",
                      " d - Ping ESP-1 UART\r\n",
                      " e - List visible wi-fi APs\r\n",
                      " f - Show IP addr\r\n",
                      " r - reset RP2040 into bootrom\r\n"};
    for (unsigned i=0; i<4; i++) { 
        tud_cdc_write(usage[i], strlen(usage[i]));
        tud_cdc_write_flush();
    }
}

static bool reset(void *data)
{
    print("%s :: %u\r\n", __func__, __LINE__);
    sleep_ms(1000);
    reset_usb_boot(0, 1);
}
//--------------------------------------------------------------------+
// WEB SERVER
//--------------------------------------------------------------------+



//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
static void cdc_task(void)
{
    if (tud_cdc_available()) {
        /* TODO:maybe switch to multi-byte processing? */
        //char buf[64];
        //uint32_t count = tud_cdc_read(buf, sizeof(buf));
        char ch;
        uint32_t count = tud_cdc_read(&ch, sizeof ch);

        if (count < 1)
            return;
      
        if (echo) {
            tud_cdc_write(&ch, sizeof ch);
            tud_cdc_write_flush();
        }

        /* ignore \r */
        if (ch == 0x0A)
            return;

        if (direction == TO_RP2040) {
            if (ch == 0x0D) {
                buffers.rp2040.data[buffers.rp2040.head_ptr++] = 0;
                process_rp2040_msg();
                already_printed_dest = false;
                return;
            }
            buffers.rp2040.data[buffers.rp2040.head_ptr++] = ch;
        } else if (direction == TO_ESP) {
            if (ch == 0x0D) {
                buffers.to_esp.data[buffers.to_esp.head_ptr++] = 0;
                process_esp_msg();
                already_printed_dest = false;
                return;
            }
            buffers.to_esp.data[buffers.to_esp.head_ptr++] = ch;
        }
    }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
    (void)itf;
}
