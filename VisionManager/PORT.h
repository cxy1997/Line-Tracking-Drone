
#pragma once
#include <iostream>
#include <Windows.h>
using namespace std;

#ifndef ACTIONS
#define ACTIONS
#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80
#define M1P1 BIT1
#define M1P2 BIT2
#define M1EN BIT3
#define M2P1 BIT4
#define M2P2 BIT5
#define M2EN BIT0
#define STOP 0x00
#define FORWARD (M1P1|M2P1)
#define BACK (M1P2|M2P2)
#define LEFT (M1P1|M2P2)
#define RIGHT (M1P2|M2P1)
#endif

class ComPort
{
public:
	HANDLE hCom;
	DWORD dwError;
	BOOL bComOpened;
	OVERLAPPED wrOverlapped;
	ComPort(LPCWSTR portName)
	{
		hCom = NULL;
		dwError = NULL;
		bComOpened = NULL;
		hCom = CreateFile(portName,
			//文件名，这里是串口号
			GENERIC_READ | GENERIC_WRITE,
			//充许读和写
			0,
			//独占方式
			NULL,
			OPEN_EXISTING, //打洙开而不是创建
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, //异皑步I/O重叠
			NULL);
		if(hCom == INVALID_HANDLE_VALUE)
		{
			dwError = GetLastError();
			//取得错误代码
			std::cout<<portName<<" Opened failed. Error code: "<<dwError<<std::endl;
		}
		else
		{
			bComOpened = TRUE;
			std::cout<<portName<<" Opened successful."<<std::endl;
		}
		if(!SetupComm(hCom,2048,2048))
		//输入缓冲区和输出缓冲区的大小
		{
			std::cout<<"SetupComm fail! Close Comm!"<<endl;
			CloseHandle(hCom);
		}
		else
			std::cout<<"SetupComm OK!"<<endl;
		COMMTIMEOUTS TimeOuts;
		memset(&TimeOuts,0,sizeof(TimeOuts));
		// read timeout
		TimeOuts.ReadIntervalTimeout=1000;
		TimeOuts.ReadTotalTimeoutMultiplier=500;
		TimeOuts.ReadTotalTimeoutConstant=5000;
		// write timeout
		TimeOuts.WriteTotalTimeoutMultiplier=500;
		TimeOuts.WriteTotalTimeoutConstant=2000;
		SetCommTimeouts(hCom,&TimeOuts); //set timeout
		DCB dcb;
		if (!GetCommState(hCom,&dcb))
		{
			std::cout<<"GetCommState fail! Comm close"<<endl;
			CloseHandle(hCom);
		}
		else
			std::cout<<"GetCommState OK!"<<endl;
		dcb.DCBlength = sizeof(dcb);
		if (!BuildCommDCB(L"9600,n,8,1",&dcb))//填充DCB的数据传输率
		{
			std::cout<<"BuileCOmmDCB fail,Comm close!"<<endl;
			CloseHandle(hCom);
		}
		if(SetCommState(hCom,&dcb))
			std::cout<<"SetCommState OK!"<<endl;
		ZeroMemory(&wrOverlapped,sizeof(wrOverlapped));
		if (wrOverlapped.hEvent != NULL)
		{
			ResetEvent(wrOverlapped.hEvent);
			wrOverlapped.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
		}
		PurgeComm(hCom,PURGE_TXCLEAR|PURGE_RXCLEAR);
		//clear port buffer
		txIndex=0;
		rxIndex=0;
	}
	virtual ~ComPort(void)
	{
		if(bComOpened)
			CloseHandle(hCom);
	}
	unsigned char rxIndex;
	unsigned char lpInBuffer[1024];
	int Sync(void)
	{
		int i=1000;
		while(rxIndex!=txIndex && txIndex!=0 && (--i))
		{
			int size=Read();
			for(int i=0;i<size;i++)
				if(((lpInBuffer[i]&0x80)==0) && (lpInBuffer[i]!=0))
					rxIndex=lpInBuffer[i];
		}
		if(i<1)
			return -1;
		return rxIndex;
	}
	int Read(void)
	{
		DWORD dwBytesRead=1024;
		COMSTAT ComStat;
		DWORD dwErrorFlags;
		OVERLAPPED m_osRead;
		memset(&m_osRead,0,sizeof(OVERLAPPED));
		m_osRead.hEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
		ClearCommError(hCom,&dwErrorFlags,&ComStat);
		dwBytesRead=min(dwBytesRead,(DWORD)ComStat.cbInQue);
		if(!dwBytesRead)
			return FALSE;
		BOOL bReadStatus;
		bReadStatus=ReadFile(hCom,lpInBuffer,
		dwBytesRead,&dwBytesRead,&m_osRead);
		if(!bReadStatus) //如?果?ReadFile函ˉ数簓返う?回?FALSE
		{
			if(GetLastError()==ERROR_IO_PENDING)
			{
				WaitForSingleObject(m_osRead.hEvent,2000);
				PurgeComm(hCom, PURGE_TXABORT|
				PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
				return dwBytesRead;
			}
			return 0;
		}
		PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
		return dwBytesRead;
	}
	unsigned char txIndex;
	int Send(unsigned char command)
	{
		//command |= 0x80;
		++txIndex;
		txIndex &=~0x80;
		DWORD dwError;
		DWORD dwWantSend = 1;
		DWORD dwRealSend = 0;
		char* pReadBuf = NULL;
		char pSendBuffer[]={command};
		if (ClearCommError(hCom,&dwError,NULL))
		{
			PurgeComm(hCom,PURGE_TXABORT | PURGE_TXCLEAR);
			//std::cout<<"PurgeComm OK!"<<endl;
		}
		if (!WriteFile(hCom,pSendBuffer,dwWantSend,&dwRealSend,&wrOverlapped))
		{
			dwError=GetLastError();
			if ( dwError == ERROR_IO_PENDING)
			{
				while (!GetOverlappedResult(hCom,&wrOverlapped,&dwRealSend,FALSE))
				{
					if (GetLastError() == ERROR_IO_INCOMPLETE)
					{
						//std::cout<<"Sending..."<<endl;
						continue;
					}
					else
					{
						std::cout<<"Send failed. Error code: "<<dwError<<endl;
						ClearCommError(hCom,&dwError,NULL);
						break;
					}
				}
			}
		}
		return dwRealSend;
	}
};

