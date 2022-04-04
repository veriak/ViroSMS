/////////////////////////////////////////////////////////////////////////////
// StrConversion.cpp
//
// Copyright (C) 2013 Veria Kalantari
// veria.kalantary@gmail.com
// https://github.com/veriak
//

#include "stdafx.h"
#include "StrConversion.h"

using namespace StrConversion;

/////////////////////////////////////////////////////////////////////////////
// CStrConverter

CStrConverter::CStrConverter(void)
{
}

CStrConverter::~CStrConverter(void)
{
}

void CStrConverter::HexStrToByteArray(const char *lpHexStr, unsigned char **lppByteArray,
									  unsigned int *lpnBuffSize)
{
	*lpnBuffSize = (unsigned int) strlen(lpHexStr) / 2;
	unsigned char *lpByteArray = (unsigned char *) malloc(*lpnBuffSize);	
	*lppByteArray = lpByteArray;
	char *lpSource = (char *) lpHexStr;

	while (*lpSource != '\0')
	{
		lpByteArray[0] = BASE16_DECODELO(lpSource[0]);
		lpByteArray[0] |= BASE16_DECODEHI(lpSource[1]);
		lpByteArray++;
		lpSource += 2;
	}	
}

void CStrConverter::ByteArrayToHexStr(const unsigned char *lpByteArray, unsigned int nBuffSize,
									  char **lppHexStr)
{
	char *lpHexStr = (char *) malloc(nBuffSize * 2 + 1);
	unsigned int nCount = 0;

	*lppHexStr = lpHexStr;

	while (nCount < nBuffSize)
	{
		lpHexStr[0] = BASE16_ENCODELO(*lpByteArray);
		lpHexStr[1] = BASE16_ENCODEHI(*lpByteArray);

		lpHexStr += 2;
		lpByteArray++;
		nCount++;
	}

	*lpHexStr = '\0';
}