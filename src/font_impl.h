// Created from bdf2c Version 3, (c) 2009, 2010 by Lutz Sammer
//	License AGPLv3: GNU Affero General Public License version 3

/* The FontStruction “Resolution 3x4”
(http://fontstruct.com/fontstructions/show/534332) by “tomchatter” is
licensed under a Creative Commons Attribution Non-commercial No Derivatives
license (http://creativecommons.org/licenses/by-nc-nd/3.0/).
*/

#include "font_def.h"

	/// character bitmap for each encoding
static code unsigned char __font_bitmap__[] = {
//  32 $20 '0020'
//	width 4, bbx 0, bby 0, bbw 0, bbh 0
	________,
	________,
	________,
	________,
	________,
	________,
//  33 $21 '0021'
//	width 3, bbx 1, bby 0, bbw 1, bbh 5
	_X______,
	_X______,
	_X______,
	________,
	_X______,
	________,
//  34 $22 '0022'
//	width 3, bbx 0, bby 3, bbw 2, bbh 2
	_X______,
	X_______,
	________,
	________,
	________,
	________,
//  35 $23 '0023'
//	width 4, bbx 0, bby 1, bbw 3, bbh 3
	X_X_____,
	________,
	X_X_____,
	________,
	________,
	________,
//  36 $24 '0024'
//	width 4, bbx 0, bby -1, bbw 3, bbh 6
	_X______,
	_XX_____,
	X_______,
	_XX_____,
	XXX_____,
	_X______,
//  38 $26 '0026'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_X______,
	X_______,
	_X______,
	X_X_____,
	________,
	________,
//  39 $27 '0027'
//	width 3, bbx 0, bby 3, bbw 2, bbh 2
	_X______,
	X_______,
	________,
	________,
	________,
	________,
//  40 $28 '0028'
//	width 3, bbx 0, bby 0, bbw 2, bbh 4
	_X______,
	X_______,
	X_______,
	_X______,
	________,
	________,
//  41 $29 '0029'
//	width 3, bbx 0, bby 0, bbw 2, bbh 4
	X_______,
	_X______,
	_X______,
	X_______,
	________,
	________,
//  42 $2a '002A'
//	width 4, bbx 0, bby 1, bbw 3, bbh 3
	_X______,
	X_X_____,
	_X______,
	________,
	________,
	________,
//  43 $2b '002B'
//	width 4, bbx 0, bby 1, bbw 3, bbh 3
	_X______,
	XXX_____,
	_X______,
	________,
	________,
	________,
//  44 $2c '002C'
//	width 2, bbx 0, bby -1, bbw 1, bbh 2
	X_______,
	X_______,
	________,
	________,
	________,
	________,
//  45 $2d '002D'
//	width 3, bbx 0, bby 2, bbw 2, bbh 1
	XX______,
	________,
	________,
	________,
	________,
	________,
//  46 $2e '002E'
//	width 2, bbx 0, bby 0, bbw 1, bbh 1
	X_______,
	________,
	________,
	________,
	________,
	________,
//  47 $2f '002F'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	__X_____,
	_X______,
	_X______,
	X_______,
	________,
	________,
//  48 $30 '0030'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_XX_____,
	X_X_____,
	X_X_____,
	XX______,
	________,
	________,
//  49 $31 '0031'
//	width 3, bbx 0, bby 0, bbw 2, bbh 4
	_X______,
	XX______,
	_X______,
	_X______,
	________,
	________,
//  50 $32 '0032'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	__X_____,
	XX______,
	XXX_____,
	________,
	________,
//  51 $33 '0033'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	_XX_____,
	__X_____,
	XX______,
	________,
	________,
//  52 $34 '0034'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_______,
	X_X_____,
	XXX_____,
	__X_____,
	________,
	________,
//  53 $35 '0035'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	X_______,
	_XX_____,
	XXX_____,
	________,
	________,
//  54 $36 '0036'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_XX_____,
	X_______,
	XXX_____,
	XXX_____,
	________,
	________,
//  55 $37 '0037'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	__X_____,
	_X______,
	_X______,
	________,
	________,
//  56 $38 '0038'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	XXX_____,
	X_X_____,
	XXX_____,
	________,
	________,
//  57 $39 '0039'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	XXX_____,
	__X_____,
	XX______,
	________,
	________,
//  58 $3a '003A'
//	width 2, bbx 0, bby 0, bbw 1, bbh 3
	X_______,
	________,
	X_______,
	________,
	________,
	________,
//  59 $3b '003B'
//	width 3, bbx 0, bby -1, bbw 2, bbh 4
	_X______,
	________,
	_X______,
	X_______,
	________,
	________,
//  61 $3d '003D'
//	width 3, bbx 0, bby 0, bbw 2, bbh 3
	XX______,
	________,
	XX______,
	________,
	________,
	________,
//  63 $3f '003F'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	__X_____,
	________,
	_X______,
	________,
	________,
//  64 $40 '0040'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	XXX_____,
	X_______,
	XXX_____,
	________,
	________,
//  65 $41 '0041'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_XX_____,
	X_X_____,
	XXX_____,
	X_X_____,
	________,
	________,
//  66 $42 '0042'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	XXX_____,
	X_X_____,
	XXX_____,
	________,
	________,
//  67 $43 '0043'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_XX_____,
	X_______,
	X_______,
	_XX_____,
	________,
	________,
//  68 $44 '0044'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	X_X_____,
	X_X_____,
	XX______,
	________,
	________,
//  69 $45 '0045'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	XX______,
	X_______,
	XXX_____,
	________,
	________,
//  70 $46 '0046'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	XX______,
	X_______,
	X_______,
	________,
	________,
//  71 $47 '0047'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_X______,
	X_______,
	X_X_____,
	_XX_____,
	________,
	________,
//  72 $48 '0048'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	XXX_____,
	X_X_____,
	________,
	________,
//  73 $49 '0049'
//	width 2, bbx 0, bby 0, bbw 1, bbh 4
	X_______,
	X_______,
	X_______,
	X_______,
	________,
	________,
//  74 $4a '004A'
//	width 3, bbx 0, bby 0, bbw 2, bbh 4
	_X______,
	_X______,
	_X______,
	X_______,
	________,
	________,
//  75 $4b '004B'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	XX______,
	X_X_____,
	________,
	________,
//  76 $4c '004C'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_______,
	X_______,
	X_______,
	XXX_____,
	________,
	________,
//  77 $4d '004D'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	XXX_____,
	X_X_____,
	X_X_____,
	________,
	________,
//  78 $4e '004E'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	X_X_____,
	X_X_____,
	X_X_____,
	________,
	________,
//  79 $4f '004F'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_X______,
	X_X_____,
	X_X_____,
	_X______,
	________,
	________,
//  80 $50 '0050'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	X_X_____,
	XX______,
	X_______,
	________,
	________,
//  81 $51 '0051'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	X_X_____,
	XXX_____,
	__X_____,
	________,
	________,
//  82 $52 '0052'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	X_X_____,
	XX______,
	X_X_____,
	________,
	________,
//  83 $53 '0053'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_XX_____,
	X_______,
	_XX_____,
	XXX_____,
	________,
	________,
//  84 $54 '0054'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	_X______,
	_X______,
	_X______,
	________,
	________,
//  85 $55 '0055'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	X_X_____,
	_XX_____,
	________,
	________,
//  86 $56 '0056'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	X_X_____,
	_X______,
	________,
	________,
//  87 $57 '0057'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	XXX_____,
	XXX_____,
	________,
	________,
//  88 $58 '0058'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	_X______,
	X_X_____,
	________,
	________,
//  89 $59 '0059'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	_X______,
	_X______,
	________,
	________,
//  90 $5a '005A'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	_XX_____,
	X_______,
	XXX_____,
	________,
	________,
//  92 $5c '005C'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_______,
	_X______,
	_X______,
	__X_____,
	________,
	________,
//  95 $5f '005F'
//	width 3, bbx 0, bby 0, bbw 2, bbh 1
	XX______,
	________,
	________,
	________,
	________,
	________,
//  97 $61 '0061'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_XX_____,
	X_X_____,
	XXX_____,
	X_X_____,
	________,
	________,
//  98 $62 '0062'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	XXX_____,
	X_X_____,
	XXX_____,
	________,
	________,
//  99 $63 '0063'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_XX_____,
	X_______,
	X_______,
	_XX_____,
	________,
	________,
// 100 $64 '0064'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	X_X_____,
	X_X_____,
	XX______,
	________,
	________,
// 101 $65 '0065'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	XX______,
	X_______,
	XXX_____,
	________,
	________,
// 102 $66 '0066'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	XX______,
	X_______,
	X_______,
	________,
	________,
// 103 $67 '0067'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_X______,
	X_______,
	X_X_____,
	_XX_____,
	________,
	________,
// 104 $68 '0068'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	XXX_____,
	X_X_____,
	________,
	________,
// 105 $69 '0069'
//	width 2, bbx 0, bby 0, bbw 1, bbh 4
	X_______,
	X_______,
	X_______,
	X_______,
	________,
	________,
// 106 $6a '006A'
//	width 3, bbx 0, bby 0, bbw 2, bbh 4
	_X______,
	_X______,
	_X______,
	X_______,
	________,
	________,
// 107 $6b '006B'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	XX______,
	X_X_____,
	________,
	________,
// 108 $6c '006C'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_______,
	X_______,
	X_______,
	XXX_____,
	________,
	________,
// 109 $6d '006D'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	XXX_____,
	X_X_____,
	X_X_____,
	________,
	________,
// 110 $6e '006E'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	X_X_____,
	X_X_____,
	X_X_____,
	________,
	________,
// 111 $6f '006F'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_XX_____,
	X_X_____,
	X_X_____,
	XX______,
	________,
	________,
// 112 $70 '0070'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	X_X_____,
	XX______,
	X_______,
	________,
	________,
// 113 $71 '0071'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	X_X_____,
	XXX_____,
	__X_____,
	________,
	________,
// 114 $72 '0072'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XX______,
	X_X_____,
	XX______,
	X_X_____,
	________,
	________,
// 115 $73 '0073'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	_XX_____,
	X_______,
	_XX_____,
	XXX_____,
	________,
	________,
// 116 $74 '0074'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	_X______,
	_X______,
	_X______,
	________,
	________,
// 117 $75 '0075'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	X_X_____,
	_XX_____,
	________,
	________,
// 118 $76 '0076'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	X_X_____,
	_X______,
	________,
	________,
// 119 $77 '0077'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	XXX_____,
	XXX_____,
	________,
	________,
// 120 $78 '0078'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	_X______,
	X_X_____,
	________,
	________,
// 121 $79 '0079'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	X_X_____,
	X_X_____,
	_X______,
	_X______,
	________,
	________,
// 122 $7a '007A'
//	width 4, bbx 0, bby 0, bbw 3, bbh 4
	XXX_____,
	_XX_____,
	X_______,
	XXX_____,
	________,
	________
};

	/// character width for each encoding
