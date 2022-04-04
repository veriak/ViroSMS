#include "stdafx.h"
#include "MemoryManagement.h"

void SetLength(LPVOID* lppBuff, UINT nElementSize, UINT nCount)
{
	HANDLE hHeap = ::GetProcessHeap();
	LPVOID pBuff = *lppBuff;

	if (nCount == 0)
	{
		::HeapFree(hHeap, 0, pBuff);
		*lppBuff = NULL;
	}
	else
	{
		if (pBuff == NULL)
			*lppBuff = ::HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nElementSize * nCount);
		else
			*lppBuff = ::HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, pBuff, nElementSize * nCount);			
	}
}

BOOL StrToWideStr(char *lpStr, wchar_t **lppWideStr)
{			
	int nBuffLen = strlen(lpStr);

	if (nBuffLen)
	{
		nBuffLen += 1;
		nBuffLen *= 2;
		*lppWideStr = ::SysAllocStringByteLen(NULL, nBuffLen);
		::MultiByteToWideChar(CP_ACP, 0, lpStr, -1, *lppWideStr, nBuffLen);

		return TRUE;
	}
	else
		return FALSE;
}

BOOL WideStrToStr(wchar_t *lpWideStr, char **lppStr)
{
	*lppStr = NULL;

	int nBuffLen = ::WideCharToMultiByte(CP_ACP, 0, lpWideStr, -1, *lppStr, 0, NULL, NULL);

	if (nBuffLen > 0)
	{
		*lppStr = (char *)::GlobalAlloc(GPTR, nBuffLen);
		::WideCharToMultiByte(CP_ACP, 0, lpWideStr, -1, *lppStr, 0, NULL, NULL);

		return TRUE;
	}
	else
		return FALSE;
}

void FreeWideStr(wchar_t **lppWideStr)
{
	::SysFreeString(*lppWideStr);	
	*lppWideStr = NULL;
}