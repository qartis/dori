#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include "fat.h"
#include "sd.h"


#define LD_CLUST(dir)   LD_16(dir+DIR_FstClusLO)


/* FatFs refers the members in the FAT structures with byte offset instead
/ of structure member because there are incompatibility of the packing option
/ between various compilers. */

#define BS_jmpBoot          0
#define BS_OEMName          3
#define BPB_BytsPerSec      11
#define BPB_SecPerClus      13
#define BPB_RsvdSecCnt      14
#define BPB_NumFATs         16
#define BPB_RootEntCnt      17
#define BPB_TotSec16        19
#define BPB_Media           21
#define BPB_FATSz16         22
#define BPB_SecPerTrk       24
#define BPB_NumHeads        26
#define BPB_HiddSec         28
#define BPB_TotSec32        32
#define BS_55AA             510

#define BS_DrvNum           36
#define BS_BootSig          38
#define BS_VolID            39
#define BS_VolLab           43
#define BS_FilSysType       54

#define BPB_FATSz32         36
#define BPB_ExtFlags        40
#define BPB_FSVer           42
#define BPB_RootClus        44
#define BPB_FSInfo          48
#define BPB_BkBootSec       50
#define BS_DrvNum32         64
#define BS_BootSig32        66
#define BS_VolID32          67
#define BS_VolLab32         71
#define BS_FilSysType32     82

#define MBR_Table           446

#define DIR_Name            0
#define DIR_Attr            11
#define DIR_NTres           12
#define DIR_CrtTime         14
#define DIR_CrtDate         16
#define DIR_FstClusHI       20
#define DIR_WrtTime         22
#define DIR_WrtDate         24
#define DIR_FstClusLO       26
#define DIR_FileSize        28


struct FatFS fs;


static CLUST get_fat (  /* 1:IO error, Else:Cluster status */
    CLUST clst  /* Cluster# to get the link information */
)
{
    uint8_t buf[4];

    if (clst < 2 || clst >= fs.n_fatent)    /* Range check */
        return 1;

    switch (fs.fs_type) {
    case FS_FAT16 :
        if (disk_readp(buf, fs.fatbase + clst / 256, (uint16_t)(((uint16_t)clst % 256) * 2), 2)) break;
        return LD_16(buf);
    }

    return 1;   /* An error occured at the disk I/O layer */
}


static
uint32_t clust2sect (   /* !=0: Sector number, 0: Failed - invalid cluster# */
    CLUST clst      /* Cluster# to be converted */
)
{
    clst -= 2;
    if (clst >= (fs.n_fatent - 2)) return 0;        /* Invalid cluster# */
    return (uint32_t)clst * fs.csize + fs.database;
}



static
FRESULT dir_rewind (
    DIR *dj         /* Pointer to directory object */
)
{
    CLUST clst;

    dj->index = 0;
    clst = dj->sclust;
    if (clst == 1 || clst >= fs.n_fatent)   /* Check start cluster range */
        return FR_DISK_ERR;

    dj->clust = clst;                       /* Current cluster */
    dj->sect = clst ? clust2sect(clst) : fs.dirbase;    /* Current sector */

    return FR_OK;   /* Seek succeeded */
}



static
FRESULT dir_next (  /* FR_OK:Succeeded, FR_NO_FILE:End of table */
    DIR *dj         /* Pointer to directory object */
)
{
    CLUST clst;
    uint16_t i;

    i = dj->index + 1;
    if (!i || !dj->sect)    /* Report EOT when index has reached 65535 */
        return FR_NO_FILE;

    if (!(i % 16)) {        /* Sector changed? */
        dj->sect++;         /* Next sector */

        if (dj->clust == 0) {   /* Static table */
            if (i >= fs.n_rootdir)  /* Report EOT when end of table */
                return FR_NO_FILE;
        }
        else {                  /* Dynamic table */
            if (((i / 16) & (fs.csize-1)) == 0) {   /* Cluster changed? */
                clst = get_fat(dj->clust);      /* Get next cluster */
                if (clst <= 1) return FR_DISK_ERR;
                if (clst >= fs.n_fatent)        /* When it reached end of dynamic table */
                    return FR_NO_FILE;          /* Report EOT */
                dj->clust = clst;               /* Initialize data for new cluster */
                dj->sect = clust2sect(clst);
            }
        }
    }

    dj->index = i;

    return FR_OK;
}



