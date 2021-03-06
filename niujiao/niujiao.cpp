/*
项目主程序文件：包含窗口相关的处理函数
*/
#include "stdafx.h"
#include <stdlib.h>
#include <Commdlg.h>
#include <process.h>
#include "niujiao.h"
#include "DbgEngine/asm.h"
#include "PubLib/NiujiaoMod.h"

#define MAX_SUB_WINDOW 100
s_wnd_list* gWndHead = nullptr;

static LPCTSTR gWndXmlName[MAX_SUB_WINDOW] = { nullptr }; //建立窗口的xml文件
CPubWnd* gWndPtr[MAX_SUB_WINDOW] = { nullptr };  //窗口指针
CStrTrie* CPubWnd::m_strTrie = nullptr;
Executor* CPubWnd::m_executor = nullptr;

TCHAR MainWndXml[16] = _T("MainWnd.xml");
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
#ifdef MOMERY_LEAK_DECTET 
	_CrtSetBreakAlloc(2699);
#endif
	
	CPaintManagerUI::SetInstance(hInstance);
	CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());
	CoInitialize(nullptr);

	CPubWnd* MainFrameWnd = new CPubWnd(MainWndXml);
	MainFrameWnd->Create(nullptr, _T("牛角调试器"), UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE | WS_EX_ACCEPTFILES);

	MainFrameWnd->CenterWindow();
	MainFrameWnd->ShowWindow(true);
	gWndXmlName[0] = MainWndXml;
	gWndPtr[0] = MainFrameWnd;
	CPaintManagerUI::MessageLoop();
	delete MainFrameWnd;

	CoUninitialize();

#ifdef MOMERY_LEAK_DECTET
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}

LRESULT CPubWnd::OnNcHitTest(UINT uMsg, WPARAM wParam, const LPARAM lParam, BOOL& bHandled)
{
	POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
	::ScreenToClient(*this, &pt);

	RECT RcClient;
	::GetClientRect(*this, &RcClient);

	if (!::IsZoomed(*this))
	{
		const RECT RcSizeBox = m_PaintManager.GetSizeBox();
		if (pt.y < RcClient.top + RcSizeBox.top)
		{
			if (pt.x < RcClient.left + RcSizeBox.left) return HTTOPLEFT;
			if (pt.x > RcClient.right - RcSizeBox.right) return HTTOPRIGHT;
			return HTTOP;
		}
		else if (pt.y > RcClient.bottom - RcSizeBox.bottom)
		{
			if (pt.x < RcClient.left + RcSizeBox.left) return HTBOTTOMLEFT;
			if (pt.x > RcClient.right - RcSizeBox.right) return HTBOTTOMRIGHT;
			return HTBOTTOM;
		}

		if (pt.x < RcClient.left + RcSizeBox.left) return HTLEFT;
		if (pt.x > RcClient.right - RcSizeBox.right) return HTRIGHT;
	}

	const RECT RcCaption = m_PaintManager.GetCaptionRect();
	if (pt.x >= RcClient.left + RcCaption.left && pt.x < RcClient.right - RcCaption.right \
		&& pt.y >= RcCaption.top && pt.y < RcCaption.bottom) {
		CControlUI* pControl = static_cast<CControlUI*>(m_PaintManager.FindControl(pt));
		if (pControl && _tcsicmp(pControl->GetClass(), _T("ButtonUI")) != 0 &&
			_tcsicmp(pControl->GetClass(), _T("OptionUI")) != 0 &&
			_tcsicmp(pControl->GetClass(), _T("SliderUI")) != 0 &&
			_tcsicmp(pControl->GetClass(), _T("EditUI")) != 0 &&
			_tcsicmp(pControl->GetClass(), _T("RichEditUI")) != 0)
			return HTCAPTION;
	}

	return HTCLIENT;
}

LRESULT CPubWnd::OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	UpdateWindowBackground();
	return LRESULT();
}
/**
 * \brief
 * \param fileName
 */
void CPubWnd::PeReadAttribute(const LPCTSTR fileName) const
{
	CImageInfo* ImageInfo = new CImageInfo();
	if (ImageInfo->ReadImageFromFile(fileName) == false)
	{
		MessageBox(NULL, _T("读取PE文件出错"), _T("错误提示"), MB_OK);
		return;
	}
	PeFillGeneral(ImageInfo);
	PeFillDataDir(ImageInfo);
	PeFillSeg(ImageInfo);
	PeFillImport(ImageInfo);
	PeFillExport(ImageInfo);
	delete ImageInfo;
}

/**
 * \brief
 * \param tmp
 */
