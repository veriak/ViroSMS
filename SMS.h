/////////////////////////////////////////////////////////////////////////////
// SMS.h
//
// Copyright (C) 2013 Veria Kalantari
// veria.kalantary@gmail.com
// https://github.com/veriak
//

#ifndef SMS_INCLUDED
#define SMS_INCLUDED
#pragma once

#include <windows.h>
#include <list>
#include <tchar.h>

#include "MemoryManagement.h"
#include "SerialCommPort.h"

using namespace SerialCommPort;

namespace SMS
{

#ifndef WM_USER_SMS_EVENT
#define WM_USER_SMS_EVENT		WM_USER + 201
#endif

#ifndef SIZEOFBUFF
#define SIZEOFBUFF				1024
#endif

class CSMS : public CSerialCommPort
{
protected:
	HINSTANCE m_hInstance;
	HWND m_hWnd;
	LPTSTR m_ptcsPortName;
	LPTSTR m_ptcsErrorMsgBuff;
	DWORD m_dwLastError;
	LPVOID m_pReadBuff;
	int m_nReadBuffSize;
	int m_nTotalReadSize;	
	
protected:
	void ClearBuffer(LPVOID &lpBuff);
	virtual void CleanUp(void);	
	
public:
	CSMS(void);
	CSMS(LPCTSTR lptcsPortName, DWORD dwBaudRate = 115200);
	~CSMS(void);

	virtual void Clear(void);
	DWORD _GetLastError(void);

	void Init(void);
	void Init(LPCTSTR lptcsPortName, DWORD dwBaudRate = 115200);

	void SetWnd(HWND hWnd);

	static LRESULT CALLBACK OnCommReadEvent(CSMS *_this, DWORD dwBytesRead);
};

}

#endif