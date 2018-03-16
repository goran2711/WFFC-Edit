#include "MFCRenderFrame.h"
#include "ToolMain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

BEGIN_MESSAGE_MAP(CChildRender, CWnd)
	ON_WM_PAINT()
    ON_WM_SIZE()
END_MESSAGE_MAP()



// CChildView message handlers

BOOL CChildRender::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), NULL);

	return TRUE;
}

void CChildRender::OnPaint()
{
	CPaintDC dc(this); // device context for painting
}

void CChildRender::OnSize(UINT nType, int width, int height)
{
    CWnd::OnSize(nType, width, height);

    if (toolSystem)
        toolSystem->OnWindowSizeChanged(width, height);
}