void CPubWnd::PeFillGeneral(CImageInfo * tmp) const
{
	OPTIONAL_PE_HEADER* OptionalPeHeader = tmp->GetOptionalHeader();

	TCHAR str[64] = { 0 };
	wsprintf(str, _T("0x%X"), OptionalPeHeader->AddressOfEntryPoint);
	static_cast<CEditUI*>(m_PaintManager.FindControl(_T("pe_entrance")))->SetText(str);

	wsprintf(str, _T("%d"), tmp->GetNumberOfSections());
	static_cast<CEditUI*>(m_PaintManager.FindControl(_T("pe_numofsec")))->SetText(str);

	wsprintf(str, _T("0x%X"), tmp->GetOptionalHeaderSize());
	static_cast<CEditUI*>(m_PaintManager.FindControl(_T("pe_optionh")))->SetText(str);

	wsprintf(str, _T("0x%X"), OptionalPeHeader->ImageBase);
	static_cast<CEditUI*>(m_PaintManager.FindControl(_T("pe_imagebase")))->SetText(str);

	wsprintf(str, _T("0x%X"), tmp->GetImageSize());
	static_cast<CEditUI*>(m_PaintManager.FindControl(_T("pe_imagesize")))->SetText(str);

	wsprintf(str, _T("0x%X"), tmp->GetNumOfRVA());
	static_cast<CEditUI*>(m_PaintManager.FindControl(_T("pe_numofrva")))->SetText(str);

	wsprintf(str, _T("0x%X"), OptionalPeHeader->BaseOfCode);
	static_cast<CEditUI*>(m_PaintManager.FindControl(_T("pe_codebase")))->SetText(str);
	wsprintf(str, _T("0x%X"), tmp->GetBaseOfData());
	static_cast<CEditUI*>(m_PaintManager.FindControl(_T("pe_database")))->SetText(str);

	wsprintf(str, _T("0x%X"), tmp->GetSizeOfHeaders());
	static_cast<CEditUI*>(m_PaintManager.FindControl(_T("pe_headsize")))->SetText(str);

	wsprintf(str, _T("0x%X"), tmp->GetAlignmentOfBlock());
	static_cast<CEditUI*>(m_PaintManager.FindControl(_T("pe_bkalign")))->SetText(str);

	wsprintf(str, _T("0x%X"), tmp->GetAlignmentOfFile());
	static_cast<CEditUI*>(m_PaintManager.FindControl(_T("pe_filealign")))->SetText(str);
	wsprintf(str, _T("%X"), tmp->GetCheckSum());
	static_cast<CEditUI*>(m_PaintManager.FindControl(_T("pe_checksum")))->SetText(str);

	CComboUI* Combo = static_cast<CComboUI*>(m_PaintManager.FindControl(_T("pe_machn")));
	switch (tmp->GetMachine())
	{
	case 0:Combo->SelectItem(0); break;
	case 0x1d3:Combo->SelectItem(1); break;
	case 0x8664:Combo->SelectItem(2); break;
	case 0x1c0:Combo->SelectItem(3); break;
	case 0xebc:Combo->SelectItem(4); break;
	case 0x14c:Combo->SelectItem(5); break;
	case 0x200:Combo->SelectItem(6); break;
	case 0x9041:Combo->SelectItem(7); break;
	case 0x266:Combo->SelectItem(8); break;
	case 0x366:Combo->SelectItem(9); break;
	case 0x466:Combo->SelectItem(10); break;
	case 0x1f0:Combo->SelectItem(11); break;
	case 0x1f1:Combo->SelectItem(12); break;
	case 0x166:Combo->SelectItem(13); break;
	case 0x1a2:Combo->SelectItem(14); break;
	case 0x1a3:Combo->SelectItem(14); break;
	case 0x1a6:Combo->SelectItem(14); break;
	case 0x1a8:Combo->SelectItem(14); break;
	case 0x1c2:Combo->SelectItem(14); break;
	case 0x169:Combo->SelectItem(14); break;
	default:;
	}

	const DWORD Characteristic = tmp->GetCharacteritic();
	wsprintf(str, _T(" 0x%X (该域取位或值，以下是各个相关的位)"), Characteristic);
	static_cast<CListLabelElementUI*>(m_PaintManager.FindControl(_T("pe_c_0")))->SetText(str);
	static_cast<CListLabelElementUI*>(m_PaintManager.FindControl(_T("pe_c_0")))->SetBkColor(0xff4cb4e7);
	static_cast<CComboUI*>(m_PaintManager.FindControl(_T("pe_charater")))->SelectItem(0);
	for (UINT Counter = 0; Counter < 16; Counter++)
	{
		const UINT Off = 1;
		wsprintf(str, _T("pe_c_%d"), Counter + 1);
		if (Characteristic&(Off << Counter))
			static_cast<CListLabelElementUI*>(m_PaintManager.FindControl(str))->SetVisible(true);
		else
			static_cast<CListLabelElementUI*>(m_PaintManager.FindControl(str))->SetVisible(false);
	}

	Combo = static_cast<CComboUI*>(m_PaintManager.FindControl(_T("pe_subsystem")));
	switch (tmp->GetSubSystem())
	{
	case 0:
	case 1:
	case 2:
	case 3:Combo->SelectItem(tmp->GetSubSystem()); break;
	case 7:Combo->SelectItem(4); break;
	case 9:Combo->SelectItem(5); break;
	case 10:Combo->SelectItem(6); break;
	case 11:Combo->SelectItem(7); break;
	case 12:Combo->SelectItem(8); break;
	case 13:Combo->SelectItem(9); break;
	case 14:Combo->SelectItem(10); break;
	}
	CDateTimeUI* DateTime = static_cast<CDateTimeUI*>(m_PaintManager.FindControl(_T("pe_datetime")));
	SYSTEMTIME SystemTime = { 0 };
	FILETIME FileTime = { 0 };
	FileTime.dwLowDateTime = tmp->GetDateTimeStamp();
	FileTimeToSystemTime(&FileTime, &SystemTime);
	DateTime->SetTime(&SystemTime);
}
void CPubWnd::PeFillDataDir(CImageInfo * tmp) const
{
	TCHAR str[16];
	TCHAR DataName[][20] = { _T("  导出表"), _T("  导入表"), _T("  资源表"), _T("  异常表"), _T("  属性证书表"), _T("  基址重定位表"),
		_T("  调试数据起始地址"), _T("  全局指针"), _T("  线程局部存储"), _T("  加载配置表"), _T("  绑定导入表"), _T("  导入地址表"), _T("  延迟导入描述表"),
		_T("  CLR运行时") };
	DATA_DIRECTORY* dd = tmp->GetDataDirectory();
	static_cast<CListUI*>(m_PaintManager.FindControl(_T("pe_datadirlist")))->RemoveAll();
	for (int i = 0; i < DD_MAX_DIRECTORY_NAME_VALUE; i++)
	{
		CListTextElementUI* p = new CListTextElementUI();

		static_cast<CListUI*>(m_PaintManager.FindControl(_T("pe_datadirlist")))->Add(p);
		p->SetText(0, DataName[i]);
		wsprintf(str, _T("  0x%X"), (dd + i)->VirtualAddress);
		p->SetText(1, str);
		wsprintf(str, _T("  0x%X"), (dd + i)->Size);
		p->SetText(2, str);
	}
}
/**
 * \brief
 * \param tmp
 */
