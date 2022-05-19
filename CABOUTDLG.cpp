// CABOUTDLG.cpp: 实现文件
//

#include "pch.h"
#include "SPI_Programmer.h"
#include "CABOUTDLG.h"
#include "afxdialogex.h"


// CABOUTDLG 对话框

IMPLEMENT_DYNAMIC(CABOUTDLG, CDialogEx)

CABOUTDLG::CABOUTDLG(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ABOUTBOX, pParent)
{

}

CABOUTDLG::~CABOUTDLG()
{
}

void CABOUTDLG::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CABOUTDLG, CDialogEx)
END_MESSAGE_MAP()


// CABOUTDLG 消息处理程序
