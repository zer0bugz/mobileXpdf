// TextOutDialog.cpp : implementation file
//

#include "stdafx.h"
#include "MobileXpdf.h"
#include "TextOutDialog.h"

const DWORD dwAdornmentFlags = 0; // exit button

// CTextOutDialog dialog

IMPLEMENT_DYNAMIC(CTextOutDialog, CDialog)

CTextOutDialog::CTextOutDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CTextOutDialog::IDD, pParent)
	, m_pPDFDoc(NULL)
	, m_CurrentPage(1)
	, m_DoubleFontSize(false)
	, m_FontSize(12)
	, m_FullScreen(false)
{
	CWinApp* pApp = AfxGetApp();
	m_FontSize = pApp->GetProfileInt(L"Xpdf",L"TextOnlyFontsize",12);
	m_DoubleFontSize = pApp->GetProfileInt(L"Xpdf",L"TextOnlyDoubleFontsize",0)!=0;

}

CTextOutDialog::~CTextOutDialog()
{
	CWinApp* pApp = AfxGetApp();
	pApp->WriteProfileInt(L"Xpdf",L"TextOnlyFontsize",m_FontSize);
	
	pApp->WriteProfileInt(L"Xpdf",L"TextOnlyDoubleFontsize",m_DoubleFontSize);
}

void CTextOutDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//	DDX_Control(pDX, IDC_EDIT_TEXT_VIEW, m_TextEdit);
	DDX_Control(pDX, IDC_EDIT_TEXT_VIEW, m_TextEdit);
}


BEGIN_MESSAGE_MAP(CTextOutDialog, CDialog)
	ON_COMMAND(ID_NEXT_PAGE, &CTextOutDialog::OnNextPage)
	//ON_COMMAND(ID_PREV_PAGE, &CTextOutDialog::OnPrevPage)
	ON_COMMAND(ID_TOOLS_FULLSCREEN, &CTextOutDialog::OnToolsFullscreen)
	//ON_COMMAND(ID_TOOLS_GOTOPAGE, &CTextOutDialog::OnToolsGotopage)
	ON_COMMAND_RANGE(ID_FONTSIZE_7,ID_FONTSIZE_12,&CTextOutDialog::OnFontsize)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FONTSIZE_7,ID_FONTSIZE_12, &CTextOutDialog::OnUpdateFontsize)
	ON_COMMAND(ID_VIEW_PAGEVIEW, &CTextOutDialog::OnReturnTool)
	ON_COMMAND(ID_FONTSIZE_DOUBLESIZE, &CTextOutDialog::OnFontsizeDoublesize)
	ON_UPDATE_COMMAND_UI(ID_FONTSIZE_DOUBLESIZE, &CTextOutDialog::OnUpdateFontsizeDoublesize)
	ON_WM_INITMENUPOPUP()
	ON_UPDATE_COMMAND_UI(ID_TOOLS_FULLSCREEN, &CTextOutDialog::OnUpdateToolsFullscreen)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_POPUP_COPY, &CTextOutDialog::OnPopupCopy)
	ON_BN_CLICKED(IDCANCEL, &CTextOutDialog::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, &CTextOutDialog::OnBnClickedOk)

	ON_UPDATE_COMMAND_UI(ID_NEXT_PAGE, &CTextOutDialog::OnUpdateNextPage)
	ON_UPDATE_COMMAND_UI(ID_PREV_PAGE, &CTextOutDialog::OnUpdatePrevPage)
END_MESSAGE_MAP()


// CTextOutDialog message handlers

BOOL CTextOutDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	CFont font;
	/*VERIFY(font.CreateFont(
   18,                        // nHeight
   0,                         // nWidth
   0,                         // nEscapement
   0,                         // nOrientation
   FW_NORMAL,                 // nWeight
   FALSE,                     // bItalic
   FALSE,                     // bUnderline
   0,                         // cStrikeOut
   ANSI_CHARSET,              // nCharSet
   OUT_DEFAULT_PRECIS,        // nOutPrecision
   CLIP_DEFAULT_PRECIS,       // nClipPrecision
   DEFAULT_QUALITY,           // nQuality
   DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
   L"Tahoma"));                 // lpszFacename

	m_TextEdit.SetFont(&font);
	//Need to delete it on close
*/
	
	m_CommandBar.Create(this);
	//m_CommandBar.InsertMenuBar(IDR_MAINFRAME);
	m_CommandBar.InsertMenuBar(IDR_TEXTONLYMENU);
	