void CPubWnd::PeFillSeg(CImageInfo * tmp) const
{
	TCHAR Str[16];
	PE_SECTION_HEADER* PeSectionHeader = tmp->GetSectionHeader();
	static_cast<CListUI*>(m_PaintManager.FindControl(_T("pe_seglist")))->RemoveAll();
	for (UINT Seq = 0; Seq < tmp->GetNumberOfSections(); Seq++)
	{
		CListTextElementUI* p = new CListTextElementUI();
		static_cast<CListUI*>(m_PaintManager.FindControl(_T("pe_seglist")))->Add(p);
		MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<char*>((PeSectionHeader + Seq)->Name), 8, Str, 16);
		p->SetText(0, Str);
		wsprintf(Str, _T("  0x%X"), (PeSectionHeader + Seq)->VirtualAddress);
		p->SetText(1, Str);
		wsprintf(Str, _T("  0x%X"), (PeSectionHeader + Seq)->VirtualSize);
		p->SetText(2, Str);
		wsprintf(Str, _T("  0x%X"), (PeSectionHeader + Seq)->PointerToRawData);
		p->SetText(3, Str);
		wsprintf(Str, _T("  0x%X"), (PeSectionHeader + Seq)->SizeOfRawData);
		p->SetText(4, Str);
		wsprintf(Str, _T("  %X"), (PeSectionHeader + Seq)->Characteristics);
		p->SetText(5, Str);
	}
}
void CPubWnd::PeFillImport(CImageInfo * tmp) const
{
	static_cast<CListUI*>(m_PaintManager.FindControl(_T("pe_impfunc")))->RemoveAll();
	static_cast<CListUI*>(m_PaintManager.FindControl(_T("pe_impdll")))->RemoveAll();
	DATA_DIRECTORY* DataDir = tmp->GetDataDirectory();
	UINT64 ImpTblFileOffset = tmp->VoaToFoa((DataDir + 1)->VirtualAddress);
	if (ImpTblFileOffset)
	{
		for (int Seq = 0;; Seq++)
		{

			IMPORT_DIRECTORT_TABLE* ImportDirectoryTable = (IMPORT_DIRECTORT_TABLE*)((UINT64)tmp->GetMapFileAddr() + ImpTblFileOffset) + Seq;
			if (ImportDirectoryTable->ForwarderChain == 0
				&& ImportDirectoryTable->ImportAdressTableRVA == 0
				&& ImportDirectoryTable->ImportLookUpTableRVA == 0
				&& ImportDirectoryTable->NameRVA == 0
				&& ImportDirectoryTable->TimeDateStamp == 0)
			{
				break;
			}
			UINT64 TmpNameAddr = tmp->VoaToFoa(ImportDirectoryTable->ImportAdressTableRVA) + (UINT64)tmp->GetMapFileAddr();  //函数名称总表 pecoff 44

			TCHAR WideCharName[128] = { 0 };
			TCHAR wSeq[4] = { 0 };
			for(int FuncNum=0;; FuncNum++)
			{				
				memset(WideCharName, 0x00, sizeof(WideCharName));	
				if (tmp->Is32Image())
				{
					if (*((UINT*)TmpNameAddr + FuncNum) == 0) break;
					//部分函数是由序号导出的，而非名称
					if (*((UINT*)TmpNameAddr + FuncNum) & 0x80000000)
					{
						wsprintf(WideCharName, _T("%08X"), *((UINT*)TmpNameAddr + FuncNum)- 0x80000000);
					}
					else
					{
						UINT TmpAddr = (UINT)tmp->VoaToFoa(*(UINT*)(TmpNameAddr + FuncNum)) + (UINT)tmp->GetMapFileAddr() + 2;
						MultiByteToWideChar(CP_ACP, 0, (char*)TmpAddr, strlen((char*)TmpAddr), WideCharName, sizeof(WideCharName));
					}
				}
				else
				{
					if (*((UINT64*)TmpNameAddr + FuncNum) == 0) break;
					if (*((UINT64*)TmpNameAddr + FuncNum) & 0x8000000000000000)
					{
						wsprintf(WideCharName, _T("%08X"), *((UINT64*)TmpNameAddr + FuncNum)- 0x8000000000000000);
					}
					else
					{
						UINT64 TmpAddr = tmp->VoaToFoa(*((UINT64*)TmpNameAddr + FuncNum)) + (UINT64)tmp->GetMapFileAddr() + 2;
						MultiByteToWideChar(CP_ACP, 0, (char*)TmpAddr, strlen((char*)TmpAddr), WideCharName, sizeof(WideCharName));
					}
				}
				CListTextElementUI* p = new CListTextElementUI();
				static_cast<CListUI*>(m_PaintManager.FindControl(_T("pe_impfunc")))->Add(p);
				p->SetText(0, WideCharName);
				_itot(Seq, wSeq, 10);
				p->SetUserData(wSeq);
			}

			//返回dll名称
			TmpNameAddr = tmp->VoaToFoa(ImportDirectoryTable->NameRVA) + (UINT64)tmp->GetMapFileAddr();

			memset(WideCharName, 0x00, sizeof(WideCharName));
			MultiByteToWideChar(CP_ACP, 0, (char*)TmpNameAddr, strlen((char*)TmpNameAddr), WideCharName, sizeof(WideCharName));
			CListTextElementUI* p = new CListTextElementUI();
			static_cast<CListUI*>(m_PaintManager.FindControl(_T("pe_impdll")))->Add(p);
			p->SetText(0, WideCharName);
		}
		static_cast<CListUI*>(m_PaintManager.FindControl(_T("pe_impdll")))->SelectItem(0);
	}
}
void CPubWnd::PeFillExport(CImageInfo * tmp) const
{
}

