// SPDX-License-Identifier: GPL-2.0-or-later

/***************************************************************************
 *   Copyright (C) 2018 by Andreas Bolsch                                  *
 *   andreas.bolsch@mni.thm.de                                             *
 *                                                                         *
 *   Copyright (C) 2012 by George Harris                                   *
 *   george@luminairecoffee.com                                            *
 *                                                                         *
 *   Copyright (C) 2010 by Antonio Borneo                                  *
 *   borneo.antonio@gmail.com                                              *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "imp.h"
#include "spi.h"
#include <jtag/jtag.h>

 /* Shared table of known SPI flash devices for SPI-based flash drivers. Taken
  * from device datasheets and Linux SPI flash drivers. */
const struct flash_device flash_devices[] = {
	/* Note: device_id is usually 3 bytes long, however the unused highest byte counts
	 * continuation codes for manufacturer id as per JEP106xx.
	 *
	 * All sizes (page, sector/block and flash) are in bytes.
	 *
	 * Guide to select a proper erase command (if both sector and block erase cmds are available):
	 * Use 4kbit sector erase cmd and set erase size to the size of sector for small devices
	 * (4Mbit and less, size <= 0x80000) to prevent too raw erase granularity.
	 * Use 64kbit block erase cmd and set erase size to the size of block for bigger devices
	 * (8Mbit and more, size >= 0x100000) to keep erase speed reasonable.
	 * If the device implements also 32kbit block erase, use it for 8Mbit, size == 0x100000.
	 */
	/*        name                  read qread  page  erase chip  device_id   page   erase   flash
	 *                              _cmd _cmd   _prog _cmd* _erase            size   size*   size
	 *                                          _cmd        _cmd
	 */
	FLASH_ID("st m25pe10",          0x03, 0x00, 0x02, 0xd8, 0x00, 0x00118020, 0x100, 0x10000, 0x20000),
	FLASH_ID("st m25pe20",          0x03, 0x00, 0x02, 0xd8, 0x00, 0x00128020, 0x100, 0x10000, 0x40000),
	FLASH_ID("st m25pe40",          0x03, 0x00, 0x02, 0xd8, 0x00, 0x00138020, 0x100, 0x10000, 0x80000),
	FLASH_ID("st m25pe80",          0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00148020, 0x100, 0x10000, 0x100000),
	FLASH_ID("st m25pe16",          0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00158020, 0x100, 0x10000, 0x200000),
	FLASH_ID("st m25p05",           0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00102020, 0x80,  0x8000,  0x10000),
	FLASH_ID("st m25p10",           0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00112020, 0x80,  0x8000,  0x20000),
	FLASH_ID("st m25p20",           0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00122020, 0x100, 0x10000, 0x40000),
	FLASH_ID("st m25p40",           0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00132020, 0x100, 0x10000, 0x80000),
	FLASH_ID("st m25p80",           0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00142020, 0x100, 0x10000, 0x100000),
	FLASH_ID("st m25p16",           0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00152020, 0x100, 0x10000, 0x200000),
	FLASH_ID("st m25p32",           0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00162020, 0x100, 0x10000, 0x400000),
	FLASH_ID("st m25p64",           0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00172020, 0x100, 0x10000, 0x800000),
	FLASH_ID("st m25p128",          0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00182020, 0x100, 0x40000, 0x1000000),
	FLASH_ID("st m45pe10",          0x03, 0x00, 0x02, 0xd8, 0xd8, 0x00114020, 0x100, 0x10000, 0x20000),
	FLASH_ID("st m45pe20",          0x03, 0x00, 0x02, 0xd8, 0xd8, 0x00124020, 0x100, 0x10000, 0x40000),
	FLASH_ID("st m45pe40",          0x03, 0x00, 0x02, 0xd8, 0xd8, 0x00134020, 0x100, 0x10000, 0x80000),
	FLASH_ID("st m45pe80",          0x03, 0x00, 0x02, 0xd8, 0xd8, 0x00144020, 0x100, 0x10000, 0x100000),
	FLASH_ID("sp s25fl004",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00120201, 0x100, 0x10000, 0x80000),
	FLASH_ID("sp s25fl008",         0x03, 0x08, 0x02, 0xd8, 0xc7, 0x00130201, 0x100, 0x10000, 0x100000),
	FLASH_ID("sp s25fl016",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00140201, 0x100, 0x10000, 0x200000),
	FLASH_ID("sp s25fl116k",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00154001, 0x100, 0x10000, 0x200000),
	FLASH_ID("sp s25fl032",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00150201, 0x100, 0x10000, 0x400000),
	FLASH_ID("sp s25fl132k",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00164001, 0x100, 0x10000, 0x400000),
	FLASH_ID("sp s25fl064",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00160201, 0x100, 0x10000, 0x800000),
	FLASH_ID("sp s25fl164k",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00174001, 0x100, 0x10000, 0x800000),
	FLASH_ID("sp s25fl128s",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00182001, 0x100, 0x10000, 0x1000000),
	FLASH_ID("sp s25fl256s",        0x13, 0x00, 0x12, 0xdc, 0xc7, 0x00190201, 0x100, 0x10000, 0x2000000),
	FLASH_ID("sp s25fl512s",        0x13, 0x00, 0x12, 0xdc, 0xc7, 0x00200201, 0x200, 0x40000, 0x4000000),
	FLASH_ID("cyp s25fl064l",       0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00176001, 0x100, 0x10000, 0x800000),
	FLASH_ID("cyp s25fl128l",       0x03, 0x00, 0x02, 0xd8, 0xc7, 0x00186001, 0x100, 0x10000, 0x1000000),
	FLASH_ID("cyp s25fl256l",       0x13, 0x00, 0x12, 0xdc, 0xc7, 0x00196001, 0x100, 0x10000, 0x2000000),
	FLASH_ID("cyp s28hl256t",       0x13, 0xec, 0x12, 0xdc, 0xc7, 0x00195a34, 0x100, 0x40000, 0x2000000), /* page! */
	FLASH_ID("cyp s28hs256t",       0x13, 0xec, 0x12, 0xdc, 0xc7, 0x00195b34, 0x100, 0x40000, 0x2000000), /* page! */
	FLASH_ID("cyp s28hl512t",       0x13, 0xec, 0x12, 0xdc, 0xc7, 0x001a5a34, 0x100, 0x40000, 0x4000000), /* page! */
	FLASH_ID("cyp s28hs512t",       0x13, 0xec, 0x12, 0xdc, 0xc7, 0x001a5b34, 0x100, 0x40000, 0x4000000), /* page! */
	FLASH_ID("cyp s28hl01gt",       0x13, 0xec, 0x12, 0xdc, 0xc7, 0x001b5a34, 0x100, 0x40000, 0x8000000), /* page! */
	FLASH_ID("cyp s28hs01gt",       0x13, 0xec, 0x12, 0xdc, 0xc7, 0x001b5b34, 0x100, 0x40000, 0x8000000), /* page! */
	FLASH_ID("atmel 25f512",        0x03, 0x00, 0x02, 0x52, 0xc7, 0x0065001f, 0x80,  0x8000,  0x10000),
	FLASH_ID("atmel 25f1024",       0x03, 0x00, 0x02, 0x52, 0x62, 0x0060001f, 0x100, 0x8000,  0x20000),
	FLASH_ID("atmel 25f2048",       0x03, 0x00, 0x02, 0x52, 0x62, 0x0063001f, 0x100, 0x10000, 0x40000),
	FLASH_ID("atmel 25f4096",       0x03, 0x00, 0x02, 0x52, 0x62, 0x0064001f, 0x100, 0x10000, 0x80000),
	FLASH_ID("atmel 25fs040",       0x03, 0x00, 0x02, 0xd7, 0xc7, 0x0004661f, 0x100, 0x10000, 0x80000),
	FLASH_ID("adesto 25sf041b",     0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0001841f, 0x100, 0x10000, 0x80000),
	FLASH_ID("adesto 25df081a",     0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0001451f, 0x100, 0x10000, 0x100000),
	FLASH_ID("adesto 25sf081b",     0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0001851f, 0x100, 0x10000, 0x100000),
	FLASH_ID("adesto 25sf161b",     0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0001861f, 0x100, 0x10000, 0x200000),
	FLASH_ID("adesto 25df321b",     0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0001471f, 0x100, 0x10000, 0x400000),
	FLASH_ID("adesto 25sf321b",     0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0001871f, 0x100, 0x10000, 0x400000),
	FLASH_ID("adesto 25xf641b",     0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0001881f, 0x100, 0x10000, 0x800000), /* sf/qf */
	FLASH_ID("adesto 25xf128a",     0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0001891f, 0x100, 0x10000, 0x1000000), /* sf/qf */
	FLASH_ID("adesto xp032",        0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0700a743, 0x100, 0x10000, 0x400000), /* 4-byte */
	FLASH_ID("adesto xp064b",       0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0001a81f, 0x100, 0x10000, 0x800000), /* 4-byte */
	FLASH_ID("adesto xp128",        0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0000a91f, 0x100, 0x10000, 0x1000000), /* 4-byte */
	FLASH_ID("mac 25l512",          0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001020c2, 0x010, 0x10000, 0x10000),
	FLASH_ID("mac 25l1005",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001120c2, 0x010, 0x10000, 0x20000),
	FLASH_ID("mac 25l2005",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001220c2, 0x010, 0x10000, 0x40000),
	FLASH_ID("mac 25l4005",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001320c2, 0x010, 0x10000, 0x80000),
	FLASH_ID("mac 25l8005",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001420c2, 0x010, 0x10000, 0x100000),
	FLASH_ID("mac 25l1605",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001520c2, 0x100, 0x10000, 0x200000),
	FLASH_ID("mac 25l3205",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001620c2, 0x100, 0x10000, 0x400000),
	FLASH_ID("mac 25l6405",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001720c2, 0x100, 0x10000, 0x800000),
	FLASH_ID("mac 25l12845",        0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x001820c2, 0x100, 0x10000, 0x1000000),
	FLASH_ID("mac 25l25645",        0x13, 0xec, 0x12, 0xdc, 0xc7, 0x001920c2, 0x100, 0x10000, 0x2000000),
	FLASH_ID("mac 25l51245",        0x13, 0xec, 0x12, 0xdc, 0xc7, 0x001a20c2, 0x100, 0x10000, 0x4000000),
	FLASH_ID("mac 25lm51245",       0x13, 0xec, 0x12, 0xdc, 0xc7, 0x003a85c2, 0x100, 0x10000, 0x4000000),
	FLASH_ID("mac 25r512f",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001028c2, 0x100, 0x10000, 0x10000),
	FLASH_ID("mac 25r1035f",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001128c2, 0x100, 0x10000, 0x20000),
	FLASH_ID("mac 25r2035f",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001228c2, 0x100, 0x10000, 0x40000),
	FLASH_ID("mac 25r4035f",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001328c2, 0x100, 0x10000, 0x80000),
	FLASH_ID("mac 25r8035f",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001428c2, 0x100, 0x10000, 0x100000),
	FLASH_ID("mac 25r1635f",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001528c2, 0x100, 0x10000, 0x200000),
	FLASH_ID("mac 25r3235f",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001628c2, 0x100, 0x10000, 0x400000),
	FLASH_ID("mac 25r6435f",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001728c2, 0x100, 0x10000, 0x800000),
	FLASH_ID("mac 25u1635e",        0x03, 0xeb, 0x02, 0x20, 0xc7, 0x003525c2, 0x100, 0x1000,  0x100000),
	FLASH_ID("mac 66l1g45g",        0x13, 0xec, 0x12, 0xdc, 0xc7, 0x001b20c2, 0x100, 0x10000, 0x8000000),
	FLASH_ID("mac 66um1g45g",       0x13, 0xec, 0x12, 0xdc, 0xc7, 0x003b80c2, 0x100, 0x10000, 0x8000000),
	FLASH_ID("mac 66lm1g45g",       0x13, 0xec, 0x12, 0xdc, 0xc7, 0x003b85c2, 0x100, 0x10000, 0x8000000),
	FLASH_ID("micron n25q032",      0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x0016ba20, 0x100, 0x10000, 0x400000),
	FLASH_ID("micron n25q064",      0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x0017ba20, 0x100, 0x10000, 0x800000),
	FLASH_ID("micron n25q128",      0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x0018ba20, 0x100, 0x10000, 0x1000000),
	FLASH_ID("micron n25q256 3v",   0x13, 0xec, 0x12, 0xdc, 0xc7, 0x0019ba20, 0x100, 0x10000, 0x2000000),
	FLASH_ID("micron n25q256 1.8v", 0x13, 0xec, 0x12, 0xdc, 0xc7, 0x0019bb20, 0x100, 0x10000, 0x2000000),
	FLASH_ID("micron mt25ql512",    0x13, 0xec, 0x12, 0xdc, 0xc7, 0x0020ba20, 0x100, 0x10000, 0x4000000),
	FLASH_ID("micron mt25ql01",     0x13, 0xec, 0x12, 0xdc, 0xc7, 0x0021ba20, 0x100, 0x10000, 0x8000000),
	FLASH_ID("micron mt25qu01",     0x13, 0xec, 0x12, 0xdc, 0xc7, 0x0021bb20, 0x100, 0x10000, 0x8000000),
	FLASH_ID("micron mt25ql02",     0x13, 0xec, 0x12, 0xdc, 0xc7, 0x0022ba20, 0x100, 0x10000, 0x10000000),
	FLASH_ID("win w25q80bv",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001440ef, 0x100, 0x10000, 0x100000),
	FLASH_ID("win w25q16jv",        0x03, 0x00, 0x02, 0x20, 0xc7, 0x001540ef, 0x100, 0x1000,  0x200000),
	FLASH_ID("win w25q16jv",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001570ef, 0x100, 0x10000, 0x200000), /* QPI / DTR */
	FLASH_ID("win w25q32fv/jv",     0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x001640ef, 0x100, 0x10000, 0x400000),
	FLASH_ID("win w25q32fv",        0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x001660ef, 0x100, 0x10000, 0x400000), /* QPI mode */
	FLASH_ID("win w25q32jv",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001670ef, 0x100, 0x10000, 0x400000),
	FLASH_ID("win w25q64fv/jv",     0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x001740ef, 0x100, 0x10000, 0x800000),
	FLASH_ID("win w25q64fv",        0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x001760ef, 0x100, 0x10000, 0x800000), /* QPI mode */
	FLASH_ID("win w25q64jv",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001770ef, 0x100, 0x10000, 0x800000),
	FLASH_ID("win w25q128fv/jv",    0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x001840ef, 0x100, 0x10000, 0x1000000),
	FLASH_ID("win w25q128fv",       0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x001860ef, 0x100, 0x10000, 0x1000000), /* QPI mode */
	FLASH_ID("win w25q128jv",       0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001870ef, 0x100, 0x10000, 0x1000000),
	FLASH_ID("win w25q256fv/jv",    0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x001940ef, 0x100, 0x10000, 0x2000000),
	FLASH_ID("win w25q256fv",       0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x001960ef, 0x100, 0x10000, 0x2000000), /* QPI mode */
	FLASH_ID("win w25q256jv",       0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001970ef, 0x100, 0x10000, 0x2000000),
	FLASH_ID("win w25q512jv",       0x03, 0x00, 0x02, 0xd8, 0xc7, 0x002040ef, 0x100, 0x10000, 0x4000000),
	FLASH_ID("win w25q01jv",        0x13, 0x00, 0x12, 0xdc, 0xc7, 0x002140ef, 0x100, 0x10000, 0x8000000),
	FLASH_ID("win w25q01jv-dtr",    0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x002170ef, 0x100, 0x10000, 0x8000000),
	FLASH_ID("win w25q02jv-dtr",    0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x002270ef, 0x100, 0x10000, 0x10000000),
	FLASH_ID("gd gd25q512",         0x03, 0x00, 0x02, 0x20, 0xc7, 0x001040c8, 0x100, 0x1000,  0x10000),
	FLASH_ID("gd gd25q10",          0x03, 0x00, 0x02, 0x20, 0xc7, 0x001140c8, 0x100, 0x1000,  0x20000),
	FLASH_ID("gd gd25q20",          0x03, 0x00, 0x02, 0x20, 0xc7, 0x001240c8, 0x100, 0x1000,  0x40000),
	FLASH_ID("gd gd25q40",          0x03, 0x00, 0x02, 0x20, 0xc7, 0x001340c8, 0x100, 0x1000,  0x80000),
	FLASH_ID("gd gd25q16c",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001540c8, 0x100, 0x10000, 0x200000),
	FLASH_ID("gd gd25q32c",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001640c8, 0x100, 0x10000, 0x400000),
	FLASH_ID("gd gd25q64c",         0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001740c8, 0x100, 0x10000, 0x800000),
	FLASH_ID("gd gd25q128c",        0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x001840c8, 0x100, 0x10000, 0x1000000),
	FLASH_ID("gd gd25q256c",        0x13, 0x00, 0x12, 0xdc, 0xc7, 0x001940c8, 0x100, 0x10000, 0x2000000),
	FLASH_ID("gd gd25q512mc",       0x13, 0x00, 0x12, 0xdc, 0xc7, 0x002040c8, 0x100, 0x10000, 0x4000000),
	FLASH_ID("gd gd25lx256e",       0x13, 0x0c, 0x12, 0xdc, 0xc7, 0x001968c8, 0x100, 0x10000, 0x2000000),
	FLASH_ID("zbit zb25vq16",       0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0015605e, 0x100, 0x10000, 0x200000),
	FLASH_ID("zbit zb25vq32",       0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0016405e, 0x100, 0x10000, 0x400000),
	FLASH_ID("zbit zb25vq32",       0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0016605e, 0x100, 0x10000, 0x400000), /* QPI mode */
	FLASH_ID("zbit zb25vq64",       0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0017405e, 0x100, 0x10000, 0x800000),
	FLASH_ID("zbit zb25vq64",       0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0017605e, 0x100, 0x10000, 0x800000), /* QPI mode */
	FLASH_ID("zbit zb25vq128",      0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0018405e, 0x100, 0x10000, 0x1000000),
	FLASH_ID("zbit zb25vq128",      0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0018605e, 0x100, 0x10000, 0x1000000), /* QPI mode */
	FLASH_ID("issi is25lq040b",     0x03, 0xeb, 0x02, 0x20, 0xc7, 0x0013409d, 0x100, 0x1000,  0x80000),
	FLASH_ID("issi is25lp032",      0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0016609d, 0x100, 0x10000, 0x400000),
	FLASH_ID("issi is25lp064",      0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0017609d, 0x100, 0x10000, 0x800000),
	FLASH_ID("issi is25lp128d",     0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x0018609d, 0x100, 0x10000, 0x1000000),
	FLASH_ID("issi is25wp128d",     0x03, 0xeb, 0x02, 0xd8, 0xc7, 0x0018709d, 0x100, 0x10000, 0x1000000),
	FLASH_ID("issi is25lp256d",     0x13, 0xec, 0x12, 0xdc, 0xc7, 0x0019609d, 0x100, 0x10000, 0x2000000),
	FLASH_ID("issi is25wp256d",     0x13, 0xec, 0x12, 0xdc, 0xc7, 0x0019709d, 0x100, 0x10000, 0x2000000),
	FLASH_ID("issi is25lp512m",     0x13, 0xec, 0x12, 0xdc, 0xc7, 0x001a609d, 0x100, 0x10000, 0x4000000),
	FLASH_ID("issi is25wp512m",     0x13, 0xec, 0x12, 0xdc, 0xc7, 0x001a709d, 0x100, 0x10000, 0x4000000),
	FLASH_ID("xtx xt25f02e",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0012400b, 0x100, 0x10000, 0x40000),
	FLASH_ID("xtx xt25f04d",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0013400b, 0x100, 0x10000, 0x80000),
	FLASH_ID("xtx xt25f08b",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0014400b, 0x100, 0x10000, 0x100000),
	FLASH_ID("xtx xt25f16b",        0x03, 0x00, 0x02, 0xd8, 0xc7, 0x0015400b, 0x100, 0x10000, 0x200000),
	FLASH_ID("xtx xt25f32b",        0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0016400b, 0x100, 0x10000, 0x200000),
	FLASH_ID("xtx xt25f64b",        0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0017400b, 0x100, 0x10000, 0x400000),
	FLASH_ID("xtx xt25f128b",       0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0018400b, 0x100, 0x10000, 0x800000),
	FLASH_ID("xtx xt25q08d",        0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0014600b, 0x100, 0x10000, 0x100000),
	FLASH_ID("xtx xt25q16b",        0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0015600b, 0x100, 0x10000, 0x200000),
	FLASH_ID("xtx xt25q32b",        0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0016600b, 0x100, 0x10000, 0x400000), /* exists ? */
	FLASH_ID("xtx xt25q64b",        0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0017600b, 0x100, 0x10000, 0x800000),
	FLASH_ID("xtx xt25q128b",       0x03, 0x0b, 0x02, 0xd8, 0xc7, 0x0018600b, 0x100, 0x10000, 0x1000000),
	FLASH_ID("zetta zd25q16",       0x03, 0x00, 0x02, 0xd8, 0xc7, 0x001560ba, 0x100, 0x10000, 0x200000),

	/* FRAM, no erase commands, no write page or sectors */

	/*        name                  read qread  page  device_id   total
	 *                              _cmd _cmd   _prog             size
	 *                                          _cmd
	 */
	FRAM_ID("fu mb85rs16n",         0x03, 0,    0x02, 0x00010104, 0x800),
	FRAM_ID("fu mb85rs32v",         0x03, 0,    0x02, 0x00010204, 0x1000), /* exists ? */
	FRAM_ID("fu mb85rs64v",         0x03, 0,    0x02, 0x00020304, 0x2000),
	FRAM_ID("fu mb85rs128b",        0x03, 0,    0x02, 0x00090404, 0x4000),
	FRAM_ID("fu mb85rs256b",        0x03, 0,    0x02, 0x00090504, 0x8000),
	FRAM_ID("fu mb85rs512t",        0x03, 0,    0x02, 0x00032604, 0x10000),
	FRAM_ID("fu mb85rs1mt",         0x03, 0,    0x02, 0x00032704, 0x20000),
	FRAM_ID("fu mb85rs2mta",        0x03, 0,    0x02, 0x00034804, 0x40000),
	FRAM_ID("cyp fm25v01a",         0x03, 0,    0x02, 0x060821c2, 0x4000),
	FRAM_ID("cyp fm25v02",          0x03, 0,    0x02, 0x060022c2, 0x8000),
	FRAM_ID("cyp fm25v02a",         0x03, 0,    0x02, 0x060822c2, 0x8000),
	FRAM_ID("cyp fm25v05",          0x03, 0,    0x02, 0x060023c2, 0x10000),
	FRAM_ID("cyp fm25v10",          0x03, 0,    0x02, 0x060024c2, 0x20000),
	FRAM_ID("cyp fm25v20a",         0x03, 0,    0x02, 0x060825c2, 0x40000),
	FRAM_ID("cyp fm25v40",          0x03, 0,    0x02, 0x064026c2, 0x80000),
	FRAM_ID("cyp cy15b102qn",       0x03, 0,    0x02, 0x06002ac2, 0x40000),
	FRAM_ID("cyp cy15v102qn",       0x03, 0,    0x02, 0x06042ac2, 0x40000),
	FRAM_ID("cyp cy15b104qi",       0x03, 0,    0x02, 0x06012dc2, 0x80000),
	FRAM_ID("cyp cy15b104qi",       0x03, 0,    0x02, 0x06a12dc2, 0x80000),
	FRAM_ID("cyp cy15v104qi",       0x03, 0,    0x02, 0x06052dc2, 0x80000),
	FRAM_ID("cyp cy15v104qi",       0x03, 0,    0x02, 0x06a52dc2, 0x80000),
	FRAM_ID("cyp cy15b104qn",       0x03, 0,    0x02, 0x06402cc2, 0x80000),
	FRAM_ID("cyp cy15b108qi",       0x03, 0,    0x02, 0x06012fc2, 0x100000),
	FRAM_ID("cyp cy15b108qi",       0x03, 0,    0x02, 0x06a12fc2, 0x100000),
	FRAM_ID("cyp cy15v108qi",       0x03, 0,    0x02, 0x06052fc2, 0x100000),
	FRAM_ID("cyp cy15v108qi",       0x03, 0,    0x02, 0x06a52fc2, 0x100000),
	FRAM_ID("cyp cy15b108qn",       0x03, 0,    0x02, 0x06012ec2, 0x100000),
	FRAM_ID("cyp cy15b108qn",       0x03, 0,    0x02, 0x06032ec2, 0x100000),
	FRAM_ID("cyp cy15b108qn",       0x03, 0,    0x02, 0x06a12ec2, 0x100000),
	FRAM_ID("cyp cy15v108qn",       0x03, 0,    0x02, 0x06052ec2, 0x100000),
	FRAM_ID("cyp cy15v108qn",       0x03, 0,    0x02, 0x06072ec2, 0x100000),
	FRAM_ID("cyp cy15v108qn",       0x03, 0,    0x02, 0x06a52ec2, 0x100000),

	FLASH_ID(NULL,                  0,    0,    0,    0,    0,    0,          0,     0,       0)
};
