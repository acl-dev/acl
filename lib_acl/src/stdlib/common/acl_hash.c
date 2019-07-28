#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_hash.h"

#endif

static const unsigned short crc16tab[256]= {
	0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
	0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
	0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
	0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
	0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
	0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
	0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
	0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
	0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
	0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
	0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
	0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
	0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
	0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
	0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
	0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
	0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
	0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
	0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
	0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
	0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
	0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
	0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
	0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
	0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
	0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
	0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
	0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
	0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
	0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
	0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
	0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

unsigned short acl_hash_crc16(const void *buf, size_t len)
{
	const unsigned char* ptr = (const unsigned char *) buf;
	size_t counter;
	unsigned short crc = 0;
	for (counter = 0; counter < len; counter++)
		crc = (crc<<8) ^ crc16tab[((crc>>8) ^ *ptr++) & 0x00FF];
	return crc;
}

#if 0
/* 该实现有可能在64位系统存在一定问题 */

static unsigned long m_table32n[256] = {
    0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
    0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
    0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75,
    0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,
    0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039, 0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,
    0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
    0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
    0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D,
    0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072,
    0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA,
    0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02,
    0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
    0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692,
    0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A,
    0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2,
    0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A,
    0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
    0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53,
    0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B,
    0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF, 0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623,
    0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B,
    0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
    0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B,
    0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3,
    0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C,
    0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8, 0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24,
    0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
    0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654,
    0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C,
    0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4,
    0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C,
    0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668, 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4
};

unsigned acl_hash_crc32(const void *key, size_t len)
{
	unsigned long m_crc;
	const unsigned char* ptr = (const unsigned char *) key;

	m_crc = 0xFFFFFFFF;
	while (len-- > 0)
		m_crc = m_table32n[((m_crc >> 24) ^ *ptr++)] ^ (m_crc << 8);

	return (m_crc);
}

#else
/* ========================================================================
* Table of CRC-32's of all single-byte values (made by make_crc_table)
*/
static const unsigned long crc32_table[256] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
	0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	0x2d02ef8dL
};
/* ========================================================================= */
#define DO1(buf) crc = crc32_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

/* ========================================================================= */
unsigned acl_hash_crc32(const void *buf, size_t len)
{
	const unsigned char* ptr = (const unsigned char *) buf;
	unsigned long crc = 0;

	if (ptr == 0)
		return 0L;
	crc = crc ^ 0xffffffffL;
	while (len >= 8) {
		DO8(ptr);
		len -= 8;
	}
	if (len > 0)
		do {
			DO1(ptr);
		} while (--len > 0);
	return (unsigned) crc ^ 0xffffffffL;
}
#endif

/*
 * Improved calculation of CRC-64 values for protein sequences
 * By David T. Jones (dtj@cs.ucl.ac.uk)  - September 28th 2002
 * 
 * Modified from code at URL:
 * ftp://ftp.ebi.ac.uk/pub/software/swissprot/Swissknife/old/SPcrc.tar.gz
 */

/* If you want to try the old CRC-64 function, currently employed in
   SWISSPROT/TrEMBL then uncomment the next line */
/* add by guozhaohui, 2008.4.25 */
/* #define OLDCRC */

#ifdef OLDCRC
  #define POLY64REV	0xd800000000000000ll
  #define INITIALCRC	0x0000000000000000ll
#elif defined(MS_VC6)
  #define POLY64REV     0x95ac9329ac4bc9b5
  #define INITIALCRC    0xffffffffffffffff
#else
  #define POLY64REV     0x95ac9329ac4bc9b5ll
  #define INITIALCRC    0xffffffffffffffffll
#endif

