/*********************************************************************
* Multi-Column Tree View, version 1.4 (July 7, 2005)
* Copyright (C) 2003-2005 Michal Mecinski.
*
* You may freely use and modify this code, but don't remove
* this copyright note.
*
* THERE IS NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, FOR
* THIS CODE. THE AUTHOR DOES NOT TAKE THE RESPONSIBILITY
* FOR ANY DAMAGE RESULTING FROM THE USE OF IT.
*
* E-mail: mimec@mimec.org
* WWW: http://www.mimec.org
********************************************************************/

#include "stdafx.h"
#include "CTList.h"
#include "cpuidsdk.h"
#include "ColumnTreeCtrl.h"
#include "ColumnTreeView.h"

#include <shlwapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifndef TVS_NOHSCROLL
#define TVS_NOHSCROLL 0x8000	// IE 5.0 or higher required
#endif

IMPLEMENT_DYNCREATE(CColumnTreeView, CView)

BEGIN_MESSAGE_MAP(CColumnTreeView, CView)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_NOTIFY(HDN_ITEMCHANGED, HeaderID, OnHeaderItemChanged)
	ON_NOTIFY(HDN_DIVIDERDBLCLICK, HeaderID, OnHeaderDividerDblClick)
	ON_NOTIFY(NM_CUSTOMDRAW, TreeID, OnTreeCustomDraw)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CColumnTreeView::CColumnTreeView()
{
}

CColumnTreeView::~CColumnTreeView()
{
}


BOOL CColumnTreeView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

void CColumnTreeView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	if (m_Tree.m_hWnd)
		return;

	// create tree and header controls as children
	m_Tree.Create(WS_CHILD | WS_VISIBLE | TVS_NOHSCROLL | TVS_NOTOOLTIPS,
				  CRect(),
				  this,
				  TreeID);

	m_Header.Create(WS_CHILD | WS_VISIBLE | HDS_FULLDRAG,
					CRect(),
					this,
					HeaderID);
	
	m_HeaderFont.CreateFont(15,	// nHeight
							0,								// nWidth
							0,								// nEscapement
							0,								// nOrientation
							FW_NORMAL,						// nWeight
							FALSE,							// bItalic
							FALSE,							// bUnderline
							0,								// cStrikeOut
							ANSI_CHARSET,					// nCharSet
							OUT_DEFAULT_PRECIS,				// nOutPrecision
							CLIP_DEFAULT_PRECIS,			// nClipPrecision
							DEFAULT_QUALITY,				// nQuality
							DEFAULT_PITCH | FF_SWISS,		// nPitchAndFamily
							_T(""));						// lpszFacename

	m_Header.SetFont(&m_HeaderFont);

	// check if the common controls library version 6.0 is available
	BOOL bIsComCtl6 = FALSE;

	HMODULE hComCtlDll = LoadLibrary(_T("comctl32.dll"));
	if (hComCtlDll)
	{
		typedef HRESULT (CALLBACK *PFNDLLGETVERSION)(DLLVERSIONINFO*);

		PFNDLLGETVERSION pfnDllGetVersion = (PFNDLLGETVERSION)GetProcAddress(hComCtlDll, "DllGetVersion");

		if (pfnDllGetVersion)
		{
			DLLVERSIONINFO dvi;
			ZeroMemory(&dvi, sizeof(dvi));
			dvi.cbSize = sizeof(dvi);

			HRESULT hRes = (*pfnDllGetVersion)(&dvi);

			if (SUCCEEDED(hRes) && dvi.dwMajorVersion >= 6)
				bIsComCtl6 = TRUE;
		}
		FreeLibrary(hComCtlDll);
	}

	// calculate correct header's height
	CDC* pDC = GetDC();
	pDC->SelectObject(&m_HeaderFont);
	CSize szExt = pDC->GetTextExtent(_T("A"));
	m_cyHeader = szExt.cy + (bIsComCtl6 ? 7 : 4);
	ReleaseDC(pDC);

	// offset from column start to text start
	m_xOffset = bIsComCtl6 ? 9 : 6;

	m_xPos = 0;
	UpdateColumns();
}


void CColumnTreeView::OnPaint()
{
	// do nothing
	CPaintDC dc(this);
}

BOOL CColumnTreeView::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CColumnTreeView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	UpdateScroller();
	RepositionControls();
}

void CColumnTreeView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CRect rcClient;
	GetClientRect(&rcClient);
	int cx = rcClient.Width();

	int xLast = m_xPos;

	switch (nSBCode)
	{
		case SB_LINELEFT:
			m_xPos -= 15;
			break;
		case SB_LINERIGHT:
			m_xPos += 15;
			break;
		case SB_PAGELEFT:
			m_xPos -= cx;
			break;
		case SB_PAGERIGHT:
			m_xPos += cx;
			break;
		case SB_LEFT:
			m_xPos = 0;
			break;
		case SB_RIGHT:
			m_xPos = m_cxTotal - cx;
			break;
		case SB_THUMBTRACK:
			m_xPos = nPos;
			break;
		default:
			break;
	}

	if (m_xPos < 0)
		m_xPos = 0;
	else if (m_xPos > m_cxTotal - cx)
		m_xPos = m_cxTotal - cx;

	if (xLast == m_xPos)
		return;

	SetScrollPos(SB_HORZ, m_xPos);
	RepositionControls();
}