TCHAR* CPubWnd::GenerateClassName()
{
#ifdef _UNICODE
	TCHAR* ClassName = (TCHAR*)malloc(32);
	memset(ClassName, 0x00, 32);
	char RandValue[16] = { 0 };
#else
	char* RandValue = (char*)malloc(16);
	memset(RandValue, 0x00, 16);
#endif
	srand(GetTickCount());
	int a = rand();
	int size = a % 3 + 5;
	for (int i = 0; i < size; i++)
	{
		a = rand();
		RandValue[i] = (33 + a % 93);
	}

#ifdef _UNICODE
	MultiByteToWideChar(CP_ACP, 0, RandValue, sizeof(RandValue), ClassName, 32);
	return ClassName;
#else
	return RandValue;
#endif
}

void CPubWnd::UpdateWindowBackground()
{
	HWND  hWnd = GetForegroundWindow();
	RECT rc;
	GetClientRect(hWnd, &rc);
	RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
	
}

void CPubWnd::HandleCommandEvt(CPubWnd* pBase, TNotifyUI& msg, int Value)
{
	LPCTSTR TmpName = msg.pSender->GetUserData();
	if (TmpName != nullptr && lstrlen(TmpName))
	{
		char ProcName[1024] = { 0 };
		WideCharToMultiByte(CP_ACP, 0, TmpName, lstrlen(TmpName), ProcName, sizeof(ProcName), nullptr, nullptr);
		system(ProcName); //TODO 运行会闪过一个控制台窗口  
	}
}

