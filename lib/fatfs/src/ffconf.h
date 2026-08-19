#ifndef _FFCONF
#define _FFCONF 32020

/* Basic configuration */
#define _FS_READONLY        0
#define _FS_MINIMIZE        0
#define _USE_STRFUNC        0
#define _USE_FIND           0
#define _USE_MKFS           1
#define _USE_FASTSEEK       0
#define _USE_EXPAND         0
#define _USE_CHMOD          0
#define _USE_LABEL          0
#define _USE_FORWARD       0
#define _USE_STRFUNC        0

/* Volume / sector configuration */
#define _VOLUMES            1
#define _STR_VOLUME_ID      0
#define _MAX_SS             512
#define _MIN_SS             512

/* File system configuration */
#define _MULTI_PARTITION    0
#define _FS_RPATH           0

/* Character code */
#define _CODE_PAGE          437

/* Long file name */
#define _USE_LFN            0
#define _MAX_LFN            255
#define _LFN_UNICODE        0
#define _LFN_BUF            255
#define _FS_REENTRANT       0

/* Synchronization */
#define _FS_TIMEOUT         1000

/* Integer types */
#define _WORD_ACCESS        0

/* System dependent definitions */
#define _FS_NORTC           1
#define _NORTC_MON          1
#define _NORTC_MDAY         1
#define _NORTC_YEAR         2026

#endif