void CColumnTreeView::OnHeaderItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	UpdateColumns();

	m_Tree.Invalidate();
}

void CColumnTreeView::OnHeaderDividerDblClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMHEADER* pNMHeader = (NMHEADER*)pNMHDR;

	AdjustColumnWidth(pNMHeader->iItem, TRUE);
}

void CColumnTreeView::OnTreeCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMCUSTOMDRAW* pNMCustomDraw = (NMCUSTOMDRAW*)pNMHDR;
	NMTVCUSTOMDRAW* pNMTVCustomDraw = (NMTVCUSTOMDRAW*)pNMHDR;

	switch (pNMCustomDraw->dwDrawStage)
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:
			*pResult = CDRF_DODEFAULT | CDRF_NOTIFYPOSTPAINT;
			break;

		case CDDS_ITEMPOSTPAINT:
			{
				HTREEITEM hItem = (HTREEITEM)pNMCustomDraw->dwItemSpec;
				CRect rcItem = pNMCustomDraw->rc;

				if (rcItem.IsRectEmpty())
				{
				//	Nothing to paint
					*pResult = CDRF_DODEFAULT;
					break;
				}

				CDC dc;
				dc.Attach(pNMCustomDraw->hdc);

				CRect rcLabel;
				m_Tree.GetItemRect(hItem, &rcLabel, TRUE);

				CRect rcTree;
				GetClientRect(rcTree);

				COLORREF crTextBk = pNMTVCustomDraw->clrTextBk;
				//COLORREF crTextBk = RGB(240, 240, 240);

				COLORREF crText = pNMTVCustomDraw->clrText;
				COLORREF crWnd = GetSysColor(COLOR_WINDOW);

			//	Clear the original label rectangle
				CRect rcClear = rcLabel;
				if (rcClear.left > m_arrColWidths[0] - 1)
					rcClear.left = m_arrColWidths[0] - 1;
				dc.FillSolidRect(&rcClear, crWnd);

				int nColsCnt = m_Header.GetItemCount();
				int xOffset = 0;

			//	Draw horizontal lines...				
				for (int i=0; i<nColsCnt; i++)
				{
					xOffset += m_arrColWidths[i];
					rcItem.right = xOffset-1;
					dc.DrawEdge(&rcItem, BDR_SUNKENINNER, BF_RIGHT);
				}
			//	...and the vertical ones
				dc.DrawEdge(&rcItem, BDR_SUNKENINNER, BF_BOTTOM);

				CString strText = m_Tree.GetItemText(hItem);
				CString strSub;
				AfxExtractSubString(strSub, strText, 0, '\t');

			//	Calculate main label's size
				CRect rcText(0, 0, 0, 0);
				dc.DrawText(strSub, &rcText, DT_NOPREFIX | DT_CALCRECT);
				rcLabel.right = min(rcLabel.left + rcText.right + 4, m_arrColWidths[0] - 4);

				CRect rcBack = rcLabel;
				if (GetWindowLong(m_Tree.m_hWnd, GWL_STYLE) & TVS_FULLROWSELECT)
				{
					int nWidth = 0;
					for (int i=0; i<nColsCnt; i++)
						nWidth += m_arrColWidths[i];
					rcBack.right = nWidth - 1;

					if (rcBack.left > m_arrColWidths[0] - 1)
						rcBack.left = m_arrColWidths[0] - 1;
				}

				if (rcBack.Width() < 0)
					crTextBk = crWnd;

			//	Draw label's background
				if (crTextBk != crWnd)
					dc.FillSolidRect(&rcBack, crTextBk);

			//	Draw focus rectangle if necessary
				if (pNMCustomDraw->uItemState & CDIS_FOCUS)
					dc.DrawFocusRect(&rcBack);

			//	Draw main label
				rcText = rcLabel;
				rcText.DeflateRect(2, 1);
				dc.SetTextColor(crText);
				dc.DrawText(strSub, &rcText, DT_NOPREFIX | DT_END_ELLIPSIS);

				xOffset = m_arrColWidths[0];
				dc.SetBkMode(TRANSPARENT);

				if (!(GetWindowLong(m_Tree.m_hWnd, GWL_STYLE) & TVS_FULLROWSELECT))
				{
					dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
				}

			//	Draw other columns text
				for (int i=1; i<nColsCnt; i++)
				{
					if (AfxExtractSubString(strSub, strText, i, '\t'))
					{
						rcText = rcLabel;
						rcText.left = xOffset;
						rcText.right = xOffset + m_arrColWidths[i];
						rcText.DeflateRect(m_xOffset, 1, 2, 1);
						dc.DrawText(strSub, &rcText, DT_NOPREFIX | DT_END_ELLIPSIS);
					}
					xOffset += m_arrColWidths[i];
				}
				dc.Detach();

				*pResult = CDRF_DODEFAULT;
			}			
			break;

		default:
			*pResult = CDRF_DODEFAULT;
			break;
	}
}


