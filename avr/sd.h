/* Results of Disk Functions */
typedef enum {
    RES_OK = 0,     /* 0: Function succeeded */
    RES_ERROR,      /* 1: Disk error */
    RES_NOTRDY,     /* 2: Not ready */
    RES_PARERR      /* 3: Invalid parameter */
} disk_result_t;


/*---------------------------------------*/
/* Prototypes for disk control functions */

void sd_hw_init(void);
void sd_init(void);
uint8_t disk_initialize (void);
disk_result_t disk_readp (uint8_t *, uint32_t, uint16_t, uint16_t);
disk_result_t disk_writep (const uint8_t *, uint32_t);

#define SD_DESELECT() PORTD |=  (1 << PORTD7)
#define SD_SELECT()   PORTD &= ~(1 << PORTD7)

#define STA_NOINIT      0x01    /* Drive not initialized */
#define STA_NODISK      0x02    /* No medium in the drive */

/* Card type flags (CardType) */
#define CT_MMC              0x01    /* MMC ver 3 */
#define CT_SD1              0x02    /* SD ver 1 */
#define CT_SD2              0x04    /* SD ver 2 */
#define CT_SDC              (CT_SD1|CT_SD2) /* SD */
#define CT_BLOCK            0x08    /* Block addressing */

