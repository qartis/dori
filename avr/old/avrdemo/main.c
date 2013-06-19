#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>

#include "uart.h"
#include "onewire.h"
#include "ds18x20.h"
#include "motor.h"

void _delay_ms_long(uint16_t arg)
{
    if (arg > 6000) {
        _delay_ms(6000);
        _delay_ms(arg - 6000);
    } else {
        _delay_ms(arg);
    }
}

void adc_init(void)
{
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel)
{
    ADMUX = (1 << REFS0) | channel; /* internal vref */

    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) {};

    uint8_t low = ADCL;
    uint8_t high = ADCH;
    return ((uint16_t)(high<<8)) | low;
}

uint16_t get_arm_angle(void)
{
    uint16_t val;

    val = adc_read(4);

    if (val < 30) {
        val = 0;
    } else {
        val -= 30;
    }

    printf("angle %d\n", val);

    return val;
}

int main(void)
{
    uint8_t nSensors;
    uint16_t i;
    int16_t decicelsius;
    char s[32], prev[32];
    char c, pos;

    uart_init(BAUD(9600));
    adc_init();
    motor_init();

    sei();

    PORTC = 0;
    PORTB = 0;
    PORTD = 0;
    DDRC = 0b11101111;
    DDRB = 0xff;
    DDRD = 0xff;

restart:
    nSensors = search_sensors();
    printf("%d sensors found\n",nSensors);

    if (nSensors == 0) {
        _delay_ms(500);
        goto restart;
    }

    int16_t left, right;

    for (;;) {
        c = getchar();

        switch (c) {
        case 't':
            if (DS18X20_start_meas(DS18X20_POWER_PARASITE, NULL)) {
                print("short circuit?\n");
                goto restart;
            }

            _delay_ms(DS18B20_TCONV_12BIT);
            for (i = 0; i < nSensors; i++) {
                if (DS18X20_read_decicelsius(&sensor_ids[i][0], &decicelsius)) {
                    print("CRC Error\n");
                    goto restart;
                }

                prints(decicelsius);
                uart_putchar('\n');
                //print_temp(decicelsius, s, 10);
                //printf("%s\n",s);
            }
            break;

        case 'm':
            fgets(s, sizeof(s), stdin);

            s[strlen(s) - 1] = '\0';
            if (s[strlen(s) - 1] == 'x') {
                printf("cancelled\n");
                continue;
            }

            strcpy(prev, s);

            sscanf(s, "%d %d", &left, &right);

            if (left > 0) {
                PORTB |=  (1 << PORTB0);
            } else if (left < 0) {
                left = -left;
                PORTD |=  (1 << PORTD7);
            }

            if (right > 0) {
                PORTD |=  (1 << PORTD6);
            } else if (right < 0) {
                right = -right;
                PORTD |=  (1 << PORTD5);
            }

            if (left > right) {
                _delay_ms_long(right);
                PORTD &= ~(1 << PORTD6);
                PORTD &= ~(1 << PORTD5);
                _delay_ms_long(left - right);
            } else {
                _delay_ms_long(left);
                PORTB &= ~(1 << PORTB0);
                PORTD &= ~(1 << PORTD7);
                _delay_ms_long(right - left);
            }


            PORTB &= ~(1 << PORTB0);
            PORTD &= ~(1 << PORTD7);
            PORTD &= ~(1 << PORTD6);
            PORTD &= ~(1 << PORTD5);

            break;

        case 'd':
            PORTB |=  (1 << PORTB2);
            _delay_ms(50);
            PORTB &= ~(1 << PORTB2);
            get_arm_angle();
            break;

        case 'u':
            PORTB |=  (1 << PORTB1);
            _delay_ms(50);
            PORTB &= ~(1 << PORTB1);
            get_arm_angle();
            break;

        case 'p':
            pos = getchar();
            if (isdigit(pos)) {
                uint16_t goal = (pos - '0') * 61;
                uint16_t now = get_arm_angle();
                printf("goal: %d\n", goal);
                printf("now: %d\n", now);
                if ((now + 20) > goal && (now - 20) < goal) {
                    //nothing
                } else if (now > goal) {
                    PORTB |=  (1 << PORTB1);
                    while (get_arm_angle() > goal) { _delay_ms(10); };
                    PORTB &= ~(1 << PORTB1);
                } else {
                    PORTB |=  (1 << PORTB2);
                    while (get_arm_angle() < goal) { _delay_ms(10); };
                    PORTB &= ~(1 << PORTB2);
                }
            }
            break;

        case 'a':
            motor_cw();
            break;
        case 's':
            motor_ccw();
            break;
        case 'A':
            for (i = 0; i < 170 * 2; i++) {
                motor_cw();
                _delay_ms(10);
            }
            break;
        case 'S':
            for (i = 0; i < 170 * 2; i++) {
                motor_ccw();
                _delay_ms(10);
            }
            break;
        default:
            printf("wtf cmd\n");
            break;
        }
    }
}