void CPubWnd::HandleNewConsole(CPubWnd * pBase, TNotifyUI & msg, int Value)
{
	_beginthread(CPubWnd::ConsoleThreadFunc, 16*1024, NULL);
}

void CPubWnd::HandleNewWndEvt(CPubWnd* pBase, TNotifyUI& msg, int Value)
{
	LPCTSTR XmlName = msg.pSender->GetUserData().GetData();
	if (XmlName != nullptr && lstrlen(XmlName))
	{
		//登记窗口
		int i = 0;
		for (i = 0; gWndXmlName[i] && i < MAX_SUB_WINDOW; i++)
		{
			if (gWndXmlName[i] == XmlName)
			{
				static_cast<CPubWnd*>(gWndPtr[i])->ShowWindow(True);
				return;
			}
		}
		//TODO 如果添加了换肤功能 xml名称地址应该会改变，此处也要跟着改
		CPubWnd *SubDlg = new CPubWnd(XmlName);
		SubDlg->hWnd = SubDlg->Create(pBase->GetHWND(), msg.pSender->GetText(), WS_CHILD | WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);

		RECT rc1, rc2;
		GetWindowRect(SubDlg->hWnd, &rc1);
		GetWindowRect(pBase->GetHWND(), &rc2);

		MoveWindow(SubDlg->hWnd, ((rc2.right - rc2.left) - (rc1.right - rc1.left)) / 2, ((rc2.bottom - rc2.top) - (rc1.bottom - rc1.top)) / 2, rc1.right - rc1.left, rc1.bottom - rc1.top, True);
		SubDlg->ShowWindow(True);

		gWndPtr[i] = SubDlg;
		gWndXmlName[i] = XmlName;
	}
}

void CPubWnd::HandleFunction(CPubWnd* pBase, TNotifyUI& msg, int value)
{
	char NameStr[MAX_STRING_LENGTH] = { 0 };
	LPCTSTR TmpStr = msg.pSender->GetName().GetData();
	WideCharToMultiByte(CP_ACP, 0, TmpStr, lstrlen(TmpStr), NameStr, MAX_STRING_LENGTH, nullptr, nullptr);
	UINT64 Func = 0, Para = 0;
	if (m_strTrie->GetDataInTrie(NameStr, &Para, &Func))
	{
		switch(Para)
		{
		case DBG_RESTART:   m_executor->DebuggerRestart();      break;
		case DBG_RUN:       m_executor->DebuggerRun();          break;
		case DBG_STEPINTO:  m_executor->DebuggerStepInto();     break;
		case DBG_STEPOVER:  m_executor->DebuggerStepOver();     break;
		case DBG_STOP:      m_executor->DebuggerClose();        break;
		case DBG_SUSPEND:   m_executor->DebuggerSuspend();      break;
		}
	}
}

void CPubWnd::HandleCaptionMsg(CPubWnd* pBase, TNotifyUI& msg, int Value)
{
	pBase->SendMessage(WM_SYSCOMMAND, Value, 0);
}

void CPubWnd::HandleQuit(CPubWnd* pBase, TNotifyUI& msg, int Value)
{
	PostQuitMessage(0);
}

void CPubWnd::HandleSubWndClose(CPubWnd* pBase, TNotifyUI& msg, int Value)
{
	//TODO 删除窗口之后，按钮相关内存会被释放。但是后续触发的LButtonUp事件会寻找按钮的内存信息，会报错
	//先设置隐藏窗口  后续再处理此问题
	//((CPubWnd*)pBase)->Destroyed = True;
	pBase->ShowWindow(False);
}

void CPubWnd::HandleBrowserFile(CPubWnd* pBase, TNotifyUI& msg, int Value)
{
	OPENFILENAME ofn = { 0 };
	TCHAR FileName[255 + 1] = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = _T("*.exe;*.dll\0*.exe;*.dll\0全部\0*.*\0\0");
	ofn.lpstrInitialDir = _T("C:\\");
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = sizeof(FileName);
	ofn.nFilterIndex = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
	if (GetOpenFileName(&ofn))
	{
		CEditUI *Edit = static_cast<CEditUI*> (pBase->m_PaintManager.FindControl(_T("browser_edit")));
		if (Edit)
		{
			Edit->SetText(FileName);
		}
	}
	else
	{
		if (CommDlgExtendedError()) //取消或者关闭时为0
			MessageBox(nullptr, _T("错误提示"), _T("获取文件名称失败"), MB_OK);
	}
}