#ifdef MS_VC6
static acl_uint64 crc64_table[256] = {
	0x0,
        0x7ad870c830358979, 0xf5b0e190606b12f2, 0x8f689158505e9b8b,
	0xc038e5739841b68f, 0xbae095bba8743ff6, 0x358804e3f82aa47d,
	0x4f50742bc81f2d04, 0xab28ecb46814fe75, 0xd1f09c7c5821770c,
	0x5e980d24087fec87, 0x24407dec384a65fe, 0x6b1009c7f05548fa,
	0x11c8790fc060c183, 0x9ea0e857903e5a08, 0xe478989fa00bd371,
	0x7d08ff3b88be6f81, 0x7d08ff3b88be6f8, 0x88b81eabe8d57d73,
	0xf2606e63d8e0f40a, 0xbd301a4810ffd90e, 0xc7e86a8020ca5077,
	0x4880fbd87094cbfc, 0x32588b1040a14285, 0xd620138fe0aa91f4,
	0xacf86347d09f188d, 0x2390f21f80c18306, 0x594882d7b0f40a7f,
	0x1618f6fc78eb277b, 0x6cc0863448deae02, 0xe3a8176c18803589,
	0x997067a428b5bcf0, 0xfa11fe77117cdf02, 0x80c98ebf2149567b,
	0xfa11fe77117cdf0, 0x75796f2f41224489, 0x3a291b04893d698d,
	0x40f16bccb908e0f4, 0xcf99fa94e9567b7f, 0xb5418a5cd963f206,
	0x513912c379682177, 0x2be1620b495da80e, 0xa489f35319033385,
	0xde51839b2936bafc, 0x9101f7b0e12997f8, 0xebd98778d11c1e81,
	0x64b116208142850a, 0x1e6966e8b1770c73, 0x8719014c99c2b083,
	0xfdc17184a9f739fa, 0x72a9e0dcf9a9a271, 0x8719014c99c2b08,
	0x4721e43f0183060c, 0x3df994f731b68f75, 0xb29105af61e814fe,
	0xc849756751dd9d87, 0x2c31edf8f1d64ef6, 0x56e99d30c1e3c78f,
	0xd9810c6891bd5c04, 0xa3597ca0a188d57d, 0xec09088b6997f879,
	0x96d1784359a27100, 0x19b9e91b09fcea8b, 0x636199d339c963f2, 
	0xdf7adabd7a6e2d6f, 0xa5a2aa754a5ba416, 0x2aca3b2d1a053f9d, 
	0x50124be52a30b6e4, 0x1f423fcee22f9be0, 0x659a4f06d21a1299,
	0xeaf2de5e82448912, 0x902aae96b271006b, 0x74523609127ad31a,
	0xe8a46c1224f5a63, 0x81e2d7997211c1e8, 0xfb3aa75142244891, 
	0xb46ad37a8a3b6595, 0xceb2a3b2ba0eecec, 0x41da32eaea507767,
	0x3b024222da65fe1e, 0xa2722586f2d042ee, 0xd8aa554ec2e5cb97,
	0x57c2c41692bb501c, 0x2d1ab4dea28ed965, 0x624ac0f56a91f461, 
	0x1892b03d5aa47d18, 0x97fa21650afae693, 0xed2251ad3acf6fea, 
	0x95ac9329ac4bc9b, 0x7382b9faaaf135e2, 0xfcea28a2faafae69, 
	0x8632586aca9a2710, 0xc9622c4102850a14, 0xb3ba5c8932b0836d,
	0x3cd2cdd162ee18e6, 0x460abd1952db919f, 0x256b24ca6b12f26d, 
	0x5fb354025b277b14, 0xd0dbc55a0b79e09f, 0xaa03b5923b4c69e6,
	0xe553c1b9f35344e2, 0x9f8bb171c366cd9b, 0x10e3202993385610, 
	0x6a3b50e1a30ddf69, 0x8e43c87e03060c18, 0xf49bb8b633338561,
	0x7bf329ee636d1eea, 0x12b592653589793, 0x4e7b2d0d9b47ba97, 
	0x34a35dc5ab7233ee, 0xbbcbcc9dfb2ca865, 0xc113bc55cb19211c, 
	0x5863dbf1e3ac9dec, 0x22bbab39d3991495, 0xadd33a6183c78f1e, 
	0xd70b4aa9b3f20667, 0x985b3e827bed2b63, 0xe2834e4a4bd8a21a,
	0x6debdf121b863991, 0x1733afda2bb3b0e8, 0xf34b37458bb86399, 
	0x8993478dbb8deae0, 0x6fbd6d5ebd3716b, 0x7c23a61ddbe6f812,
	0x3373d23613f9d516, 0x49aba2fe23cc5c6f, 0xc6c333a67392c7e4, 
	0xbc1b436e43a74e9d, 0x95ac9329ac4bc9b5, 0xef74e3e19c7e40cc,
	0x601c72b9cc20db47, 0x1ac40271fc15523e, 0x5594765a340a7f3a, 
	0x2f4c0692043ff643, 0xa02497ca54616dc8, 0xdafce7026454e4b1,
	0x3e847f9dc45f37c0, 0x445c0f55f46abeb9, 0xcb349e0da4342532, 
	0xb1eceec59401ac4b, 0xfebc9aee5c1e814f, 0x8464ea266c2b0836,
	0xb0c7b7e3c7593bd, 0x71d40bb60c401ac4, 0xe8a46c1224f5a634, 
	0x927c1cda14c02f4d, 0x1d148d82449eb4c6, 0x67ccfd4a74ab3dbf,
	0x289c8961bcb410bb, 0x5244f9a98c8199c2, 0xdd2c68f1dcdf0249,
	0xa7f41839ecea8b30, 0x438c80a64ce15841, 0x3954f06e7cd4d138, 
	0xb63c61362c8a4ab3, 0xcce411fe1cbfc3ca, 0x83b465d5d4a0eece,
	0xf96c151de49567b7, 0x76048445b4cbfc3c, 0xcdcf48d84fe7545,
	0x6fbd6d5ebd3716b7, 0x15651d968d029fce, 0x9a0d8ccedd5c0445, 
	0xe0d5fc06ed698d3c, 0xaf85882d2576a038, 0xd55df8e515432941, 
	0x5a3569bd451db2ca, 0x20ed197575283bb3, 0xc49581ead523e8c2, 
	0xbe4df122e51661bb, 0x3125607ab548fa30, 0x4bfd10b2857d7349,
	0x4ad64994d625e4d, 0x7e7514517d57d734, 0xf11d85092d094cbf, 
	0x8bc5f5c11d3cc5c6, 0x12b5926535897936, 0x686de2ad05bcf04f, 
	0xe70573f555e26bc4, 0x9ddd033d65d7e2bd, 0xd28d7716adc8cfb9, 
	0xa85507de9dfd46c0, 0x273d9686cda3dd4b, 0x5de5e64efd965432,
	0xb99d7ed15d9d8743, 0xc3450e196da80e3a, 0x4c2d9f413df695b1,
	0x36f5ef890dc31cc8, 0x79a59ba2c5dc31cc, 0x37deb6af5e9b8b5, 
	0x8c157a32a5b7233e, 0xf6cd0afa9582aa47, 0x4ad64994d625e4da, 
	0x300e395ce6106da3, 0xbf66a804b64ef628, 0xc5bed8cc867b7f51,
	0x8aeeace74e645255, 0xf036dc2f7e51db2c, 0x7f5e4d772e0f40a7,
	0x5863dbf1e3ac9de, 0xe1fea520be311aaf, 0x9b26d5e88e0493d6,
	0x144e44b0de5a085d, 0x6e963478ee6f8124, 0x21c640532670ac20,
	0x5b1e309b16452559, 0xd476a1c3461bbed2, 0xaeaed10b762e37ab,
	0x37deb6af5e9b8b5b, 0x4d06c6676eae0222, 0xc26e573f3ef099a9,
	0xb8b627f70ec510d0, 0xf7e653dcc6da3dd4, 0x8d3e2314f6efb4ad, 
	0x256b24ca6b12f26, 0x788ec2849684a65f, 0x9cf65a1b368f752e,
	0xe62e2ad306bafc57, 0x6946bb8b56e467dc, 0x139ecb4366d1eea5, 
	0x5ccebf68aecec3a1, 0x2616cfa09efb4ad8, 0xa97e5ef8cea5d153, 
	0xd3a62e30fe90582a, 0xb0c7b7e3c7593bd8, 0xca1fc72bf76cb2a1,
	0x45775673a732292a, 0x3faf26bb9707a053, 0x70ff52905f188d57, 
	0xa2722586f2d042e, 0x854fb3003f739fa5, 0xff97c3c80f4616dc, 
	0x1bef5b57af4dc5ad, 0x61372b9f9f784cd4, 0xee5fbac7cf26d75f, 
	0x9487ca0fff135e26, 0xdbd7be24370c7322, 0xa10fceec0739fa5b,
	0x2e675fb4576761d0, 0x54bf2f7c6752e8a9, 0xcdcf48d84fe75459, 
	0xb71738107fd2dd20, 0x387fa9482f8c46ab, 0x42a7d9801fb9cfd2, 
	0xdf7adabd7a6e2d6, 0x772fdd63e7936baf, 0xf8474c3bb7cdf024, 
	0x829f3cf387f8795d, 0x66e7a46c27f3aa2c, 0x1c3fd4a417c62355,
	0x935745fc4798b8de, 0xe98f353477ad31a7, 0xa6df411fbfb21ca3,
	0xdc0731d78f8795da, 0x536fa08fdfd90e51, 0x29b7d047efec8728
};
#else
static acl_uint64 crc64_table[256] = {
        0x0ll,
        0x7ad870c830358979ll, 0xf5b0e190606b12f2ll, 0x8f689158505e9b8bll,
	0xc038e5739841b68fll, 0xbae095bba8743ff6ll, 0x358804e3f82aa47dll,
	0x4f50742bc81f2d04ll, 0xab28ecb46814fe75ll, 0xd1f09c7c5821770cll,
	0x5e980d24087fec87ll, 0x24407dec384a65fell, 0x6b1009c7f05548fall,
	0x11c8790fc060c183ll, 0x9ea0e857903e5a08ll, 0xe478989fa00bd371ll,
	0x7d08ff3b88be6f81ll, 0x7d08ff3b88be6f8ll, 0x88b81eabe8d57d73ll,
	0xf2606e63d8e0f40all, 0xbd301a4810ffd90ell, 0xc7e86a8020ca5077ll,
	0x4880fbd87094cbfcll, 0x32588b1040a14285ll, 0xd620138fe0aa91f4ll,
	0xacf86347d09f188dll, 0x2390f21f80c18306ll, 0x594882d7b0f40a7fll,
	0x1618f6fc78eb277bll, 0x6cc0863448deae02ll, 0xe3a8176c18803589ll,
	0x997067a428b5bcf0ll, 0xfa11fe77117cdf02ll, 0x80c98ebf2149567bll,
	0xfa11fe77117cdf0ll, 0x75796f2f41224489ll, 0x3a291b04893d698dll,
	0x40f16bccb908e0f4ll, 0xcf99fa94e9567b7fll, 0xb5418a5cd963f206ll,
	0x513912c379682177ll, 0x2be1620b495da80ell, 0xa489f35319033385ll,
	0xde51839b2936bafcll, 0x9101f7b0e12997f8ll, 0xebd98778d11c1e81ll,
	0x64b116208142850all, 0x1e6966e8b1770c73ll, 0x8719014c99c2b083ll,
	0xfdc17184a9f739fall, 0x72a9e0dcf9a9a271ll, 0x8719014c99c2b08ll,
	0x4721e43f0183060cll, 0x3df994f731b68f75ll, 0xb29105af61e814fell,
	0xc849756751dd9d87ll, 0x2c31edf8f1d64ef6ll, 0x56e99d30c1e3c78fll,
	0xd9810c6891bd5c04ll, 0xa3597ca0a188d57dll, 0xec09088b6997f879ll,
	0x96d1784359a27100ll, 0x19b9e91b09fcea8bll, 0x636199d339c963f2ll, 
	0xdf7adabd7a6e2d6fll, 0xa5a2aa754a5ba416ll, 0x2aca3b2d1a053f9dll, 
	0x50124be52a30b6e4ll, 0x1f423fcee22f9be0ll, 0x659a4f06d21a1299ll,
	0xeaf2de5e82448912ll, 0x902aae96b271006bll, 0x74523609127ad31all,
	0xe8a46c1224f5a63ll, 0x81e2d7997211c1e8ll, 0xfb3aa75142244891ll, 
	0xb46ad37a8a3b6595ll, 0xceb2a3b2ba0eececll, 0x41da32eaea507767ll,
	0x3b024222da65fe1ell, 0xa2722586f2d042eell, 0xd8aa554ec2e5cb97ll,
	0x57c2c41692bb501cll, 0x2d1ab4dea28ed965ll, 0x624ac0f56a91f461ll, 
	0x1892b03d5aa47d18ll, 0x97fa21650afae693ll, 0xed2251ad3acf6feall, 
	0x95ac9329ac4bc9bll, 0x7382b9faaaf135e2ll, 0xfcea28a2faafae69ll, 
	0x8632586aca9a2710ll, 0xc9622c4102850a14ll, 0xb3ba5c8932b0836dll,
	0x3cd2cdd162ee18e6ll, 0x460abd1952db919fll, 0x256b24ca6b12f26dll, 
	0x5fb354025b277b14ll, 0xd0dbc55a0b79e09fll, 0xaa03b5923b4c69e6ll,
	0xe553c1b9f35344e2ll, 0x9f8bb171c366cd9bll, 0x10e3202993385610ll, 
	0x6a3b50e1a30ddf69ll, 0x8e43c87e03060c18ll, 0xf49bb8b633338561ll,
	0x7bf329ee636d1eeall, 0x12b592653589793ll, 0x4e7b2d0d9b47ba97ll, 
	0x34a35dc5ab7233eell, 0xbbcbcc9dfb2ca865ll, 0xc113bc55cb19211cll, 
	0x5863dbf1e3ac9decll, 0x22bbab39d3991495ll, 0xadd33a6183c78f1ell, 
	0xd70b4aa9b3f20667ll, 0x985b3e827bed2b63ll, 0xe2834e4a4bd8a21all,
	0x6debdf121b863991ll, 0x1733afda2bb3b0e8ll, 0xf34b37458bb86399ll, 
	0x8993478dbb8deae0ll, 0x6fbd6d5ebd3716bll, 0x7c23a61ddbe6f812ll,
	0x3373d23613f9d516ll, 0x49aba2fe23cc5c6fll, 0xc6c333a67392c7e4ll, 
	0xbc1b436e43a74e9dll, 0x95ac9329ac4bc9b5ll, 0xef74e3e19c7e40ccll,
	0x601c72b9cc20db47ll, 0x1ac40271fc15523ell, 0x5594765a340a7f3all, 
	0x2f4c0692043ff643ll, 0xa02497ca54616dc8ll, 0xdafce7026454e4b1ll,
	0x3e847f9dc45f37c0ll, 0x445c0f55f46abeb9ll, 0xcb349e0da4342532ll, 
	0xb1eceec59401ac4bll, 0xfebc9aee5c1e814fll, 0x8464ea266c2b0836ll,
	0xb0c7b7e3c7593bdll, 0x71d40bb60c401ac4ll, 0xe8a46c1224f5a634ll, 
	0x927c1cda14c02f4dll, 0x1d148d82449eb4c6ll, 0x67ccfd4a74ab3dbfll,
	0x289c8961bcb410bbll, 0x5244f9a98c8199c2ll, 0xdd2c68f1dcdf0249ll,
	0xa7f41839ecea8b30ll, 0x438c80a64ce15841ll, 0x3954f06e7cd4d138ll, 
	0xb63c61362c8a4ab3ll, 0xcce411fe1cbfc3call, 0x83b465d5d4a0eecell,
	0xf96c151de49567b7ll, 0x76048445b4cbfc3cll, 0xcdcf48d84fe7545ll,
	0x6fbd6d5ebd3716b7ll, 0x15651d968d029fcell, 0x9a0d8ccedd5c0445ll, 
	0xe0d5fc06ed698d3cll, 0xaf85882d2576a038ll, 0xd55df8e515432941ll, 
	0x5a3569bd451db2call, 0x20ed197575283bb3ll, 0xc49581ead523e8c2ll, 
	0xbe4df122e51661bbll, 0x3125607ab548fa30ll, 0x4bfd10b2857d7349ll,
	0x4ad64994d625e4dll, 0x7e7514517d57d734ll, 0xf11d85092d094cbfll, 
	0x8bc5f5c11d3cc5c6ll, 0x12b5926535897936ll, 0x686de2ad05bcf04fll, 
	0xe70573f555e26bc4ll, 0x9ddd033d65d7e2bdll, 0xd28d7716adc8cfb9ll, 
	0xa85507de9dfd46c0ll, 0x273d9686cda3dd4bll, 0x5de5e64efd965432ll,
	0xb99d7ed15d9d8743ll, 0xc3450e196da80e3all, 0x4c2d9f413df695b1ll,
	0x36f5ef890dc31cc8ll, 0x79a59ba2c5dc31ccll, 0x37deb6af5e9b8b5ll, 
	0x8c157a32a5b7233ell, 0xf6cd0afa9582aa47ll, 0x4ad64994d625e4dall, 
	0x300e395ce6106da3ll, 0xbf66a804b64ef628ll, 0xc5bed8cc867b7f51ll,
	0x8aeeace74e645255ll, 0xf036dc2f7e51db2cll, 0x7f5e4d772e0f40a7ll,
	0x5863dbf1e3ac9dell, 0xe1fea520be311aafll, 0x9b26d5e88e0493d6ll,
	0x144e44b0de5a085dll, 0x6e963478ee6f8124ll, 0x21c640532670ac20ll,
	0x5b1e309b16452559ll, 0xd476a1c3461bbed2ll, 0xaeaed10b762e37abll,
	0x37deb6af5e9b8b5bll, 0x4d06c6676eae0222ll, 0xc26e573f3ef099a9ll,
	0xb8b627f70ec510d0ll, 0xf7e653dcc6da3dd4ll, 0x8d3e2314f6efb4adll, 
	0x256b24ca6b12f26ll, 0x788ec2849684a65fll, 0x9cf65a1b368f752ell,
	0xe62e2ad306bafc57ll, 0x6946bb8b56e467dcll, 0x139ecb4366d1eea5ll, 
	0x5ccebf68aecec3a1ll, 0x2616cfa09efb4ad8ll, 0xa97e5ef8cea5d153ll, 
	0xd3a62e30fe90582all, 0xb0c7b7e3c7593bd8ll, 0xca1fc72bf76cb2a1ll,
	0x45775673a732292all, 0x3faf26bb9707a053ll, 0x70ff52905f188d57ll, 
	0xa2722586f2d042ell, 0x854fb3003f739fa5ll, 0xff97c3c80f4616dcll, 
	0x1bef5b57af4dc5adll, 0x61372b9f9f784cd4ll, 0xee5fbac7cf26d75fll, 
	0x9487ca0fff135e26ll, 0xdbd7be24370c7322ll, 0xa10fceec0739fa5bll,
	0x2e675fb4576761d0ll, 0x54bf2f7c6752e8a9ll, 0xcdcf48d84fe75459ll, 
	0xb71738107fd2dd20ll, 0x387fa9482f8c46abll, 0x42a7d9801fb9cfd2ll, 
	0xdf7adabd7a6e2d6ll, 0x772fdd63e7936bafll, 0xf8474c3bb7cdf024ll, 
	0x829f3cf387f8795dll, 0x66e7a46c27f3aa2cll, 0x1c3fd4a417c62355ll,
	0x935745fc4798b8dell, 0xe98f353477ad31a7ll, 0xa6df411fbfb21ca3ll,
	0xdc0731d78f8795dall, 0x536fa08fdfd90e51ll, 0x29b7d047efec8728ll
};
#endif  /* MS_VC6 */

