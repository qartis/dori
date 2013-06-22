#define _USE_READ   1   /* 1:Enable pf_read() */

#define _USE_DIR    1   /* 1:Enable pf_opendir() and pf_readdir() */

#define _USE_LSEEK  0   /* 1:Enable pf_lseek() */

#define _USE_WRITE  1   /* 1:Enable pf_write() */

#define _FS_FAT12   0   /* 1:Enable FAT12 support */


#define CLUST   uint16_t


/* File system object structure */

typedef struct {
    uint8_t  fs_type;    /* FAT sub type */
    uint8_t  flag;       /* File status flags */
    uint8_t  csize;      /* Number of sectors per cluster */
    uint8_t  pad1;
    uint16_t n_rootdir;  /* Number of root directory entries (0 on FAT32) */
    CLUST    n_fatent;   /* Number of FAT entries (= number of clusters + 2) */
    uint32_t fatbase;    /* FAT start sector */
    uint32_t dirbase;    /* Root directory start sector (Cluster# on FAT32) */
    uint32_t database;   /* Data start sector */
    uint32_t fptr;       /* File R/W pointer */
    uint32_t fsize;      /* File size */
    CLUST    org_clust;  /* File start cluster */
    CLUST    curr_clust; /* File current cluster */
    uint32_t dsect;      /* File current data sector */
} FATFS;


extern FATFS fs;


/* Directory object structure */

typedef struct {
    uint16_t    index;      /* Current read/write index number */
    uint8_t*    fn;         /* Pointer to the SFN (in/out) {file[8],ext[3],status[1]} */
    CLUST   sclust;     /* Table start cluster (0:Static table) */
    CLUST   clust;      /* Current cluster */
    uint32_t    sect;       /* Current sector */
} DIR;



/* File status structure */

typedef struct {
    uint32_t    fsize;      /* File size */
    uint16_t    fdate;      /* Last modified date */
    uint16_t    ftime;      /* Last modified time */
    uint8_t fattrib;    /* Attribute */
    char    fname[13];  /* File name */
} FILINFO;



/* File function return code (FRESULT) */

typedef enum {
    FR_OK = 0,          /* 0 */
    FR_DISK_ERR,        /* 1 */
    FR_NOT_READY,       /* 2 */
    FR_NO_FILE,         /* 3 */
    FR_NO_PATH,         /* 4 */
    FR_NOT_OPENED,      /* 5 */
    FR_NOT_ENABLED,     /* 6 */
    FR_NO_FILESYSTEM    /* 7 */
} FRESULT;


FRESULT pf_mount(void);
FRESULT pf_open(const char*);                  /* Open a file */
FRESULT pf_read(void*, uint16_t, uint16_t*);           /* Read data from the open file */
FRESULT pf_write(const void*, uint16_t, uint16_t*);    /* Write data to the open file */
FRESULT pf_lseek(uint32_t);                        /* Move file pointer of the open file */
FRESULT pf_opendir(DIR*, const char*);         /* Open a directory */
FRESULT pf_readdir(DIR*, FILINFO*);            /* Read a directory item from the open directory */


/* Flags and offset address                                     */
/* File status flag (FATFS.flag) */
#define FA_OPENED   0x01
#define FA_WPRT     0x02
#define FA__WIP     0x40

/* FAT sub type (FATFS.fs_type) */
#define FS_FAT12    1
#define FS_FAT16    2
#define FS_FAT32    3


/* File attribute bits for directory entry */
#define AM_RDO  0x01    /* Read only */
#define AM_HID  0x02    /* Hidden */
#define AM_SYS  0x04    /* System */
#define AM_VOL  0x08    /* Volume label */
#define AM_LFN  0x0F    /* LFN entry */
#define AM_DIR  0x10    /* Directory */
#define AM_ARC  0x20    /* Archive */
#define AM_MASK 0x3F    /* Mask of defined bits */


/*--------------------------------*/
/* Multi-byte word access macros  */

#define LD_16(ptr)      (uint16_t)(*(uint16_t*)(uint8_t*)(ptr))
#define LD_32(ptr)      (uint32_t)(*(uint32_t*)(uint8_t*)(ptr))
#define ST_16(ptr,val)  *(uint16_t*)(uint8_t*)(ptr)=(uint16_t)(val)
#define ST_32(ptr,val)  *(uint32_t*)(uint8_t*)(ptr)=(uint32_t)(val)