//	m_TextEdit.SetBkColor(RGB(255,255,255));
// 	RECT Rect;
// 	RECT cbRect;
// 	this->GetWindowRect(&Rect);
// 	m_CommandBar.GetWindowRect(&cbRect);
// 	Rect.bottom=cbRect.top;
// 	this->MoveWindow(&Rect);
// 	this->GetClientRect(&Rect);
// 	m_TextEdit.MoveWindow(&Rect);

	updateFontSize();
	renderPage();


	if (!m_wndCommandBar.Create(this) 
		//|| !m_wndCommandBar.InsertMenuBar(IDR_MAINFRAME,0)
		|| !m_wndCommandBar.InsertMenuBar(IDR_TEXTONLYMENU,0) 
		|| !m_wndCommandBar.AddAdornments(dwAdornmentFlags) 
//#ifndef WIN32_PLATFORM_WFSP
//		//|| AfxGetAygshellUIModel()!=Smartphone? !m_wndCommandBar.LoadToolBar(toolbar_id):false
//		|| AfxGetAygshellUIModel()!=Smartphone? !m_wndCommandBar.LoadToolBar(IDR_MAINFRAME):false
//#endif
		)
	{
		TRACE0("Failed to create CommandBar\n");
		return -1;      // fail to create
	}
	m_wndCommandBar.SetBarStyle(m_wndCommandBar.GetBarStyle() | CBRS_SIZE_FIXED);
	CWnd* pWnd = CWnd::FromHandlePermanent(m_wndCommandBar.m_hWnd);

	m_wndCommandBar.DrawMenuBar(0);
	m_wndCommandBar.Show(TRUE);

	//return FALSE;  //FALSE, so that the text is not initially selected
	return TRUE;
}

void CTextOutDialog::OnNextPage()
{
	if(m_CurrentPage<m_pPDFDoc->getNumPages()){
		m_CurrentPage++;
		renderPage();		
	}
}
void CTextOutDialog::OnPrevPage()
{
	if(m_CurrentPage>1){
		m_CurrentPage--;
		renderPage();
	}
	
}
void CTextOutDialog::renderPage(void)
{
	CString output;
	TextOutputDev dev( NULL, gFalse, 0., gFalse, gFalse);
	m_pPDFDoc->displayPage(&dev, m_CurrentPage, 72, 72, 0, gFalse, gTrue, gFalse);
	extractTextFromPdf(&dev,&output);
	m_TextEdit.SetWindowText(output); //this will output the text to the memo box
}

void CTextOutDialog::extractTextFromPdf(TextOutputDev *dev,CString * output){
	TextFlow *flow;
	TextBlock *blk;
	TextLine *line;
	TextWord *word;
	TextPage * text = dev->takeText();
    for (flow = text->getFlows(); flow; flow = flow->getNext()) {
        for (blk = flow->getBlocks(); blk; blk = blk->getNext()) {
            for (line = blk->getLines(); line; line = line->getNext()) {
                for (word = line->getWords(); word; word = word->getNext()) {
					wchar_t * str = word->getTextUnicode();
					output->Append(str);
					delete str;
					output->Append(L" ");
				}
			}
			output->Append(L"\r\n");
		}
    }

  delete text;
}
void CTextOutDialog::OnToolsFullscreen()
{
// $$$AD
#if 0
	static RECT origsize;
	m_FullScreen=!m_FullScreen;
	if (m_FullScreen){
		//Save non-fullscreen size
		SystemParametersInfo(SPI_GETWORKAREA, 0, &origsize, 0);
		
		
		BOOL chgScreen = SHFullScreen(m_hWnd,SHFS_HIDESTARTICON | SHFS_HIDESIPBUTTON | SHFS_HIDETASKBAR);
		if (chgScreen == FALSE)
		{
			MessageBox(_T("Could not modify the window."),MB_OK);
		}

		//Removing task bar could be done by setting m_bFullscreen to false and call
		//SHInitDialog with SHIDIF_FULLSCREENNOMENUBAR during OnInitDialog
		//But we currently dont want that

		RECT fsRect;
		RECT cbRect;
		m_CommandBar.GetWindowRect(&cbRect);
		fsRect.top=0;
		fsRect.left=0;
		fsRect.right=origsize.right;
		fsRect.bottom=cbRect.top;
		origsize.bottom=cbRect.top;
		this->MoveWindow(&fsRect);
		this->GetClientRect(&fsRect);
//		m_TextEdit.MoveWindow(&fsRect);

	}else {
		this->MoveWindow(&origsize);
		this->GetClientRect(&origsize);
//		m_TextEdit.MoveWindow(&origsize);
		//m_CommandBar.Show(TRUE);
		BOOL chgScreen = SHFullScreen(m_hWnd,SHFS_SHOWSTARTICON | SHFS_SHOWSIPBUTTON | SHFS_SHOWTASKBAR);

	}
#endif
}

