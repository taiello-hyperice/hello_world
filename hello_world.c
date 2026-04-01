#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include <math.h>
#include "lwip/ip4_addr.h"
#include "lwip/tcp.h"
#include "wifi_config.h"



float current_temp = 0.0f;
uint32_t current_free_memory = 0;

uint32_t get_free_memory() {
    extern char __StackLimit;
    register uint32_t stack_pointer __asm("sp");
    return stack_pointer - (uint32_t)&__StackLimit;
}

float read_temperature() {
        // Read the raw ADC value, convert it to voltage, and then calculate the temperature
        int32_t raw_reading = adc_read();
        float voltage = raw_reading*3.3f/4096.0f;
        float temperature =27.0f - (voltage - 0.706f) / 0.001721f;
        
        // Turn on the LED if the temperature is an "even" number, otherwise turn it off
        int temp_int = (int)roundf(temperature * 100); // Multiply by 100 to consider two decimal places
        bool is_even = (temp_int % 2 == 0);
        if (is_even)
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        else
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

        printf("Temperature: %.2f\n", temperature);
        printf(is_even ? "(EVEN)\n" : "(ODD)\n");

        current_free_memory = get_free_memory();
        printf("Free memory: %d bytes\n", current_free_memory);
        current_temp = temperature;
        return temperature;
    }

static err_t receive_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    tcp_recved(tpcb, p->tot_len);

    float temp = current_temp;

    char response[1024];
    int len;
    
    if (strstr(p->payload, "GET /temp") != NULL) {
        len = snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"
            "%.2f,%d",
            temp, current_free_memory);
    } else {
        len = snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
            "<html><body>"
            "<h1>Pico W Temperature</h1>"
            "<p>Temperature: <span id='t'>%.2f</span> C</p>"
            "<p>Free memory: <span id='h'>%d</span> bytes</p>"
            "<script>"
            "setInterval(function(){"
            "fetch('/temp').then(r=>r.text()).then(d=>{"
            "var parts=d.split(',');"
            "document.getElementById('t').textContent=parts[0];"
            "document.getElementById('h').textContent=parts[1];"
            "});"
            "},2000);"
            "</script>"
            "</body></html>",
            temp, current_free_memory);
    }

    tcp_write(tpcb, response, len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    pbuf_free(p);
    tcp_close(tpcb);

    return ERR_OK;
}  

static void error_callback(void *arg, err_t err) {
    printf("TCP error: %d\n", err);
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_err(newpcb, error_callback);
    tcp_recv(newpcb, receive_callback);
    return ERR_OK;
}

void start_http_server() {
    struct tcp_pcb *pcb = tcp_new();
    tcp_bind(pcb, IP_ADDR_ANY, 80);
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("HTTP server started on port 80\n");
}

int main()
{
    stdio_init_all();
    sleep_ms(3000);

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA) != 0) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // Connect to Wi-Fi network
    cyw43_arch_enable_sta_mode();
    
    sleep_ms(1000);

    printf("Attempting to connect to Wi-Fi...\n");
    
    int connection_result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID_1, WIFI_PASS_1, CYW43_AUTH_WPA2_AES_PSK, 15000);
    if (connection_result != 0) {
        printf("Office failed, trying home...\n");
        connection_result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID_2, WIFI_PASS_2, CYW43_AUTH_WPA2_AES_PSK, 15000);
    }
    if (connection_result != 0) {
        printf("Wi-Fi connection failed: %d\n", connection_result);
        return -1;
    }
    // Prints the IP address assigned to the device
    printf("Connected!\n");
    printf("IP Address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));

    start_http_server();

    // Initialise the ADC, enable the internal temperature sensor, and select the correct input
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    sleep_ms(3000); // Wait for the ADC to stabilize

    int loop_count = 0;
    while(true)
    {
        cyw43_arch_poll();
        if (loop_count >= 10) {
            read_temperature();
            loop_count = 0;
        }
        loop_count++;
        sleep_ms(100);
    }

}