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

#pragma once

class CColumnTreeView : public CView
{
	DECLARE_DYNCREATE(CColumnTreeView)
protected:
	CColumnTreeView();

public:
	enum ChildrenIDs { HeaderID = 1, TreeID = 2 };

	void UpdateColumns();
	void AdjustColumnWidth(int nColumn, BOOL bIgnoreCollapsed);

	CFont m_HeaderFont;

	CTreeCtrl& GetTreeCtrl() { return m_Tree; }
	CHeaderCtrl& GetHeaderCtrl() { return m_Header; }

public:
	virtual ~CColumnTreeView();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC* pDC) {}

protected:
	void UpdateScroller();
	void RepositionControls();
	int GetMaxColumnWidth(HTREEITEM hItem, int nColumn, int nDepth, BOOL bIgnoreCollapsed);

protected:
	CColumnTreeCtrl		m_Tree;
	CHeaderCtrl			m_Header;
	int					m_cyHeader;
	int					m_cxTotal;
	int					m_xPos;
	int					m_arrColWidths[16];
	int					m_xOffset;

	DECLARE_MESSAGE_MAP()
protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHeaderItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHeaderDividerDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTreeCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
public:
	afx_msg void OnDestroy();
};