static
FRESULT dir_find (
    DIR *dj,        /* Pointer to the directory object linked to the file name */
    uint8_t *dir        /* 32-byte working buffer */
)
{
    FRESULT res;
    uint8_t c;


    res = dir_rewind(dj);           /* Rewind directory object */
    if (res != FR_OK) return res;

    do {
        res = disk_readp(dir, dj->sect, (uint16_t)((dj->index % 16) * 32), 32)  /* Read an entry */
            ? FR_DISK_ERR : FR_OK;
        if (res != FR_OK) break;
        c = dir[DIR_Name];  /* First character */
        if (c == 0) { res = FR_NO_FILE; break; }    /* Reached to end of table */
        if (!(dir[DIR_Attr] & AM_VOL) && !memcmp(dir, dj->fn, 11)) /* Is it a valid entry? */
            break;
        res = dir_next(dj);                 /* Next entry */
    } while (res == FR_OK);

    return res;
}



static
FRESULT dir_read (
    DIR *dj,        /* Pointer to the directory object to store read object name */
    uint8_t *dir        /* 32-byte working buffer */
)
{
    FRESULT res;
    uint8_t a, c;


    res = FR_NO_FILE;
    while (dj->sect) {
        res = disk_readp(dir, dj->sect, (uint16_t)((dj->index % 16) * 32), 32)  /* Read an entry */
            ? FR_DISK_ERR : FR_OK;
        if (res != FR_OK) break;
        c = dir[DIR_Name];
        if (c == 0) { res = FR_NO_FILE; break; }    /* Reached to end of table */
        a = dir[DIR_Attr] & AM_MASK;
        if (c != 0xE5 && c != '.' && !(a & AM_VOL)) /* Is it a valid entry? */
            break;
        res = dir_next(dj);         /* Next entry */
        if (res != FR_OK) break;
    }

    if (res != FR_OK) dj->sect = 0;

    return res;
}



static
FRESULT create_name (
    DIR *dj,            /* Pointer to the directory object */
    const char **path   /* Pointer to pointer to the segment in the path string */
)
{
    uint8_t c, ni, si, i, *sfn;
    const char *p;

    /* Create file name in directory form */
    sfn = dj->fn;
    memset(sfn, ' ', 11);
    si = i = 0; ni = 8;
    p = *path;
    for (;;) {
        c = p[si++];
        if (c <= ' ' || c == '/') break;    /* Break on end of segment */
        if (c == '.' || i >= ni) {
            if (ni != 8 || c != '.') break;
            i = 8; ni = 11;
            continue;
        }
        if (islower(c)) c -= 0x20;  /* toupper */
        sfn[i++] = c;
    }
    *path = &p[si];                     /* Rerurn pointer to the next segment */

    sfn[11] = (c <= ' ') ? 1 : 0;       /* Set last segment flag if end of path */

    return FR_OK;
}



static
void get_fileinfo (     /* No return code */
    DIR *dj,            /* Pointer to the directory object */
    uint8_t *dir,           /* 32-byte working buffer */
    FILINFO *fno        /* Pointer to store the file information */
)
{
    uint8_t i, c;
    char *p;

    p = fno->fname;
    if (dj->sect) {
        for (i = 0; i < 8; i++) {   /* Copy file name body */
            c = dir[i];
            if (c == ' ') break;
            if (c == 0x05) c = 0xE5;
            *p++ = c;
        }
        if (dir[8] != ' ') {        /* Copy file name extension */
            *p++ = '.';
            for (i = 8; i < 11; i++) {
                c = dir[i];
                if (c == ' ') break;
                *p++ = c;
            }
        }
        fno->fattrib = dir[DIR_Attr];               /* Attribute */
        fno->fsize = LD_32(dir+DIR_FileSize);   /* Size */
        fno->fdate = LD_16(dir+DIR_WrtDate);        /* Date */
        fno->ftime = LD_16(dir+DIR_WrtTime);        /* Time */
    }
    *p = 0;
}

