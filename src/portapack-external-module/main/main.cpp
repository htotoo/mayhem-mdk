#include <iostream>
#include <memory>
#include <cstring>
#include <vector>
#include <queue>
#include <algorithm>

#include "driver/i2c.h"
#include "driver/uart.h"
#include "uart_app.h"

static_assert(sizeof(uart_app) % 32 == 0, "app size must be multiple of 32 bytes. fill with 0s");

// NANO:D2 is NORA:J8 is ESP32:9 is GPIO:4
#define I2C_SLAVE_SDA_IO GPIO_NUM_5

// NANO:D3 is NORA:J7 is ESP32:10 is GPIO:5
#define I2C_SLAVE_SCL_IO GPIO_NUM_6

// A7 is GPIO:14
#define UART_RX GPIO_NUM_14

#define ESP_SLAVE_ADDR 0x51

#define LED_RED GPIO_NUM_46
#define LED_GREEN GPIO_NUM_0
#define LED_BLUE GPIO_NUM_45

void initialize_uart(uint32_t baudrate);
void deinitialize_uart();

uint32_t baudrate = 115200;

std::vector<uint32_t> baudrates = {50, 75, 110, 134, 150, 200, 300, 600,
                                   1200, 2400, 4800, 9600, 14400, 19200,
                                   28800, 38400, 57600, 115200, 230400,
                                   460800, 576000, 921600, 1843200, 3686400};

typedef struct
{
    uint32_t api_version;
    uint32_t module_version;
    char module_name[20];
    uint32_t application_count;
} device_info;

enum app_location_t : uint32_t
{
    UTILITIES = 0,
    RX,
    TX,
    DEBUG,
    HOME
};

typedef struct
{
    uint32_t header_version;
    uint8_t app_name[16];
    uint8_t bitmap_data[32];
    uint32_t icon_color;
    app_location_t menu_location;
    uint32_t binary_size;
} standalone_app_info;

#define USER_COMMANDS_START 0x7F01

enum class Command : uint16_t
{
    COMMAND_NONE = 0,

    // will respond with device_info
    COMMAND_INFO = 0x18F0,

    // will respond with info of application
    COMMAND_APP_INFO = 0xA90B,

    // will respond with application data
    COMMAND_APP_TRANSFER = 0x4183,

    // UART specific commands
    COMMAND_UART_REQUESTDATA_SHORT = USER_COMMANDS_START,
    COMMAND_UART_REQUESTDATA_LONG,
    COMMAND_UART_BAUDRATE_INC,
    COMMAND_UART_BAUDRATE_DEC,
    COMMAND_UART_BAUDRATE_GET
};

volatile Command command_state = Command::COMMAND_NONE;
volatile uint16_t app_counter = 0;
volatile uint16_t app_transfer_block = 0;

void initialize_gpio()
{
    gpio_install_isr_service(0);

    gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_BLUE, GPIO_MODE_OUTPUT);

    gpio_set_level(LED_RED, 1);
    gpio_set_level(LED_GREEN, 1);
    gpio_set_level(LED_BLUE, 1);
}

#include "i2c_slave_driver.h"

QueueHandle_t slave_queue;
std::queue<uint8_t> uart_queue;

void on_command_ISR(Command command, std::vector<uint8_t> additional_data)
{
    command_state = command;

    switch (command)
    {
    case Command::COMMAND_APP_INFO:
        if (additional_data.size() == 2)
            app_counter = *(uint16_t *)additional_data.data();
        break;

    case Command::COMMAND_APP_TRANSFER:
        if (additional_data.size() == 4)
        {
            app_counter = *(uint16_t *)additional_data.data();
            app_transfer_block = *(uint16_t *)(additional_data.data() + 2);
        }
        break;

    case Command::COMMAND_UART_BAUDRATE_INC:

        deinitialize_uart();

        if (baudrate == baudrates.back())
            baudrate = baudrates.front();
        else
        {
            auto it = std::find(baudrates.begin(), baudrates.end(), baudrate);
            baudrate = *(it + 1);
        }

        esp_rom_printf("COMMAND_UART_BAUDRATE_INC: %d\n", baudrate);

        initialize_uart(baudrate);
        break;

    case Command::COMMAND_UART_BAUDRATE_DEC:
        deinitialize_uart();

        if (baudrate == baudrates.front())
            baudrate = baudrates.back();
        else
        {
            auto it = std::find(baudrates.begin(), baudrates.end(), baudrate);
            baudrate = *(it - 1);
        }

        esp_rom_printf("COMMAND_UART_BAUDRATE_DEC: %d\n", baudrate);

        initialize_uart(baudrate);
        break;

    default:
        break;
    }

    BaseType_t high_task_wakeup = pdFALSE;
    xQueueSendFromISR(slave_queue, &command, &high_task_wakeup);
}