void CPubWnd::HandleStartDebugger(CPubWnd* pBase, TNotifyUI& msg, int Value)
{
	//检查文件是否存在
	CEditUI* EditUi = static_cast<CEditUI*>(pBase->m_PaintManager.FindControl(_T("browser_edit")));
	if(EditUi)
	{
		CDuiString FileName =EditUi->GetText();
		if (FileName.GetLength() < 1)
		{
			EditUi = static_cast<CEditUI*>(pBase->m_PaintManager.FindControl(_T("warning_edit")));
			if(EditUi)
				EditUi->SetText(_T("文件路径不能为空！"));
			return;
		}
		HANDLE TmpHandle = CreateFile(FileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, NULL, nullptr);
		if (TmpHandle == INVALID_HANDLE_VALUE)
		{
			EditUi = static_cast<CEditUI*>(pBase->m_PaintManager.FindControl(_T("warning_edit")));
			if (EditUi)
				EditUi->SetText(_T("文件路径不存在！"));
			return;
		}
		CloseHandle(TmpHandle);
		if (m_executor == nullptr)
			m_executor = m_executor->GetInstance();
		if (m_executor == nullptr)
			return;
		if (m_executor->IsRunning())
		{
			if (IDNO == MessageBox(nullptr, _T("温馨提示"), _T("已有调试任务在运行，是否重新开始?"), MB_OK))
				return;
		}
		pBase->ShowWindow(false);
		CEditUI* ParaEdit = static_cast<CEditUI*>(pBase->m_PaintManager.FindControl(_T("parameter_edit")));
		if(ParaEdit==nullptr)
			if (IDNO == MessageBox(nullptr, _T("温馨提示"), _T("获取参数窗口失败，是否继续？"), MB_OK))
				return;
		CDuiString ParaStr = ParaEdit->GetText();
		m_executor->DebuggerStart(FileName.GetData(), ParaStr.GetData(), nullptr);
	}
}

void CPubWnd::HandlePeBrowser(CPubWnd* pBase, TNotifyUI& msg, int Value)
{
	OPENFILENAME ofn = { 0 };
	TCHAR FileName[255 + 1] = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = _T("*.exe;*.dll\0*.exe;*.dll\0全部\0*.*\0\0");
	ofn.lpstrInitialDir = _T("C:\\");
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = sizeof(FileName);
	ofn.nFilterIndex = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
	if (GetOpenFileName(&ofn))
	{
		((CPubWnd*)pBase)->PeReadAttribute(FileName);
	}
	else
	{
		if (CommDlgExtendedError()) //取消或者关闭时为0
			MessageBox(nullptr, _T("错误提示"), _T("获取文件名称失败"), MB_OK);
	}
}

void CPubWnd::HandlePeSwitchImportDll(CPubWnd* pBase, TNotifyUI& msg, int Value)
{
	TCHAR wSeq[8] = { 0 };
	_itot(msg.wParam, wSeq, 10);
	CListUI* pList = static_cast<CListUI*>(pBase->m_PaintManager.FindControl(_T("pe_impfunc")));
	int Count = pList->GetCount();
	for (int i = 0; i < Count; i++)
	{
		if (pList->GetItemAt(i)->GetUserData() == wSeq)
			pList->GetItemAt(i)->SetVisible(True);
		else
			pList->GetItemAt(i)->SetVisible(False);
	}
}

void CPubWnd::HandleAsmTest(CPubWnd* pBase, TNotifyUI& msg, int Value)
{
	static_cast<CListUI*>(pBase->m_PaintManager.FindControl(_T("asm_list")))->RemoveAll();
	SAsmResultSet AsmResultSet = { 0 };
	LPCTSTR AsmStr = static_cast<CEditUI*>(pBase->m_PaintManager.FindControl(_T("asm_edit")))->GetText();
	CAsm Asm;
	Asm.AsmFromStr(AsmStr, &AsmResultSet);
	for(int i=0;i<AsmResultSet.m_TotalRecord;i++)
	{
		if (AsmResultSet.m_AsmResult[i].m_TotalLength < 1) //失败的记录 不予处理
			continue;
		CListTextElementUI *p = new CListTextElementUI;
		static_cast<CListUI*>(pBase->m_PaintManager.FindControl(_T("asm_list")))->Add(p);
		TCHAR tmp[16]={0};
		_itow_s(i, tmp, 10);
		p->SetText(0,tmp);
		TCHAR result[64] = { 0 };
		for(int jj=0;jj<AsmResultSet.m_AsmResult[i].m_TotalLength;jj++)
		{
			memset(tmp, 0x00, sizeof(tmp));
			_itow_s(AsmResultSet.m_AsmResult[i].m_Result[jj], tmp, 16);
			if (lstrlen(tmp) == 1)
				lstrcat(result, _T("0"));
			lstrcat(result, tmp);
		}
		p->SetText(1, result);
	}
}

