// (C) Copyright 2002-2005 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//----- CAcDocMgrReator.cpp : Implementation of CAcDocMgrReator
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "CAcDocMgrReator.h"
//#include "RTabCtrl.h"

//extern CRTabCtrl *gpTabCtrl;

//-----------------------------------------------------------------------------
ACRX_CONS_DEFINE_MEMBERS(CAcDocMgrReator, AcApDocManagerReactor, 1)

//-----------------------------------------------------------------------------
CAcDocMgrReator::CAcDocMgrReator (const bool autoInitAndRelease) : AcApDocManagerReactor(), mbAutoInitAndRelease(autoInitAndRelease) {
	if ( autoInitAndRelease ) {
		if ( acDocManager )
			acDocManager->addReactor (this) ;
		else
			mbAutoInitAndRelease =false ;
	}
}

//-----------------------------------------------------------------------------
CAcDocMgrReator::~CAcDocMgrReator () {
	Detach () ;
}

//-----------------------------------------------------------------------------
void CAcDocMgrReator::Attach () {
	Detach () ;
	if ( !mbAutoInitAndRelease ) {
		if ( acDocManager ) {
			acDocManager->addReactor (this) ;
			mbAutoInitAndRelease =true ;
		}
	}
}

void CAcDocMgrReator::Detach () {
	if ( mbAutoInitAndRelease ) {
		if ( acDocManager ) {
			acDocManager->removeReactor (this) ;
			mbAutoInitAndRelease =false ;
		}
	}
}

AcApDocManager *CAcDocMgrReator::Subject () const {
	return (acDocManager) ;
}

bool CAcDocMgrReator::IsAttached () const {
	return (mbAutoInitAndRelease) ;
}

void CAcDocMgrReator::documentToBeDestroyed(AcApDocument* pDocToDestroy)
{
	int nIndex = 0;
// 	if (!gpTabCtrl->m_DocVoidPtrArray.find( pDocToDestroy, nIndex ))
// 	{
// 		return ;
// 	}
// 	gpTabCtrl->DeleteItem( nIndex );
// 	gpTabCtrl->m_DocVoidPtrArray.remove( pDocToDestroy );	
// 
// 	//acutPrintf( ACRX_T("\nd ocumentToBeDestroyed "));
// 
// 	if ( gpTabCtrl->m_DocVoidPtrArray.length() <=1 )
// 	{
// 		gpTabCtrl->MoveWindow(CRect(0,0,0.1,0.1));
// 		gpTabCtrl->GetParentFrame()->RecalcLayout();
// 	}
// 	else
// 	{
// 		gpTabCtrl->MoveWindow(CRect(0,0,100,20));
// 		gpTabCtrl->GetParentFrame()->RecalcLayout();
// 	}
}

void CAcDocMgrReator::documentToBeActivated(AcApDocument* pActivatingDoc )
{
	Acad::ErrorStatus es;

	int nDocPos = 0;
//	if ( !gpTabCtrl->m_DocVoidPtrArray.find( pActivatingDoc, nDocPos ) )
//	{
//  		nDocPos = acDocManagerPtr()->documentCount();
//  		gpTabCtrl->m_DocVoidPtrArray.append( pActivatingDoc );
// 		CString strFile = pActivatingDoc->fileName();
// 		strFile = strFile.Mid(strFile.ReverseFind('\\') + 1);
//  		gpTabCtrl->InsertItem( nDocPos, strFile );
//  		gpTabCtrl->SetItemTooltipText( nDocPos, pActivatingDoc->docTitle() );
//		return;
//	}
//	acDocManagerPtr()->lockDocument( pActivatingDoc );

// 	CString strTmp;
// 	strTmp.Format( _T("\n BeActivated 文档【%s】 , myLockMode = %d"), pActivatingDoc->fileName(),pActivatingDoc->myLockMode() );
// 	AfxMessageBox(strTmp);
// 	if ( pActivatingDoc->myLockMode() != AcAp::kNotLocked )
// 	{
// 		es = acDocManagerPtr()->unlockDocument( pActivatingDoc );
// 		if ( es != Acad::eOk )
// 		{
// 			acutPrintf( ACRX_T("\n无法解锁文档【%s】，错误代码：【%s】"), pActivatingDoc->fileName(), acadErrorStatusText( es ) );
// 			return;
// 		}
// 	}

// 	int nIndex = 0;
// 	if (gpTabCtrl->m_DocVoidPtrArray.find( pActivatingDoc, nIndex ))
// 	{
// 		gpTabCtrl->SetCurSel( nIndex );
// 	}
// 	else
// 	{
// 		gpTabCtrl->SetCurSel( 0);
// 	}
// 
// 	//acutPrintf( ACRX_T("\n documentToBeActivated") );
// 
// 	if ( acDocManagerPtr()->documentCount() <=1 )
// 	{
// 		gpTabCtrl->MoveWindow(CRect(0,0,0.1,0.1));
// 		gpTabCtrl->GetParentFrame()->RecalcLayout();
// 	}
// 	else
// 	{
// 		gpTabCtrl->MoveWindow(CRect(0,0,100,20));
// 		gpTabCtrl->GetParentFrame()->RecalcLayout();
// 	}
}

