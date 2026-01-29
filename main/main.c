#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_timer.h"

//gpio definitions

// inputs
#define DRI_SEAT_WHITE      GPIO_NUM_6 
#define PASS_SEAT_BLUE      GPIO_NUM_15
#define DRI_BELT_YELLOW     GPIO_NUM_5
#define PASS_BELT_RED       GPIO_NUM_7
#define IGNI_BUTTON_GREEN   GPIO_NUM_4
#define HIGH_BEAM_SWITCH    GPIO_NUM_13

//outputs
#define GREEN_LED           GPIO_NUM_1
#define RED_LED             GPIO_NUM_2
#define BUZZER              GPIO_NUM_18
#define L_LOW               GPIO_NUM_16
#define R_LOW               GPIO_NUM_17
#define L_HIGH              GPIO_NUM_38
#define R_HIGH              GPIO_NUM_39 


// ADC channels
#define LIGHT_SENSOR_ADC    ADC1_CHANNEL_7   // GPIO7
#define MODE_SELECTOR_ADC   ADC1_CHANNEL_9   // GPIO9

//constants
#define DAYLIGHT_THRESHOLD  3000
#define DUSK_THRESHOLD      1500
#define AUTO_ON_DELAY_US    1000000 //1 second delay
#define AUTO_OFF_DELAY_US   2000000 //2 second delay

//variables
static bool engine_running = false;
static bool headlights_on = false;
static bool last_driver_seated = false;
static int64_t light_change_time = 0;

//helper functions
void set_low_beams(bool on) {
    gpio_set_level(L_LOW, on);
    gpio_set_level(R_LOW, on);
}

void set_high_beams(bool on) {
    gpio_set_level(L_HIGH, on);
    gpio_set_level(R_HIGH, on);
}

//gpio setup
void setup_gpio(void) {

    // inputs
    gpio_set_direction(DRI_SEAT_WHITE, GPIO_MODE_INPUT);
    gpio_set_direction(PASS_SEAT_BLUE, GPIO_MODE_INPUT);
    gpio_set_direction(DRI_BELT_YELLOW, GPIO_MODE_INPUT);
    gpio_set_direction(PASS_BELT_RED, GPIO_MODE_INPUT);
    gpio_set_direction(IGNI_BUTTON_GREEN, GPIO_MODE_INPUT);
    gpio_set_direction(HIGH_BEAM_SWITCH, GPIO_MODE_INPUT);

    gpio_set_pull_mode(DRI_SEAT_WHITE, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PASS_SEAT_BLUE, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(DRI_BELT_YELLOW, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PASS_BELT_RED, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(IGNI_BUTTON_GREEN, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(HIGH_BEAM_SWITCH, GPIO_PULLUP_ONLY);

    // outputs
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BUZZER, GPIO_MODE_OUTPUT);
    gpio_set_direction(L_LOW, GPIO_MODE_OUTPUT);
    gpio_set_direction(R_LOW, GPIO_MODE_OUTPUT);
    gpio_set_direction(L_HIGH, GPIO_MODE_OUTPUT);
    gpio_set_direction(R_HIGH, GPIO_MODE_OUTPUT);

    // all outputs set to OFF
    gpio_set_level(GREEN_LED, 0);
    gpio_set_level(RED_LED, 0);
    gpio_set_level(BUZZER, 0);
    set_low_beams(false);
    set_high_beams(false);
}

//main code
void app_main(void) {

    setup_gpio();

    //ADC setup
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(LIGHT_SENSOR_ADC, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(MODE_SELECTOR_ADC, ADC_ATTEN_DB_11);

    printf("System initialized.\n");

    while (1) {

        // active-low inputs
        bool driver_seat    = (gpio_get_level(DRI_SEAT_WHITE) == 0);
        bool passenger_seat = (gpio_get_level(PASS_SEAT_BLUE) == 0);
        bool driver_belt    = (gpio_get_level(DRI_BELT_YELLOW) == 0);
        bool passenger_belt = (gpio_get_level(PASS_BELT_RED) == 0);
        bool ignition_press = (gpio_get_level(IGNI_BUTTON_GREEN) == 0);

        // welcome message when driver sits down
        if (driver_seat && !last_driver_seated) {
            printf("Welcome to enhanced alarm system model 218-W26\n");
        }
        last_driver_seated = driver_seat;

        // green LED indicates the readiness to start the engine
        bool ready = driver_seat && passenger_seat && driver_belt && passenger_belt && !engine_running;

        gpio_set_level(GREEN_LED, ready);

        // ignition button handling
        if (ignition_press) {
            vTaskDelay(pdMS_TO_TICKS(200)); // debounce

            if (!engine_running) {

                if (ready) {
                    engine_running = true;
                    gpio_set_level(RED_LED, 1);
                    printf("Engine started.\n");
                } else {
                    printf("Ignition inhibited.\n");
                    gpio_set_level(BUZZER, 1);
                    vTaskDelay(pdMS_TO_TICKS(300));
                    gpio_set_level(BUZZER, 0);

                    if (!driver_seat)    printf("- Driver seat not occupied\n");
                    if (!passenger_seat) printf("- Passenger seat not occupied\n");
                    if (!driver_belt)    printf("- Driver seatbelt not fastened\n");
                    if (!passenger_belt) printf("- Passenger seatbelt not fastened\n");
                }

            } else {
                //stop the engine 
                engine_running = false;
                gpio_set_level(RED_LED, 0);
                set_low_beams(false);
                set_high_beams(false);
                headlights_on = false;
                printf("Engine stopped.\n");
            }

        
            vTaskDelay(pdMS_TO_TICKS(300));
        }

        // headlights only when engine is running
        if (!engine_running) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        int mode  = adc1_get_raw(MODE_SELECTOR_ADC);
        int light = adc1_get_raw(LIGHT_SENSOR_ADC);

        // OFF
        if (mode < 1000) {
            set_low_beams(false);
            headlights_on = false;
        }
        // ON
        else if (mode > 3000) {
            set_low_beams(true);
            headlights_on = true;
        }
        // AUTO
        else {
            int64_t now = esp_timer_get_time();

            if (light > DAYLIGHT_THRESHOLD && headlights_on) {
                if (light_change_time == 0) light_change_time = now;
                if (now - light_change_time > AUTO_OFF_DELAY_US) {
                    set_low_beams(false);
                    headlights_on = false;
                    light_change_time = 0;
                }
            }
            else if (light < DUSK_THRESHOLD && !headlights_on) {
                if (light_change_time == 0) light_change_time = now;
                if (now - light_change_time > AUTO_ON_DELAY_US) {
                    set_low_beams(true);
                    headlights_on = true;
                    light_change_time = 0;
                }
            }
            else {
                light_change_time = 0;
            }
        }

        // high beams
        bool high_beam = (gpio_get_level(HIGH_BEAM_SWITCH) == 0);
        set_high_beams(high_beam && headlights_on);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}