acl_uint64 acl_hash_crc64(const void *buf, size_t len)
{
	const unsigned char* ptr = (const unsigned char *) buf;
	acl_uint64 crc = INITIALCRC;

	while (len-- > 0) {
		crc = crc64_table[(int)((crc ^ *ptr++) & 0xff)] ^ (crc >> 8);
	}

	return crc;
}

unsigned acl_hash_bin(const void *buf, size_t len)
{
	unsigned long h = 0;
	unsigned long g;
	const unsigned char *k = (const unsigned char *) buf;

	/*
	 * From the "Dragon" book by Aho, Sethi and Ullman.
	 */

	while (len-- > 0) {
		h = (h << 4) + *k++;
		if ((g = (h & 0xf0000000)) != 0) {
			h ^= (g >> 24);
			h ^= g;
		}
	}

	return (unsigned) h;
}

unsigned acl_hash_test(const void *buf, size_t len acl_unused)
{
	unsigned long result = 0;
	const unsigned char *ptr = (const unsigned char *) buf;
	int   c = 0;
	int   i = 0;

	for (i = 1; (c = *ptr++) != 0; i++)
		result += c * 3 * i;

	return (unsigned) result;
}

/* the following function(s) were adapted from
 * usr/src/lib/libc/db/hash_func.c, 4.4 BSD lite
 */