static
FRESULT follow_path (   /* FR_OK(0): successful, !=0: error code */
    DIR *dj,            /* Directory object to return last directory and found object */
    uint8_t *dir,           /* 32-byte working buffer */
    const char *path    /* Full-path string to find a file or directory */
)
{
    FRESULT res;


    while (*path == ' ') path++;        /* Skip leading spaces */
    if (*path == '/') path++;           /* Strip heading separator */
    dj->sclust = 0;                     /* Set start directory (always root dir) */

    if ((uint8_t)*path <= ' ') {            /* Null path means the root directory */
        res = dir_rewind(dj);
        dir[0] = 0;

    } else {                            /* Follow path */
        for (;;) {
            res = create_name(dj, &path);   /* Get a segment */
            if (res != FR_OK) break;
            res = dir_find(dj, dir);        /* Find it */
            if (res != FR_OK) {             /* Could not find the object */
                if (res == FR_NO_FILE && !*(dj->fn+11))
                    res = FR_NO_PATH;
                break;
            }
            if (*(dj->fn+11)) break;        /* Last segment match. Function completed. */
            if (!(dir[DIR_Attr] & AM_DIR)) { /* Cannot follow because it is a file */
                res = FR_NO_PATH; break;
            }
            dj->sclust = LD_CLUST(dir);
        }
    }

    return res;
}



static
uint8_t check_fs (  /* 0:The FAT boot record, 1:Valid boot record but not an FAT, 2:Not a boot record, 3:Error */
    uint8_t *buf,   /* Working buffer */
    uint32_t sect   /* Sector# (lba) to check if it is an FAT boot record or not */
)
{
    if (disk_readp(buf, sect, 510, 2))      /* Read the boot sector */
        return 3;
    if (LD_16(buf) != 0xAA55)               /* Check record signature */
        return 2;

    if (!disk_readp(buf, sect, BS_FilSysType, 2) && LD_16(buf) == 0x4146)   /* Check FAT12/16 */
        return 0;

    return 1;
}



FRESULT pf_mount(void)
{
    uint8_t fmt, buf[36];
    uint32_t bsect, fsize, tsect, mclst;

    if (disk_initialize() & STA_NOINIT) /* Check if the drive is ready or not */
        return FR_NOT_READY;

    /* Search FAT partition on the drive */
    bsect = 0;
    fmt = check_fs(buf, bsect);         /* Check sector 0 as an SFD format */
    if (fmt == 1) {                     /* Not an FAT boot record, it may be FDISK format */
        /* Check a partition listed in top of the partition table */
        if (disk_readp(buf, bsect, MBR_Table, 16)) {    /* 1st partition entry */
            fmt = 3;
        } else {
            if (buf[4]) {                   /* Is the partition existing? */
                bsect = LD_32(&buf[8]); /* Partition offset in LBA */
                fmt = check_fs(buf, bsect); /* Check the partition */
            }
        }
    }
    if (fmt == 3) return FR_DISK_ERR;
    if (fmt) return FR_NO_FILESYSTEM;   /* No valid FAT patition is found */

    /* Initialize the file system object */
    if (disk_readp(buf, bsect, 13, sizeof(buf))) return FR_DISK_ERR;

    fsize = LD_16(buf+BPB_FATSz16-13);              /* Number of sectors per FAT */
    if (!fsize) fsize = LD_32(buf+BPB_FATSz32-13);

    fsize *= buf[BPB_NumFATs-13];                       /* Number of sectors in FAT area */
    fs.fatbase = bsect + LD_16(buf+BPB_RsvdSecCnt-13); /* FAT start sector (lba) */
    fs.csize = buf[BPB_SecPerClus-13];                  /* Number of sectors per cluster */
    fs.n_rootdir = LD_16(buf+BPB_RootEntCnt-13);        /* Nmuber of root directory entries */
    tsect = LD_16(buf+BPB_TotSec16-13);             /* Number of sectors on the file system */
    if (!tsect) tsect = LD_32(buf+BPB_TotSec32-13);
    mclst = (tsect                      /* Last cluster# + 1 */
        - LD_16(buf+BPB_RsvdSecCnt-13) - fsize - fs.n_rootdir / 16
        ) / fs.csize + 2;
    fs.n_fatent = (CLUST)mclst;

    fmt = FS_FAT16;                         /* Determine the FAT sub type */
    if (mclst < 0xFF7)                      /* Number of clusters < 0xFF5 */
#if _FS_FAT12
        fmt = FS_FAT12;
#else
        return FR_NO_FILESYSTEM;
#endif
    if (mclst >= 0xFFF7)                    /* Number of clusters >= 0xFFF5 */
        return FR_NO_FILESYSTEM;

    fs.fs_type = fmt;       /* FAT sub-type */
    fs.dirbase = fs.fatbase + fsize;                /* Root directory start sector (lba) */
    fs.database = fs.fatbase + fsize + fs.n_rootdir / 16;   /* Data start sector (lba) */

    fs.flag = 0;

    return FR_OK;
}

