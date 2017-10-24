//
//  MODULE: Ado.h
//
//	AUTHOR: Carlos Antollini 
//
//  mailto: cantollini@hotmail.com
//
//	Date: 11/12/2002
//
//	Version 2.07
// 

#ifndef _ADO_H_2004_09_07
#define _ADO_H_2004_09_07

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include <afx.h>
#include <afxdisp.h>
#include <math.h>

#pragma warning(disable: 4251)
#pragma warning(disable: 4275)

#pragma warning (disable: 4146)
// CG : In order to use this code against a different version of ADO, the appropriate
// ADO library needs to be used in the #import statement
#import "C:\Program Files\Common Files\System\ADO\msado15.dll" rename_namespace("ADO") rename("EOF", "ACS_EOF") rename("EOS","ACS_EOS")
using namespace ADO;
//#import "C:\Program Files\Common Files\System\ADO\msado15.dll" rename("EOF", "ACS_EOF")
//using namespace ADODB;


#pragma warning (default: 4146)

#include "icrsint.h"

class CADOCommand;

struct CADOFieldInfo
{
	TCHAR m_strName[30]; 
	short m_nType;
	long m_lSize; 
	long m_lDefinedSize;
	long m_lAttributes;
	short m_nOrdinalPosition;
	BOOL m_bRequired;   
	BOOL m_bAllowZeroLength; 
	long m_lCollatingOrder;
};

CString IntToStr(int nVal);

CString LongToStr(long lVal);

CString ULongToStr(unsigned long ulVal);

CString DblToStr(double dblVal, int ndigits = 20);

CString DblToStr(float fltVal);


class CADODatabase
{
public:
	CADODatabase()
	{
		::CoInitialize(NULL);

		m_pConnection = NULL;
		m_strConnection = _T("");
		m_strLastError = _T("");
		m_dwLastError = 0;
		m_pConnection.CreateInstance(__uuidof(Connection));
		m_nRecordsAffected = 0;
		m_nConnectionTimeout = 0;
	}

	~CADODatabase()
	{
		Close();
		m_pConnection.Release();
		m_pConnection = NULL;
		m_strConnection = _T("");
		m_strLastError = _T("");
		m_dwLastError = 0;
		::CoUninitialize();
	}

	BOOL Open(LPCTSTR lpstrConnection = _T(""), LPCTSTR lpstrUserID = _T(""),
		LPCTSTR lpstrPassword = _T(""));
	_ConnectionPtr GetActiveConnection()
	{
		return m_pConnection;
	};
	BOOL Execute(LPCTSTR lpstrExec);
	int GetRecordsAffected()
	{
		return m_nRecordsAffected;
	};
	DWORD GetRecordCount(_RecordsetPtr m_pRs);
	long BeginTransaction()
	{
		return m_pConnection->BeginTrans();
	};
	long CommitTransaction()
	{
		return m_pConnection->CommitTrans();
	};
	long RollbackTransaction()
	{
		return m_pConnection->RollbackTrans();
	};
	BOOL IsOpen();
	void Close();
	void SetConnectionString(LPCTSTR lpstrConnection)
	{
		m_strConnection = lpstrConnection;
	};
	CString GetConnectionString()
	{
		return m_strConnection;
	};
	CString GetLastErrorString()
	{
		return m_strLastError;
	};
	DWORD GetLastError()
	{
		return m_dwLastError;
	};
	void SetConnectionTimeout(long nConnectionTimeout = 30)
	{
		m_nConnectionTimeout = nConnectionTimeout;
	};

protected:
	void dump_com_error(_com_error& e);

public:
	_ConnectionPtr m_pConnection;

protected:
	CString m_strConnection;
	CString m_strLastError;
	DWORD m_dwLastError;
	int m_nRecordsAffected;
	long m_nConnectionTimeout;
};

class CADORecordset
{
public:
	BOOL Clone(CADORecordset& pRs);
	enum cadoOpenEnum
	{
		openUnknown			= 0,
		openQuery			= 1,
		openTable			= 2,
		openStoredProc		= 3
	};

	enum cadoEditEnum
	{
		dbEditNone		= 0,
		dbEditNew		= 1,
		dbEdit			= 2
	};

	enum cadoPositionEnum
	{
		positionUnknown		= -1,
		positionBOF			= -2,
		positionEOF			= -3
	};