void CPubWnd::ConsoleThreadFunc(PVOID param)
{
	if (AllocConsole() == false)
	{
		_endthread();
		return;
	}
	SetConsoleTitle(_T("Python 控制台"));
	freopen("CONOUT$", "wt", stdout);
	freopen("CONOUT$", "wt", stderr);
	freopen("CONIN$", "wt", stdin);
	PyImport_AppendInittab("niujiao", PyInit_niujiao);
	Py_Initialize();
	PyImport_ImportModule("niujiao");
	while (True)
	{
		char TmpWideChar[1024] = { 0 };
		char TmpChar[512] = { 0 };
		DWORD Length = 0;
		printf(">>> ");
		if (ReadConsole(GetStdHandle(STD_INPUT_HANDLE), TmpWideChar, 1024, &Length, NULL) == false)  //TODO 编译时如果本文件没有重新编译 第一次运行时控制台会报错，VS链接的问题?
		{
			MessageBox(NULL,  _T("读取控制台内容发生错误!"), _T("错误提示"), MB_OK);
			FreeConsole();
			_endthread();
			return;
		}
		WideCharToMultiByte(CP_ACP, 0, (TCHAR*)TmpWideChar, lstrlen((TCHAR*)TmpWideChar), TmpChar, 1024, nullptr, nullptr);
		while (TmpChar[Length - 3] == '\\')
		{
			printf("... ");
			char TmpWideChar2[1024] = { 0 };
			char TmpChar2[512] = { 0 };
			DWORD TmpLen = 0;
			ReadConsole(GetStdHandle(STD_INPUT_HANDLE), TmpWideChar2, 1024, &TmpLen, NULL);
			WideCharToMultiByte(CP_ACP, 0, (TCHAR*)TmpWideChar2, lstrlen((TCHAR*)TmpWideChar2), TmpChar2, 1024, nullptr, nullptr);
			strcat(TmpChar, TmpChar2);
			Length += TmpLen;
		}
		PyRun_SimpleString(TmpChar);
	}
	Py_Finalize();
}

void CPubWnd::UpdateDisasmData(HANDLE hFile)
{
	if (disasm)
	{
		delete disasm;
		disasm = nullptr;
	}
	for (int i = 0; gWndPtr[i] && i < MAX_SUB_WINDOW; i++)
	{
		CListUI* ListUi = static_cast<CListUI*>(gWndPtr[i]->m_PaintManager.FindControl(_T("disasm_list")));
		if (ListUi)
		{
			if (hFile == INVALID_HANDLE_VALUE)
				return;
			if (disasm == nullptr)
			{
				disasm = new Disasm();
				if (disasm->DisasmFromHandle(hFile) == false)
				{
					delete disasm;
					return;
				}
			}
			//ListUi->RemoveAll();
			CListBodyUI* ListBody = static_cast<CListBodyUI*>(ListUi->GetList()); 
			ListBody->ProcessVLScrollbar(ListUi->GetList()->GetPos(), (disasm->GetTotalDisasmRecord())*20); //记录数*行宽
			//SendMessage(WM_PAINT, 0, 0);
			//UpdateWindowBackground();
			//printf("%d\n",ListBody->GetScrollPos());
			//下面的语句是为了刷新界面
			ListUi->LineDown();
			ListUi->LineUp();
		}
	}
}

void CPubWnd::UpdateLogData()
{
	for (int i = 0; gWndPtr[i] && i < MAX_SUB_WINDOW; i++)
	{
		CListUI* ListUi = static_cast<CListUI*>(gWndPtr[i]->m_PaintManager.FindControl(_T("log_list")));
		if (ListUi)
		{
			CListTextElementUI* p = new CListTextElementUI();
			ListUi->Add(p);
			p->SetText(0, _T("haha"));
		}
	}
}


