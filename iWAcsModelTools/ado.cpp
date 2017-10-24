//
//  MODULE: Ado.cpp
//
//	AUTHOR: Carlos Antollini 
//
//  mailto: cantollini@hotmail.com
//
//	Date: 11/12/2002
//
//	Version 2.07
//
#include "StdAfx.h"
#include "ado.h"

#define ChunkSize 100
///////////////////////////////////////////////////////
//
// CADODatabase Class
//
BOOL SaveBitmapToMemFile(HBITMAP hBitmap, LPVOID* pVoid, DWORD& nSize)
{
	if (hBitmap == NULL)
		return FALSE;

	HDC hDC; //设备描述表  	
	int iBits;//当前显示分辨率下每个像素所占字节数	
	WORD wBitCount;//位图中每个像素所占字节数	
	DWORD dwPaletteSize = 0,//定义调色板大小， 位图中像素字节大小 ，位图文件大小 ， 写入文件字节数		
		dwBmBitsSize, dwDIBSize;
	BITMAP Bitmap; 	
	BITMAPFILEHEADER bmfHdr;//位图属性结构    	
	BITMAPINFOHEADER bi;//位图文件头结构	   	
	LPBITMAPINFOHEADER lpbi;//位图信息头结构	 	
	HANDLE hDib, hPal, hOldPal = NULL;   //指向位图信息头结构,定义文件，分配内存句柄，调色板句柄
	//计算位图文件每个像素所占字节数	
	hDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);	
	iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);	
	DeleteDC(hDC);

	if (iBits <= 1)
		wBitCount = 1;
	else if (iBits <= 4)
		wBitCount = 4;
	else if (iBits <= 8)
		wBitCount = 8;
	else if (iBits <= 24)
		wBitCount = 24;
	else if (iBits <= 32)
		wBitCount = 32;

	//计算调色板大小	
	if (wBitCount <= 8)
		dwPaletteSize = (1 << wBitCount) * sizeof(RGBQUAD);

	//设置位图信息头结构	
	if (GetObject(hBitmap, sizeof(BITMAP), (LPTSTR) & Bitmap) == 0)
		return FALSE;

	bi.biSize = sizeof(BITMAPINFOHEADER);	
	bi.biWidth = Bitmap.bmWidth;	
	bi.biHeight = Bitmap.bmHeight;	
	bi.biPlanes = 1;	
	bi.biBitCount = wBitCount;	
	bi.biCompression = BI_RGB;	
	bi.biSizeImage = 0;	
	bi.biXPelsPerMeter = 0;	
	bi.biYPelsPerMeter = 0;	
	bi.biClrUsed = 0;	
	bi.biClrImportant = 0;	

	dwBmBitsSize = ((Bitmap.bmWidth * wBitCount + 31) / 32) * 4 * Bitmap.bmHeight ;	

	//为位图内容分配内存
	hDib = GlobalAlloc(GHND,
			dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
	lpbi = (LPBITMAPINFOHEADER) GlobalLock(hDib);

	if (lpbi == NULL)
		return FALSE;
	*lpbi = bi;


	// 处理调色板   	
	hPal = GetStockObject(DEFAULT_PALETTE);

	if (hPal)
	{
		hDC = ::GetDC(NULL);		
		hOldPal = SelectPalette(hDC, (HPALETTE) hPal, FALSE);		
		RealizePalette(hDC);
	}

	// 获取该调色板下新的像素值	
	GetDIBits(hDC, hBitmap, 0, (UINT) Bitmap.bmHeight,
		(LPTSTR) lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize,
		(LPBITMAPINFO) lpbi, DIB_RGB_COLORS);

	//恢复调色板   
	if (hOldPal)
	{
		SelectPalette(hDC, (HPALETTE) hOldPal, TRUE);		
		RealizePalette(hDC);		
		::ReleaseDC(NULL, hDC);
	}	

	// 设置位图文件头	
	bmfHdr.bfType = 0x4D42;  // _T("BM")	
	dwDIBSize = sizeof(BITMAPFILEHEADER) +
		sizeof(BITMAPINFOHEADER) +
		dwPaletteSize +
		dwBmBitsSize; 	
	bmfHdr.bfSize = dwDIBSize;
	bmfHdr.bfReserved1 = 0;	
	bmfHdr.bfReserved2 = 0;	
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) +
		(DWORD)sizeof(BITMAPINFOHEADER) +
		dwPaletteSize;		

	nSize = dwDIBSize;// + sizeof(BITMAPFILEHEADER);
	*pVoid = (LPVOID)new TCHAR[nSize];
	if (*pVoid == NULL)
		return FALSE;

	memcpy(*pVoid, &bmfHdr, sizeof(BITMAPFILEHEADER));
	memcpy((TCHAR *) *pVoid + sizeof(BITMAPFILEHEADER), lpbi,
		nSize - sizeof(BITMAPFILEHEADER));

	//消除内存分配  	
	GlobalUnlock(hDib);	
	GlobalFree(hDib);	

	return TRUE;
}

DWORD CADODatabase::GetRecordCount(_RecordsetPtr m_pRs)
{
	DWORD numRows = 0;

	numRows = m_pRs->GetRecordCount();

	if (numRows == -1)
	{
		if (m_pRs->ACS_EOF != VARIANT_TRUE)
			m_pRs->MoveFirst();

		while (m_pRs->ACS_EOF != VARIANT_TRUE)
		{
			numRows++;
			m_pRs->MoveNext();
		}
		if (numRows > 0)
			m_pRs->MoveFirst();
	}
	return numRows;
}