void CAcDocMgrReator::documentCreated(AcApDocument* pDocCreated)
{
// 	//pDocCreated->database()->addReactor( this );
// 	int nIndex = 0;
// 	int nNewDocPos = 0;
// 	CString strFile = pDocCreated->fileName();
// 	strFile = strFile.Mid(strFile.ReverseFind('\\') + 1);
// 	if (!gpTabCtrl->m_DocVoidPtrArray.find( pDocCreated, nIndex ))
// 	{
// 		gpTabCtrl->m_DocVoidPtrArray.append( pDocCreated );
// 		nNewDocPos = acDocManagerPtr()->documentCount()-1;
// 		gpTabCtrl->InsertItem( nNewDocPos,  strFile);
// 		gpTabCtrl->SetItemTooltipText( nNewDocPos, pDocCreated->docTitle() );
// 	}
// 	else
// 	{
// 		gpTabCtrl->SetItemText( nIndex,  strFile);
// 		gpTabCtrl->SetItemTooltipText( nIndex, pDocCreated->docTitle() );
// 	}
// 
// 	gpTabCtrl->SetCurSel( nNewDocPos );
// 	//acutPrintf( ACRX_T("\ndocumentCreated %s "), strFile);
// 
// 	if ( acDocManagerPtr()->documentCount() <=1 )
// 	{
// 		gpTabCtrl->MoveWindow(CRect(0,0,0.1,0.1));
// 		gpTabCtrl->GetParentFrame()->RecalcLayout();
// 	}
// 	else
// 	{
// 		gpTabCtrl->MoveWindow(CRect(0,0,100,20));
// 		gpTabCtrl->GetParentFrame()->RecalcLayout();
// 	}
}

void CAcDocMgrReator::documentToBeDeactivated(AcApDocument* pDeActivatedDoc )
{
 	Acad::ErrorStatus es;
 //	es = acDocManagerPtr()->unlockDocument( pDeActivatedDoc );

 //	acutPrintf( _T("\n BeDeactivated 文档【%s】"), pDeActivatedDoc->fileName() );

// 	if ( es != Acad::eOk )
// 	{
// 		acutPrintf( ACRX_T("\n无法锁定文档【%s】，错误代码：【%s】"), pDeActivatedDoc->fileName(), acadErrorStatusText( es ) );
// 		return;
// 	}
// 
// 	//acutPrintf( ACRX_T("\n documentToBeDeactivated"));
// 
// 	if ( acDocManagerPtr()->documentCount() <=1 )
// 	{
// 		gpTabCtrl->MoveWindow(CRect(0,0,0.1,0.1));
// 		gpTabCtrl->GetParentFrame()->RecalcLayout();
// 	}
// 	else
// 	{
// 		gpTabCtrl->MoveWindow(CRect(0,0,100,20));
// 		gpTabCtrl->GetParentFrame()->RecalcLayout();
// 	}
}

void CAcDocMgrReator::documentActivated( AcApDocument* pActivatedDoc )
{
// 	CString strTmp;
// 	strTmp.Format( _T("\n Activated 文档【%s】 , myLockMode = %d"), pActivatedDoc->fileName(),pActivatedDoc->myLockMode() );
// 	AfxMessageBox(strTmp);
// 	Acad::ErrorStatus es;
// 	es = acDocManagerPtr()->lockDocument( pActivatedDoc );
}