CPubWnd::CPubWnd(LPCTSTR pszXMLPath) : m_strXMLPath(pszXMLPath)
{
	disasm = nullptr;
	m_className = GenerateClassName();
	if (gWndHead == nullptr)
	{
		gWndHead = new s_wnd_list;
		gWndHead->WndClass = this;
		gWndHead->Next = nullptr;
	}
	else
	{
		s_wnd_list* Tmp;
		for (Tmp = gWndHead; Tmp->Next != nullptr; Tmp = Tmp->Next) {}
		Tmp->Next = new s_wnd_list;
		Tmp->Next->WndClass = this;
		Tmp->Next->Next = nullptr;
	}

	//初始化前缀树 
	if (m_strTrie == nullptr)
	{
		m_strTrie = new CStrTrie();
		for (int i = 0; i < (sizeof(gEventString) / sizeof(SEventString)); i++)
		{
			m_strTrie->TrieAddStr(gEventString[i].m_Str, gEventString[i].Value, UINT64(gEventString[i].EventHandle));
		}
	}
}

CPubWnd::~CPubWnd()
{
	if (gWndHead)
	{
		if (gWndHead->WndClass == this)
		{
			gWndHead = gWndHead->Next;
		}
		else
		{
			s_wnd_list* Tmp;
			for (Tmp = gWndHead; Tmp->Next->WndClass == this; Tmp = Tmp->Next);
			delete Tmp->Next;
			Tmp->Next = Tmp->Next->Next;
		}
	}
	free(m_className);
	if (m_strTrie) delete m_strTrie;
}

void CPubWnd::Notify(TNotifyUI & msg)
{
	if (msg.sType == DUI_MSGTYPE_CLICK || msg.sType == DUI_MSGTYPE_ITEMSELECT)
	{
		char NameStr[MAX_STRING_LENGTH] = { 0 };
		LPCTSTR TmpStr = msg.pSender->GetName().GetData();
		WideCharToMultiByte(CP_ACP, 0, TmpStr, lstrlen(TmpStr), NameStr, MAX_STRING_LENGTH, nullptr, nullptr);
		UINT64 Func = 0, Value = 0;
	
		if (m_strTrie->GetDataInTrie(NameStr, &Value, &Func))
		{
			if (Func)
			{
				const SEventHandle HandleFunc = SEventHandle(Func);
				HandleFunc(this, msg, (int)Value);
			}
		}
	}
	else if(msg.sType==DUI_MSGTYPE_MENU)
	{
		CPoint point = msg.ptMouse;
		ClientToScreen(m_hWnd, &point);
		CMenuWnd::CreateMenu(NULL, msg.pSender->GetUserData().GetData(), point, &m_PaintManager, nullptr);	
	}
	else if (msg.sType == DUI_MSGTYPE_SCROLL)
	{
		if (disasm == nullptr)
			return;
		static int pos = 0;
		pos += msg.lParam;
		int seq = pos / 20;
		for(int j=0; gWndPtr[j]&&j< MAX_SUB_WINDOW;j++)
		{
			CListUI* ListUi = static_cast<CListUI*>(gWndPtr[j]->m_PaintManager.FindControl(_T("disasm_list")));
			if (ListUi == nullptr) continue;
			ListUi->RemoveAll();
			for (int i = seq; i < (seq + 21); i++)
			{
				DISASM_RESULT* DisasmResult = (DISASM_RESULT*)*(disasm->GetDisasmResultBySeq() + i);
				CListTextElementUI *p = new CListTextElementUI;
				ListUi->Add(p);
				TCHAR ch[128];

				memset(ch, 0x00, sizeof(ch));
				wsprintf(ch, _T("%08x"), DisasmResult->CurFileOffset +disasm->GetImageInfo()->GetImageBase());
				p->SetText(0, ch);

				memset(ch, 0x00, sizeof(ch));
				for (UINT k = 0; k < DisasmResult->CurrentLen; k++)
				{
					wsprintf(ch + k * 2, _T("%X"), DisasmResult->MachineCode[k]);
				}
				p->SetText(1, ch);

				memset(ch, 0x00, sizeof(ch));
				char tmpStr[64] = { 0 };
				strcat(tmpStr, DisasmResult->Opcode);
				if (DisasmResult->OperandNum > 0)
				{
					strcat(tmpStr, " ");
					strcat(tmpStr, DisasmResult->FirstOperand);
				}
				if (DisasmResult->OperandNum > 1)
				{
					strcat(tmpStr, ",");
					strcat(tmpStr, DisasmResult->SecondOperand);
				}
				if (DisasmResult->OperandNum > 2)
				{
					strcat(tmpStr, ",");
					strcat(tmpStr, DisasmResult->ThirdOperand);
				}
				if (DisasmResult->OperandNum > 3)
				{
					strcat(tmpStr, ",");
					strcat(tmpStr, DisasmResult->ForthOperand);
				}
				MultiByteToWideChar(CP_ACP, 0, tmpStr, 64, ch, 128);
				p->SetText(2, ch);
			}	
		}		
	}
}