FRESULT pf_open (
    const char *path    /* Pointer to the file name */
)
{
    FRESULT res;
    DIR dj;
    uint8_t sp[12], dir[32];

    fs.flag = 0;
    dj.fn = sp;
    res = follow_path(&dj, dir, path);  /* Follow the file path */
    if (res != FR_OK) return res;       /* Follow failed */
    if (!dir[0] || (dir[DIR_Attr] & AM_DIR))    /* It is a directory */
        return FR_NO_FILE;

    fs.org_clust = LD_CLUST(dir);           /* File start cluster */
    fs.fsize = LD_32(dir+DIR_FileSize); /* File size */
    fs.fptr = 0;                        /* File pointer */
    fs.flag = FA_OPENED;

    return FR_OK;
}


FRESULT pf_lseek (
    uint32_t ofs       /* File pointer from top of file */
)
{
    CLUST clst;
    uint32_t bcs, sect, ifptr;

    if (!(fs.flag & FA_OPENED))        /* Check if opened */
            return FR_NOT_OPENED;

    if (ofs > fs.fsize) ofs = fs.fsize;   /* Clip offset with the file size */
    ifptr = fs.fptr;
    fs.fptr = 0;
    if (ofs > 0) {
        bcs = (uint32_t)fs.csize * 512;   /* Cluster size (byte) */
        if (ifptr > 0 &&
            (ofs - 1) / bcs >= (ifptr - 1) / bcs) { /* When seek to same or following cluster, */
            fs.fptr = (ifptr - 1) & ~(bcs - 1);    /* start from the current cluster */
            ofs -= fs.fptr;
            clst = fs.curr_clust;
        } else {                            /* When seek to back cluster, */
            clst = fs.org_clust;           /* start from the first cluster */
            fs.curr_clust = clst;
        }
        while (ofs > bcs) {             /* Cluster following loop */
            clst = get_fat(clst);       /* Follow cluster chain */
            if (clst <= 1 || clst >= fs.n_fatent) goto fe_abort;
            fs.curr_clust = clst;
            fs.fptr += bcs;
            ofs -= bcs;
        }
        fs.fptr += ofs;
        sect = clust2sect(clst);        /* Current sector */
        if (!sect) goto fe_abort;
        fs.dsect = sect + (fs.fptr / 512 & (fs.csize - 1));
    }

    return FR_OK;

fe_abort:
    fs.flag = 0;
    return FR_DISK_ERR;
}


FRESULT pf_read (
    void* buff,     /* Pointer to the read buffer (NULL:Forward data to the stream)*/
    uint16_t btr,       /* Number of bytes to read */
    uint16_t* br        /* Pointer to number of bytes read */
)
{
    disk_result_t dr;
    CLUST clst;
    uint32_t sect, remain;
    uint16_t rcnt;
    uint8_t cs, *rbuff = buff;

    *br = 0;
    if (!(fs.flag & FA_OPENED))     /* Check if opened */
        return FR_NOT_OPENED;

    remain = fs.fsize - fs.fptr;
    if (btr > remain) btr = (uint16_t)remain;           /* Truncate btr by remaining bytes */

    while (btr) {                                   /* Repeat until all data transferred */
        if ((fs.fptr % 512) == 0) {             /* On the sector boundary? */
            cs = (uint8_t)(fs.fptr / 512 & (fs.csize - 1)); /* Sector offset in the cluster */
            if (!cs) {                              /* On the cluster boundary? */
                clst = (fs.fptr == 0) ?         /* On the top of the file? */
                    fs.org_clust : get_fat(fs.curr_clust);
                if (clst <= 1) goto fr_abort;
                fs.curr_clust = clst;               /* Update current cluster */
            }
            sect = clust2sect(fs.curr_clust);       /* Get current sector */
            if (!sect) goto fr_abort;
            fs.dsect = sect + cs;
        }
        /* Get partial sector data from sector buffer */
        rcnt = (uint16_t)(512 - (fs.fptr % 512));
        if (rcnt > btr) rcnt = btr;
        dr = disk_readp(!buff ? 0 : rbuff, fs.dsect,
                (uint16_t)(fs.fptr % 512), rcnt);
        if (dr) goto fr_abort;
        /* Update pointers and counters */
        fs.fptr += rcnt; rbuff += rcnt;
        btr -= rcnt; *br += rcnt;
    }

    return FR_OK;

fr_abort:
    fs.flag = 0;
    return FR_DISK_ERR;
}