	enum cadoSearchEnum
	{
		searchForward		= 1,
		searchBackward		= -1
	};

	enum cadoDataType
	{
		typeEmpty				= adEmpty,
		typeTinyInt				= adTinyInt,
		typeSmallInt			= adSmallInt,
		typeInteger				= adInteger,
		typeBigInt				= adBigInt,
		typeUnsignedTinyInt		= adUnsignedTinyInt,
		typeUnsignedSmallInt	= adUnsignedSmallInt,
		typeUnsignedInt			= adUnsignedInt,
		typeUnsignedBigInt		= adUnsignedBigInt,
		typeSingle				= adSingle,
		typeDouble				= adDouble,
		typeCurrency			= adCurrency,
		typeDecimal				= adDecimal,
		typeNumeric				= adNumeric,
		typeBoolean				= adBoolean,
		typeError				= adError,
		typeUserDefined			= adUserDefined,
		typeVariant				= adVariant,
		typeIDispatch			= adIDispatch,
		typeIUnknown			= adIUnknown,
		typeGUID				= adGUID,
		typeDate				= adDate,
		typeDBDate				= adDBDate,
		typeDBTime				= adDBTime,
		typeDBTimeStamp			= adDBTimeStamp,
		typeBSTR				= adBSTR,
		typeChar				= adChar,
		typeVarChar				= adVarChar,
		typeLongVarChar			= adLongVarChar,
		typeWChar				= adWChar,
		typeVarWChar			= adVarWChar,
		typeLongVarWChar		= adLongVarWChar,
		typeBinary				= adBinary,
		typeVarBinary			= adVarBinary,
		typeLongVarBinary		= adLongVarBinary,
		typeChapter				= adChapter,
		typeFileTime			= adFileTime,
		typePropVariant			= adPropVariant,
		typeVarNumeric			= adVarNumeric,
		typeArray				= adVariant
	};

	BOOL SetFieldValue(int nIndex, int nValue);
	BOOL SetFieldValue(LPCTSTR lpFieldName, int nValue);
	BOOL SetFieldValue(int nIndex, long lValue);
	BOOL SetFieldValue(LPCTSTR lpFieldName, long lValue);
	BOOL SetFieldValue(int nIndex, unsigned long lValue);
	BOOL SetFieldValue(LPCTSTR lpFieldName, unsigned long lValue);
	BOOL SetFieldValue(int nIndex, double dblValue);
	BOOL SetFieldValue(LPCTSTR lpFieldName, double dblValue);
	BOOL SetFieldValue(int nIndex, CString strValue);
	BOOL SetFieldValue(LPCTSTR lpFieldName, CString strValue);
	BOOL SetFieldValue(int nIndex, COleDateTime time);
	BOOL SetFieldValue(LPCTSTR lpFieldName, COleDateTime time);
	BOOL SetFieldValue(int nIndex, bool bValue);
	BOOL SetFieldValue(LPCTSTR lpFieldName, bool bValue);
	BOOL SetFieldValue(int nIndex, COleCurrency cyValue);
	BOOL SetFieldValue(LPCTSTR lpFieldName, COleCurrency cyValue);
	BOOL SetFieldValue(int nIndex, _variant_t vtValue);
	BOOL SetFieldValue(LPCTSTR lpFieldName, _variant_t vtValue);

	BOOL SetFieldEmpty(int nIndex);
	BOOL SetFieldEmpty(LPCTSTR lpFieldName);


	void CancelUpdate();
	BOOL Update();
	void Edit();
	BOOL AddNew();
	BOOL AddNew(CADORecordBinding& pAdoRecordBinding);

	BOOL Find(LPCTSTR lpFind,
		int nSearchDirection = CADORecordset::searchForward);
	BOOL FindFirst(LPCTSTR lpFind);
	BOOL FindNext();

	CADORecordset();

	CADORecordset(CADODatabase* pAdoDatabase);

	~CADORecordset()
	{
		Close();
		if (m_pRecordset)
			m_pRecordset.Release();
		if (m_pCmd)
			m_pCmd.Release();
		m_pRecordset = NULL;
		m_pCmd = NULL;
		m_pRecBinding = NULL;
		m_strQuery = _T("");
		m_strLastError = _T("");
		m_dwLastError = 0;
		m_nEditStatus = dbEditNone;
		m_pConnection = NULL;
	}