void CTextOutDialog::OnToolsGotopage()
{
	// TODO: Add your command handler code here
}

void CTextOutDialog::OnFontsize(UINT nID)
{
	m_FontSize=11+nID-ID_FONTSIZE_7;
	updateFontSize();
}

void CTextOutDialog::OnUpdateFontsize(CCmdUI *pCmdUI)
{

	pCmdUI->SetCheck(m_FontSize==11+(pCmdUI->m_nID-ID_FONTSIZE_7));
}


void CTextOutDialog::OnFontsizeDoublesize()
{
	m_DoubleFontSize=!m_DoubleFontSize;

	updateFontSize();
}

void CTextOutDialog::OnUpdateFontsizeDoublesize(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_DoubleFontSize);
}

void CTextOutDialog::updateFontSize(void)
{	
	m_TextEdit.ChangeFontHeight(m_DoubleFontSize?m_FontSize*2:m_FontSize);
}

void CTextOutDialog::OnReturnTool(void)
{
	this->OnOK();
}

BOOL CTextOutDialog::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// TODO: Add your specialized code here and/or call the base class

	return CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL CTextOutDialog::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->wParam == 39 && pMsg->lParam > 0) //right
		OnNextPage();
	if (pMsg->wParam == 37 && pMsg->lParam > 0) //left
		OnPrevPage();
	// TODO: Add your specialized code here and/or call the base class

	//UpdateDialogControls(this,true);
	return CDialog::PreTranslateMessage(pMsg);
}

void CTextOutDialog::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	int count =pPopupMenu->GetMenuItemCount();
	CCmdUI state;
	state.m_nIndexMax=count;
	for (int i=0;i<count;i++){
		state.m_nID=pPopupMenu->GetMenuItemID(i);
		state.m_nIndex=i;
		state.m_pMenu=pPopupMenu;
		state.DoUpdate(this,FALSE);
		

		CMenu * subMenu = pPopupMenu->GetSubMenu(i);
		if (subMenu!=NULL){
		state.m_nIndexMax=subMenu->GetMenuItemCount();
			
			for (unsigned int j=0;j<subMenu->GetMenuItemCount();j++){
				state.m_nID=subMenu->GetMenuItemID(j);
				state.m_nIndex=j;
				state.m_pMenu=subMenu;
				state.DoUpdate(this,FALSE);
		
			}


		}

	}
	CDialog::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	// TODO: Add your message handler code here
}

void CTextOutDialog::OnUpdateToolsFullscreen(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_FullScreen);
}

void CTextOutDialog::OnContextMenu(CWnd* pWnd, CPoint point)
{
	CMenu menu;
	menu.LoadMenuW(IDR_TEXTONLYCONTEXT);

	int selStart,selEnd;
//	m_TextEdit.GetSel(selStart,selEnd);
	CMenu *submenu =menu.GetSubMenu(0);
	if ((selEnd-selStart)!=0 && m_pPDFDoc->okToCopy()){
		submenu->EnableMenuItem(ID_POPUP_COPY,MF_ENABLED);
	} else {
		submenu->EnableMenuItem(ID_POPUP_COPY,MF_DISABLED|MF_GRAYED);
	}
	submenu->TrackPopupMenu(TPM_LEFTALIGN,point.x,point.y,this);

}

void CTextOutDialog::OnPopupCopy()
{
//	m_TextEdit.Copy();
}

void CTextOutDialog::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

void CTextOutDialog::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();
}

void CTextOutDialog::OnUpdatePrevPage(CCmdUI *pCmdUI)
{
	if (m_CurrentPage>1){
		pCmdUI->Enable();
	} else {
		pCmdUI->Enable(false);
	}
}

void CTextOutDialog::OnUpdateNextPage(CCmdUI *pCmdUI)
{
	if (m_CurrentPage<m_pPDFDoc->getNumPages()){
		pCmdUI->Enable();
	} else {
		pCmdUI->Enable(false);
	}
}