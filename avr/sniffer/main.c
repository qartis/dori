#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/sleep.h>
#include <avr/power.h>

#include "irq.h"
#include "time.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "node.h"
#include "can.h"

static char const unknown_string [] PROGMEM = "???";

#define X(name, value) static char const temp_type_ ## name [] PROGMEM = #name;
TYPE_LIST(X)
#undef X

#define X(name, value) static char const temp_id_ ## name [] PROGMEM = #name;
ID_LIST(X)
#undef X

#define X(name, value) static char const temp_sensor_ ## name [] PROGMEM = #name;
SENSOR_LIST(X)
#undef X

static PGM_P const type_names[] PROGMEM = {
    [0 ... 128] = unknown_string,
#define X(name, value) [value] = temp_type_ ##name,
    TYPE_LIST(X)
#undef X
};


static PGM_P const id_names[] PROGMEM = {
    [0 ... 64] = unknown_string,
#define X(name, value) [value] = temp_id_ ##name,
    ID_LIST(X)
#undef X
};


static PGM_P const sensor_names[] PROGMEM = {
    [0 ... 128] = unknown_string,
#define X(name, value) [value] = temp_sensor_ ##name,
    SENSOR_LIST(X)
#undef X
};



#define TOTAL_IDS 16
// there are different squelch levels.  0 is no squelch, 1 is squelch until next command , 2 is squelch until unsquelched.
uint8_t squelch = 0; 
uint8_t filter[TOTAL_IDS] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,};

uint8_t streq_P(const char *a, const char * PROGMEM b)
{
    return strcmp_P(a, b) == 0;
}

uint8_t from_hex(char a)
{
    if (isupper(a)) {
        return a - 'A' + 10;
    } else if (islower(a)) {
        return a - 'a' + 10;
    } else {
        return a - '0';
    }
}

void show_usage(void)
{
    putchar('?');
}

void show_send_usage(void)
{
    /*
    printf_P(PSTR("send type id.sensor [02 ff ab ..]\n"));
    printf_P(PSTR("valid types:"));

#if 1 
#define X(name, value)  printf_P(PSTR(" " #name));
TYPE_LIST(X)
#undef X
	
	printf_P(PSTR("\nvalid ids:"));
#define X(name, value)  printf_P(PSTR(" " #name));
ID_LIST(X)
#undef X
	printf_P(PSTR("\nvalid sensors:"));
#define X(name, value)  printf_P(PSTR(" " #name));
SENSOR_LIST(X)
#undef X
#endif
	printf_P(PSTR("\n"));
    */
}
void show_filter_usage(void)
{
    return;
    printf_P(PSTR("filter id [0/1]\n"));
#if 1
	printf_P(PSTR("\nvalid ids:"));
#define X(name, value)  printf_P(temp_id_##name); putchar(' ');
ID_LIST(X)
#undef X
#endif
	printf_P(PSTR("\n"));
}

uint8_t parse_arg(const char *arg)
{

    if (arg[0] == '\'' && arg[1] != '\0' && arg[2] == '\'')
        return arg[1];

    /* first check all the type names*/
#define X(name, value) if (streq_P(arg, temp_type_##name)) return TYPE_ ##name; \

    TYPE_LIST(X)
#undef X

    /* then try all the id names*/
#define X(name, value) if (streq_P(arg, temp_id_##name)) return ID_ ##name; \

    ID_LIST(X)
#undef X

    /* then try all the id names*/
#define X(name, value) if (streq_P(arg, temp_sensor_##name)) return SENSOR_ ##name; \

    SENSOR_LIST(X)
#undef X


    /* otherwise maybe it's a 2-digit hex */
    if (strlen(arg) == 2) {
        return 16 * from_hex(arg[0]) + from_hex(arg[1]);
    }

    /* otherwise maybe it's a 1-digit hex */
    if (strlen(arg) == 1) {
        return from_hex(arg[0]);
    }

    return 0xff;
}

