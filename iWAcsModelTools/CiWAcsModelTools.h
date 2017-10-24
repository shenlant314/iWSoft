#pragma once
#include "ado.h"
typedef std::map<CString, CString>  MAPStringToString;
typedef std::map<CString, AcDbObjectIdArray> MAPStringToIdArray;

class CiWAcsModelTools
{
public:
	CiWAcsModelTools(void);
	~CiWAcsModelTools(void);

	// ��õ�ǰ�����װ·��
	static CString GetSoftPath();

	// �ж��Ƿ񳬹��Ϸ�ʹ��ʱ��
	static bool Normalize();

	static bool IsExist(LPCTSTR szFileName);

	// �����������ݵ������ļ�
	static bool iwCopyToProvideFile();

	static bool EraseXrefModelAllEntity( AcDbDatabase& pDatab );

	// ������� ���ݿ⣬һ�������������������
	static bool OpenGPSDataBase();
	static CADODatabase g_pGPSDbaseConn;


	static const TCHAR * acadErrorStatusTextChs(Acad::ErrorStatus es);

	static void replace(TCHAR *s,TCHAR *s1,TCHAR *s2);
};
