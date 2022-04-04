/////////////////////////////////////////////////////////////////////////////
// StrConversion.h
//
// Copyright (C) 2009 Ideas of East Ltd. All Rights Reserved.
// http://www.ideasofeast.com
// info@ideasofeast.com
//
// Developers:
//   Veria Kalantari, veria@ideasofeast.com
//

#ifndef STRING_CONVERSION_INCLUDED
#define STRING_CONVERSION_INCLUDED
#pragma once

#include <windows.h>
#include <ctype.h>

namespace StrConversion
{

#define BASE16SYM ("0123456789ABCDEF")
#define BASE16VAL ("\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9:;<=>?@\xA\xB\xC\xD\xE\xF")

#define BASE16_ENCODELO(b) (BASE16SYM[((unsigned char)(b)) >> 4])
#define BASE16_ENCODEHI(b) (BASE16SYM[((unsigned char)(b)) & 0xF])

#define BASE16_DECODELO(b) (BASE16VAL[toupper(b) - '0'] << 4)
#define BASE16_DECODEHI(b) (BASE16VAL[toupper(b) - '0'])

class CStrConverter
{
public:
	CStrConverter(void);
	~CStrConverter(void);
	void HexStrToByteArray(const char *lpHexStr, unsigned char **lppByteArray, unsigned int *lpnBuffSize);
	void ByteArrayToHexStr(const unsigned char *lpByteArray, unsigned int nBuffSize, char **lppHexStr);
};

}

#endif
