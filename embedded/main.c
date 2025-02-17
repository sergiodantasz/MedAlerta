#include <stdio.h>

#include "b64/cencode.h"
#include "cjson/cJSON.h"
#include "display/ssd1306.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "inc/bitmaps.h"
#include "lwip/dns.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

#define BUZZER_PIN 21
#define BUZZER_FREQUENCY 2000

#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6

#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"

#define USERNAME "USERNAME"
#define PASSWORD "PASSWORD"

#define API_HOST "med-alerta.vercel.app"
#define API_PORT 443
#define API_URL "/api/medicine/"

ssd1306_t ssd_bm;

static volatile int dns_resolved = 0;

static volatile bool is_request_in_progress = false;
static volatile bool is_alert_running = false;

bool is_button_A_pressed = false;
bool is_button_B_pressed = false;

bool is_timer_active = false;

struct repeating_timer request_timer;

static char response_buffer[4096];
static size_t response_len = 0;

uint medicine_id_value;

char credentials[64];
char encoded_credentials[128];

typedef enum {
    STATUS_TAKEN,
    STATUS_PENDING,
    STATUS_NOT_TAKEN
} MedicineStatus;

const char *status_to_string(MedicineStatus status) {
    switch (status) {
        case STATUS_TAKEN:
            return "Taken";
        case STATUS_PENDING:
            return "Pending";
        case STATUS_NOT_TAKEN:
            return "Not taken";
    }
}

typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_UNKNOWN
} HttpMethod;

static HttpMethod current_http_method = HTTP_METHOD_UNKNOWN;

bool is_button_pressed(uint gpio) {
    if (!gpio_get(gpio)) {
        busy_wait_ms(50);
        if (!gpio_get(gpio)) {
            return true;
        }
    }
    return false;
}

bool request_timer_callback(struct repeating_timer *t);
void start_request_timer();
void stop_request_timer();
void post_medicine(uint id, MedicineStatus status);

void clear_response_buffer() {
    response_len = 0;
    memset(response_buffer, 0, sizeof(response_buffer));
}

void encode_to_base64(char *input, char *output) {
    base64_encodestate state;
    base64_init_encodestate(&state);
    int len = base64_encode_block(input, strlen(input), output, &state);
    len += base64_encode_blockend(output + len, &state);
    output[len] = '\0';
}

void setup_buzzer_pwm() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096));
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