	CString GetQuery()
	{
		return m_strQuery;
	};
	void SetQuery(LPCTSTR strQuery)
	{
		m_strQuery = strQuery;
	};
	BOOL RecordBinding(CADORecordBinding& pAdoRecordBinding);
	DWORD GetRecordCount();
	BOOL IsOpen();
	void Close();
	BOOL Open(_ConnectionPtr mpdb, LPCTSTR lpstrExec = _T(""),
		int nOption = CADORecordset::openUnknown);
	BOOL Open(LPCTSTR lpstrExec = _T(""),
		int nOption = CADORecordset::openUnknown);
	long GetFieldCount()
	{
		return m_pRecordset->Fields->GetCount();
	};
	BOOL GetFieldValue(LPCTSTR lpFieldName, int& nValue);
	BOOL GetFieldValue(int nIndex, int& nValue);
	BOOL GetFieldValue(LPCTSTR lpFieldName, long& lValue);
	BOOL GetFieldValue(int nIndex, long& lValue);
	BOOL GetFieldValue(LPCTSTR lpFieldName, unsigned long& ulValue);
	BOOL GetFieldValue(int nIndex, unsigned long& ulValue);
	BOOL GetFieldValue(LPCTSTR lpFieldName, double& dbValue);
	BOOL GetFieldValue(int nIndex, double& dbValue);
	BOOL GetFieldValue(LPCTSTR lpFieldName, CString& strValue,
		CString strDateFormat = _T(""));
	BOOL GetFieldValue(int nIndex, CString& strValue,
		CString strDateFormat = _T(""));
	BOOL GetFieldValue(LPCTSTR lpFieldName, COleDateTime& time);
	BOOL GetFieldValue(int nIndex, COleDateTime& time);
	BOOL GetFieldValue(int nIndex, bool& bValue);
	BOOL GetFieldValue(LPCTSTR lpFieldName, bool& bValue);
	BOOL GetFieldValue(int nIndex, COleCurrency& cyValue);
	BOOL GetFieldValue(LPCTSTR lpFieldName, COleCurrency& cyValue);
	BOOL GetFieldValue(int nIndex, _variant_t& vtValue);
	BOOL GetFieldValue(LPCTSTR lpFieldName, _variant_t& vtValue);

	BOOL GetFieldInfo(LPCTSTR lpFieldName, CADOFieldInfo* fldInfo);
	BOOL GetFieldInfo(int nIndex, CADOFieldInfo* fldInfo);

	// �ֶμ� -------------------------------
	FieldsPtr GetFields();

	// �ֶζ��� -----------------------------
	FieldPtr GetField(int lIndex);
	FieldPtr GetField(LPCTSTR lpszFieldName);

	// �ֶ��� -------------------------------
	CString GetFieldName(int lIndex);

	// �ֶ��������� -------------------------
	ADO::DataTypeEnum GetFieldType(int lIndex);
	ADO::DataTypeEnum GetFieldType(LPCTSTR lpszFieldName);

	// �ֶ����� -----------------------------
	long GetFieldAttributes(int lIndex);
	long GetFieldAttributes(LPCTSTR lpszFieldName);

	// �ֶζ��峤�� -------------------------
	long GetFieldDefineSize(int lIndex);
	long GetFieldDefineSize(LPCTSTR lpszFieldName);

	// �ֶ�ʵ�ʳ��� -------------------------
	long GetFieldActualSize(int lIndex);
	long GetFieldActualSize(LPCTSTR lpszFieldName);

	// �ֶ��Ƿ�ΪNULL -----------------------
	BOOL IsFieldNull(LPCTSTR lpFieldName);
	BOOL IsFieldNull(int nIndex);
	BOOL IsFieldEmpty(LPCTSTR lpFieldName);
	BOOL IsFieldEmpty(int nIndex);