void CColumnTreeView::UpdateColumns()
{
	m_cxTotal = 0;

	HDITEM hditem;
	hditem.mask = HDI_WIDTH;
	int nCnt = m_Header.GetItemCount();
	if (nCnt > 16)
		nCnt = 16;

//	Get column widths from the header control
	for (int i=0; i<nCnt; i++)
	{
		if (m_Header.GetItem(i, &hditem))
		{
			m_cxTotal += m_arrColWidths[i] = hditem.cxy;
			if (i==0)
				m_Tree.m_cxFirstCol = hditem.cxy;
		}
	}
	m_Tree.m_cxTotal = m_cxTotal;

	UpdateScroller();
	RepositionControls();
}

void CColumnTreeView::UpdateScroller()
{
	CRect rcClient;
	GetClientRect(&rcClient);
	int cx = rcClient.Width();

	int lx = m_xPos;

	if (m_xPos > m_cxTotal - cx)
		m_xPos = m_cxTotal - cx;
	if (m_xPos < 0)
		m_xPos = 0;

	SCROLLINFO scrinfo;
	scrinfo.cbSize = sizeof(scrinfo);
	scrinfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	scrinfo.nPage = cx;
	scrinfo.nMin = 0;
	scrinfo.nMax = m_cxTotal;
	scrinfo.nPos = m_xPos;
	SetScrollInfo(SB_HORZ, &scrinfo);
}

void CColumnTreeView::RepositionControls()
{
	// reposition child controls
	if (m_Tree.m_hWnd)
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		int cx = rcClient.Width();
		int cy = rcClient.Height();

		// move to a negative offset if scrolled horizontally
		int x = 0;
		if (cx < m_cxTotal)
		{
			x = GetScrollPos(SB_HORZ);
			cx += x;
		}
		m_Header.MoveWindow(-x, 0, cx, m_cyHeader);
		m_Tree.MoveWindow(-x, m_cyHeader, cx, cy-m_cyHeader);
	}
}

void CColumnTreeView::AdjustColumnWidth(int nColumn, BOOL bIgnoreCollapsed)
{
	HTREEITEM hItem;
	int nMaxWidth, currentWidth;

	nMaxWidth = 0;
	
	hItem = m_Tree.GetRootItem();
	currentWidth = GetMaxColumnWidth(hItem, nColumn, 0, bIgnoreCollapsed);
	if (currentWidth > nMaxWidth)
		nMaxWidth = currentWidth;

	hItem = m_Tree.GetNextItem(hItem, TVGN_NEXT);
	while (hItem != NULL)
	{		
		currentWidth = GetMaxColumnWidth(hItem, nColumn, 0, bIgnoreCollapsed);
		if (currentWidth > nMaxWidth)
			nMaxWidth = currentWidth;

		hItem = m_Tree.GetNextItem(hItem, TVGN_NEXT);
	}
		
	HDITEM hditem;
	hditem.mask = HDI_WIDTH;
	m_Header.GetItem(nColumn, &hditem);
	hditem.cxy = nMaxWidth + 20;
	m_Header.SetItem(nColumn, &hditem);
}

int CColumnTreeView::GetMaxColumnWidth(HTREEITEM hItem, int nColumn, int nDepth, BOOL bIgnoreCollapsed)
{
	int nMaxWidth = 0;

	CString strText = m_Tree.GetItemText(hItem);
	CString strSub;
	if (AfxExtractSubString(strSub, strText, nColumn, '\t'))
	{
		CDC dc;
		dc.CreateCompatibleDC(NULL);
		CFont* pOldFont = dc.SelectObject(m_Tree.GetFont());

		// calculate text width
		nMaxWidth = dc.GetTextExtent(strSub).cx;

		dc.SelectObject(pOldFont);
	}

	// add indent and image space if first column
	if (nColumn == 0)
	{
		int nIndent = nDepth;

		if (GetWindowLong(m_Tree.m_hWnd, GWL_STYLE) & TVS_LINESATROOT)
			nIndent++;

		int nImage, nSelImage;
		m_Tree.GetItemImage(hItem, nImage, nSelImage);
		if (nImage >= 0)
			nIndent++;

		nMaxWidth += nIndent * m_Tree.GetIndent();
	}

	if (!bIgnoreCollapsed || (m_Tree.GetItemState(hItem, TVIS_EXPANDED) & TVIS_EXPANDED))
	{
		// process child items recursively
		HTREEITEM hSubItem = m_Tree.GetChildItem(hItem);
		while (hSubItem)
		{
			int nSubWidth = GetMaxColumnWidth(hSubItem, nColumn, nDepth + 1, bIgnoreCollapsed);
			if (nSubWidth > nMaxWidth)
				nMaxWidth = nSubWidth;

			hSubItem = m_Tree.GetNextSiblingItem(hSubItem);
		}
	}
	return nMaxWidth;
}

void CColumnTreeView::OnDestroy()
{
	CView::OnDestroy();

	// TODO: Add your message handler code here
	m_HeaderFont.DeleteObject();
}
