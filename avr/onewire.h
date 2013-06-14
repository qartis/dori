// rom-code size including CRC
#define OW_ROMCODE_SIZE 8

#define MAXSENSORS 10

extern uint8_t sensor_ids[MAXSENSORS][OW_ROMCODE_SIZE];
uint8_t search_sensors(void);


#define OW_PIN  PD7
#define OW_IN   PIND
#define OW_OUT  PORTD
#define OW_DDR  DDRD
#define OW_CONF_DELAYOFFSET 0

// Recovery time (T_Rec) minimum 1usec - increase for long lines 
// 5 usecs is a value give in some Maxim AppNotes
// 30u secs seem to be reliable for longer lines
#define OW_RECOVERY_TIME         10

#define OW_MATCH_ROM    0x55
#define OW_SKIP_ROM     0xCC
#define OW_SEARCH_ROM   0xF0

#define OW_SEARCH_FIRST 0xFF        // start new search
#define OW_PRESENCE_ERR 0xFF
#define OW_DATA_ERR     0xFE
#define OW_LAST_DEVICE  0x00        // last device found


extern uint8_t ow_reset(void);

extern uint8_t ow_bit_io( uint8_t b );
extern uint8_t ow_byte_wr( uint8_t b );
extern uint8_t ow_byte_rd( void );

extern uint8_t ow_rom_search( uint8_t diff, uint8_t *id );

extern void ow_command( uint8_t command, uint8_t *id );
extern void ow_command_with_parasite_enable( uint8_t command, uint8_t *id );

extern void ow_parasite_enable( void );
extern void ow_parasite_disable( void );
extern uint8_t ow_input_pin_state( void );