std::vector<uint8_t> on_send_ISR()
{
    switch (command_state)
    {
    case Command::COMMAND_INFO:
    {
        device_info info = {
            /* api_version = */ 1,
            /* module_version = */ 1,
            /* module_name = */ "ESP32-S3-PPDEVKIT",
            /* application_count = */ 1};

        return std::vector<uint8_t>((uint8_t *)&info, (uint8_t *)&info + sizeof(info));
    }

    case Command::COMMAND_APP_INFO:
    {
        if (app_counter == 0)
        {
            standalone_app_info app_info;
            std::memset(&app_info, 0, sizeof(app_info));
            std::memcpy(&app_info, uart_app, sizeof(app_info) - 4);
            app_info.binary_size = sizeof(uart_app);

            app_counter = app_counter + 1;

            return std::vector<uint8_t>((uint8_t *)&app_info, (uint8_t *)&app_info + sizeof(app_info));
        }
        break;
    }

    case Command::COMMAND_APP_TRANSFER:
    {
        if (app_counter == 0 && app_transfer_block < sizeof(uart_app) / 128)
        {
            return std::vector<uint8_t>(uart_app + app_transfer_block * 128, uart_app + app_transfer_block * 128 + 128);
        }
        break;
    }

    case Command::COMMAND_UART_REQUESTDATA_SHORT:
    {
        // 1 bit: more data available
        // 7 bit: data length [0 to 4]
        // 4 bytes: data from uart_queue optionally filled with 0xFF

        const int max_data_length = 4;
        std::vector<uint8_t> data(1 + max_data_length);

        uint8_t bytesToSend = uart_queue.size() > max_data_length ? max_data_length : uart_queue.size();
        bool moreData = uart_queue.size() > max_data_length ? 1 : 0;
        data[0] = (bytesToSend & 0x7F) | (moreData << 7);

        for (int i = bytesToSend; i < max_data_length; i++)
            data[i + 1] = 0xFF;

        for (int i = 0; i < bytesToSend; i++)
        {
            data[i + 1] = uart_queue.front();
            uart_queue.pop();
        }

        return data;
    }

    case Command::COMMAND_UART_REQUESTDATA_LONG:
    {
        // 1 bit: more data available
        // 7 bit: data length [0 to 127]
        // 127 bytes: data from uart_queue optionally filled with 0xFF

        const int max_data_length = 127;
        std::vector<uint8_t> data(1 + max_data_length);

        uint8_t bytesToSend = uart_queue.size() > max_data_length ? max_data_length : uart_queue.size();
        bool moreData = uart_queue.size() > max_data_length ? 1 : 0;
        data[0] = (bytesToSend & 0x7F) | (moreData << 7);

        for (int i = bytesToSend; i < max_data_length; i++)
            data[i + 1] = 0xFF;

        for (int i = 0; i < bytesToSend; i++)
        {
            data[i + 1] = uart_queue.front();
            uart_queue.pop();
        }

        return data;
    }

    case Command::COMMAND_UART_BAUDRATE_GET:
    {
        std::vector<uint8_t> data(4);
        esp_rom_printf("COMMAND_UART_BAUDRATE_GET: %d\n", baudrate);
        *(uint32_t *)data.data() = baudrate;
        return data;
    }
    break;

    default:
        break;
    }

    return {0xFF};
}