void setup_display() {
    i2c_init(I2C_PORT, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init();
}

void show_bitmap_with_delay(const uint8_t *bitmap, uint delay_ms) {
    ssd1306_draw_bitmap(&ssd_bm, bitmap);
    busy_wait_ms(delay_ms);
}

bool check_http_status_code(const char *response, int expected_code) {
    char response_copy[4096];
    strncpy(response_copy, response, sizeof(response_copy) - 1);
    response_copy[sizeof(response_copy) - 1] = '\0';
    char *status_line = strtok(response_copy, "\r\n");
    if (status_line == NULL) {
        return false;
    }
    char *http_version = strtok(status_line, " ");
    char *status_code_str = strtok(NULL, " ");
    if (status_code_str == NULL) {
        return false;
    }
    int status_code = atoi(status_code_str);
    return (status_code == expected_code);
}

void buzzer_on() {
    pwm_set_gpio_level(BUZZER_PIN, 1024);
}

void buzzer_off() {
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

static err_t http_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p != NULL) {
        struct pbuf *q;
        for (q = p; q != NULL; q = q->next) {
            size_t remaining = sizeof(response_buffer) - response_len - 1;
            if (remaining <= 0) {
                break;
            }
            size_t to_copy = q->len < remaining ? q->len : remaining;
            memcpy(response_buffer + response_len, q->payload, to_copy);
            response_len += to_copy;
        }
        response_buffer[response_len] = '\0';
        tcp_recved(tpcb, p->tot_len);
        pbuf_free(p);
        if (strstr(response_buffer, "\r\n\r\n") != NULL) {
            if (check_http_status_code(response_buffer, 401)) {
                clear_response_buffer();
                tcp_close(tpcb);
                show_bitmap_with_delay(bitmap_invalid_credentials, 4000);
                is_request_in_progress = false;
                return ERR_OK;
            }
            char *body = strstr(response_buffer, "\r\n\r\n");
            if (body != NULL) {
                body += 4;
                cJSON *json_object = cJSON_Parse(body);
                if (json_object != NULL) {
                    if (current_http_method == HTTP_METHOD_GET) {
                        if (check_http_status_code(response_buffer, 200)) {
                            show_bitmap_with_delay(bitmap_medicine_found, 4000);
                            cJSON *medicine_id = cJSON_GetObjectItem(json_object, "id");
                            cJSON *has_time_reached = cJSON_GetObjectItem(json_object, "has_time_reached");
                            if (medicine_id != NULL && cJSON_IsNumber(medicine_id) && has_time_reached != NULL && cJSON_IsBool(has_time_reached)) {
                                medicine_id_value = medicine_id->valueint;
                                bool time_reached = cJSON_IsTrue(has_time_reached);
                                if (time_reached) {
                                    buzzer_on();
                                    show_bitmap_with_delay(bitmap_medicine_is_now, 4000);
                                    show_bitmap_with_delay(bitmap_mark_as, 0);
                                    stop_request_timer();
                                    is_alert_running = true;
                                }
                            }
                            cJSON_Delete(json_object);
                        } else if (check_http_status_code(response_buffer, 404)) {
                            show_bitmap_with_delay(bitmap_no_medicine_found, 4000);
                        }
                    } else if (current_http_method == HTTP_METHOD_POST) {
                        if (check_http_status_code(response_buffer, 200)) {
                            show_bitmap_with_delay(bitmap_status_updated, 4000);
                        } else if (check_http_status_code(response_buffer, 404)) {
                            show_bitmap_with_delay(bitmap_error, 4000);
                        }
                    }
                }
            }
            clear_response_buffer();
            tcp_close(tpcb);
            current_http_method = HTTP_METHOD_UNKNOWN;
            is_request_in_progress = false;
        }
    } else {
        clear_response_buffer();
        tcp_close(tpcb);
        current_http_method = HTTP_METHOD_UNKNOWN;
        is_request_in_progress = false;
    }
    return ERR_OK;
}

static err_t http_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    return ERR_OK;
}

static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *arg) {
    ip_addr_t *resolved_ip = (ip_addr_t *)arg;
    if (ipaddr && ipaddr->addr != 0) {
        *resolved_ip = *ipaddr;
        dns_resolved = 1;
        printf("Resolved IP: %s\n", inet_ntoa(*ipaddr));
    } else {
        dns_resolved = -1;
    }
}

struct tcp_pcb *get_tcp_connection() {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        return NULL;
    }
    ip_addr_t server_ip;
    err_t err;
    err = dns_gethostbyname(API_HOST, &server_ip, dns_callback, &server_ip);
    if (err == ERR_OK) {
        dns_resolved = 1;
    } else if (err == ERR_INPROGRESS) {
        while (dns_resolved == 0) {
            tight_loop_contents();
        }
    }
    if (dns_resolved == -1) {
        return NULL;
    }
    err = tcp_connect(pcb, &server_ip, API_PORT, NULL);
    if (err != ERR_OK) {
        tcp_close(pcb);
        return NULL;
    }
    tcp_recv(pcb, http_client_recv);
    tcp_sent(pcb, http_client_sent);
    return pcb;
}

void http_request(const char *request) {
    if (is_request_in_progress) {
        return;
    }
    is_request_in_progress = true;
    clear_response_buffer();
    struct tcp_pcb *pcb = get_tcp_connection();
    if (!pcb) {
        is_request_in_progress = false;
        return;
    }
    err_t err = tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        tcp_close(pcb);
        is_request_in_progress = false;
        return;
    }
    tcp_output(pcb);
}