/* Phong Vo's linear congruential hash. */
#define	DCHARHASH(h, c)	((h) = 0x63c63cd9*(h) + 0x9c39c33d + (c))

unsigned acl_hash_func2(const void *buf, size_t len)
{
	const unsigned char *e, *k;
	unsigned h;
	unsigned char c;

	k = (const unsigned char *) buf;
	e = k + len;
	for (h = 0; k != e;) {
		c = *k++;
		if (!c && k > e)
			break;
		DCHARHASH(h, c);
	}

	return h;
}

/**
 * Ozan Yigit's original sdbm hash.
 *
 * Ugly, but fast.  Break the string up into 8 byte units.  On the first time
 * through the loop get the "leftover bytes" (strlen % 8).  On every other
 * iteration, perform 8 HASHC's so we handle all 8 bytes.  Essentially, this
 * saves us 7 cmp & branch instructions.
 */

unsigned acl_hash_func3(const void *buf, size_t len)
{
	const unsigned char *k;
	unsigned n, loop;

#define	HASHC	n = *k++ + 65599 * n
	n = 0;
	k = (const unsigned char *) buf;

	loop = ((unsigned int) len + 8 - 1) >> 3;
	switch (len & (8 - 1)) {
	case 0:
		do {
			HASHC;
	case 7:
			HASHC;
	case 6:
			HASHC;
	case 5:
			HASHC;
	case 4:
			HASHC;
	case 3:
			HASHC;
	case 2:
			HASHC;
	case 1:
			HASHC;
		} while (--loop);
	}

	return n;
}