	BOOL IsEof()
	{
		return m_pRecordset->ACS_EOF == VARIANT_TRUE;
	};
	BOOL IsEOF()
	{
		return m_pRecordset->ACS_EOF == VARIANT_TRUE;
	};
	BOOL IsBof()
	{
		return m_pRecordset->BOF == VARIANT_TRUE;
	};
	BOOL IsBOF()
	{
		return m_pRecordset->BOF == VARIANT_TRUE;
	};
	void MoveFirst()
	{
		m_pRecordset->MoveFirst();
	};
	void MoveNext()
	{
		m_pRecordset->MoveNext();
	};
	void MovePrevious()
	{
		m_pRecordset->MovePrevious();
	};
	void MoveLast()
	{
		m_pRecordset->MoveLast();
	};
	long GetAbsolutePage()
	{
		return m_pRecordset->GetAbsolutePage();
	};
	void SetAbsolutePage(int nPage)
	{
		m_pRecordset->PutAbsolutePage((enum PositionEnum) nPage);
	};
	long GetPageCount()
	{
		return m_pRecordset->GetPageCount();
	};
	long GetPageSize()
	{
		return m_pRecordset->GetPageSize();
	};
	void SetPageSize(int nSize)
	{
		m_pRecordset->PutPageSize(nSize);
	};
	long GetAbsolutePosition()
	{
		return m_pRecordset->GetAbsolutePosition();
	};
	void SetAbsolutePosition(int nPosition)
	{
		m_pRecordset->PutAbsolutePosition((enum PositionEnum) nPosition);
	};

	BOOL AppendChunk(LPCTSTR lpFieldName, LPVOID lpData, UINT nBytes);
	BOOL AppendChunk(int nIndex, LPVOID lpData, UINT nBytes);
	BOOL AppendChunk(int index, LPCTSTR lpszFileName);
	BOOL AppendChunk(LPCTSTR strFieldName, LPCTSTR lpszFileName);
	BOOL AppendChunk(LPCTSTR lpFieldName, const HBITMAP& hBitmap);
	BOOL AppendChunk(int index, const HBITMAP& hBitmap);

	BOOL GetChunk(LPCTSTR lpFieldName, CString& strValue);
	BOOL GetChunk(int nIndex, CString& strValue);

	BOOL GetChunk(LPCTSTR lpFieldName, LPVOID pData);
	BOOL GetChunk(int nIndex, LPVOID pData);
	BOOL GetChunk(LPCTSTR strFieldName, CBitmap& bitmap);
	BOOL GetChunk(int index, CBitmap& bitmap);

	CString GetString(LPCTSTR lpCols, LPCTSTR lpRows, LPCTSTR lpNull,
		long numRows = 0);
	CString GetLastErrorString()
	{
		return m_strLastError;
	};
	DWORD GetLastError()
	{
		return m_dwLastError;
	};
	void GetBookmark()
	{
		m_varBookmark = m_pRecordset->Bookmark;
	};
	BOOL SetBookmark();
	BOOL Delete();
	BOOL IsConnectionOpen()
	{
		return m_pConnection != NULL &&
			m_pConnection->GetState() != adStateClosed;
	};
	_RecordsetPtr GetRecordset()
	{
		return m_pRecordset;
	};
	_ConnectionPtr GetActiveConnection()
	{
		return m_pConnection;
	};

	BOOL SetFilter(LPCTSTR strFilter);
	BOOL SetSort(LPCTSTR lpstrCriteria);
	BOOL SaveAsXML(LPCTSTR lpstrXMLFile);
	BOOL OpenXML(LPCTSTR lpstrXMLFile);
	BOOL Execute(CADOCommand* pCommand);
	BOOL Requery();

public:
	_RecordsetPtr m_pRecordset;
	_CommandPtr m_pCmd;

protected:
	_ConnectionPtr m_pConnection;
	int m_nSearchDirection;
	CString m_strFind;
	_variant_t m_varBookFind;
	_variant_t m_varBookmark;
	int m_nEditStatus;
	CString m_strLastError;
	DWORD m_dwLastError;
	void dump_com_error(_com_error& e);
	IADORecordBinding* m_pRecBinding;
	CString m_strQuery;

protected:
	BOOL PutFieldValue(LPCTSTR lpFieldName, _variant_t vtFld);
	BOOL PutFieldValue(_variant_t vtIndex, _variant_t vtFld);
	BOOL GetFieldInfo(FieldPtr pField, CADOFieldInfo* fldInfo);
	BOOL GetChunk(FieldPtr pField, CString& strValue);
	BOOL GetChunk(FieldPtr pField, LPVOID lpData);
	BOOL AppendChunk(FieldPtr pField, LPVOID lpData, UINT nBytes);
};

class CADOParameter
{
public:

	enum cadoParameterDirection
	{
		paramUnknown		= adParamUnknown,
		paramInput			= adParamInput,
		paramOutput			= adParamOutput,
		paramInputOutput	= adParamInputOutput,
		paramReturnValue	= adParamReturnValue
	};