uint8_t uart_irq(void)
{
    char buf[UART_BUF_SIZE];
    char *arg;

    uint8_t i = 0;
    uint8_t sendbuf[10] = {0};
    uint8_t type = 0;
    uint8_t id = 0;
    uint8_t rc;
    uint8_t enabled = 0;
    uint8_t sensor = 0;

    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';

    arg = strtok(buf, " ");

    if (arg == NULL) {
        mcp2515_send_wait(TYPE_xfer_cts, ID_cam, NULL, 0, 0);
    } else if (strcmp_P(arg, PSTR("send")) == 0) {
        arg = strtok(NULL, " ");
        if (arg == NULL) {
            //show_send_usage();
            goto uart_irq_end;
        }
        type = parse_arg(arg);
        arg = strtok(NULL, " ");
        if (arg == NULL) {
            //show_send_usage();
            goto uart_irq_end;
        }

        char *sensor_dot = strchr(arg, '.');
        if(sensor_dot) {
            *sensor_dot = '\0';
        }

        id = parse_arg(arg);
        sensor = 0;
        if(sensor_dot) {
            sensor = parse_arg(sensor_dot + 1);
        }

        i = 0;
        for (;;) {
            arg = strtok(NULL, " ");
            if (arg == NULL) {
                break;
            }
            sendbuf[i] = parse_arg(arg);
            i++;
        }

        if(type == TYPE_value_periodic &&
           sensor == SENSOR_time) {
            uint32_t new_time;
            new_time =
                (uint32_t)sendbuf[0] << 24 |
                (uint32_t)sendbuf[1] << 16 |
                (uint32_t)sendbuf[2] << 8  |
                (uint32_t)sendbuf[3] << 0;
            time_set(new_time);
        }


        if (squelch != 2){
            squelch = 0;  // so we can hear the reply.
        }
        rc = mcp2515_send_wait(type, id, sendbuf, i, sensor);
        _delay_ms(200);
        printf_P(PSTR("snd %d\n"), rc);
    } else if (strcmp_P(arg, PSTR("squelch")) == 0){
        arg = strtok(NULL, " ");
        if (arg == NULL) {
            squelch = squelch?0:1;
            goto uart_irq_end;
        }
        squelch = parse_arg(arg);
    } else if (strcmp_P(arg, PSTR("filter")) == 0){
        arg = strtok(NULL, " ");
        if (arg == NULL) {
            //show_filter_usage();
            goto uart_irq_end;
        }
        id = parse_arg(arg);

        if (id == ID_any) // disable filtering
        {
            arg = strtok(NULL, " ");
            if (arg == NULL) {
                enabled = 0;
            }
            else{
                enabled = parse_arg(arg);
            }
            for (i = 0; i < TOTAL_IDS; i++)
                filter[i] = enabled;
        }
        else
        {
            arg = strtok(NULL, " ");
            if (arg == NULL) {
                filter[id] = filter[id]?0:1;
                goto uart_irq_end;
            }
            filter[id] = parse_arg(arg);
        }
    } else if (strcmp_P(arg, PSTR("mcp")) == 0){
        mcp2515_dump();
    } else {
        show_usage();
    }
uart_irq_end:
    putchar('>');
    return 0;
}

uint8_t periodic_irq(void)
{
    return 0;
}

uint8_t can_irq(void)
{
    uint8_t i;

    packet.unread = 0;


    uint8_t len = packet.len;

    if (squelch == 0 && filter[packet.id] != 0){
        printf_P(PSTR("[%lu:%02lu:%02lu] %S [%x] %S [%x] %S [%x] %db: "),
            now/3600, (now/60) % 60, now % 60,
            (PGM_P)pgm_read_word(&(type_names[packet.type])),
            packet.type,
            (PGM_P)pgm_read_word(&(id_names[packet.id])),
            packet.id,
            (PGM_P)pgm_read_word(&(sensor_names[packet.sensor])),
            packet.sensor,
            len);

        for (i = 0; i < len; i++) {
            printf_P(PSTR("%x,"), packet.data[i]);
        }

        putchar('\n');
    }

    return 0;
}

int main(void)
{
    NODE_INIT();

    sei();

    NODE_MAIN();
}