void get_medicine() {
    current_http_method = HTTP_METHOD_GET;
    char request[256];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Authorization: Basic %s\r\n"
             "Connection: close\r\n"
             "Accept: application/json\r\n\r\n",
             API_URL, API_HOST, API_PORT, encoded_credentials);
    http_request(request);
}

void post_medicine(uint id, MedicineStatus status) {
    current_http_method = HTTP_METHOD_POST;
    char request[256];
    char data[256];
    const char *status_str = status_to_string(status);
    snprintf(data, sizeof(data), "{\"id\": \"%u\", \"status\": \"%s\"}", id, status_str);
    snprintf(request, sizeof(request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Authorization: Basic %s\r\n"
             "Connection: close\r\n"
             "Content-Type: application/json\r\n"
             "Accept: application/json\r\n"
             "Content-Length: %d\r\n\r\n"
             "%s",
             API_URL, API_HOST, API_PORT, encoded_credentials, strlen(data), data);
    http_request(request);
}

void setup_wifi() {
    while (true) {
        show_bitmap_with_delay(bitmap_initializing_wifi, 2000);
        if (cyw43_arch_init() == 0) {
            break;
        }
        show_bitmap_with_delay(bitmap_error_initializing_wifi, 2000);
        show_bitmap_with_delay(bitmap_trying_again, 2000);
    }
    show_bitmap_with_delay(bitmap_initialized_wifi, 2000);
    cyw43_arch_enable_sta_mode();
    while (true) {
        show_bitmap_with_delay(bitmap_connecting_to_wifi, 0);
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000) == 0) {
            break;
        }
        show_bitmap_with_delay(bitmap_error_connecting_to_wifi, 2000);
        show_bitmap_with_delay(bitmap_trying_again, 2000);
    }
    show_bitmap_with_delay(bitmap_connected_to_wifi, 4000);
}

void setup_buttons() {
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);
}

bool request_timer_callback(struct repeating_timer *t) {
    while (is_request_in_progress) {
        tight_loop_contents();
    }
    show_bitmap_with_delay(bitmap_requesting_api, 0);
    get_medicine();
    return true;
}

void start_request_timer() {
    if (!is_timer_active) {
        add_repeating_timer_ms(10000, request_timer_callback, NULL, &request_timer);
        is_timer_active = true;
    }
}

void stop_request_timer() {
    if (is_timer_active) {
        cancel_repeating_timer(&request_timer);
        is_timer_active = false;
    }
}

int main() {
    stdio_init_all();
    setup_buttons();
    setup_buzzer_pwm();
    setup_display();
    ssd1306_init_bm(&ssd_bm, 128, 64, false, 0x3C, I2C_PORT);
    ssd1306_config(&ssd_bm);
    show_bitmap_with_delay(bitmap_welcome, 4000);
    setup_wifi();
    snprintf(credentials, sizeof(credentials), "%s:%s", USERNAME, PASSWORD);
    encode_to_base64(credentials, encoded_credentials);
    request_timer_callback(NULL);
    start_request_timer();
    while (true) {
        if (is_alert_running) {
            is_button_A_pressed = !gpio_get(BUTTON_A_PIN);
            is_button_B_pressed = !gpio_get(BUTTON_B_PIN);
            if (is_button_A_pressed || is_button_B_pressed) {
                MedicineStatus status;
                if (is_button_A_pressed) {
                    status = STATUS_TAKEN;
                } else if (is_button_B_pressed) {
                    status = STATUS_NOT_TAKEN;
                }
                post_medicine(medicine_id_value, status);
                is_alert_running = false;
                buzzer_off();
                start_request_timer();
            }
            busy_wait_ms(50);
        }
        busy_wait_ms(100);
    }
    cyw43_arch_deinit();
    return 0;
}
