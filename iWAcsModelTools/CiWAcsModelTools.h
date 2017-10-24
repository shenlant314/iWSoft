#pragma once
#include "ado.h"
typedef std::map<CString, CString>  MAPStringToString;
typedef std::map<CString, AcDbObjectIdArray> MAPStringToIdArray;

class CiWAcsModelTools
{
public:
	CiWAcsModelTools(void);
	~CiWAcsModelTools(void);

	// 获得当前软件安装路径
	static CString GetSoftPath();

	// 判断是否超过合法使用时间
	static bool Normalize();

	static bool IsExist(LPCTSTR szFileName);

	// 复制提资内容到提资文件
	static bool iwCopyToProvideFile();

	static bool EraseXrefModelAllEntity( AcDbDatabase& pDatab );

	// 软件基础 数据库，一般可以在软件启动后加载
	static bool OpenGPSDataBase();
	static CADODatabase g_pGPSDbaseConn;


	static const TCHAR * acadErrorStatusTextChs(Acad::ErrorStatus es);

	static void replace(TCHAR *s,TCHAR *s1,TCHAR *s2);
};
