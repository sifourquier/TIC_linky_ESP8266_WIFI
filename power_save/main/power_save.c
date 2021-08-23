/* Power save Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
   this example shows how to use power save mode
   set a router or a AP using the same SSID&PASSWORD as configuration of this example.
   start esp8266 and when it connected to AP it will enter power save mode
*/
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "driver/hw_timer.h"
#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>



#if CONFIG_EXAMPLE_POWER_SAVE_MIN_MODEM
#define DEFAULT_PS_MODE WIFI_PS_MIN_MODEM
#elif CONFIG_EXAMPLE_POWER_SAVE_MAX_MODEM
#define DEFAULT_PS_MODE WIFI_PS_MAX_MODEM
#elif CONFIG_EXAMPLE_POWER_SAVE_NONE
#define DEFAULT_PS_MODE WIFI_PS_NONE
#else
#define DEFAULT_PS_MODE WIFI_PS_NONE
#endif /*CONFIG_POWER_SAVE_MODEM*/

#define PORT 23

#define BUF_SIZE (1024)


static const char *TAG = "TCP";
int sock=-1;

static void uart_read_task()
{
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, data, BUF_SIZE, 20 / portTICK_RATE_MS);
		//uart_write_bytes(UART_NUM_0, (char*)data,len);
		//uart_flush(UART_NUM_0);
        // Write data back to the UART
		vTaskDelay(10 / portTICK_PERIOD_MS);
		if (sock != -1) {
			int err = send(sock, data, len, 0);
			if (err < 0) {
				ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
				break;
			}
		}
    }
}

/*static void ping()
{
	while (1) {
		char* str = "p";
		uart_write_bytes(UART_NUM_0,str ,sizeof(str));

		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}*/

// ISR to Fire when Timer is triggered
void hw_timer_callback() {
	/*char* str = "p";
	uart_write_bytes(UART_NUM_0,str ,sizeof(str));*/
	gpio_set_level(GPIO_NUM_1, 1);
	volatile int l;
	for(l=0;l<1000;l++);
	gpio_set_level(GPIO_NUM_1, 0);
}

static void tcp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
	
	//vTaskDelay(10000 / portTICK_PERIOD_MS);
	
	
	xTaskCreate(uart_read_task, "uart_read_task", 1024, NULL, 10, NULL);

    while (1) {

        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket binded");

        err = listen(listen_sock, 1);
        if (err != 0) {
            ESP_LOGE(TAG, "Error occured during listen: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_in sourceAddr;

        uint addrLen = sizeof(sourceAddr);
        sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket accepted");

        while (1) {
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occured during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            // Connection closed
            else if (len == 0) {
                ESP_LOGI(TAG, "Connection closed");
                break;
            }
            // Data received
            else {

                inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);

                //rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
               // ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                //ESP_LOGI(TAG, "%s", rx_buffer);
				uart_tx_chars(0,rx_buffer,len);

                
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    // Configure parameters of an UART driver,
    // communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 1200,
        .data_bits = UART_DATA_7_BITS,
        .parity    = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);
	
	uint64_t mask = 2;
	gpio_config_t config;
	config.pin_bit_mask = mask;
	config.mode = GPIO_MODE_OUTPUT;
	config.pull_up_en = GPIO_PULLUP_ENABLE;
	config.pull_down_en = GPIO_PULLDOWN_DISABLE;
	config.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&config); //switch tx as GPIO
	
	ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
	//xTaskCreate(ping, "ping", 1024, NULL, 10, NULL);
	
	gpio_set_level(GPIO_NUM_1, 1);
	gpio_set_level(GPIO_NUM_1, 0);
	gpio_set_level(GPIO_NUM_1, 1);
	gpio_set_level(GPIO_NUM_1, 0);
	gpio_set_level(GPIO_NUM_1, 1);
	gpio_set_level(GPIO_NUM_1, 0);

      uint32_t value = 50000; // 1s
      uint32_t counter = ((TIMER_BASE_CLK >> TIMER_CLKDIV_16) / 1000000) * value; // value units are micro-seconds
      ESP_LOGI(TAG, "Initialize hw_timer for callback, value %u, counter %u (0x%x)", value, counter, counter);
      hw_timer_init(hw_timer_callback, NULL);
      hw_timer_set_reload(true); // false - also works
      hw_timer_set_clkdiv(TIMER_CLKDIV_16);
      hw_timer_set_intr_type(TIMER_EDGE_INT);
      hw_timer_set_load_data(counter);
      ESP_LOGI(TAG, "Retrieved counter %u", hw_timer_get_load_data());
      hw_timer_enable(true);


	esp_wifi_set_ps(DEFAULT_PS_MODE);
	esp_wifi_set_mode(WIFI_MODE_STA);
	ESP_ERROR_CHECK(example_connect());

	
	
	xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);


}
