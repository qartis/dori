#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "uart.h"
#include "spi.h"
#include "mcp2515.h"

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

void periodic_callback(void)
{
    (void)0;
}

void mcp2515_irq_callback(void)
{
    uint8_t i;


#define X(name, value) static const char  temp_type_ ## name[] PROGMEM =  "TYPE_" #name;
TYPE_LIST(X)
#undef X

#define X(name, value) static const char  temp_id_ ## name[] PROGMEM = "ID_" #name;
ID_LIST(X)
#undef X

    static const char * type_names []	 = {
#define X(name, value) temp_type_ ##name,

TYPE_LIST(X)
#undef X
    };

   static const char *  id_names []  = {
#define X(name, value) [value] = temp_id_ ##name,

ID_LIST(X)
#undef X
    };

    printf_P(PSTR("Sniffer received %S [%x] %S [%x] %db: "),type_names[packet.type] , packet.type, id_names[packet.id], packet.id, packet.len);
    //printf_P(PSTR("Sniffer received [%x] [%x] %db: "),  packet.type, packet.id, packet.len);
    for (i = 0; i < packet.len; i++) {
        printf_P(PSTR("%x,"), packet.data[i]);
    }
    printf_P(PSTR("\n"));


}

void usage(void)
{
    printf_P(PSTR("cmds: send, ...\n"));
}

void send_usage(void)
{
    printf_P(PSTR("send type id [02 ff ab ..]\n"));
	 printf_P(PSTR("valid types:"));

#define X(name, value)  printf_P(PSTR(" " #name));
TYPE_LIST(X)
#undef X
	
	printf_P(PSTR("\nvalid ids:"));
#define X(name, value)  printf_P(PSTR(" " #name));
ID_LIST(X)
#undef X
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
	 test_pins();
    int buflen = 64;
    char buf[buflen];
    uint8_t i = 0;

    uart_init(BAUD(38400));
    //spi_init();

    _delay_ms(200);
    printf("sniffer start\n");

    while (mcp2515_init()) {
        printf_P(PSTR("mcp: init\n"));
        _delay_ms(500);
    }

    sei();

    uint8_t sendbuf[10] = {0};
    uint8_t type = 0;
    uint8_t id = 0;
    uint8_t rc;
    char *arg;
    for (;;) {
        printf_P(PSTR("> "));

        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf)-1] = '\0';

        arg = strtok(buf, " ");

        if (strcmp(arg, "send") == 0) {
            arg = strtok(NULL, " ");
            if (arg == NULL) {
                send_usage();
                continue;
            }
            type = parse_arg(arg);

            arg = strtok(NULL, " ");
            if (arg == NULL) {
                send_usage();
                continue;
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
            usage();
        }
    }
}