bool i2c_slave_callback_ISR(struct i2c_slave_device_t *dev, I2CSlaveCallbackReason reason)
{
    switch (reason)
    {
    case I2C_CALLBACK_REPEAT_START:
        gpio_set_level(LED_RED, 0);
        gpio_set_level(LED_GREEN, 1);
        gpio_set_level(LED_BLUE, 1);

        break;

    case I2C_CALLBACK_SEND_DATA:
        gpio_set_level(LED_RED, 1);
        gpio_set_level(LED_GREEN, 0);
        gpio_set_level(LED_BLUE, 1);

        if (dev->state == I2C_STATE_SEND)
        {
            auto data = on_send_ISR();
            if (data.size() > 0)
                i2c_slave_send_data(dev, data.data(), data.size());
        }

        break;

    case I2C_CALLBACK_DONE:

        if (dev->state == I2C_STATE_RECV)
        {
            gpio_set_level(LED_RED, 1);
            gpio_set_level(LED_GREEN, 1);
            gpio_set_level(LED_BLUE, 0);

            uint16_t command = *(uint16_t *)&dev->buffer[dev->bufstart];
            std::vector<uint8_t> additional_data(dev->buffer + dev->bufstart + 2, dev->buffer + dev->bufend);

            on_command_ISR((Command)command, additional_data);
        }
        break;

    default:
        return false;
    }

    return true;
}

i2c_slave_config_t i2c_slave_config = {
    i2c_slave_callback_ISR,
    ESP_SLAVE_ADDR,
    I2C_SLAVE_SCL_IO,
    I2C_SLAVE_SDA_IO,
    I2C_NUM_0};

i2c_slave_device_t *slave_device;

void initialize_fixed_i2c()
{
    slave_queue = xQueueCreate(1, sizeof(uint16_t));

    ESP_ERROR_CHECK(i2c_slave_new(&i2c_slave_config, &slave_device));
}

#define BUF_SIZE (1024)

void initialize_uart(uint32_t baudrate)
{
    uart_config_t uart_config = {
        .baud_rate = (int)baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {.backup_before_sleep = 0}};

    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, UART_PIN_NO_CHANGE, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void deinitialize_uart()
{
    ESP_ERROR_CHECK(uart_driver_delete(UART_NUM_1));
}

static void i2c_task(void *arg)
{
    while (true)
    {
        try
        {
            uint16_t rx_data;
            auto success = xQueueReceive(slave_queue, &rx_data, pdMS_TO_TICKS(10000));

            gpio_set_level(LED_RED, 1);
            gpio_set_level(LED_GREEN, 1);
            gpio_set_level(LED_BLUE, 1);

            if (success != pdTRUE)
            {
                continue;
            }

            // std::cout << "-- Received: 0x" << std::hex << rx_data << std::endl;
        }
        catch (const std::exception &ex)
        {
            std::cout << "Exception: " << std::endl;
            std::cout << ex.what() << std::endl;
        }
    }
}

static void uart_task(void *arg)
{
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    while (true)
    {
        try
        {
            int len = uart_read_bytes(UART_NUM_1, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
            if (len != 0)
            {
                // std::cout << "uart len: " << len << std::endl;
                // std::cout << "uart data: " << std::string((char *)data, len) << std::endl;

                for (int i = 0; i < len; i++)
                    uart_queue.push(data[i]);
            }
        }
        catch (const std::exception &ex)
        {
            std::cout << "Exception: " << std::endl;
            std::cout << ex.what() << std::endl;
        }
    }
}

extern "C" void app_main(void)
{
    initialize_gpio();
    initialize_fixed_i2c();
    initialize_uart(baudrate);

    xTaskCreate(i2c_task, "i2c_task", 1024 * 2, (void *)0, 10, NULL);
    xTaskCreate(uart_task, "uart_task", 1024 * 2, (void *)0, 10, NULL);

    std::cout << "[PP MDK] PortaPack - Module Develoment Kit is ready." << std::endl;
}
