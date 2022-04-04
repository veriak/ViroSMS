/////////////////////////////////////////////////////////////////////////////
// SerialCommPort.h
//
// Copyright (C) 2013 Veria Kalantari
// veria.kalantary@gmail.com
// https://github.com/veriak
//

#ifndef SERIAL_COMPORT_INCLUDED
#define SERIAL_COMPORT_INCLUDED
#pragma once

#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <process.h>
#include <tchar.h>

#include "MemoryManagement.h"

namespace SerialCommPort
{

#ifndef SIZEOFCOMMBUFF
#define SIZEOFCOMMBUFF				1024
#endif

class CSerialCommPort  
{
private:
	LPVOID m_pReadBuff;
	int m_nReadBuffSize;
	int m_nTotalReadSize;
	HWND m_hWnd;
	UINT m_uReadMsg;
	LRESULT CALLBACK m_pCallBackFunc;
	HANDLE m_hCommPort;
	HANDLE m_hThreadTerm;
	HANDLE m_hThread;
	HANDLE m_hDataRx;		
	CRITICAL_SECTION m_csLock;

private:
	BOOL CloseAndCleanHandle(HANDLE &hHandle);

public:
	CSerialCommPort();
	virtual ~CSerialCommPort();

	void LockThis();
	void UnLockThis();
	void InitLock();
	void DelLock();

	void SetDataReadEvent();

	HRESULT	Write(const void *lpBuffer, DWORD dwBuffSize);	
	HRESULT	Init(LPCTSTR lptcsPortName, DWORD dwBaudRate = 9600, BYTE byParity = NOPARITY,
		BYTE byStopBits = 1, BYTE byByteSize = 8);
	HRESULT	Init(HWND hWnd, UINT uReadMsg, LPCTSTR lptcsPortName, DWORD dwBaudRate = 9600,
		BYTE byParity = NOPARITY, BYTE byStopBits = 1, BYTE byByteSize = 8);
	HRESULT	Init(LRESULT CALLBACK lpCallBackFunc, LPCTSTR lptcsPortName, DWORD dwBaudRate = 9600,
		BYTE byParity = NOPARITY, BYTE byStopBits = 1, BYTE byByteSize = 8);
	HRESULT	UnInit();

	void StartCommRead();
	void SetCommRead(HWND hWnd, UINT uReadMsg);
	void SetCommRead(LRESULT CALLBACK lpCallBackFunc);
	BOOL ReadExact(void *lpReadBuff, UINT nBuffSize, UINT nReadSize, UINT *lpNeededSize);
	BOOL ReadAll(void *lpReadBuff, UINT nBuffSize, UINT *lpNeededSize);
	
	static unsigned __stdcall ThreadFn(void *pvParam);
};

}

#endif