FRESULT pf_write (
    const void* buff,       /* Pointer to the data to be written */
    uint16_t btw,           /* Number of bytes to write (0:Finalize the current write operation) */
    uint16_t* bw            /* Pointer to number of bytes written */
)
{
    CLUST clst;
    uint32_t sect, remain;
    const uint8_t *p = buff;
    uint8_t cs;
    uint16_t wcnt;

    *bw = 0;
    if (!(fs.flag & FA_OPENED))     /* Check if opened */
        return FR_NOT_OPENED;

    if (!btw) {     /* Finalize request */
        if ((fs.flag & FA__WIP) && disk_writep(0, 0)) goto fw_abort;
        fs.flag &= ~FA__WIP;
        return FR_OK;
    } else {        /* Write data request */
        if (!(fs.flag & FA__WIP))       /* Round-down fptr to the sector boundary */
            fs.fptr &= 0xFFFFFE00;
    }
    remain = fs.fsize - fs.fptr;
    if (btw > remain) btw = (uint16_t)remain;           /* Truncate btw by remaining bytes */

    while (btw) {                                   /* Repeat until all data transferred */
        if (((uint16_t)fs.fptr % 512) == 0) {           /* On the sector boundary? */
            cs = (uint8_t)(fs.fptr / 512 & (fs.csize - 1)); /* Sector offset in the cluster */
            if (!cs) {                              /* On the cluster boundary? */
                clst = (fs.fptr == 0) ?         /* On the top of the file? */
                    fs.org_clust : get_fat(fs.curr_clust);
                if (clst <= 1) goto fw_abort;
                fs.curr_clust = clst;               /* Update current cluster */
            }
            sect = clust2sect(fs.curr_clust);       /* Get current sector */
            if (!sect) goto fw_abort;
            fs.dsect = sect + cs;
            if (disk_writep(0, fs.dsect)) goto fw_abort;    /* Initiate a sector write operation */
            fs.flag |= FA__WIP;
        }
        wcnt = 512 - ((uint16_t)fs.fptr % 512);     /* Number of bytes to write to the sector */
        if (wcnt > btw) wcnt = btw;
        if (disk_writep(p, wcnt)) goto fw_abort;    /* Send data to the sector */
        fs.fptr += wcnt; p += wcnt;             /* Update pointers and counters */
        btw -= wcnt; *bw += wcnt;
        if (((uint16_t)fs.fptr % 512) == 0) {
            if (disk_writep(0, 0)) goto fw_abort;   /* Finalize the currtent secter write operation */
            fs.flag &= ~FA__WIP;
        }
    }

    return FR_OK;

fw_abort:
    fs.flag = 0;
    return FR_DISK_ERR;
}



FRESULT pf_opendir (
    DIR *dj,            /* Pointer to directory object to create */
    const char *path    /* Pointer to the directory path */
)
{
    FRESULT res;
    uint8_t sp[12], dir[32];

    dj->fn = sp;
    res = follow_path(dj, dir, path);       /* Follow the path to the directory */
    if (res == FR_OK) {                     /* Follow completed */
        if (dir[0]) {                       /* It is not the root dir */
            if (dir[DIR_Attr] & AM_DIR)     /* The object is a directory */
                dj->sclust = LD_CLUST(dir);
            else                            /* The object is not a directory */
                res = FR_NO_PATH;
        }
        if (res == FR_OK)
            res = dir_rewind(dj);           /* Rewind dir */
    }
    if (res == FR_NO_FILE) res = FR_NO_PATH;

    return res;
}

FRESULT pf_readdir (
    DIR *dj,            /* Pointer to the open directory object */
    FILINFO *fno        /* Pointer to file information to return */
)
{
    FRESULT res;
    uint8_t sp[12], dir[32];

    dj->fn = sp;
    if (!fno) {
        res = dir_rewind(dj);
    } else {
        res = dir_read(dj, dir);
        if (res == FR_NO_FILE) {
            dj->sect = 0;
            res = FR_OK;
        }
        if (res == FR_OK) {             /* A valid entry is found */
            get_fileinfo(dj, dir, fno); /* Get the object information */
            res = dir_next(dj);         /* Increment index for next */
            if (res == FR_NO_FILE) {
                dj->sect = 0;
                res = FR_OK;
            }
        }
    }

    return res;
}