BOOL CADODatabase::Open(LPCTSTR lpstrConnection, LPCTSTR lpstrUserID,
	LPCTSTR lpstrPassword)
{
	HRESULT hr = S_OK;

	if (IsOpen())
		Close();

	if (_tcscmp(lpstrConnection, _T("")) != 0)
		m_strConnection = lpstrConnection;

	ASSERT(!m_strConnection.IsEmpty());

	try
	{
		if (m_nConnectionTimeout != 0)
			m_pConnection->PutConnectionTimeout(m_nConnectionTimeout);
		hr = m_pConnection->Open(_bstr_t(m_strConnection),
								_bstr_t(lpstrUserID), _bstr_t(lpstrPassword),
								NULL);
		return hr == S_OK;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

void CADODatabase::dump_com_error(_com_error& e)
{
	CString ErrorStr;


	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());
	ErrorStr.Format(_T("CADODataBase Error\n\tCode = %08lx\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n"),
				e.Error(), e.ErrorMessage(), (LPCTSTR)
				bstrSource,
				(LPCTSTR)
				bstrDescription);
	m_strLastError = _T("Connection String = ") + GetConnectionString() + _T('\n') + ErrorStr;
	m_dwLastError = e.Error(); 
#ifdef _DEBUG
	AfxMessageBox(ErrorStr, MB_OK | MB_ICONERROR);
#endif
}

BOOL CADODatabase::IsOpen()
{
	if (m_pConnection)
		return m_pConnection->GetState() != adStateClosed;
	return FALSE;
}

void CADODatabase::Close()
{
	if (IsOpen())
		m_pConnection->Close();
}


///////////////////////////////////////////////////////
//
// CADORecordset Class
//

CADORecordset::CADORecordset()
{
	m_pRecordset = NULL;
	m_pCmd = NULL;
	m_strQuery = _T("");
	m_strLastError = _T("");
	m_dwLastError = 0;
	m_pRecBinding = NULL;
	m_pRecordset.CreateInstance(__uuidof(Recordset));
	m_pCmd.CreateInstance(__uuidof(Command));
	m_nEditStatus = CADORecordset::dbEditNone;
	m_nSearchDirection = CADORecordset::searchForward;
	m_pConnection = NULL;
}

CADORecordset::CADORecordset(CADODatabase* pAdoDatabase)
{
	m_pRecordset = NULL;
	m_pCmd = NULL;
	m_strQuery = _T("");
	m_strLastError = _T("");
	m_dwLastError = 0;
	m_pRecBinding = NULL;
	m_pRecordset.CreateInstance(__uuidof(Recordset));
	m_pCmd.CreateInstance(__uuidof(Command));
	m_nEditStatus = CADORecordset::dbEditNone;
	m_nSearchDirection = CADORecordset::searchForward;

	m_pConnection = pAdoDatabase->GetActiveConnection();
}

BOOL CADORecordset::Open(_ConnectionPtr mpdb, LPCTSTR lpstrExec, int nOption)
{
	Close();

	m_pConnection = mpdb;
	ASSERT(m_pConnection != NULL);
	ASSERT(m_pConnection->GetState() != adStateClosed);

	if (_tcscmp(lpstrExec, _T("")) != 0)
		m_strQuery = lpstrExec;

	ASSERT(!m_strQuery.IsEmpty());


	m_strQuery.TrimLeft();
	BOOL bIsSelect = m_strQuery.Mid(0, _tcslen(_T("Select "))).CompareNoCase(_T("select ")) ==
		0 &&
		nOption ==
		openUnknown;

	try
	{
		m_pRecordset->CursorType = adOpenStatic;
		m_pRecordset->CursorLocation = adUseServer;

		if (bIsSelect || nOption == openQuery || nOption == openUnknown)
			m_pRecordset->Open((LPCTSTR) m_strQuery,
							_variant_t((IDispatch *) mpdb, TRUE),
							adOpenStatic, adLockOptimistic, adCmdUnknown);
		else if (nOption == openTable)
			m_pRecordset->Open((LPCTSTR) m_strQuery,
							_variant_t((IDispatch *) mpdb, TRUE),
							adOpenKeyset, adLockOptimistic, adCmdTable);
		else if (nOption == openStoredProc)
		{
			m_pCmd->ActiveConnection = mpdb;
			m_pCmd->CommandText = _bstr_t(m_strQuery);
			m_pCmd->CommandType = adCmdStoredProc;
			m_pConnection->CursorLocation = adUseClient;

			m_pRecordset = m_pCmd->Execute(NULL, NULL, adCmdText);
		}
		else
		{
			TRACE(_T("Unknown parameter. %d"), nOption);
			return FALSE;
		}
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}

	return m_pRecordset != NULL;
}

BOOL CADORecordset::Open(LPCTSTR lpstrExec, int nOption)
{
	ASSERT(m_pConnection != NULL);
	ASSERT(m_pConnection->GetState() != adStateClosed);
	return Open(m_pConnection, lpstrExec, nOption);
}

BOOL CADORecordset::Requery()
{
	if (IsOpen())
	{
		try
		{
			m_pRecordset->Requery(adOptionUnspecified);
		}
		catch (_com_error& e)
		{
			dump_com_error(e);
			return FALSE;
		}
	}
	return TRUE;
}


BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, double& dbValue)
{
	double val = (double) NULL;
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch (vtFld.vt)
		{
		case VT_R4:
			val = vtFld.fltVal;
			break;
		case VT_R8:
			val = vtFld.dblVal;
			break;
		case VT_DECIMAL:
			//Corrected by Jos?Carlos Mart韓ez Gal醤
			val = vtFld.decVal.Lo32;
			val *= (vtFld.decVal.sign == 128) ? -1 : 1;
			val /= pow(10.0, vtFld.decVal.scale); 
			break;
		case VT_UI1:
			val = vtFld.iVal;
			break;
		case VT_I2:
		case VT_I4:
			val = vtFld.lVal;
			break;
		case VT_INT:
			val = vtFld.intVal;
			break;
		case VT_NULL:
		case VT_EMPTY:
			val = 0;
			break;
		default:
			val = vtFld.dblVal;
		}
		dbValue = val;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


BOOL CADORecordset::GetFieldValue(int nIndex, double& dbValue)
{
	double val = (double) NULL;
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch (vtFld.vt)
		{
		case VT_R4:
			val = vtFld.fltVal;
			break;
		case VT_R8:
			val = vtFld.dblVal;
			break;
		case VT_DECIMAL:
			//Corrected by Jos?Carlos Mart韓ez Gal醤
			val = vtFld.decVal.Lo32;
			val *= (vtFld.decVal.sign == 128) ? -1 : 1;
			val /= pow(10.0, vtFld.decVal.scale); 
			break;
		case VT_UI1:
			val = vtFld.iVal;
			break;
		case VT_I2:
		case VT_I4:
			val = vtFld.lVal;
			break;
		case VT_INT:
			val = vtFld.intVal;
			break;
		case VT_NULL:
		case VT_EMPTY:
			val = 0;
			break;
		default:
			val = 0;
		}
		dbValue = val;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, long& lValue)
{
	long val = (long) NULL;
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		if (vtFld.vt != VT_NULL && vtFld.vt != VT_EMPTY)
			val = vtFld.lVal;
		lValue = val;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, long& lValue)
{
	long val = (long) NULL;
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		if (vtFld.vt != VT_NULL && vtFld.vt != VT_EMPTY)
			val = vtFld.lVal;
		lValue = val;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, unsigned long& ulValue)
{
	long val = (long) NULL;
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		if (vtFld.vt != VT_NULL && vtFld.vt != VT_EMPTY)
			val = vtFld.ulVal;
		ulValue = val;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, unsigned long& ulValue)
{
	long val = (long) NULL;
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		if (vtFld.vt != VT_NULL && vtFld.vt != VT_EMPTY)
			val = vtFld.ulVal;
		ulValue = val;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, int& nValue)
{
	int val = NULL;
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch (vtFld.vt)
		{
		case VT_BOOL:
			val = vtFld.boolVal;
			break;
		case VT_I2:
		case VT_UI1:
			val = vtFld.iVal;
			break;
		case VT_INT:
			val = vtFld.intVal;
			break;
		case VT_NULL:
		case VT_EMPTY:
			val = 0;
			break;
		default:
			val = vtFld.iVal;
		}	
		nValue = val;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, int& nValue)
{
	int val = (int) NULL;
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch (vtFld.vt)
		{
		case VT_BOOL:
			val = vtFld.boolVal;
			break;
		case VT_I2:
		case VT_UI1:
			val = vtFld.iVal;
			break;
		case VT_INT:
			val = vtFld.intVal;
			break;
		case VT_NULL:
		case VT_EMPTY:
			val = 0;
			break;
		default:
			val = vtFld.iVal;
		}	
		nValue = val;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, CString& strValue,
	CString strDateFormat)
{
	CString str = _T("");
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch (vtFld.vt)
		{
		case VT_R4:
			str = DblToStr(vtFld.fltVal);
			break;
		case VT_R8:
			str = DblToStr(vtFld.dblVal);
			break;
		case VT_BSTR:
			str = vtFld.bstrVal;
			break;
		case VT_I2:
		case VT_UI1:
			str = IntToStr(vtFld.iVal);
			break;
		case VT_INT:
			str = IntToStr(vtFld.intVal);
			break;
		case VT_I4:
			str = LongToStr(vtFld.lVal);
			break;
		case VT_UI4:
			str = ULongToStr(vtFld.ulVal);
			break;
		case VT_DECIMAL:
			{
				//Corrected by Jos?Carlos Mart韓ez Gal醤
				double val = vtFld.decVal.Lo32;
				val *= (vtFld.decVal.sign == 128) ? -1 : 1;
				val /= pow(10.0, vtFld.decVal.scale); 
				str = DblToStr(val);
			}
			break;
		case VT_DATE:
			{
				COleDateTime dt(vtFld);

				if (strDateFormat.IsEmpty())
					strDateFormat = _T("%Y-%m-%d %H:%M:%S");
				str = dt.Format(strDateFormat);
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			str.Empty();
			break;
		default:
			str.Empty();
			return FALSE;
		}
		strValue = str;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, CString& strValue,
	CString strDateFormat)
{
	CString str = _T("");
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch (vtFld.vt)
		{
		case VT_R4:
			str = DblToStr(vtFld.fltVal);
			break;
		case VT_R8:
			str = DblToStr(vtFld.dblVal);
			break;
		case VT_BSTR:
			str = vtFld.bstrVal;
			break;
		case VT_I2:
		case VT_UI1:
			str = IntToStr(vtFld.iVal);
			break;
		case VT_INT:
			str = IntToStr(vtFld.intVal);
			break;
		case VT_I4:
			str = LongToStr(vtFld.lVal);
			break;
		case VT_UI4:
			str = ULongToStr(vtFld.ulVal);
			break;
		case VT_DECIMAL:
			{
				//Corrected by Jos?Carlos Mart韓ez Gal醤
				double val = vtFld.decVal.Lo32;
				val *= (vtFld.decVal.sign == 128) ? -1 : 1;
				val /= pow(10.0, vtFld.decVal.scale); 
				str = DblToStr(val);
			}
			break;
		case VT_DATE:
			{
				COleDateTime dt(vtFld);

				if (strDateFormat.IsEmpty())
					strDateFormat = _T("%Y-%m-%d %H:%M:%S");
				str = dt.Format(strDateFormat);
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			str.Empty();
			break;
		default:
			str.Empty();
			return FALSE;
		}
		strValue = str;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, COleDateTime& time)
{
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch (vtFld.vt)
		{
		case VT_DATE:
			{
				COleDateTime dt(vtFld);
				time = dt;
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			time.SetStatus(COleDateTime::null);
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, COleDateTime& time)
{
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch (vtFld.vt)
		{
		case VT_DATE:
			{
				COleDateTime dt(vtFld);
				time = dt;
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			time.SetStatus(COleDateTime::null);
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, bool& bValue)
{
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch (vtFld.vt)
		{
		case VT_BOOL:
			bValue = vtFld.boolVal == TRUE ? true : false;
			break;
		case VT_EMPTY:
		case VT_NULL:
			bValue = false;
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, bool& bValue)
{
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch (vtFld.vt)
		{
		case VT_BOOL:
			bValue = vtFld.boolVal == TRUE ? true : false;
			break;
		case VT_EMPTY:
		case VT_NULL:
			bValue = false;
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, COleCurrency& cyValue)
{
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch (vtFld.vt)
		{
		case VT_CY:
			cyValue = (CURRENCY) vtFld.cyVal;
			break;
		case VT_EMPTY:
		case VT_NULL:
			{
				cyValue = COleCurrency();
				cyValue.m_status = COleCurrency::null;
			}
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, COleCurrency& cyValue)
{
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch (vtFld.vt)
		{
		case VT_CY:
			cyValue = (CURRENCY) vtFld.cyVal;
			break;
		case VT_EMPTY:
		case VT_NULL:
			{
				cyValue = COleCurrency();
				cyValue.m_status = COleCurrency::null;
			}
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(LPCTSTR lpFieldName, _variant_t& vtValue)
{
	try
	{
		vtValue = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::GetFieldValue(int nIndex, _variant_t& vtValue)
{
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtValue = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::IsFieldNull(LPCTSTR lpFieldName)
{
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		return vtFld.vt == VT_NULL;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::IsFieldNull(int nIndex)
{
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		return vtFld.vt == VT_NULL;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::IsFieldEmpty(LPCTSTR lpFieldName)
{
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		return vtFld.vt == VT_EMPTY || vtFld.vt == VT_NULL;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::IsFieldEmpty(int nIndex)
{
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		return vtFld.vt == VT_EMPTY || vtFld.vt == VT_NULL;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::SetFieldEmpty(LPCTSTR lpFieldName)
{
	_variant_t vtFld;
	vtFld.vt = VT_EMPTY;

	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldEmpty(int nIndex)
{
	_variant_t vtFld;
	vtFld.vt = VT_EMPTY;

	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtFld);
}


DWORD CADORecordset::GetRecordCount()
{
	DWORD nRows = 0;

	nRows = m_pRecordset->GetRecordCount();

	if (nRows == -1)
	{
		nRows = 0;
		if (m_pRecordset->ACS_EOF != VARIANT_TRUE)
			m_pRecordset->MoveFirst();

		while (m_pRecordset->ACS_EOF != VARIANT_TRUE)
		{
			nRows++;
			m_pRecordset->MoveNext();
		}
		if (nRows > 0)
			m_pRecordset->MoveFirst();
	}

	return nRows;
}

BOOL CADORecordset::IsOpen()
{
	if (m_pRecordset != NULL && IsConnectionOpen())
		return m_pRecordset->GetState() != adStateClosed;
	return FALSE;
}

void CADORecordset::Close()
{
	if (IsOpen())
	{
		if (m_nEditStatus != dbEditNone)
			CancelUpdate();

		m_pRecordset->PutSort(_T(""));
		m_pRecordset->Close();
	}
}


BOOL CADODatabase::Execute(LPCTSTR lpstrExec)
{
	ASSERT(m_pConnection != NULL);
	ASSERT(_tcscmp(lpstrExec, _T("")) != 0);
	_variant_t vRecords;

	m_nRecordsAffected = 0;

	try
	{
		m_pConnection->CursorLocation = adUseClient;
		m_pConnection->Execute(_bstr_t(lpstrExec), &vRecords,
						adExecuteNoRecords);
		m_nRecordsAffected = vRecords.iVal;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::RecordBinding(CADORecordBinding& pAdoRecordBinding)
{
	HRESULT hr;
	m_pRecBinding = NULL;

	//Open the binding interface.
	if (FAILED(hr = m_pRecordset->QueryInterface(__uuidof(IADORecordBinding),
									(LPVOID *) &m_pRecBinding)))
	{
		_com_issue_error(hr);
		return FALSE;
	}

	//Bind the recordset to class
	if (FAILED(hr = m_pRecBinding->BindToRecordset(&pAdoRecordBinding)))
	{
		_com_issue_error(hr);
		return FALSE;
	}
	return TRUE;
}

BOOL CADORecordset::GetFieldInfo(LPCTSTR lpFieldName, CADOFieldInfo* fldInfo)
{
	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);

	return GetFieldInfo(pField, fldInfo);
}

BOOL CADORecordset::GetFieldInfo(int nIndex, CADOFieldInfo* fldInfo)
{
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);

	return GetFieldInfo(pField, fldInfo);
}


BOOL CADORecordset::GetFieldInfo(FieldPtr pField, CADOFieldInfo* fldInfo)
{
	memset(fldInfo, 0, sizeof(CADOFieldInfo));

	_tcscpy(fldInfo->m_strName, (LPCTSTR) pField->GetName());
	fldInfo->m_lDefinedSize = pField->GetDefinedSize();
	fldInfo->m_nType = pField->GetType();
	fldInfo->m_lAttributes = pField->GetAttributes();
	if (!IsEof())
		fldInfo->m_lSize = pField->GetActualSize();
	return TRUE;
}

/*========================================================================
Remarks:	Recordset 对象包括 Field 对象组成的 Fields 集合. 每个Field
对象对应 Recordset 集中的一列.
==========================================================================*/
FieldsPtr CADORecordset::GetFields()
{
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->GetFields();
	}
	catch (_com_error e)
	{
		TRACE(_T("Warning: GetFields 方法发生异常. 错误信息: %s; 文件: %s; 行: %d\n"),
			e.ErrorMessage(), _T(__FILE__), __LINE__);
		return NULL;
	} 
	return NULL;
}

/*========================================================================
Name:	取得指定列的字段对象的指针.	
==========================================================================*/
FieldPtr CADORecordset::GetField(int lIndex)
{
	try
	{
		return GetFields()->GetItem(_variant_t((long) lIndex));
	}
	catch (_com_error e)
	{
		TRACE(_T("Warning: GetField发生异常. 错误信息: %s; 文件: %s; 行: %d\n"),
			e.ErrorMessage(), _T(__FILE__), __LINE__);
		return NULL;
	}
}

FieldPtr CADORecordset::GetField(LPCTSTR lpszFieldName)
{
	try
	{
		return GetFields()->GetItem(_variant_t(lpszFieldName));
	}
	catch (_com_error e)
	{
		TRACE(_T("Warning: GetField发生异常. 错误信息: %s; 文件: %s; 行: %d\n"),
			e.ErrorMessage(), _T(__FILE__), __LINE__);
		return NULL;
	}
}

/*========================================================================
	Remarks:	取得指定列字段的字段名.
==========================================================================*/
CString CADORecordset::GetFieldName(int lIndex)
{
	ASSERT(m_pRecordset != NULL);
	CString strFieldName;
	try
	{
		strFieldName = LPCTSTR(m_pRecordset->Fields->GetItem(_variant_t((long)
																lIndex))->GetName());
	}
	catch (_com_error e)
	{
		TRACE(_T("Warning: GetFieldName 方法发生异常. 错误信息: %s; 文件: %s; 行: %d\n"),
			e.ErrorMessage(), _T(__FILE__), __LINE__);
	}
	return strFieldName;
}

/*========================================================================
	name:		取得 Field 对象一项或多项属性.
	----------------------------------------------------------
	returns:	对于 Field 对象, Attributes 属性为只读, 其值可能为以下任意
		一个或多个 FieldAttributeEnum 值的和.
	  [常量]				[说明] 
	  -------------------------------------------
	  adFldMayDefer			指示字段被延迟, 即不从拥有整个记录的数据源检索
						字段值, 仅在显式访问这些字段时才进行检索. 
	  adFldUpdatable		指示可以写入该字段. 
	  adFldUnknownUpdatable 指示提供者无法确定是否可以写入该字段. 
	  adFldFixed			指示该字段包含定长数据. 
	  adFldIsNullable		指示该字段接受 Null 值. 
	  adFldMayBeNull		指示可以从该字段读取 Null 值. 
	  adFldLong				指示该字段为长二进制字段. 并指示可以使用 AppendChunk 
						和 GetChunk 方法. 
	  adFldRowID			指示字段包含持久的行标识符, 该标识符无法被写入
						并且除了对行进行标识(如记录号、唯一标识符等)外不
						存在有意义的值. 
	  adFldRowVersion		指示该字段包含用来跟踪更新的某种时间或日期标记. 
	  adFldCacheDeferred	指示提供者缓存了字段值, 并已完成随后对缓存的读取. 
==========================================================================*/
long CADORecordset::GetFieldAttributes(int lIndex)
{
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->Fields->GetItem(_variant_t((long) lIndex))->GetAttributes();
	}
	catch (_com_error e)
	{
		TRACE(_T("Warning: GetFieldAttributes 方法发生异常. 错误信息: %s; 文件: %s; 行: %d\n"),
			e.ErrorMessage(), _T(__FILE__), __LINE__);
		return -1;
	}
}

long CADORecordset::GetFieldAttributes(LPCTSTR lpszFieldName)
{
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->Fields->GetItem(_variant_t(lpszFieldName))->GetAttributes();
	}
	catch (_com_error e)
	{
		TRACE(_T("Warning: GetFieldAttributes 方法发生异常. 错误信息: %s; 文件: %s; 行: %d\n"),
			e.ErrorMessage(), _T(__FILE__), __LINE__);
		return -1;
	}
}

/*========================================================================
	Name:		指示 Field 对象所定义的长度.
	----------------------------------------------------------
	returns:	返回某个字段定义的长度(按字节数)的长整型值.
	----------------------------------------------------------
	Remarks:	使用 DefinedSize 属性可确定 Field 对象的数据容量.
==========================================================================*/
long CADORecordset::GetFieldDefineSize(int lIndex)
{
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->Fields->GetItem(_variant_t((long) lIndex))->DefinedSize;
	}
	catch (_com_error e)
	{
		TRACE(_T("Warning: GetDefineSize 方法发生异常. 错误信息: %s; 文件: %s; 行: %d\n"),
			e.ErrorMessage(), _T(__FILE__), __LINE__);
		return -1;
	}
}

long CADORecordset::GetFieldDefineSize(LPCTSTR lpszFieldName)
{
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->Fields->GetItem(_variant_t(lpszFieldName))->DefinedSize;
	}
	catch (_com_error e)
	{
		TRACE(_T("Warning: GetDefineSize 方法发生异常. 错误信息: %s; 文件: %s; 行: %d\n"),
			e.ErrorMessage(), _T(__FILE__), __LINE__);
		return -1;
	}
}

/*========================================================================
	Name:	取得字段的值的实际长度.
	----------------------------------------------------------
	returns:	返回长整型值.某些提供者允许设置该属性以便为 BLOB 数据预留
			空间, 在此情况下默认值为 0.
	----------------------------------------------------------
	Remarks:	使用 ActualSize 属性可返回 Field 对象值的实际长度.对于所有
			字段,ActualSize 属性为只读.如果 ADO 无法确定 Field 对象值的实
			际长度, ActualSize 属性将返回 adUnknown.
				如以下范例所示, ActualSize 和  DefinedSize 属性有所不同: 
			adVarChar 声明类型且最大长度为 50 个字符的 Field 对象将返回为 
			50 的 DefinedSize 属性值, 但是返回的 ActualSize 属性值是当前记
			录的字段中存储的数据的长度.
==========================================================================*/
long CADORecordset::GetFieldActualSize(int lIndex)
{
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->Fields->GetItem(_variant_t((long) lIndex))->ActualSize;
	}
	catch (_com_error e)
	{
		TRACE(_T("Warning: GetFieldActualSize 方法发生异常. 错误信息: %s; 文件: %s; 行: %d\n"),
			e.ErrorMessage(), _T(__FILE__), __LINE__);
		return -1;
	}
}

long CADORecordset::GetFieldActualSize(LPCTSTR lpszFieldName)
{
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->Fields->GetItem(_variant_t(lpszFieldName))->ActualSize;
	}
	catch (_com_error e)
	{
		TRACE(_T("Warning: GetFieldActualSize 方法发生异常. 错误信息: %s; 文件: %s; 行: %d\n"),
			e.ErrorMessage(), _T(__FILE__), __LINE__);
		return -1;
	}
}

/*========================================================================
	returns:	返回下列值之一. 相应的 OLE DB 类型标识符在下表的说明栏的括
			号中给出.
	  [常量]			[说明] 
	  ---------------------------------------------------------
	  adArray			与其他类型一起加入逻辑 OR 以指示该数据是那种类型的
					安全数组 (DBTYPE_ARRAY). 
	  adBigInt			8 字节带符号的整数 (DBTYPE_I8). 
	  adBinary			二进制值 (DBTYPE_BYTES). 
	  adBoolean			布尔型值 (DBTYPE_BOOL). 
	  adByRef			与其他类型一起加入逻辑 OR 以指示该数据是其他类型数
					据的指针 (DBTYPE_BYREF). 
	  adBSTR			以空结尾的字符串 (Unicode) (DBTYPE_BSTR). 
	  adChar			字符串值 (DBTYPE_STR). 
	  adCurrency		货币值 (DBTYPE_CY).货币数字的小数点位置固定、小数
					点右侧有四位数字.该值保存为 8 字节范围为10,000 的带符
					号整型值. 
	  adDate			日期值 (DBTYPE_DATE).日期按双精度型数值来保存, 数
					字全部表示从 1899 年 12 月 30 开始的日期数.小数部分是
					一天当中的片段时间. 
	  adDBDate			日期值 (yyyymmdd) (DBTYPE_DBDATE). 
	  adDBTime			时间值 (hhmmss) (DBTYPE_DBTIME). 
	  adDBTimeStamp		时间戳 (yyyymmddhhmmss 加 10 亿分之一的小数)(DBTYPE_DBTIMESTAMP). 
	  adDecimal			具有固定精度和范围的精确数字值 (DBTYPE_DECIMAL). 
	  adDouble			双精度浮点值 (DBTYPE_R8). 
	  adEmpty			未指定值 (DBTYPE_EMPTY). 
	  adError			32 - 位错误代码 (DBTYPE_ERROR). 
	  adGUID			全局唯一的标识符 (GUID) (DBTYPE_GUID). 
	  adIDispatch		OLE 对象上 Idispatch 接口的指针 (DBTYPE_IDISPATCH). 
	  adInteger			4 字节的带符号整型 (DBTYPE_I4). 
	  adIUnknown		OLE 对象上 IUnknown 接口的指针 (DBTYPE_IUNKNOWN).
	  adLongVarBinary	长二进制值. 
	  adLongVarChar		长字符串值. 
	  adLongVarWChar	以空结尾的长字符串值. 
	  adNumeric			具有固定精度和范围的精确数字值 (DBTYPE_NUMERIC). 
	  adSingle			单精度浮点值 (DBTYPE_R4). 
	  adSmallInt		2 字节带符号整型 (DBTYPE_I2). 
	  adTinyInt			1 字节带符号整型 (DBTYPE_I1). 
	  adUnsignedBigInt	8 字节不带符号整型 (DBTYPE_UI8). 
	  adUnsignedInt		4 字节不带符号整型 (DBTYPE_UI4). 
	  adUnsignedSmallInt 2 字节不带符号整型 (DBTYPE_UI2). 
	  adUnsignedTinyInt 1 字节不带符号整型 (DBTYPE_UI1). 
	  adUserDefined		用户定义的变量 (DBTYPE_UDT). 
	  adVarBinary		二进制值. 
	  adVarChar			字符串值. 
	  adVariant			自动变体型 (DBTYPE_VARIANT). 
	  adVector			与其他类型一起加入逻辑 OR 中, 指示数据是 DBVECTOR 
					结构(由 OLE DB 定义).该结构含有元素的计数和其他类型 
					(DBTYPE_VECTOR) 数据的指针. 
	  adVarWChar		以空结尾的 Unicode 字符串. 
	  adWChar			以空结尾的 Unicode 字符串 (DBTYPE_WSTR). 
	----------------------------------------------------------
	Remarks:	返回指定字段的数据类型.
==========================================================================*/
ADO::DataTypeEnum CADORecordset::GetFieldType(int lIndex)
{
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->Fields->GetItem(_variant_t((long) lIndex))->GetType();
	}
	catch (_com_error e)
	{
		TRACE(_T("Warning: GetField 方法发生异常. 错误信息: %s; 文件: %s; 行: %d\n"),
			e.ErrorMessage(), _T(__FILE__), __LINE__);
		return adEmpty;
	}
}

ADO::DataTypeEnum CADORecordset::GetFieldType(LPCTSTR lpszFieldName)
{
	ASSERT(m_pRecordset != NULL);
	try
	{
		return m_pRecordset->Fields->GetItem(_variant_t(lpszFieldName))->GetType();
	}
	catch (_com_error e)
	{
		TRACE(_T("Warning: GetField发生异常. 错误信息: %s; 文件: %s; 行: %d\n"),
			e.ErrorMessage(), _T(__FILE__), __LINE__);
		return adEmpty;
	}
}

BOOL CADORecordset::GetChunk(LPCTSTR lpFieldName, CString& strValue)
{
	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);

	return GetChunk(pField, strValue);
}

BOOL CADORecordset::GetChunk(int nIndex, CString& strValue)
{
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);

	return GetChunk(pField, strValue);
}


BOOL CADORecordset::GetChunk(FieldPtr pField, CString& strValue)
{
	CString str = _T("");
	long lngSize, lngOffSet = 0;
	_variant_t varChunk;

	lngSize = pField->ActualSize;

	str.Empty();
	while (lngOffSet < lngSize)
	{
		try
		{
			varChunk = pField->GetChunk(ChunkSize);

			str += varChunk.bstrVal;
			lngOffSet += ChunkSize;
		}
		catch (_com_error& e)
		{
			dump_com_error(e);
			return FALSE;
		}
	}

	lngOffSet = 0;
	strValue = str;
	return TRUE;
}

BOOL CADORecordset::GetChunk(LPCTSTR lpFieldName, LPVOID lpData)
{
	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);

	return GetChunk(pField, lpData);
}

BOOL CADORecordset::GetChunk(int nIndex, LPVOID lpData)
{
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);

	return GetChunk(pField, lpData);
}

BOOL CADORecordset::GetChunk(FieldPtr pField, LPVOID lpData)
{
	long lngSize, lngOffSet = 0;
	_variant_t varChunk;	
	UCHAR chData;
	HRESULT hr;
	long lBytesCopied = 0;

	lngSize = pField->ActualSize;

	while (lngOffSet < lngSize)
	{
		try
		{
			varChunk = pField->GetChunk(ChunkSize);

			//Copy the data only upto the Actual Size of Field.  
			for (long lIndex = 0; lIndex <= (ChunkSize - 1); lIndex++)
			{
				hr = SafeArrayGetElement(varChunk.parray, &lIndex, &chData);
				if (SUCCEEDED(hr))
				{
					//Take BYTE by BYTE and advance Memory Location
					//hr = SafeArrayPutElement((SAFEARRAY FAR*)lpData, &lBytesCopied ,&chData); 
					((UCHAR *) lpData)[lBytesCopied] = chData;
					lBytesCopied++;
				}
				else
					break;
			}
			lngOffSet += ChunkSize;
		}
		catch (_com_error& e)
		{
			dump_com_error(e);
			return FALSE;
		}
	}

	lngOffSet = 0;
	return TRUE;
}

BOOL CADORecordset::GetChunk(int index, CBitmap& bitmap)
{
	return GetChunk(GetFieldName(index), bitmap);
}

BOOL CADORecordset::GetChunk(LPCTSTR strFieldName, CBitmap& bitmap)
{
	BOOL bret = FALSE;
	long size = GetFieldActualSize(strFieldName);
	if ((adFldLong & GetFieldAttributes(strFieldName)) && size > 0)
	{
		BYTE* lpData = new BYTE[size];

		if (GetChunk(GetField(strFieldName), (LPVOID) lpData))
		{
			BITMAPFILEHEADER bmpHeader;
			DWORD bmfHeaderLen = sizeof(bmpHeader);
			_tcsncpy((LPTSTR) & bmpHeader, (LPTSTR) lpData, bmfHeaderLen);

			// 是否是位图 ----------------------------------------
			if (bmpHeader.bfType == (*(WORD *) _T("BM")))
			{
				BYTE* lpDIBBits = lpData + bmfHeaderLen;
				BITMAPINFOHEADER& bmpiHeader = *(LPBITMAPINFOHEADER)
					lpDIBBits;
				BITMAPINFO& bmpInfo = *(LPBITMAPINFO) lpDIBBits;
				lpDIBBits = lpData + ((BITMAPFILEHEADER *) lpData)->bfOffBits;

				// 创建位图 --------------------------------------
				CDC dc;
				HDC hdc = GetDC(NULL);
				dc.Attach(hdc);
				HBITMAP hBmp = CreateDIBitmap(dc.m_hDC, &bmpiHeader, CBM_INIT,
								lpDIBBits, &bmpInfo, DIB_RGB_COLORS);
				if (bitmap.GetSafeHandle() != NULL)
					bitmap.DeleteObject();
				bitmap.Attach(hBmp);
				dc.Detach();
				ReleaseDC(NULL, hdc);
				bret = TRUE;
			}
		}
		delete[] lpData;
		lpData = NULL;
	}
	return bret;
}

BOOL CADORecordset::AppendChunk(LPCTSTR lpFieldName, LPVOID lpData,
	UINT nBytes)
{
	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);

	return AppendChunk(pField, lpData, nBytes);
}


BOOL CADORecordset::AppendChunk(int nIndex, LPVOID lpData, UINT nBytes)
{
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);

	return AppendChunk(pField, lpData, nBytes);
}

BOOL CADORecordset::AppendChunk(FieldPtr pField, LPVOID lpData, UINT nBytes)
{
	HRESULT hr;
	_variant_t varChunk;
	long lngOffset = 0;
	UCHAR chData;
	SAFEARRAY FAR* psa = NULL;
	SAFEARRAYBOUND rgsabound[1];

	try
	{
		//Create a safe array to store the array of BYTES 
		rgsabound[0].lLbound = 0;
		rgsabound[0].cElements = nBytes;
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

		while (lngOffset < (long) nBytes)
		{
			chData = ((UCHAR *) lpData)[lngOffset];
			hr = SafeArrayPutElement(psa, &lngOffset, &chData);

			if (FAILED(hr))
				return FALSE;

			lngOffset++;
		}
		lngOffset = 0;

		//Assign the Safe array  to a variant. 
		varChunk.vt = VT_ARRAY | VT_UI1;
		varChunk.parray = psa;

		hr = pField->AppendChunk(varChunk);

		if (SUCCEEDED(hr))
			return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}

	return FALSE;
}
BOOL CADORecordset::AppendChunk(LPCTSTR strFieldName, LPCTSTR lpszFileName)
{
	ASSERT(m_pRecordset != NULL);
	ASSERT(lpszFileName != NULL);
	BOOL bret = FALSE;
	if (adFldLong & GetFieldAttributes(strFieldName))
	{
		CFile file;
		if (file.Open(lpszFileName, CFile::modeRead))
		{
			long length = (long) file.GetLength();
			TCHAR* pbuf = new TCHAR[length];
			if (pbuf != NULL && file.Read(pbuf, length) == (DWORD) length)
			{
				bret = AppendChunk(GetField(strFieldName), pbuf, length);
			}
			if (pbuf != NULL)
				delete[] pbuf;
		}
		file.Close();
	}
	return bret;
}

BOOL CADORecordset::AppendChunk(int index, LPCTSTR lpszFileName)
{
	ASSERT(m_pRecordset != NULL);
	ASSERT(lpszFileName != NULL);
	BOOL bret = FALSE;
	if (adFldLong & GetFieldAttributes(index))
	{
		CFile file;
		if (file.Open(lpszFileName, CFile::modeRead))
		{
			long length = (long) file.GetLength();
			TCHAR* pbuf = new TCHAR[length];
			if (pbuf != NULL && file.Read(pbuf, length) == (DWORD) length)
			{
				bret = AppendChunk(GetField(index), pbuf, length);
			}
			if (pbuf != NULL)
				delete[] pbuf;
		}
		file.Close();
	}
	return bret;
}

BOOL CADORecordset::AppendChunk(LPCTSTR lpFieldName, const HBITMAP& hBitmap)
{
	BOOL bReturn;
	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);

	LPVOID pVoid;
	DWORD nSize = 0;

	if (SaveBitmapToMemFile(hBitmap, &pVoid, nSize))
	{
		bReturn = AppendChunk(pField, pVoid, nSize);
		delete[] pVoid;
	}

	return bReturn;
}

BOOL CADORecordset::AppendChunk(int index, const HBITMAP& hBitmap)
{
	BOOL bReturn;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = index;
	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);

	LPVOID pVoid;
	DWORD nSize = 0;
	if (SaveBitmapToMemFile(hBitmap, &pVoid, nSize))
	{
		bReturn = AppendChunk(pField, pVoid, nSize);
		delete[] pVoid;
	}

	return FALSE;
}


CString CADORecordset::GetString(LPCTSTR lpCols, LPCTSTR lpRows,
	LPCTSTR lpNull, long numRows)
{
	_bstr_t varOutput;
	_bstr_t varNull(_T(""));
	_bstr_t varCols(_T("\t"));
	_bstr_t varRows(_T("\r"));

	if (_tcslen(lpCols) != 0)
		varCols = _bstr_t(lpCols);

	if (_tcslen(lpRows) != 0)
		varRows = _bstr_t(lpRows);

	if (numRows == 0)
		numRows = (long) GetRecordCount();			

	varOutput = m_pRecordset->GetString(adClipString, numRows, varCols,
								varRows, varNull);

	return (LPCTSTR) varOutput;
}

CString IntToStr(int nVal)
{
	CString strRet;
	TCHAR buff[10];

	_itot(nVal, buff, 10);
	strRet = buff;
	return strRet;
}

CString LongToStr(long lVal)
{
	CString strRet;
	TCHAR buff[20];

	_ltot(lVal, buff, 10);
	strRet = buff;
	return strRet;
}

CString ULongToStr(unsigned long ulVal)
{
	CString strRet;
	TCHAR buff[20];

	_ultot(ulVal, buff, 10);
	strRet = buff;
	return strRet;
}



CString DblToStr(double dblVal, int ndigits)
{
	int nBits = ndigits;
	dblVal *= pow(10.0,nBits); 
	if(dblVal < 0)
		dblVal -= 0.5;
	else
		dblVal += 0.5;
	dblVal /= pow(10.0,nBits);

	CString szEx = _T("");
	szEx.Format(_T("%f"),dblVal);
	szEx = szEx.Left(szEx.Find(_T('.'),0) + ((nBits == 0) ? 0 : nBits + 1));

	return szEx;

	// 	CString strRet;
	// 	TCHAR buff[50];
	// 
	//    _gcvt(dblVal, ndigits, buff);
	// 	strRet = buff;
	// 	return strRet;
}

CString DblToStr(float fltVal)
{
	int nBits = 10;
	fltVal *= pow(10.0,nBits); 
	if(fltVal < 0)
		fltVal -= 0.5;
	else
		fltVal += 0.5;
	fltVal /= pow(10.0,nBits);

	CString szEx = _T("");
	szEx.Format(_T("%f"),fltVal);
	szEx = szEx.Left(szEx.Find(_T('.'),0) + ((nBits == 0) ? 0 : nBits + 1));

	return szEx;

	// 	CString strRet = _T("");
	// 	TCHAR buff[50];
	// 	
	// 	 _gcvt(fltVal, 10, buff);
	// 
	// 	strRet = buff;
	//	return strRet;
}

void CADORecordset::Edit()
{
	m_nEditStatus = dbEdit;
}

BOOL CADORecordset::AddNew()
{
	m_nEditStatus = dbEditNone;
	if (m_pRecordset->AddNew() != S_OK)
		return FALSE;

	m_nEditStatus = dbEditNew;
	return TRUE;
}

BOOL CADORecordset::AddNew(CADORecordBinding& pAdoRecordBinding)
{
	try
	{
		if (m_pRecBinding->AddNew(&pAdoRecordBinding) != S_OK)
		{
			return FALSE;
		}
		else
		{
			m_pRecBinding->Update(&pAdoRecordBinding);
			return TRUE;
		}
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::Update()
{
	BOOL bret = TRUE;

	if (m_nEditStatus != dbEditNone)
	{
		try
		{
			if (m_pRecordset->Update() != S_OK)
				bret = FALSE;
		}
		catch (_com_error& e)
		{
			dump_com_error(e);
			bret = FALSE;
		}

		if (!bret)
			m_pRecordset->CancelUpdate();
		m_nEditStatus = dbEditNone;
	}
	return bret;
}

void CADORecordset::CancelUpdate()
{
	m_pRecordset->CancelUpdate();
	m_nEditStatus = dbEditNone;
}

BOOL CADORecordset::SetFieldValue(int nIndex, CString strValue)
{
	_variant_t vtFld;
	_variant_t vtIndex;	

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	if (!strValue.IsEmpty())
		vtFld.vt = VT_BSTR;
	else
		vtFld.vt = VT_NULL;

	//Corrected by Giles Forster 10/03/2001
	vtFld.bstrVal = strValue.AllocSysString();

	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, CString strValue)
{
	_variant_t vtFld;

	if (!strValue.IsEmpty())
		vtFld.vt = VT_BSTR;
	else
		vtFld.vt = VT_NULL;

	//Corrected by Giles Forster 10/03/2001
	vtFld.bstrVal = strValue.AllocSysString();

	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldValue(int nIndex, int nValue)
{
	_variant_t vtFld;

	vtFld.vt = VT_I2;
	vtFld.iVal = nValue;

	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, int nValue)
{
	_variant_t vtFld;

	vtFld.vt = VT_I2;
	vtFld.iVal = nValue;


	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldValue(int nIndex, long lValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_I4;
	vtFld.lVal = lValue;

	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, long lValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_I4;
	vtFld.lVal = lValue;

	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldValue(int nIndex, unsigned long ulValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_UI4;
	vtFld.ulVal = ulValue;

	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, unsigned long ulValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_UI4;
	vtFld.ulVal = ulValue;

	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldValue(int nIndex, double dblValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_R8;
	vtFld.dblVal = dblValue;

	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, double dblValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_R8;
	vtFld.dblVal = dblValue;

	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldValue(int nIndex, COleDateTime time)
{
	_variant_t vtFld;
	vtFld.vt = VT_DATE;
	vtFld.date = time;

	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, COleDateTime time)
{
	_variant_t vtFld;
	vtFld.vt = VT_DATE;
	vtFld.date = time;

	return PutFieldValue(lpFieldName, vtFld);
}



BOOL CADORecordset::SetFieldValue(int nIndex, bool bValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_BOOL;
	vtFld.boolVal = bValue;

	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, bool bValue)
{
	_variant_t vtFld;
	vtFld.vt = VT_BOOL;
	vtFld.boolVal = bValue;

	return PutFieldValue(lpFieldName, vtFld);
}


BOOL CADORecordset::SetFieldValue(int nIndex, COleCurrency cyValue)
{
	if (cyValue.m_status == COleCurrency::invalid)
		return FALSE;

	_variant_t vtFld;

	vtFld.vt = VT_CY;
	vtFld.cyVal = cyValue.m_cur;

	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtFld);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, COleCurrency cyValue)
{
	if (cyValue.m_status == COleCurrency::invalid)
		return FALSE;

	_variant_t vtFld;

	vtFld.vt = VT_CY;
	vtFld.cyVal = cyValue.m_cur;	

	return PutFieldValue(lpFieldName, vtFld);
}

BOOL CADORecordset::SetFieldValue(int nIndex, _variant_t vtValue)
{
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtValue);
}

BOOL CADORecordset::SetFieldValue(LPCTSTR lpFieldName, _variant_t vtValue)
{
	return PutFieldValue(lpFieldName, vtValue);
}


BOOL CADORecordset::SetBookmark()
{
	if (m_varBookmark.vt != VT_EMPTY)
	{
		m_pRecordset->Bookmark = m_varBookmark;
		return TRUE;
	}
	return FALSE;
}

BOOL CADORecordset::Delete()
{
	if (m_pRecordset->Delete(adAffectCurrent) != S_OK)
		return FALSE;

	if (m_pRecordset->Update() != S_OK)
		return FALSE;

	m_nEditStatus = dbEditNone;
	return TRUE;
}

BOOL CADORecordset::Find(LPCTSTR lpFind, int nSearchDirection)
{
	m_strFind = lpFind;
	m_nSearchDirection = nSearchDirection;

	ASSERT(!m_strFind.IsEmpty());

	if (m_nSearchDirection == searchForward)
	{
		m_pRecordset->Find(_bstr_t(m_strFind), 0, adSearchForward, _T(""));
		if (!IsEof())
		{
			m_varBookFind = m_pRecordset->Bookmark;
			return TRUE;
		}
	}
	else if (m_nSearchDirection == searchBackward)
	{
		m_pRecordset->Find(_bstr_t(m_strFind), 0, adSearchBackward, _T(""));
		if (!IsBof())
		{
			m_varBookFind = m_pRecordset->Bookmark;
			return TRUE;
		}
	}
	else
	{
		TRACE(_T("Unknown parameter. %d"), nSearchDirection);
		m_nSearchDirection = searchForward;
	}
	return FALSE;
}

BOOL CADORecordset::FindFirst(LPCTSTR lpFind)
{
	m_pRecordset->MoveFirst();
	return Find(lpFind);
}

BOOL CADORecordset::FindNext()
{
	if (m_nSearchDirection == searchForward)
	{
		m_pRecordset->Find(_bstr_t(m_strFind), 1, adSearchForward,
						m_varBookFind);
		if (!IsEof())
		{
			m_varBookFind = m_pRecordset->Bookmark;
			return TRUE;
		}
	}
	else
	{
		m_pRecordset->Find(_bstr_t(m_strFind), 1, adSearchBackward,
						m_varBookFind);
		if (!IsBof())
		{
			m_varBookFind = m_pRecordset->Bookmark;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CADORecordset::PutFieldValue(LPCTSTR lpFieldName, _variant_t vtFld)
{
	if (m_nEditStatus == dbEditNone)
		return FALSE;

	try
	{
		m_pRecordset->Fields->GetItem(lpFieldName)->Value = vtFld; 
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


BOOL CADORecordset::PutFieldValue(_variant_t vtIndex, _variant_t vtFld)
{
	if (m_nEditStatus == dbEditNone)
		return FALSE;

	try
	{
		m_pRecordset->Fields->GetItem(vtIndex)->Value = vtFld;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::Clone(CADORecordset& pRs)
{
	try
	{
		pRs.m_pRecordset = m_pRecordset->Clone(adLockUnspecified);
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::SetFilter(LPCTSTR strFilter)
{
	ASSERT(IsOpen());

	try
	{
		m_pRecordset->PutFilter(strFilter);
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::SetSort(LPCTSTR strCriteria)
{
	ASSERT(IsOpen());

	try
	{
		m_pRecordset->PutSort(strCriteria);
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::SaveAsXML(LPCTSTR lpstrXMLFile)
{
	HRESULT hr;

	ASSERT(IsOpen());

	try
	{
		hr = m_pRecordset->Save(lpstrXMLFile, adPersistXML);
		return hr == S_OK;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
	return TRUE;
}

BOOL CADORecordset::OpenXML(LPCTSTR lpstrXMLFile)
{
	HRESULT hr = S_OK;

	if (IsOpen())
		Close();

	try
	{
		hr = m_pRecordset->Open(lpstrXMLFile, _T("Provider=MSPersist;"),
							adOpenForwardOnly, adLockOptimistic, adCmdFile);
		return hr == S_OK;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADORecordset::Execute(CADOCommand* pAdoCommand)
{
	if (IsOpen())
		Close();

	ASSERT(!pAdoCommand->GetText().IsEmpty());
	try
	{
		m_pConnection->CursorLocation = adUseClient;
		m_pRecordset = pAdoCommand->GetCommand()->Execute(NULL, NULL,
													pAdoCommand->GetType());
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

void CADORecordset::dump_com_error(_com_error& e)
{
	CString ErrorStr;


	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());
	ErrorStr.Format(_T("CADORecordset Error\n\tCode = %08lx\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n"),
				e.Error(), e.ErrorMessage(), (LPCTSTR)
				bstrSource,
				(LPCTSTR)
				bstrDescription);
	m_strLastError = _T("Query = ") + GetQuery() + _T('\n') + ErrorStr;
	m_dwLastError = e.Error();
#ifdef _DEBUG
	AfxMessageBox(ErrorStr, MB_OK | MB_ICONERROR);
#endif	
}


///////////////////////////////////////////////////////
//
// CADOCommad Class
//

CADOCommand::CADOCommand(CADODatabase* pAdoDatabase, CString strCommandText,
	int nCommandType)
{
	m_pCommand = NULL;
	m_pCommand.CreateInstance(__uuidof(Command));
	m_strCommandText = strCommandText;
	m_pCommand->CommandText = m_strCommandText.AllocSysString();
	m_nCommandType = nCommandType;
	m_pCommand->CommandType = (CommandTypeEnum) m_nCommandType;
	m_pCommand->ActiveConnection = pAdoDatabase->GetActiveConnection();	
	m_nRecordsAffected = 0;
}

BOOL CADOCommand::AddParameter(CADOParameter* pAdoParameter)
{
	ASSERT(pAdoParameter->GetParameter() != NULL);

	try
	{
		m_pCommand->Parameters->Append(pAdoParameter->GetParameter());
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOCommand::AddParameter(CString strName, int nType, int nDirection,
	long lSize, int nValue)
{
	_variant_t vtValue;

	vtValue.vt = VT_I2;
	vtValue.iVal = nValue;

	return AddParameter(strName, nType, nDirection, lSize, vtValue);
}

BOOL CADOCommand::AddParameter(CString strName, int nType, int nDirection,
	long lSize, long lValue)
{
	_variant_t vtValue;

	vtValue.vt = VT_I4;
	vtValue.lVal = lValue;

	return AddParameter(strName, nType, nDirection, lSize, vtValue);
}

BOOL CADOCommand::AddParameter(CString strName, int nType, int nDirection,
	long lSize, double dblValue, int nPrecision, int nScale)
{
	_variant_t vtValue;

	vtValue.vt = VT_R8;
	vtValue.dblVal = dblValue;

	return AddParameter(strName, nType, nDirection, lSize, vtValue,
			nPrecision, nScale);
}

BOOL CADOCommand::AddParameter(CString strName, int nType, int nDirection,
	long lSize, CString strValue)
{
	_variant_t vtValue;

	vtValue.vt = VT_BSTR;
	vtValue.bstrVal = strValue.AllocSysString();

	return AddParameter(strName, nType, nDirection, lSize, vtValue);
}

BOOL CADOCommand::AddParameter(CString strName, int nType, int nDirection,
	long lSize, COleDateTime time)
{
	_variant_t vtValue;

	vtValue.vt = VT_DATE;
	vtValue.date = time;

	return AddParameter(strName, nType, nDirection, lSize, vtValue);
}


BOOL CADOCommand::AddParameter(CString strName, int nType, int nDirection,
	long lSize, _variant_t vtValue, int nPrecision, int nScale)
{
	try
	{
		_ParameterPtr pParam = m_pCommand->CreateParameter(strName.AllocSysString(),
											(ADO::DataTypeEnum)
											nType,
											(ADO::ParameterDirectionEnum)
											nDirection,
											lSize, vtValue);
		pParam->PutPrecision(nPrecision);
		pParam->PutNumericScale(nScale);
		m_pCommand->Parameters->Append(pParam);

		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


void CADOCommand::SetText(CString strCommandText)
{
	ASSERT(!strCommandText.IsEmpty());

	m_strCommandText = strCommandText;
	m_pCommand->CommandText = m_strCommandText.AllocSysString();
}

void CADOCommand::SetType(int nCommandType)
{
	m_nCommandType = nCommandType;
	m_pCommand->CommandType = (CommandTypeEnum) m_nCommandType;
}

BOOL CADOCommand::Execute()
{
	_variant_t vRecords;
	m_nRecordsAffected = 0;
	try
	{
		m_pCommand->Execute(&vRecords, NULL, adCmdStoredProc);
		m_nRecordsAffected = vRecords.iVal;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

void CADOCommand::dump_com_error(_com_error& e)
{
	CString ErrorStr;


	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());
	ErrorStr.Format(_T("CADOCommand Error\n\tCode = %08lx\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n"),
				e.Error(), e.ErrorMessage(), (LPCTSTR)
				bstrSource,
				(LPCTSTR)
				bstrDescription);
	m_strLastError = ErrorStr;
	m_dwLastError = e.Error();
#ifdef _DEBUG
	AfxMessageBox(ErrorStr, MB_OK | MB_ICONERROR);
#endif	
}


///////////////////////////////////////////////////////
//
// CADOParameter Class
//

CADOParameter::CADOParameter(int nType, long lSize, int nDirection,
	CString strName)
{
	m_pParameter = NULL;
	m_pParameter.CreateInstance(__uuidof(Parameter));
	m_strName = _T("");
	m_pParameter->Direction = (ADO::ParameterDirectionEnum) nDirection;
	m_strName = strName;
	m_pParameter->Name = m_strName.AllocSysString();
	m_pParameter->Type = (ADO::DataTypeEnum) nType;
	m_pParameter->Size = lSize;
	m_nType = nType;
}

BOOL CADOParameter::SetValue(int nValue)
{
	_variant_t vtVal;

	ASSERT(m_pParameter != NULL);

	vtVal.vt = VT_I2;
	vtVal.iVal = nValue;

	try
	{
		if (m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(int);

		m_pParameter->Value = vtVal;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


BOOL CADOParameter::SetValue(long lValue)
{
	_variant_t vtVal;

	ASSERT(m_pParameter != NULL);

	vtVal.vt = VT_I4;
	vtVal.lVal = lValue;

	try
	{
		if (m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(long);

		m_pParameter->Value = vtVal;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::SetValue(double dblValue)
{
	_variant_t vtVal;

	ASSERT(m_pParameter != NULL);

	vtVal.vt = VT_R8;
	vtVal.dblVal = dblValue;

	try
	{
		if (m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(double);

		m_pParameter->Value = vtVal;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::SetValue(CString strValue)
{
	_variant_t vtVal;

	ASSERT(m_pParameter != NULL);

	if (!strValue.IsEmpty())
		vtVal.vt = VT_BSTR;
	else
		vtVal.vt = VT_NULL;

	//Corrected by Giles Forster 10/03/2001
	vtVal.bstrVal = strValue.AllocSysString();

	try
	{
		if (m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(TCHAR) * strValue.GetLength();

		m_pParameter->Value = vtVal;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::SetValue(COleDateTime time)
{
	_variant_t vtVal;

	ASSERT(m_pParameter != NULL);

	vtVal.vt = VT_DATE;
	vtVal.date = time;

	try
	{
		if (m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(DATE);

		m_pParameter->Value = vtVal;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::SetValue(_variant_t vtValue)
{
	ASSERT(m_pParameter != NULL);

	try
	{
		if (m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(VARIANT);

		m_pParameter->Value = vtValue;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::GetValue(int& nValue)
{
	_variant_t vtVal;
	int nVal = 0;

	try
	{
		vtVal = m_pParameter->Value;

		switch (vtVal.vt)
		{
		case VT_BOOL:
			nVal = vtVal.boolVal;
			break;
		case VT_I2:
		case VT_UI1:
			nVal = vtVal.iVal;
			break;
		case VT_INT:
			nVal = vtVal.intVal;
			break;
		case VT_NULL:
		case VT_EMPTY:
			nVal = 0;
			break;
		default:
			nVal = vtVal.iVal;
		}	
		nValue = nVal;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::GetValue(long& lValue)
{
	_variant_t vtVal;
	long lVal = 0;

	try
	{
		vtVal = m_pParameter->Value;
		if (vtVal.vt != VT_NULL && vtVal.vt != VT_EMPTY)
			lVal = vtVal.lVal;
		lValue = lVal;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::GetValue(double& dbValue)
{
	_variant_t vtVal;
	double dblVal;
	try
	{
		vtVal = m_pParameter->Value;
		switch (vtVal.vt)
		{
		case VT_R4:
			dblVal = vtVal.fltVal;
			break;
		case VT_R8:
			dblVal = vtVal.dblVal;
			break;
		case VT_DECIMAL:
			//Corrected by Jos?Carlos Mart韓ez Gal醤
			dblVal = vtVal.decVal.Lo32;
			dblVal *= (vtVal.decVal.sign == 128) ? -1 : 1;
			dblVal /= pow(10.0, vtVal.decVal.scale); 
			break;
		case VT_UI1:
			dblVal = vtVal.iVal;
			break;
		case VT_I2:
		case VT_I4:
			dblVal = vtVal.lVal;
			break;
		case VT_INT:
			dblVal = vtVal.intVal;
			break;
		case VT_NULL:
		case VT_EMPTY:
			dblVal = 0;
			break;
		default:
			dblVal = 0;
		}
		dbValue = dblVal;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::GetValue(CString& strValue, CString strDateFormat)
{
	_variant_t vtVal;
	CString strVal = _T("");

	try
	{
		vtVal = m_pParameter->Value;
		switch (vtVal.vt)
		{
		case VT_R4:
			strVal = DblToStr(vtVal.fltVal);
			break;
		case VT_R8:
			strVal = DblToStr(vtVal.dblVal);
			break;
		case VT_BSTR:
			strVal = vtVal.bstrVal;
			break;
		case VT_I2:
		case VT_UI1:
			strVal = IntToStr(vtVal.iVal);
			break;
		case VT_INT:
			strVal = IntToStr(vtVal.intVal);
			break;
		case VT_I4:
			strVal = LongToStr(vtVal.lVal);
			break;
		case VT_DECIMAL:
			{
				//Corrected by Jos?Carlos Mart韓ez Gal醤
				double val = vtVal.decVal.Lo32;
				val *= (vtVal.decVal.sign == 128) ? -1 : 1;
				val /= pow(10.0, vtVal.decVal.scale); 
				strVal = DblToStr(val);
			}
			break;
		case VT_DATE:
			{
				COleDateTime dt(vtVal);

				if (strDateFormat.IsEmpty())
					strDateFormat = _T("%Y-%m-%d %H:%M:%S");
				strVal = dt.Format(strDateFormat);
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			strVal.Empty();
			break;
		default:
			strVal.Empty();
			return FALSE;
		}
		strValue = strVal;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::GetValue(COleDateTime& time)
{
	_variant_t vtVal;

	try
	{
		vtVal = m_pParameter->Value;
		switch (vtVal.vt)
		{
		case VT_DATE:
			{
				COleDateTime dt(vtVal);
				time = dt;
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			time.SetStatus(COleDateTime::null);
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

BOOL CADOParameter::GetValue(_variant_t& vtValue)
{
	try
	{
		vtValue = m_pParameter->Value;
		return TRUE;
	}
	catch (_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


void CADOParameter::dump_com_error(_com_error& e)
{
	CString ErrorStr;


	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());
	ErrorStr.Format(_T("CADOParameter Error\n\tCode = %08lx\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n"),
				e.Error(), e.ErrorMessage(), (LPCTSTR)
				bstrSource,
				(LPCTSTR)
				bstrDescription);
	m_strLastError = ErrorStr;
	m_dwLastError = e.Error();
#ifdef _DEBUG
	AfxMessageBox(ErrorStr, MB_OK | MB_ICONERROR);
#endif	
}

IMPLEMENT_DYNAMIC(CADOException, CException)

CADOException::CADOException(int nCause, CString strErrorString) : CException(TRUE)
{
	m_nCause = nCause;
	m_strErrorString = strErrorString;
}

CADOException::~CADOException()
{
}

int CADOException::GetError(int nADOError)
{
	switch (nADOError)
	{
	case noError:
		return CADOException::noError;
		break;
	default:
		return CADOException::Unknown;
	}
}

void AfxThrowADOException(int nADOError, CString strErrorString)
{
	throw new CADOException(nADOError, strErrorString);
}