/**
 * Chris Torek's hash function.  Although this function performs only
 * slightly worse than __ham_func5 on strings, it performs horribly on numbers.
 */

unsigned acl_hash_func4(const void *buf, size_t len)
{
	const unsigned char *k = (const unsigned char *) buf;
	size_t loop;
	unsigned int h;

#define HASH4a   h = (h << 5) - h + *k++;
#define HASH4b   h = (h << 5) + h + *k++;
#define HASH4 HASH4b

	h = 0;
	loop = len >> 3;
	switch (len & (8 - 1)) {
	case 0:
		break;
	case 7:
		HASH4;
		/* FALLTHROUGH */
	case 6:
		HASH4;
		/* FALLTHROUGH */
	case 5:
		HASH4;
		/* FALLTHROUGH */
	case 4:
		HASH4;
		/* FALLTHROUGH */
	case 3:
		HASH4;
		/* FALLTHROUGH */
	case 2:
		HASH4;
		/* FALLTHROUGH */
	case 1:
		HASH4;
	}

	while (loop--) {
		HASH4;
		HASH4;
		HASH4;
		HASH4;
		HASH4;
		HASH4;
		HASH4;
		HASH4;
	}

	return h;
}

/*
 * Fowler/Noll/Vo hash
 *
 * The basis of the hash algorithm was taken from an idea sent by email to the
 * IEEE Posix P1003.2 mailing list from Phong Vo (kpv@research.att.com) and
 * Glenn Fowler (gsf@research.att.com).  Landon Curt Noll (chongo@toad.com)
 * later improved on their algorithm.
 *
 * The magic is in the interesting relationship between the special prime
 * 16777619 (2^24 + 403) and 2^32 and 2^8.
 *
 * This hash produces the fewest collisions of any function that we've seen so
 * far, and works well on both numbers and strings.
 *
 */

unsigned acl_hash_func5(const void *buf, size_t len)
{
	const unsigned char *k, *e;
	unsigned h;

	k = (const unsigned char *) buf;
	e = k + len;
	for (h = 0; k < e; ++k) {
		h *= 16777619;
		h ^= *k;
	}

	return h;
}

/* come from squid */
unsigned acl_hash_func6(const void *buf, size_t len)
{
	const unsigned char *s = (const unsigned char *) buf;
	unsigned int n = 0;
	unsigned int j = 0;
	unsigned int i = 0;

	while (len-- > 0) {
		j++;
		n ^= 271 * (unsigned) *s++;
	}

	i = n ^ (j * 271);
	return i;
}
