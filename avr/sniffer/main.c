#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "time.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "node.h"

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
    printf_P(PSTR("cmds: send, ...\n"));
}

void show_send_usage(void)
{
    printf_P(PSTR("send type id [02 ff ab ..]\n"));
	 printf_P(PSTR("valid types:"));

#if 0
#define X(name, value)  printf_P(PSTR(" " #name));
TYPE_LIST(X)
#undef X
	
	printf_P(PSTR("\nvalid ids:"));
#define X(name, value)  printf_P(PSTR(" " #name));
ID_LIST(X)
#undef X
#endif
	printf_P(PSTR("\n"));
}


uint8_t parse_arg(const char *arg)
{
    /* first check all the type names*/
#define X(name, value) if (streq_P(arg, PSTR(#name))) return TYPE_ ##name; \

    TYPE_LIST(X)
#undef X

    /* then try all the id names*/
#define X(name, value) if (streq_P(arg, PSTR(#name))) return ID_ ##name; \

    ID_LIST(X)
#undef X

    /* otherwise maybe it's a 2-digit hex */
    if (strlen(arg) == 2) {
        return 16 * from_hex(arg[0]) + from_hex(arg[1]);
    }

    /* otherwise maybe it's a 1-digit hex */
    if (strlen(arg) == 1) {
        return from_hex(arg[0]);
    }

    printf("WTF! given arg '%s'\n", arg);
    return 0xff;
}

void uart_irq(void)
{
    char buf[64];
    char *arg;

    uint8_t i = 0;
    uint8_t sendbuf[10] = {0};
    uint8_t type = 0;
    uint8_t id = 0;
    uint8_t rc;

    //uart_getbuf(buf);
    fgets(buf, sizeof(buf), stdin);
	 arg = strtok(buf, " ");
    if (strcmp(arg, "send") == 0) {
        arg = strtok(NULL, " ");
        if (arg == NULL) {
            show_send_usage();
            return;
        }
        type = parse_arg(arg);

        arg = strtok(NULL, " ");
        if (arg == NULL) {
            show_send_usage();
            return;
        }
        id = parse_arg(arg);

        i = 0;
        for (;;) {
            arg = strtok(NULL, " ");
            if (arg == NULL) {
                break;
            }
            sendbuf[i] = parse_arg(arg);
            i++;
        }

        rc = mcp2515_send(type, id, i, sendbuf);
        _delay_ms(200);
        printf_P(PSTR("mcp2515_send: %d\n"), rc);
    } else {
        show_usage();
    }
}

void periodic_irq(void)
{
    (void)0;
}

void can_irq(void)
{
    uint8_t i;
	 packet.unread = 0;

#define X(name, value) static char const temp_type_ ## name [] PROGMEM = #name;
TYPE_LIST(X)
#undef X

#define X(name, value) static char const temp_id_ ## name [] PROGMEM = #name;
ID_LIST(X)
#undef X


    static char const unknown_string [] PROGMEM = "???";

    static PGM_P const type_names[] PROGMEM = {
        [0 ... 128] = unknown_string,
#define X(name, value) [value] = temp_type_ ##name,
TYPE_LIST(X)
#undef X
    };


    static PGM_P const id_names[] PROGMEM = {
        [0 ... 128] = unknown_string,
#define X(name, value) [value] = temp_id_ ##name,
ID_LIST(X)
#undef X
    };


    printf_P(PSTR("Sniffer received %S [%x] %S [%x] %db: "),
        (PGM_P)pgm_read_word(&(type_names[packet.type])),
        packet.type,
        (PGM_P)pgm_read_word(&(id_names[packet.id])),
        packet.id,
        packet.len);

    for (i = 0; i < packet.len; i++) {
        printf_P(PSTR("%x,"), packet.data[i]);
    }
    printf_P(PSTR("\n"));
}

void test_pins(void)
{
		  DDRC = 0b11111111;        //set all pins of port c as outputs
		  DDRD = 0b11111111;        //set all pins of port d as outputs
		  PORTD = 0xff;
		  PORTC = 0xff;
}
		  
void main(void)
{
    NODE_INIT();

    for (;;) {
        puts_P(PSTR("\n" XSTR(MY_ID) "> "));

        while (irq_signal == 0) {};

        if (irq_signal & IRQ_CAN) {
            can_irq();
            irq_signal &= ~IRQ_CAN;
        }

        if (irq_signal & IRQ_TIMER) {
            periodic_irq();
            irq_signal &= ~IRQ_TIMER;
        }

        if (irq_signal & IRQ_UART) {
            uart_irq();
            irq_signal &= ~IRQ_UART;
        }

    }
}