static code unsigned char __font_widths__[] = {
	4,
	3,
	3,
	4,
	4,
	4,
	3,
	3,
	3,
	4,
	4,
	2,
	3,
	2,
	4,
	4,
	3,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	2,
	3,
	3,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	2,
	3,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	3,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	2,
	3,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	2,
	5,
	5,
	4,
	4,
	4,
	4,
	5,
	5,
	3,
	3,
	4,
	4,
	4,
	4,
	5,
	5,
	4,
	4,
	2,
	2,
	3,
	3,
};

	/// character encoding for each index entry
static code unsigned char __font_index__[] = {
	32,
	33,
	34,
	35,
	36,
	38,
	39,
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	49,
	50,
	51,
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	61,
	63,
	64,
	65,
	66,
	67,
	68,
	69,
	70,
	71,
	72,
	73,
	74,
	75,
	76,
	77,
	78,
	79,
	80,
	81,
	82,
	83,
	84,
	85,
	86,
	87,
	88,
	89,
	90,
	92,
	95,
	97,
	98,
	99,
	100,
	101,
	102,
	103,
	104,
	105,
	106,
	107,
	108,
	109,
	110,
	111,
	112,
	113,
	114,
	115,
	116,
	117,
	118,
	119,
	120,
	121,
	122
};