	CADOParameter(int nType, long lSize = 0, int nDirection = paramInput,
		CString strName = _T(""));

	~CADOParameter()
	{
		m_pParameter.Release();
		m_pParameter = NULL;
		m_strName = _T("");
	}

	BOOL SetValue(int nValue);
	BOOL SetValue(long lValue);
	BOOL SetValue(double dbValue);
	BOOL SetValue(CString strValue);
	BOOL SetValue(COleDateTime time);
	BOOL SetValue(_variant_t vtValue);
	BOOL GetValue(int& nValue);
	BOOL GetValue(long& lValue);
	BOOL GetValue(double& dbValue);
	BOOL GetValue(CString& strValue, CString strDateFormat = _T(""));
	BOOL GetValue(COleDateTime& time);
	BOOL GetValue(_variant_t& vtValue);
	void SetPrecision(int nPrecision)
	{
		m_pParameter->PutPrecision(nPrecision);
	};
	void SetScale(int nScale)
	{
		m_pParameter->PutNumericScale(nScale);
	};

	void SetName(CString strName)
	{
		m_strName = strName;
	};
	CString GetName()
	{
		return m_strName;
	};
	int GetType()
	{
		return m_nType;
	};
	_ParameterPtr GetParameter()
	{
		return m_pParameter;
	};

protected:
	void dump_com_error(_com_error& e);

protected:
	_ParameterPtr m_pParameter;
	CString m_strName;
	int m_nType;
	CString m_strLastError;
	DWORD m_dwLastError;
};

class CADOCommand
{
public:
	enum cadoCommandType
	{
		typeCmdText				= adCmdText,
		typeCmdTable			= adCmdTable,
		typeCmdTableDirect		= adCmdTableDirect,
		typeCmdStoredProc		= adCmdStoredProc,
		typeCmdUnknown			= adCmdUnknown,
		typeCmdFile				= adCmdFile
	};

	CADOCommand(CADODatabase* pAdoDatabase, CString strCommandText = _T(""),
		int nCommandType = typeCmdStoredProc);

	~CADOCommand()
	{
		m_pCommand.Release();
		m_pCommand = NULL;
		m_strCommandText = _T("");
	}

	void SetTimeout(long nTimeOut)
	{
		m_pCommand->PutCommandTimeout(nTimeOut);
	};
	void SetText(CString strCommandText);
	void SetType(int nCommandType);
	int GetType()
	{
		return m_nCommandType;
	};
	BOOL AddParameter(CADOParameter* pAdoParameter);
	BOOL AddParameter(CString strName, int nType, int nDirection, long lSize,
		int nValue);
	BOOL AddParameter(CString strName, int nType, int nDirection, long lSize,
		long lValue);
	BOOL AddParameter(CString strName, int nType, int nDirection, long lSize,
		double dblValue, int nPrecision = 0, int nScale = 0);
	BOOL AddParameter(CString strName, int nType, int nDirection, long lSize,
		CString strValue);
	BOOL AddParameter(CString strName, int nType, int nDirection, long lSize,
		COleDateTime time);
	BOOL AddParameter(CString strName, int nType, int nDirection, long lSize,
		_variant_t vtValue, int nPrecision = 0, int nScale = 0);
	CString GetText()
	{
		return m_strCommandText;
	};
	BOOL Execute();
	int GetRecordsAffected()
	{
		return m_nRecordsAffected;
	};
	_CommandPtr GetCommand()
	{
		return m_pCommand;
	};

protected:
	void dump_com_error(_com_error& e);

protected:
	_CommandPtr m_pCommand;
	int m_nCommandType;
	int m_nRecordsAffected;
	CString m_strCommandText;
	CString m_strLastError;
	DWORD m_dwLastError;
};

class CADOException : public CException
{
public:

	enum
	{
		noError,	// no error
		Unknown,	// unknown error
	};

	DECLARE_DYNAMIC(CADOException);

	CADOException(int nCause = 0, CString strErrorString = _T(""));
	virtual ~CADOException();

	static int GetError(int nADOError);

public:
	int m_nCause;
	CString m_strErrorString;

protected:
};

void AfxThrowADOException(int nADOError = 1000,
	CString strErrorString = _T(""));

#pragma warning(default: 4251)
#pragma warning(default: 4275)

#endif