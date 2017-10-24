#include "StdAfx.h"
#include "CiWAcsModelTools.h"



const int MAX_LENGTH=1024;
extern Acad::ErrorStatus purgeDatabase(AcDbDatabase *pDb); 

CADODatabase CiWAcsModelTools::g_pGPSDbaseConn;

CiWAcsModelTools::CiWAcsModelTools(void)
{
}

CiWAcsModelTools::~CiWAcsModelTools(void)
{
}

CString CiWAcsModelTools::GetSoftPath()
{
	// 获得当前路径
	CString strPath="";
	TCHAR fullPath[MAX_LENGTH];
	GetModuleFileName( _hdllInstance, fullPath, MAX_PATH );
	strPath.Format(_T("%s"), fullPath);
	strPath = strPath.Left(strPath.ReverseFind('\\'));

	CString strModelPath= strPath;
	strModelPath = strModelPath.Left(strModelPath.ReverseFind('\\'));
	return strModelPath;
}


bool CiWAcsModelTools::Normalize()
{
	SYSTEMTIME sys; 
	GetLocalTime( &sys ); 
	for (int i = 2018; i > sys.wYear; --i)
	{
		return false;
	}

	return true;
}
//文件是否存在 
bool CiWAcsModelTools::IsExist(LPCTSTR szFileName) 
{
	static bool bExist ;
	static CString sFileName;
	bExist = false;
	sFileName=szFileName;
	//ASSERT2(!sFileName.IsEmpty(),_T("你真的要用一个空串调用IsExist吗？"));
	if (sFileName.IsEmpty())
		return false;

#ifdef _DEBUG
	CString sPrompt;
	sPrompt.Format(_T("1.在函数IsExist中要求全路径！\n2.您所传的是[%s]！"),sFileName);
	//ASSERT2(szFileName[1]==_T('\\') || szFileName[1]==_T(':'),CString(CString()+sPrompt));
#endif

	WIN32_FIND_DATA filedata;

	if (FindFirstFile(sFileName, &filedata) == INVALID_HANDLE_VALUE)
	{
		//可能是共享名
		sFileName = sFileName + _T("\\*");
		if (FindFirstFile(sFileName, &filedata) == INVALID_HANDLE_VALUE) 
			bExist = false;
		else
			bExist = true;
	}
	else
		bExist = true;

	return bExist;

} 


bool CiWAcsModelTools::OpenGPSDataBase()
{
	CString strFilePath = GetSoftPath();
	strFilePath = strFilePath + _T("\\data\\hwgpsdata.mdb");
	CString alertstr;

	if(!IsExist(strFilePath))
	{		
		AfxMessageBox(_T("\n 数据库文件不存在!"));
		return false;
	}
	//已经打开,则不需要重新打开
	if(g_pGPSDbaseConn.IsOpen())
	{
		return true;
	}
	CString sTmp=_T("Provider=Microsoft.ACE.OLEDB.12.0;Data Source=");
	sTmp = sTmp + strFilePath;

	if(!g_pGPSDbaseConn.Open(sTmp))
	{
		alertstr = _T("连接数据库失败!");

		AfxMessageBox(alertstr);

		return false; 
	}
	return true;
}


bool CiWAcsModelTools::iwCopyToProvideFile()
{
	// 遍历当前空间
	AcDbBlockTableRecordPointer pBTR(acdbCurDwg()->currentSpaceId(), AcDb::kForRead);
	if (Acad::eOk != pBTR.openStatus())
	{
		return false;
	}

	AcDbBlockTableRecordIterator *iterBlkTbl = NULL;
	pBTR->newIterator(iterBlkTbl);
	if (iterBlkTbl == NULL)
	{
		pBTR->close();
		return false;
	}
	// 先读取提资图层、图块-》提资模块映射表
	MAPStringToString layerMapString;
	MAPStringToString blockMapString;
	//CString strPath=GetSoftPath();
	//strPath += _T("\\ProvideMap.ini");
	//CHWPubFun::GetSectionAllKeyAndValue(strPath, _T("PROLAYER"), layerMapString);
	//CHWPubFun::GetSectionAllKeyAndValue(strPath, _T("PROBLOCK"), blockMapString);
	// 获得产品系列
	if (OpenGPSDataBase())
	{
		CADORecordset pRecords = CADORecordset(&g_pGPSDbaseConn) ;
		CString strSQL;
		strSQL.Format(_T("select * from HW_ProvideLayerMap "));
		if(!pRecords.Open(strSQL, CADORecordset::openQuery))
		{
			AfxMessageBox(pRecords.GetLastErrorString());
			return false;
		}
		for (int i = 0; i < pRecords.GetRecordCount(); ++i)
		{
			CString strLayerName;
			pRecords.GetFieldValue(_T("LayerName"), strLayerName);
			CString strFileName;
			pRecords.GetFieldValue(_T("ProvideFileNameA"), strFileName);
			layerMapString.insert(std::make_pair(strLayerName, strFileName));

			pRecords.MoveNext();
		}

		strSQL.Format(_T("select * from HW_ProvideBlockMap "));
		if(!pRecords.Open(strSQL, CADORecordset::openQuery))
		{
			AfxMessageBox(pRecords.GetLastErrorString());
			return false;
		}
		for (int i = 0; i < pRecords.GetRecordCount(); ++i)
		{
			CString strBlockName;
			pRecords.GetFieldValue(_T("BlockName"), strBlockName);
			CString strFileName;
			pRecords.GetFieldValue(_T("ProvideFileNameA"), strFileName);
			layerMapString.insert(std::make_pair(strBlockName, strFileName));

			pRecords.MoveNext();
		}

		pRecords.Close();
	}


	// 提资图层、图块的对象ObjectId与提资文件的映射表
	MAPStringToIdArray provideObjIdAryMap;

	for ( ; !iterBlkTbl->done(); iterBlkTbl->step())
	{
		AcDbEntity* pEnt = NULL;
		iterBlkTbl->getEntity(pEnt, AcDb::kForRead);
		if (pEnt != NULL )
		{
			// 先判断是否提资图层
			CString strLayerName = pEnt->layer();
			MAPStringToString::iterator itMapLay = layerMapString.find(strLayerName);
			if (itMapLay != layerMapString.end())
			{
				CString strProvideFileName = itMapLay->second;
				MAPStringToIdArray::iterator itMapLayIdAry = provideObjIdAryMap.find(strProvideFileName);
				if (itMapLayIdAry!= provideObjIdAryMap.end())
				{
					// 文件已经有映射，直接添加就可以
					itMapLayIdAry->second.append(pEnt->objectId());
					//acutPrintf(_T("\n %s - %s "), strLayerName, strProvideFileName);
				}
				else
				{
					// 没有映射，创建一份
					AcDbObjectIdArray tmpIdAry;
					tmpIdAry.append(pEnt->objectId());
					provideObjIdAryMap.insert(std::make_pair(strProvideFileName, tmpIdAry));

					//acutPrintf(_T("\n %s - %s "), strLayerName, strProvideFileName);
				}
			}
			if (pEnt->isKindOf(AcDbBlockReference::desc())) // 如果不是提资图层，那么如果对象是个块，还得判断一下是否提资图块
			{
				// 判断是否提资图块
				AcDbBlockReference* pBlkRef = static_cast<AcDbBlockReference*>(pEnt);
				if (pBlkRef != NULL)
				{
					AcDbObjectId blkRcdId = pBlkRef->blockTableRecord();
					AcDbObjectPointer<AcDbBlockTableRecord> pBlkRcd(blkRcdId, AcDb::kForRead);
					if (pBlkRcd.openStatus() == Acad::eOk)
					{
						// 判断图块名是否是提资图块
						TCHAR* psName = NULL;
						pBlkRcd->getName(psName);
						CString strBlkName(psName);
						MAPStringToString::iterator itMapBlk = blockMapString.find(strBlkName);
						if (itMapBlk != blockMapString.end())
						{
							// 是提资图块
							CString strProvideFileName = itMapBlk->second;
							MAPStringToIdArray::iterator itMapBlkIdAry = provideObjIdAryMap.find(strProvideFileName);
							if (itMapBlkIdAry!= provideObjIdAryMap.end())
							{
								// 文件已经有映射，直接添加就可以
								itMapBlkIdAry->second.append(pEnt->objectId());
							}
							else
							{
								// 没有映射，创建一份
								AcDbObjectIdArray tmpIdAry;
								tmpIdAry.append(pEnt->objectId());
								provideObjIdAryMap.insert(std::make_pair(strProvideFileName, tmpIdAry));
							}
						}
					}
				}
			}
			pEnt->close();
		}
	}
	pBTR->close();
	delete iterBlkTbl;

	// 当前文件路径
	CString strCurFilePath =acDocManager->curDocument()->fileName();
	strCurFilePath = strCurFilePath.Left(strCurFilePath.ReverseFind('\\') + 1 );

	// 写入外部参照文件
	MAPStringToIdArray::iterator itMapLayIdAry = provideObjIdAryMap.begin();
	for ( ; itMapLayIdAry!= provideObjIdAryMap.end(); ++itMapLayIdAry)
	{
		// 先写到临时数据库
		AcDbDatabase* pTmpDb = NULL;
		Acad::ErrorStatus res = acdbCurDwg()->wblock(pTmpDb, itMapLayIdAry->second, AcGePoint3d::kOrigin);
		if (pTmpDb == NULL)
		{
			acutPrintf(_T("\n %s"), acadErrorStatusTextChs(res));
			continue;
		}

		// 对应的外部参照路径
		CString strFile = strCurFilePath + itMapLayIdAry->first + _T(".dwg");
		acutPrintf(_T("\n %s"), strFile);
		AcDbDatabase pExternDb(Adesk::kFalse);
		res = pExternDb.readDwgFile( strFile,_SH_DENYRW);
		if (Acad::eOk != res)
		{
			acutPrintf(_T("\n %s"), acadErrorStatusTextChs(res));
			continue;;
		}
		// 先复制一份
		AcDbDatabase* pOldDb = new AcDbDatabase;
		pExternDb.wblock(pOldDb);
		//删除外部参照模型中的原有实体
		EraseXrefModelAllEntity(pExternDb);// 删除提资模块原有的所有实体
		purgeDatabase(&pExternDb);
		//pExternDb.saveAs(iterMap->first);
		// 再写入外部参照
		res = pExternDb.insert(AcGeMatrix3d::kIdentity, pTmpDb, false);
		delete pTmpDb;
		if (res == Acad::eOk) // 写入成功才保存
		{
			pExternDb.saveAs(strFile);
			acutPrintf(_T("\n 保存提资文件 - %s "), strFile);
			pOldDb->saveAs(_T("D:\\111222.dwg"));
			//delete pOldDb;
		}
		else // 不成功，保存原有的
		{
			pOldDb->saveAs(strFile);
		}
	}

	return true;	
}

bool CiWAcsModelTools::EraseXrefModelAllEntity( AcDbDatabase& pDatab )
{
	AcDbBlockTable* pOldBlkTbl;
	pDatab.getBlockTable(pOldBlkTbl, AcDb::kForWrite);
	if (pOldBlkTbl == NULL)
	{
		return false;
	}
	AcDbBlockTableRecord *pModelSpaceRcd;
	pOldBlkTbl->getAt(ACDB_MODEL_SPACE, pModelSpaceRcd,AcDb::kForWrite);
	if (pModelSpaceRcd == NULL)
	{
		pOldBlkTbl->close();
	}

	AcDbBlockTableRecordIterator * pIterBlkRcd = NULL;
	pModelSpaceRcd->newIterator(pIterBlkRcd);
	if (pIterBlkRcd == NULL)
	{
		pModelSpaceRcd->close();
		pOldBlkTbl->close();
		return false;
	}

	for ( ; !pIterBlkRcd->done(); pIterBlkRcd->step())
	{
		AcDbEntity* pEnt = NULL;
		pIterBlkRcd->getEntity(pEnt, AcDb::kForWrite);
		if (pEnt != NULL )
		{
			pEnt->erase();
		}
		pEnt->close();
	}
	delete pIterBlkRcd;
	pModelSpaceRcd->close();
	pOldBlkTbl->close();
	return true;
}


TCHAR Glb_AcadErrorInfo[2048+1]={0};
//中文版输出错误
const TCHAR * CiWAcsModelTools::acadErrorStatusTextChs(Acad::ErrorStatus es)
{
	switch(es)
	{
	case Acad::eOk:lstrcpy(Glb_AcadErrorInfo,_T("正确"));break;
	case Acad::eNotImplementedYet:lstrcpy(Glb_AcadErrorInfo,_T("尚未实现"));break;
	case Acad::eNotApplicable:lstrcpy(Glb_AcadErrorInfo,_T("不合适的"));break;
	case Acad::eInvalidInput:lstrcpy(Glb_AcadErrorInfo,_T("无效的输入"));break;
	case Acad::eAmbiguousInput:lstrcpy(Glb_AcadErrorInfo,_T("模糊不清的输入"));break;
	case Acad::eAmbiguousOutput:lstrcpy(Glb_AcadErrorInfo,_T("模糊不清的输出"));break;
	case Acad::eOutOfMemory:lstrcpy(Glb_AcadErrorInfo,_T("内存不足"));break;
	case Acad::eBufferTooSmall:lstrcpy(Glb_AcadErrorInfo,_T("缓冲区太小"));break;
	case Acad::eInvalidOpenState:lstrcpy(Glb_AcadErrorInfo,_T("无效的打开状态"));break;
	case Acad::eEntityInInactiveLayout:lstrcpy(Glb_AcadErrorInfo,_T("实体不在活动布局上"));break;
	case Acad::eHandleExists:lstrcpy(Glb_AcadErrorInfo,_T("句柄已存在"));break;
	case Acad::eNullHandle:lstrcpy(Glb_AcadErrorInfo,_T("空句柄"));break;
	case Acad::eBrokenHandle:lstrcpy(Glb_AcadErrorInfo,_T("损坏的句柄"));break;
	case Acad::eUnknownHandle:lstrcpy(Glb_AcadErrorInfo,_T("未知句柄"));break;
	case Acad::eHandleInUse:lstrcpy(Glb_AcadErrorInfo,_T("句柄被占用"));break;
	case Acad::eNullObjectPointer:lstrcpy(Glb_AcadErrorInfo,_T("对象指针为空"));break;
	case Acad::eNullObjectId:lstrcpy(Glb_AcadErrorInfo,_T("对象ID为空"));break;
	case Acad::eNullBlockName:lstrcpy(Glb_AcadErrorInfo,_T("块名称为空"));break;
	case Acad::eContainerNotEmpty:lstrcpy(Glb_AcadErrorInfo,_T("容器不为空"));break;
	case Acad::eNullEntityPointer:lstrcpy(Glb_AcadErrorInfo,_T("实体指针为空"));break;
	case Acad::eIllegalEntityType:lstrcpy(Glb_AcadErrorInfo,_T("非法的实体类型"));break;
	case Acad::eKeyNotFound:lstrcpy(Glb_AcadErrorInfo,_T("关键字未找到"));break;
	case Acad::eDuplicateKey:lstrcpy(Glb_AcadErrorInfo,_T("重复的关键字"));break;
	case Acad::eInvalidIndex:lstrcpy(Glb_AcadErrorInfo,_T("无效的索引"));break;
	case Acad::eDuplicateIndex:lstrcpy(Glb_AcadErrorInfo,_T("重复的索引"));break;
	case Acad::eAlreadyInDb:lstrcpy(Glb_AcadErrorInfo,_T("已经在数据库中了"));break;
	case Acad::eOutOfDisk:lstrcpy(Glb_AcadErrorInfo,_T("硬盘容量不足"));break;
	case Acad::eDeletedEntry:lstrcpy(Glb_AcadErrorInfo,_T("已经删除的函数入口"));break;
	case Acad::eNegativeValueNotAllowed:lstrcpy(Glb_AcadErrorInfo,_T("不允许输入负数"));break;
	case Acad::eInvalidExtents:lstrcpy(Glb_AcadErrorInfo,_T("无效的空间范围"));break;
	case Acad::eInvalidAdsName:lstrcpy(Glb_AcadErrorInfo,_T("无效的ADS名称"));break;
	case Acad::eInvalidSymbolTableName:lstrcpy(Glb_AcadErrorInfo,_T("无效的符号名称"));break;
	case Acad::eInvalidKey:lstrcpy(Glb_AcadErrorInfo,_T("无效的关键字"));break;
	case Acad::eWrongObjectType:lstrcpy(Glb_AcadErrorInfo,_T("错误的类型"));break;
	case Acad::eWrongDatabase:lstrcpy(Glb_AcadErrorInfo,_T("错误的数据库"));break;
	case Acad::eObjectToBeDeleted:lstrcpy(Glb_AcadErrorInfo,_T("对象即将被删除"));break;
	case Acad::eInvalidDwgVersion:lstrcpy(Glb_AcadErrorInfo,_T("不合理的DWG版本"));break;
	case Acad::eAnonymousEntry:lstrcpy(Glb_AcadErrorInfo,_T("多重入口"));break;
	case Acad::eIllegalReplacement:lstrcpy(Glb_AcadErrorInfo,_T("非法的替代者"));break;
	case Acad::eEndOfObject:lstrcpy(Glb_AcadErrorInfo,_T("对象结束"));break;
	case Acad::eEndOfFile:lstrcpy(Glb_AcadErrorInfo,_T("文件结束"));break;
	case Acad::eIsReading:lstrcpy(Glb_AcadErrorInfo,_T("正在读取"));break;
	case Acad::eIsWriting:lstrcpy(Glb_AcadErrorInfo,_T("正在写入"));break;
	case Acad::eNotOpenForRead:lstrcpy(Glb_AcadErrorInfo,_T("不是只读打开"));break;
	case Acad::eNotOpenForWrite:lstrcpy(Glb_AcadErrorInfo,_T("不是可写打开"));break;
	case Acad::eNotThatKindOfClass:lstrcpy(Glb_AcadErrorInfo,_T("类型不匹配"));break;
	case Acad::eInvalidBlockName:lstrcpy(Glb_AcadErrorInfo,_T("不合理的块名称"));break;
	case Acad::eMissingDxfField:lstrcpy(Glb_AcadErrorInfo,_T("DXF字段缺失"));break;
	case Acad::eDuplicateDxfField:lstrcpy(Glb_AcadErrorInfo,_T("DXF字段重复"));break;
	case Acad::eInvalidDxfCode:lstrcpy(Glb_AcadErrorInfo,_T("不合理的DXF编码"));break;



		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
		//作者:boitboy,网站:http://www.cadgj.com (CAD工具之家),大量实用ObjectArx开发教程,代码,CAD小工具
		//代码可任意转载,使用,修改,再发布,没有任何限制.
		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
	case Acad::eInvalidResBuf:lstrcpy(Glb_AcadErrorInfo,_T("不合理的ResBuf"));break;
	case Acad::eBadDxfSequence:lstrcpy(Glb_AcadErrorInfo,_T("不正确的DXF顺序"));break;
	case Acad::eFilerError:lstrcpy(Glb_AcadErrorInfo,_T("文件错误"));break;
	case Acad::eVertexAfterFace:lstrcpy(Glb_AcadErrorInfo,_T("顶点在面后面"));break;
	case Acad::eInvalidFaceVertexIndex:lstrcpy(Glb_AcadErrorInfo,_T("不合理的面顶点顺序"));break;
	case Acad::eInvalidMeshVertexIndex:lstrcpy(Glb_AcadErrorInfo,_T("不合理的mesh顺序"));break;
	case Acad::eOtherObjectsBusy:lstrcpy(Glb_AcadErrorInfo,_T("其它对象忙"));break;
	case Acad::eMustFirstAddBlockToDb:lstrcpy(Glb_AcadErrorInfo,_T("必须先把块加入到数据库"));break;
	case Acad::eCannotNestBlockDefs:lstrcpy(Glb_AcadErrorInfo,_T("不可以嵌套块定义"));break;
	case Acad::eDwgRecoveredOK:lstrcpy(Glb_AcadErrorInfo,_T("修复DWG完成"));break;
	case Acad::eDwgNotRecoverable:lstrcpy(Glb_AcadErrorInfo,_T("无法修复DWG"));break;
	case Acad::eDxfPartiallyRead:lstrcpy(Glb_AcadErrorInfo,_T("DXF部分读取"));break;
	case Acad::eDxfReadAborted:lstrcpy(Glb_AcadErrorInfo,_T("读取DXF终止"));break;
	case Acad::eDxbPartiallyRead:lstrcpy(Glb_AcadErrorInfo,_T("DXB部分读取"));break;
	case Acad::eDwgCRCDoesNotMatch:lstrcpy(Glb_AcadErrorInfo,_T("DWG文件的CRC不匹配"));break;
	case Acad::eDwgSentinelDoesNotMatch:lstrcpy(Glb_AcadErrorInfo,_T("DWG文件的校验不匹配"));break;
	case Acad::eDwgObjectImproperlyRead:lstrcpy(Glb_AcadErrorInfo,_T("DWG文件错误读取"));break;
	case Acad::eNoInputFiler:lstrcpy(Glb_AcadErrorInfo,_T("没有找到输入过滤"));break;
	case Acad::eDwgNeedsAFullSave:lstrcpy(Glb_AcadErrorInfo,_T("DWG需要完全保存"));break;
	case Acad::eDxbReadAborted:lstrcpy(Glb_AcadErrorInfo,_T("DXB读取终止"));break;
	case Acad::eFileLockedByACAD:lstrcpy(Glb_AcadErrorInfo,_T("文件被ACAD锁定"));break;
	case Acad::eFileAccessErr:lstrcpy(Glb_AcadErrorInfo,_T("无法读取文件"));break;
	case Acad::eFileSystemErr:lstrcpy(Glb_AcadErrorInfo,_T("文件系统错误"));break;
	case Acad::eFileInternalErr:lstrcpy(Glb_AcadErrorInfo,_T("文件内部错误"));break;
	case Acad::eFileTooManyOpen:lstrcpy(Glb_AcadErrorInfo,_T("文件被打开太多次"));break;
	case Acad::eFileNotFound:lstrcpy(Glb_AcadErrorInfo,_T("未找到文件"));break;
	case Acad::eDwkLockFileFound:lstrcpy(Glb_AcadErrorInfo,_T("找到DWG锁定文件"));break;
	case Acad::eWasErased:lstrcpy(Glb_AcadErrorInfo,_T("对象被删除"));break;
	case Acad::ePermanentlyErased:lstrcpy(Glb_AcadErrorInfo,_T("对象被永久删除"));break;
	case Acad::eWasOpenForRead:lstrcpy(Glb_AcadErrorInfo,_T("对象只读打开"));break;
	case Acad::eWasOpenForWrite:lstrcpy(Glb_AcadErrorInfo,_T("对象可写打开"));break;
	case Acad::eWasOpenForUndo:lstrcpy(Glb_AcadErrorInfo,_T("对象撤销打开"));break;
	case Acad::eWasNotifying:lstrcpy(Glb_AcadErrorInfo,_T("对象被通知"));break;
	case Acad::eWasOpenForNotify:lstrcpy(Glb_AcadErrorInfo,_T("对象通知打开"));break;
	case Acad::eOnLockedLayer:lstrcpy(Glb_AcadErrorInfo,_T("对象在锁定图层上"));break;
	case Acad::eMustOpenThruOwner:lstrcpy(Glb_AcadErrorInfo,_T("必须经过所有者打开"));break;
	case Acad::eSubentitiesStillOpen:lstrcpy(Glb_AcadErrorInfo,_T("子对象依然打开着"));break;
	case Acad::eAtMaxReaders:lstrcpy(Glb_AcadErrorInfo,_T("超过最大打开次数"));break;
	case Acad::eIsWriteProtected:lstrcpy(Glb_AcadErrorInfo,_T("对象被写保护"));break;
	case Acad::eIsXRefObject:lstrcpy(Glb_AcadErrorInfo,_T("对象是XRef"));break;
	case Acad::eNotAnEntity:lstrcpy(Glb_AcadErrorInfo,_T("对象不是实体"));break;
	case Acad::eHadMultipleReaders:lstrcpy(Glb_AcadErrorInfo,_T("被多重打开"));break;
	case Acad::eDuplicateRecordName:lstrcpy(Glb_AcadErrorInfo,_T("重复的记录名称"));break;
	case Acad::eXRefDependent:lstrcpy(Glb_AcadErrorInfo,_T("依赖于XREF"));break;
	case Acad::eSelfReference:lstrcpy(Glb_AcadErrorInfo,_T("引用自身"));break;
	case Acad::eMissingSymbolTable:lstrcpy(Glb_AcadErrorInfo,_T("丢失符号化表"));break;
	case Acad::eMissingSymbolTableRec:lstrcpy(Glb_AcadErrorInfo,_T("丢失符号化记录"));break;
	case Acad::eWasNotOpenForWrite:lstrcpy(Glb_AcadErrorInfo,_T("不是可写打开"));break;
	case Acad::eCloseWasNotifying:lstrcpy(Glb_AcadErrorInfo,_T("对象关闭,正在执行通知"));break;
	case Acad::eCloseModifyAborted:lstrcpy(Glb_AcadErrorInfo,_T("对象关闭,修改被取消"));break;



		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
		//作者:boitboy,网站:http://www.cadgj.com (CAD工具之家),大量实用ObjectArx开发教程,代码,CAD小工具
		//代码可任意转载,使用,修改,再发布,没有任何限制.
		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
	case Acad::eClosePartialFailure:lstrcpy(Glb_AcadErrorInfo,_T("对象关闭,部分操作未成功"));break;
	case Acad::eCloseFailObjectDamaged:lstrcpy(Glb_AcadErrorInfo,_T("对象被损坏,关闭失败"));break;
	case Acad::eCannotBeErasedByCaller:lstrcpy(Glb_AcadErrorInfo,_T("对象不可以被当前呼叫者删除"));break;
	case Acad::eCannotBeResurrected:lstrcpy(Glb_AcadErrorInfo,_T("不可以复活"));break;
	case Acad::eWasNotErased:lstrcpy(Glb_AcadErrorInfo,_T("对象未删除"));break;
	case Acad::eInsertAfter:lstrcpy(Glb_AcadErrorInfo,_T("在后面插入"));break;
	case Acad::eFixedAllErrors:lstrcpy(Glb_AcadErrorInfo,_T("修复了所有错误"));break;
	case Acad::eLeftErrorsUnfixed:lstrcpy(Glb_AcadErrorInfo,_T("剩下一些错误未修复"));break;
	case Acad::eUnrecoverableErrors:lstrcpy(Glb_AcadErrorInfo,_T("不可恢复的错误"));break;
	case Acad::eNoDatabase:lstrcpy(Glb_AcadErrorInfo,_T("没有数据库"));break;
	case Acad::eXdataSizeExceeded:lstrcpy(Glb_AcadErrorInfo,_T("扩展数据长度太大"));break;
	case Acad::eRegappIdNotFound:lstrcpy(Glb_AcadErrorInfo,_T("没有找到扩展数据注册ID"));break;
	case Acad::eRepeatEntity:lstrcpy(Glb_AcadErrorInfo,_T("重复实体"));break;
	case Acad::eRecordNotInTable:lstrcpy(Glb_AcadErrorInfo,_T("表中未找到记录"));break;
	case Acad::eIteratorDone:lstrcpy(Glb_AcadErrorInfo,_T("迭代器完成"));break;
	case Acad::eNullIterator:lstrcpy(Glb_AcadErrorInfo,_T("空的迭代器"));break;
	case Acad::eNotInBlock:lstrcpy(Glb_AcadErrorInfo,_T("不在块中"));break;
	case Acad::eOwnerNotInDatabase:lstrcpy(Glb_AcadErrorInfo,_T("所有者不在数据库中"));break;
	case Acad::eOwnerNotOpenForRead:lstrcpy(Glb_AcadErrorInfo,_T("所有者不是只读打开"));break;
	case Acad::eOwnerNotOpenForWrite:lstrcpy(Glb_AcadErrorInfo,_T("所有者不是可写打开"));break;
	case Acad::eExplodeBeforeTransform:lstrcpy(Glb_AcadErrorInfo,_T("在变换之前就被炸开了"));break;
	case Acad::eCannotScaleNonUniformly:lstrcpy(Glb_AcadErrorInfo,_T("不可以不同比例缩放"));break;
	case Acad::eNotInDatabase:lstrcpy(Glb_AcadErrorInfo,_T("不在数据库中"));break;
	case Acad::eNotCurrentDatabase:lstrcpy(Glb_AcadErrorInfo,_T("不是当前数据库"));break;
	case Acad::eIsAnEntity:lstrcpy(Glb_AcadErrorInfo,_T("是一个实体"));break;
	case Acad::eCannotChangeActiveViewport:lstrcpy(Glb_AcadErrorInfo,_T("不可以改变活动视口"));break;
	case Acad::eNotInPaperspace:lstrcpy(Glb_AcadErrorInfo,_T("不在图纸空间中"));break;
	case Acad::eCommandWasInProgress:lstrcpy(Glb_AcadErrorInfo,_T("正在执行命令"));break;
	case Acad::eGeneralModelingFailure:lstrcpy(Glb_AcadErrorInfo,_T("创建模型失败"));break;
	case Acad::eOutOfRange:lstrcpy(Glb_AcadErrorInfo,_T("超出范围"));break;
	case Acad::eNonCoplanarGeometry:lstrcpy(Glb_AcadErrorInfo,_T("没有平面几何对象"));break;
	case Acad::eDegenerateGeometry:lstrcpy(Glb_AcadErrorInfo,_T("退化的几何对象"));break;
	case Acad::eInvalidAxis:lstrcpy(Glb_AcadErrorInfo,_T("无效的轴线"));break;
	case Acad::ePointNotOnEntity:lstrcpy(Glb_AcadErrorInfo,_T("点不在实体上"));break;
	case Acad::eSingularPoint:lstrcpy(Glb_AcadErrorInfo,_T("单一的点"));break;
	case Acad::eInvalidOffset:lstrcpy(Glb_AcadErrorInfo,_T("无效的偏移"));break;
	case Acad::eNonPlanarEntity:lstrcpy(Glb_AcadErrorInfo,_T("没有平面的实体"));break;
	case Acad::eCannotExplodeEntity:lstrcpy(Glb_AcadErrorInfo,_T("不可分解的实体"));break;
	case Acad::eStringTooLong:lstrcpy(Glb_AcadErrorInfo,_T("字符串太短"));break;
	case Acad::eInvalidSymTableFlag:lstrcpy(Glb_AcadErrorInfo,_T("无效的符号化表标志"));break;
	case Acad::eUndefinedLineType:lstrcpy(Glb_AcadErrorInfo,_T("没有定义的线型"));break;
	case Acad::eInvalidTextStyle:lstrcpy(Glb_AcadErrorInfo,_T("无效的字体样式"));break;
	case Acad::eTooFewLineTypeElements:lstrcpy(Glb_AcadErrorInfo,_T("太少的线型要素"));break;
	case Acad::eTooManyLineTypeElements:lstrcpy(Glb_AcadErrorInfo,_T("太多的线型要素"));break;
	case Acad::eExcessiveItemCount:lstrcpy(Glb_AcadErrorInfo,_T("过多的项目"));break;
	case Acad::eIgnoredLinetypeRedef:lstrcpy(Glb_AcadErrorInfo,_T("忽略线型定义描述"));break;
	case Acad::eBadUCS:lstrcpy(Glb_AcadErrorInfo,_T("不好的用户坐标系"));break;
	case Acad::eBadPaperspaceView:lstrcpy(Glb_AcadErrorInfo,_T("不好的图纸空间视图"));break;
	case Acad::eSomeInputDataLeftUnread:lstrcpy(Glb_AcadErrorInfo,_T("一些输入数据未被读取"));break;
	case Acad::eNoInternalSpace:lstrcpy(Glb_AcadErrorInfo,_T("不是内部空间"));break;



		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
		//作者:boitboy,网站:http://www.cadgj.com (CAD工具之家),大量实用ObjectArx开发教程,代码,CAD小工具
		//代码可任意转载,使用,修改,再发布,没有任何限制.
		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
	case Acad::eInvalidDimStyle:lstrcpy(Glb_AcadErrorInfo,_T("无效的标注样式"));break;
	case Acad::eInvalidLayer:lstrcpy(Glb_AcadErrorInfo,_T("无效的图层"));break;
	case Acad::eUserBreak:lstrcpy(Glb_AcadErrorInfo,_T("用户打断"));break;
	case Acad::eDwgNeedsRecovery:lstrcpy(Glb_AcadErrorInfo,_T("DWG文件需要修复"));break;
	case Acad::eDeleteEntity:lstrcpy(Glb_AcadErrorInfo,_T("删除实体"));break;
	case Acad::eInvalidFix:lstrcpy(Glb_AcadErrorInfo,_T("无效的方位"));break;
	case Acad::eFSMError:lstrcpy(Glb_AcadErrorInfo,_T("FSM错误"));break;
	case Acad::eBadLayerName:lstrcpy(Glb_AcadErrorInfo,_T("不好的图层名称"));break;
	case Acad::eLayerGroupCodeMissing:lstrcpy(Glb_AcadErrorInfo,_T("图层分组编码丢失"));break;
	case Acad::eBadColorIndex:lstrcpy(Glb_AcadErrorInfo,_T("不好的颜色索引号"));break;
	case Acad::eBadLinetypeName:lstrcpy(Glb_AcadErrorInfo,_T("不好的线型名称"));break;
	case Acad::eBadLinetypeScale:lstrcpy(Glb_AcadErrorInfo,_T("不好的线型缩放比例"));break;
	case Acad::eBadVisibilityValue:lstrcpy(Glb_AcadErrorInfo,_T("不好的可见性值"));break;
	case Acad::eProperClassSeparatorExpected:lstrcpy(Glb_AcadErrorInfo,_T("本身类未找到预期的分割符号(?)"));break;
	case Acad::eBadLineWeightValue:lstrcpy(Glb_AcadErrorInfo,_T("不好的线宽值"));break;
	case Acad::eBadColor:lstrcpy(Glb_AcadErrorInfo,_T("不好的颜色"));break;
	case Acad::ePagerError:lstrcpy(Glb_AcadErrorInfo,_T("页面错误"));break;
	case Acad::eOutOfPagerMemory:lstrcpy(Glb_AcadErrorInfo,_T("页面内存不足"));break;
	case Acad::ePagerWriteError:lstrcpy(Glb_AcadErrorInfo,_T("页面不可写"));break;
	case Acad::eWasNotForwarding:lstrcpy(Glb_AcadErrorInfo,_T("不是促进(?)"));break;
	case Acad::eInvalidIdMap:lstrcpy(Glb_AcadErrorInfo,_T("无效的ID字典"));break;
	case Acad::eInvalidOwnerObject:lstrcpy(Glb_AcadErrorInfo,_T("无效的所有者"));break;
	case Acad::eOwnerNotSet:lstrcpy(Glb_AcadErrorInfo,_T("未设置所有者"));break;
	case Acad::eWrongSubentityType:lstrcpy(Glb_AcadErrorInfo,_T("错误的子对象类型"));break;
	case Acad::eTooManyVertices:lstrcpy(Glb_AcadErrorInfo,_T("太多节点"));break;
	case Acad::eTooFewVertices:lstrcpy(Glb_AcadErrorInfo,_T("太少节点"));break;
	case Acad::eNoActiveTransactions:lstrcpy(Glb_AcadErrorInfo,_T("不活动的事务"));break;
	case Acad::eNotTopTransaction:lstrcpy(Glb_AcadErrorInfo,_T("不是最顶层的事务"));break;
	case Acad::eTransactionOpenWhileCommandEnded:lstrcpy(Glb_AcadErrorInfo,_T("在命令结束的时候打开(/开始)事务"));break;
	case Acad::eInProcessOfCommitting:lstrcpy(Glb_AcadErrorInfo,_T("在提交事务的过程中"));break;
	case Acad::eNotNewlyCreated:lstrcpy(Glb_AcadErrorInfo,_T("不是新创建的"));break;
	case Acad::eLongTransReferenceError:lstrcpy(Glb_AcadErrorInfo,_T("长事务引用错误"));break;
	case Acad::eNoWorkSet:lstrcpy(Glb_AcadErrorInfo,_T("没有工作集"));break;
	case Acad::eAlreadyInGroup:lstrcpy(Glb_AcadErrorInfo,_T("已经在组中了"));break;
	case Acad::eNotInGroup:lstrcpy(Glb_AcadErrorInfo,_T("不在组中"));break;
	case Acad::eInvalidREFIID:lstrcpy(Glb_AcadErrorInfo,_T("无效的REFIID"));break;
	case Acad::eInvalidNormal:lstrcpy(Glb_AcadErrorInfo,_T("无效的标准"));break;
	case Acad::eInvalidStyle:lstrcpy(Glb_AcadErrorInfo,_T("无效的样式"));break;
	case Acad::eCannotRestoreFromAcisFile:lstrcpy(Glb_AcadErrorInfo,_T("不可以从Acis(?)文件中恢复"));break;
	case Acad::eMakeMeProxy:lstrcpy(Glb_AcadErrorInfo,_T("自我代理"));break;
	case Acad::eNLSFileNotAvailable:lstrcpy(Glb_AcadErrorInfo,_T("无效的NLS文件"));break;
	case Acad::eNotAllowedForThisProxy:lstrcpy(Glb_AcadErrorInfo,_T("不允许这个代理"));break;
	case Acad::eNotSupportedInDwgApi:lstrcpy(Glb_AcadErrorInfo,_T("在Dwg Api中不支持"));break;
	case Acad::ePolyWidthLost:lstrcpy(Glb_AcadErrorInfo,_T("多段线宽度丢失"));break;
	case Acad::eNullExtents:lstrcpy(Glb_AcadErrorInfo,_T("空的空间范围"));break;
		//case Acad::eExplodeAgain:lstrcpy(Glb_AcadErrorInfo,_T("再一次分解"));break;
	case Acad::eBadDwgHeader:lstrcpy(Glb_AcadErrorInfo,_T("坏的DWG文件头"));break;
	case Acad::eLockViolation:lstrcpy(Glb_AcadErrorInfo,_T("锁定妨碍当前操作"));break;
	case Acad::eLockConflict:lstrcpy(Glb_AcadErrorInfo,_T("锁定冲突"));break;
	case Acad::eDatabaseObjectsOpen:lstrcpy(Glb_AcadErrorInfo,_T("数据库对象打开"));break;



		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
		//作者:boitboy,网站:http://www.cadgj.com (CAD工具之家),大量实用ObjectArx开发教程,代码,CAD小工具
		//代码可任意转载,使用,修改,再发布,没有任何限制.
		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
	case Acad::eLockChangeInProgress:lstrcpy(Glb_AcadErrorInfo,_T("锁定改变中"));break;
	case Acad::eVetoed:lstrcpy(Glb_AcadErrorInfo,_T("禁止"));break;
	case Acad::eNoDocument:lstrcpy(Glb_AcadErrorInfo,_T("没有文档"));break;
	case Acad::eNotFromThisDocument:lstrcpy(Glb_AcadErrorInfo,_T("不是从这个文档"));break;
	case Acad::eLISPActive:lstrcpy(Glb_AcadErrorInfo,_T("LISP活动"));break;
	case Acad::eTargetDocNotQuiescent:lstrcpy(Glb_AcadErrorInfo,_T("目标文档活动中"));break;
	case Acad::eDocumentSwitchDisabled:lstrcpy(Glb_AcadErrorInfo,_T("禁止文档转换"));break;
	case Acad::eInvalidContext:lstrcpy(Glb_AcadErrorInfo,_T("无效的上下文环境"));break;
	case Acad::eCreateFailed:lstrcpy(Glb_AcadErrorInfo,_T("创建失败"));break;
	case Acad::eCreateInvalidName:lstrcpy(Glb_AcadErrorInfo,_T("创建无效名称"));break;
	case Acad::eSetFailed:lstrcpy(Glb_AcadErrorInfo,_T("设置失败"));break;
	case Acad::eDelDoesNotExist:lstrcpy(Glb_AcadErrorInfo,_T("删除对象不存在"));break;
	case Acad::eDelIsModelSpace:lstrcpy(Glb_AcadErrorInfo,_T("删除模型空间"));break;
	case Acad::eDelLastLayout:lstrcpy(Glb_AcadErrorInfo,_T("删除最后一个布局"));break;
	case Acad::eDelUnableToSetCurrent:lstrcpy(Glb_AcadErrorInfo,_T("删除后无法设置当前对象"));break;
	case Acad::eDelUnableToFind:lstrcpy(Glb_AcadErrorInfo,_T("没有找到删除对象"));break;
	case Acad::eRenameDoesNotExist:lstrcpy(Glb_AcadErrorInfo,_T("重命名对象不存在"));break;
	case Acad::eRenameIsModelSpace:lstrcpy(Glb_AcadErrorInfo,_T("不可以重命令模型空间"));break;
	case Acad::eRenameInvalidLayoutName:lstrcpy(Glb_AcadErrorInfo,_T("重命名无效的布局名称"));break;
	case Acad::eRenameLayoutAlreadyExists:lstrcpy(Glb_AcadErrorInfo,_T("重命名布局名称已存在"));break;
	case Acad::eRenameInvalidName:lstrcpy(Glb_AcadErrorInfo,_T("重命名无效名称"));break;
	case Acad::eCopyDoesNotExist:lstrcpy(Glb_AcadErrorInfo,_T("拷贝不存在"));break;
	case Acad::eCopyIsModelSpace:lstrcpy(Glb_AcadErrorInfo,_T("拷贝是模型空间"));break;
	case Acad::eCopyFailed:lstrcpy(Glb_AcadErrorInfo,_T("拷贝失败"));break;
	case Acad::eCopyInvalidName:lstrcpy(Glb_AcadErrorInfo,_T("拷贝无效名称"));break;
	case Acad::eCopyNameExists:lstrcpy(Glb_AcadErrorInfo,_T("拷贝名称存在"));break;
	case Acad::eProfileDoesNotExist:lstrcpy(Glb_AcadErrorInfo,_T("配置名称不存在"));break;
	case Acad::eInvalidFileExtension:lstrcpy(Glb_AcadErrorInfo,_T("无效的文件后缀名成"));break;
	case Acad::eInvalidProfileName:lstrcpy(Glb_AcadErrorInfo,_T("无效的配置文件名称"));break;
	case Acad::eFileExists:lstrcpy(Glb_AcadErrorInfo,_T("文件存在"));break;
	case Acad::eProfileIsInUse:lstrcpy(Glb_AcadErrorInfo,_T("配置文件存在"));break;
	case Acad::eCantOpenFile:lstrcpy(Glb_AcadErrorInfo,_T("打开文件失败"));break;
	case Acad::eNoFileName:lstrcpy(Glb_AcadErrorInfo,_T("没有文件名称"));break;
	case Acad::eRegistryAccessError:lstrcpy(Glb_AcadErrorInfo,_T("读取注册表错误"));break;
	case Acad::eRegistryCreateError:lstrcpy(Glb_AcadErrorInfo,_T("创建注册表项错误"));break;
	case Acad::eBadDxfFile:lstrcpy(Glb_AcadErrorInfo,_T("坏的DXF文件"));break;
	case Acad::eUnknownDxfFileFormat:lstrcpy(Glb_AcadErrorInfo,_T("未知的DXF文件格式"));break;
	case Acad::eMissingDxfSection:lstrcpy(Glb_AcadErrorInfo,_T("丢失DXF分段"));break;
	case Acad::eInvalidDxfSectionName:lstrcpy(Glb_AcadErrorInfo,_T("无效的DXF分段名称"));break;
	case Acad::eNotDxfHeaderGroupCode:lstrcpy(Glb_AcadErrorInfo,_T("无效的DXF组码"));break;
	case Acad::eUndefinedDxfGroupCode:lstrcpy(Glb_AcadErrorInfo,_T("没有定义DXF组码"));break;
	case Acad::eNotInitializedYet:lstrcpy(Glb_AcadErrorInfo,_T("没有初始化"));break;
	case Acad::eInvalidDxf2dPoint:lstrcpy(Glb_AcadErrorInfo,_T("无效的DXF二维点"));break;
	case Acad::eInvalidDxf3dPoint:lstrcpy(Glb_AcadErrorInfo,_T("无效的DXD三维点"));break;
	case Acad::eBadlyNestedAppData:lstrcpy(Glb_AcadErrorInfo,_T("坏的嵌套应用程序数据"));break;
	case Acad::eIncompleteBlockDefinition:lstrcpy(Glb_AcadErrorInfo,_T("不完整的块定义"));break;
	case Acad::eIncompleteComplexObject:lstrcpy(Glb_AcadErrorInfo,_T("不完整的合成(?复杂)对象"));break;
	case Acad::eBlockDefInEntitySection:lstrcpy(Glb_AcadErrorInfo,_T("块定义在实体段中"));break;
	case Acad::eNoBlockBegin:lstrcpy(Glb_AcadErrorInfo,_T("没有块开始"));break;
	case Acad::eDuplicateLayerName:lstrcpy(Glb_AcadErrorInfo,_T("重复的图层名称"));break;



		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
		//作者:boitboy,网站:http://www.cadgj.com (CAD工具之家),大量实用ObjectArx开发教程,代码,CAD小工具
		//代码可任意转载,使用,修改,再发布,没有任何限制.
		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
	case Acad::eBadPlotStyleName:lstrcpy(Glb_AcadErrorInfo,_T("不好的打印样式名称"));break;
	case Acad::eDuplicateBlockName:lstrcpy(Glb_AcadErrorInfo,_T("重复的块名称"));break;
	case Acad::eBadPlotStyleType:lstrcpy(Glb_AcadErrorInfo,_T("不好的打印样式类型"));break;
	case Acad::eBadPlotStyleNameHandle:lstrcpy(Glb_AcadErrorInfo,_T("不好的打印样式名称句柄"));break;
	case Acad::eUndefineShapeName:lstrcpy(Glb_AcadErrorInfo,_T("没有定义形状名称"));break;
	case Acad::eDuplicateBlockDefinition:lstrcpy(Glb_AcadErrorInfo,_T("重复的块定义"));break;
	case Acad::eMissingBlockName:lstrcpy(Glb_AcadErrorInfo,_T("丢失了块名称"));break;
	case Acad::eBinaryDataSizeExceeded:lstrcpy(Glb_AcadErrorInfo,_T("二进制数据长度太长"));break;
	case Acad::eObjectIsReferenced:lstrcpy(Glb_AcadErrorInfo,_T("对象被引用"));break;
	case Acad::eNoThumbnailBitmap:lstrcpy(Glb_AcadErrorInfo,_T("没有缩略图"));break;
	case Acad::eGuidNoAddress:lstrcpy(Glb_AcadErrorInfo,_T("未找到GUID地址"));break;
	case Acad::eMustBe0to2:lstrcpy(Glb_AcadErrorInfo,_T("必须是0到2"));break;
	case Acad::eMustBe0to3:lstrcpy(Glb_AcadErrorInfo,_T("必须是0到3"));break;
	case Acad::eMustBe0to4:lstrcpy(Glb_AcadErrorInfo,_T("必须是0到4"));break;
	case Acad::eMustBe0to5:lstrcpy(Glb_AcadErrorInfo,_T("必须是0到5"));break;
	case Acad::eMustBe0to8:lstrcpy(Glb_AcadErrorInfo,_T("必须是0到8"));break;
	case Acad::eMustBe1to8:lstrcpy(Glb_AcadErrorInfo,_T("必须是1到8"));break;
	case Acad::eMustBe1to15:lstrcpy(Glb_AcadErrorInfo,_T("必须是1到15"));break;
	case Acad::eMustBePositive:lstrcpy(Glb_AcadErrorInfo,_T("必须为正数"));break;
	case Acad::eMustBeNonNegative:lstrcpy(Glb_AcadErrorInfo,_T("必须为非负数"));break;
	case Acad::eMustBeNonZero:lstrcpy(Glb_AcadErrorInfo,_T("不可以等于0"));break;
	case Acad::eMustBe1to6:lstrcpy(Glb_AcadErrorInfo,_T("必须是1到6"));break;
	case Acad::eNoPlotStyleTranslationTable:lstrcpy(Glb_AcadErrorInfo,_T("没有打印样式事务表(?)"));break;
	case Acad::ePlotStyleInColorDependentMode:lstrcpy(Glb_AcadErrorInfo,_T("打印样式依赖颜色"));break;
	case Acad::eMaxLayouts:lstrcpy(Glb_AcadErrorInfo,_T("最大布局数量"));break;
	case Acad::eNoClassId:lstrcpy(Glb_AcadErrorInfo,_T("没有类ID"));break;
	case Acad::eUndoOperationNotAvailable:lstrcpy(Glb_AcadErrorInfo,_T("撤销操作无效"));break;
	case Acad::eUndoNoGroupBegin:lstrcpy(Glb_AcadErrorInfo,_T("撤销操作没有组开始"));break;
	case Acad::eHatchTooDense:lstrcpy(Glb_AcadErrorInfo,_T("填充太密集"));break;
	case Acad::eOpenFileCancelled:lstrcpy(Glb_AcadErrorInfo,_T("打开文件取消"));break;
	case Acad::eNotHandled:lstrcpy(Glb_AcadErrorInfo,_T("没有处理"));break;
	case Acad::eMakeMeProxyAndResurrect:lstrcpy(Glb_AcadErrorInfo,_T("将自己变成代理然后复活"));break;
	case Acad::eFileMissingSections:lstrcpy(Glb_AcadErrorInfo,_T("文件丢失分段"));break;
	case Acad::eRepeatedDwgRead:lstrcpy(Glb_AcadErrorInfo,_T("重复的读取DWG文件"));break;
	case Acad::eWrongCellType:lstrcpy(Glb_AcadErrorInfo,_T("错误的单元格类型"));break;
	case Acad::eCannotChangeColumnType:lstrcpy(Glb_AcadErrorInfo,_T("不可以改变列类型"));break;
	case Acad::eRowsMustMatchColumns:lstrcpy(Glb_AcadErrorInfo,_T("行必须匹配列"));break;
	case Acad::eFileSharingViolation:lstrcpy(Glb_AcadErrorInfo,_T("文件共享妨碍"));break;
	case Acad::eUnsupportedFileFormat:lstrcpy(Glb_AcadErrorInfo,_T("不支持的文件格式"));break;
	case Acad::eObsoleteFileFormat:lstrcpy(Glb_AcadErrorInfo,_T("废弃的文件格式"));break;
	case Acad::eDwgShareDemandLoad:lstrcpy(Glb_AcadErrorInfo,_T("DWG共享要求加载(?)"));break;
	case Acad::eDwgShareReadAccess:lstrcpy(Glb_AcadErrorInfo,_T("DWG共享读取"));break;
	case Acad::eDwgShareWriteAccess:lstrcpy(Glb_AcadErrorInfo,_T("DWG共享写入"));break;
	case Acad::eLoadFailed:lstrcpy(Glb_AcadErrorInfo,_T("加载失败"));break;
	case Acad::eDeviceNotFound:lstrcpy(Glb_AcadErrorInfo,_T("驱动未找到"));break;
	case Acad::eNoCurrentConfig:lstrcpy(Glb_AcadErrorInfo,_T("没有当前配置"));break;
	case Acad::eNullPtr:lstrcpy(Glb_AcadErrorInfo,_T("空指针"));break;
	case Acad::eNoLayout:lstrcpy(Glb_AcadErrorInfo,_T("没有布局"));break;
	case Acad::eIncompatiblePlotSettings:lstrcpy(Glb_AcadErrorInfo,_T("不兼容的打印设置"));break;
	case Acad::eNonePlotDevice:lstrcpy(Glb_AcadErrorInfo,_T("没有打印驱动"));break;



		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
		//作者:boitboy,网站:http://www.cadgj.com (CAD工具之家),大量实用ObjectArx开发教程,代码,CAD小工具
		//代码可任意转载,使用,修改,再发布,没有任何限制.
		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
	case Acad::eNoMatchingMedia:lstrcpy(Glb_AcadErrorInfo,_T("没有匹配的打印尺寸"));break;
	case Acad::eInvalidView:lstrcpy(Glb_AcadErrorInfo,_T("无效的视图"));break;
	case Acad::eInvalidWindowArea:lstrcpy(Glb_AcadErrorInfo,_T("无效的窗口范围"));break;
	case Acad::eInvalidPlotArea:lstrcpy(Glb_AcadErrorInfo,_T("无效的打印范围"));break;
	case Acad::eCustomSizeNotPossible:lstrcpy(Glb_AcadErrorInfo,_T("用户输入的打印尺寸不可能存在"));break;
	case Acad::ePageCancelled:lstrcpy(Glb_AcadErrorInfo,_T("纸张取消"));break;
	case Acad::ePlotCancelled:lstrcpy(Glb_AcadErrorInfo,_T("打印取消"));break;
	case Acad::eInvalidEngineState:lstrcpy(Glb_AcadErrorInfo,_T("无效的引擎状态"));break;
	case Acad::ePlotAlreadyStarted:lstrcpy(Glb_AcadErrorInfo,_T("已经开始在打印了"));break;
	case Acad::eNoErrorHandler:lstrcpy(Glb_AcadErrorInfo,_T("没有错误处理"));break;
	case Acad::eInvalidPlotInfo:lstrcpy(Glb_AcadErrorInfo,_T("无效的打印信息"));break;
	case Acad::eNumberOfCopiesNotSupported:lstrcpy(Glb_AcadErrorInfo,_T("不支持打印份数"));break;
	case Acad::eLayoutNotCurrent:lstrcpy(Glb_AcadErrorInfo,_T("不是当前布局"));break;
	case Acad::eGraphicsNotGenerated:lstrcpy(Glb_AcadErrorInfo,_T("绘图对象创建失败(?)"));break;
	case Acad::eCannotPlotToFile:lstrcpy(Glb_AcadErrorInfo,_T("不可以打印到文件"));break;
	case Acad::eMustPlotToFile:lstrcpy(Glb_AcadErrorInfo,_T("必须打印到文件"));break;
	case Acad::eNotMultiPageCapable:lstrcpy(Glb_AcadErrorInfo,_T("不支持多种纸张"));break;
	case Acad::eBackgroundPlotInProgress:lstrcpy(Glb_AcadErrorInfo,_T("正在后台打印"));break;
	case Acad::eSubSelectionSetEmpty:lstrcpy(Glb_AcadErrorInfo,_T("子选择集被设置为空"));break;
	case Acad::eInvalidObjectId:lstrcpy(Glb_AcadErrorInfo,_T("无效的对象ID或者对象ID不在当前数据库"));break;
	case Acad::eInvalidXrefObjectId:lstrcpy(Glb_AcadErrorInfo,_T("无效的XREF对象ID或者XREF对象ID不在当前数据库"));break;
	case Acad::eNoViewAssociation:lstrcpy(Glb_AcadErrorInfo,_T("未找到对应的视图对象"));break;
	case Acad::eNoLabelBlock:lstrcpy(Glb_AcadErrorInfo,_T("视口未找到关联的块"));break;
	case Acad::eUnableToSetViewAssociation:lstrcpy(Glb_AcadErrorInfo,_T("设置视图关联视口失败"));break;
	case Acad::eUnableToGetViewAssociation:lstrcpy(Glb_AcadErrorInfo,_T("无法找到关联的视图"));break;
	case Acad::eUnableToSetLabelBlock:lstrcpy(Glb_AcadErrorInfo,_T("无法设置关联的块"));break;
	case Acad::eUnableToGetLabelBlock:lstrcpy(Glb_AcadErrorInfo,_T("无法获取关联的块"));break;
	case Acad::eUnableToRemoveAssociation:lstrcpy(Glb_AcadErrorInfo,_T("无法移除视口关联对象"));break;
	case Acad::eUnableToSyncModelView:lstrcpy(Glb_AcadErrorInfo,_T("无法同步视口和模型空间视图"));break;
	case Acad::eSecInitializationFailure:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)初始化错误"));break;
	case Acad::eSecErrorReadingFile:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)读取文件错误"));break;
	case Acad::eSecErrorWritingFile:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)写入文件错误"));break;
	case Acad::eSecInvalidDigitalID:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)无效的数字ID"));break;
	case Acad::eSecErrorGeneratingTimestamp:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)创建时间戳错误"));break;
	case Acad::eSecErrorComputingSignature:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)电子签名错误"));break;
	case Acad::eSecErrorWritingSignature:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)写入签名错误"));break;
	case Acad::eSecErrorEncryptingData:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)加密数据错误"));break;
	case Acad::eSecErrorCipherNotSupported:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)不支持的密码"));break;
	case Acad::eSecErrorDecryptingData:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)解密数据错误"));break;
		//case Acad::eInetBase:lstrcpy(Glb_AcadErrorInfo,_T("网络错误"));break;
	case Acad::eInetOk:lstrcpy(Glb_AcadErrorInfo,_T("网络正常"));break;
	case Acad::eInetInCache:lstrcpy(Glb_AcadErrorInfo,_T("在缓冲区中"));break;
	case Acad::eInetFileNotFound:lstrcpy(Glb_AcadErrorInfo,_T("网络文件不存在"));break;
	case Acad::eInetBadPath:lstrcpy(Glb_AcadErrorInfo,_T("不好的网络路径"));break;
	case Acad::eInetTooManyOpenFiles:lstrcpy(Glb_AcadErrorInfo,_T("打开太多网络文件"));break;
	case Acad::eInetFileAccessDenied:lstrcpy(Glb_AcadErrorInfo,_T("打开网络文件被拒绝"));break;
	case Acad::eInetInvalidFileHandle:lstrcpy(Glb_AcadErrorInfo,_T("无效的网络文件句柄"));break;
	case Acad::eInetDirectoryFull:lstrcpy(Glb_AcadErrorInfo,_T("网络文件夹目录已满"));break;
	case Acad::eInetHardwareError:lstrcpy(Glb_AcadErrorInfo,_T("网络硬件错误"));break;
	case Acad::eInetSharingViolation:lstrcpy(Glb_AcadErrorInfo,_T("违反网络共享"));break;



		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
		//作者:boitboy,网站:http://www.cadgj.com (CAD工具之家),大量实用ObjectArx开发教程,代码,CAD小工具
		//代码可任意转载,使用,修改,再发布,没有任何限制.
		///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
	case Acad::eInetDiskFull:lstrcpy(Glb_AcadErrorInfo,_T("网络硬盘满了"));break;
	case Acad::eInetFileGenericError:lstrcpy(Glb_AcadErrorInfo,_T("网络文件创建错误"));break;
	case Acad::eInetValidURL:lstrcpy(Glb_AcadErrorInfo,_T("无效的URL地址"));break;
	case Acad::eInetNotAnURL:lstrcpy(Glb_AcadErrorInfo,_T("不是URL地址"));break;
	case Acad::eInetNoWinInet:lstrcpy(Glb_AcadErrorInfo,_T("没有WinInet(?)"));break;
	case Acad::eInetOldWinInet:lstrcpy(Glb_AcadErrorInfo,_T("旧的WinInet(?)"));break;
	case Acad::eInetNoAcadInet:lstrcpy(Glb_AcadErrorInfo,_T("无法连接ACAD网站"));break;
	case Acad::eInetNotImplemented:lstrcpy(Glb_AcadErrorInfo,_T("无法应用网络"));break;
	case Acad::eInetProtocolNotSupported:lstrcpy(Glb_AcadErrorInfo,_T("网络协议不支持"));break;
	case Acad::eInetCreateInternetSessionFailed:lstrcpy(Glb_AcadErrorInfo,_T("创建网络会话失败"));break;
	case Acad::eInetInternetSessionConnectFailed:lstrcpy(Glb_AcadErrorInfo,_T("连接网络会话失败"));break;
	case Acad::eInetInternetSessionOpenFailed:lstrcpy(Glb_AcadErrorInfo,_T("打开网络会话失败"));break;
	case Acad::eInetInvalidAccessType:lstrcpy(Glb_AcadErrorInfo,_T("无效的网络接收类型"));break;
	case Acad::eInetFileOpenFailed:lstrcpy(Glb_AcadErrorInfo,_T("打开网络文件失败"));break;
	case Acad::eInetHttpOpenRequestFailed:lstrcpy(Glb_AcadErrorInfo,_T("打开HTTP协议失败"));break;
	case Acad::eInetUserCancelledTransfer:lstrcpy(Glb_AcadErrorInfo,_T("用户取消了网络传输"));break;
	case Acad::eInetHttpBadRequest:lstrcpy(Glb_AcadErrorInfo,_T("不合理的网络请求"));break;
	case Acad::eInetHttpAccessDenied:lstrcpy(Glb_AcadErrorInfo,_T("HTTP协议拒绝"));break;
	case Acad::eInetHttpPaymentRequired:lstrcpy(Glb_AcadErrorInfo,_T("HTTP协议要求付费"));break;
	case Acad::eInetHttpRequestForbidden:lstrcpy(Glb_AcadErrorInfo,_T("禁止HTTP请求"));break;
	case Acad::eInetHttpObjectNotFound:lstrcpy(Glb_AcadErrorInfo,_T("HTTP对象未找到"));break;
	case Acad::eInetHttpBadMethod:lstrcpy(Glb_AcadErrorInfo,_T("不合理的HTTP请求方法"));break;
	case Acad::eInetHttpNoAcceptableResponse:lstrcpy(Glb_AcadErrorInfo,_T("不接受的HTTP回复"));break;
	case Acad::eInetHttpProxyAuthorizationRequired:lstrcpy(Glb_AcadErrorInfo,_T("要求HTTP代理授权"));break;
	case Acad::eInetHttpTimedOut:lstrcpy(Glb_AcadErrorInfo,_T("HTTP超时"));break;
	case Acad::eInetHttpConflict:lstrcpy(Glb_AcadErrorInfo,_T("HTTP冲突"));break;
	case Acad::eInetHttpResourceGone:lstrcpy(Glb_AcadErrorInfo,_T("网络资源被用光"));break;
	case Acad::eInetHttpLengthRequired:lstrcpy(Glb_AcadErrorInfo,_T("HTTP请求长度是必须的"));break;
	case Acad::eInetHttpPreconditionFailure:lstrcpy(Glb_AcadErrorInfo,_T("HTTP预处理失败"));break;
	case Acad::eInetHttpRequestTooLarge:lstrcpy(Glb_AcadErrorInfo,_T("HTTP请求太大"));break;
	case Acad::eInetHttpUriTooLong:lstrcpy(Glb_AcadErrorInfo,_T("URL地址太长"));break;
	case Acad::eInetHttpUnsupportedMedia:lstrcpy(Glb_AcadErrorInfo,_T("HTTP不支持的媒体"));break;
	case Acad::eInetHttpServerError:lstrcpy(Glb_AcadErrorInfo,_T("HTTP服务器错误"));break;
	case Acad::eInetHttpNotSupported:lstrcpy(Glb_AcadErrorInfo,_T("HTTP不支持"));break;
	case Acad::eInetHttpBadGateway:lstrcpy(Glb_AcadErrorInfo,_T("HTTP网关错误"));break;
	case Acad::eInetHttpServiceUnavailable:lstrcpy(Glb_AcadErrorInfo,_T("HTTP服务当前不可用"));break;
	case Acad::eInetHttpGatewayTimeout:lstrcpy(Glb_AcadErrorInfo,_T("HTTP网关超时"));break;
	case Acad::eInetHttpVersionNotSupported:lstrcpy(Glb_AcadErrorInfo,_T("HTTP版本不支持"));break;
	case Acad::eInetInternetError:lstrcpy(Glb_AcadErrorInfo,_T("HTTP网络错误"));break;
	case Acad::eInetGenericException:lstrcpy(Glb_AcadErrorInfo,_T("HTTP常规异常"));break;
	case Acad::eInetUnknownError:lstrcpy(Glb_AcadErrorInfo,_T("HTTP未知错误"));break;
	case Acad::eAlreadyActive:lstrcpy(Glb_AcadErrorInfo,_T("已经是活动的了"));break;
	case Acad::eAlreadyInactive:lstrcpy(Glb_AcadErrorInfo,_T("已经是不活动的了"));break;
	default:
		lstrcpy(Glb_AcadErrorInfo,acadErrorStatusText(es));
		//Unknown error
		replace(Glb_AcadErrorInfo,_T("Unknown error"),_T("未知错误"));
		break;
	}
	return Glb_AcadErrorInfo;
}


///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
//作者:boitboy,网站:http://www.cadgj.com (CAD工具之家),大量实用ObjectArx开发教程,代码,CAD小工具
//代码可任意转载,使用,修改,再发布,没有任何限制.
//当前版本:V1.0.0
//发布日期:20130819
//鉴于作者英语水平有限，部分翻译可能不准确，欢迎批量指正
//QQ:245804736,EMail:wdzhangsl@126.com
///////////////请体谅作者的辛苦劳动,保留作者的版权申明////////////////////
void CiWAcsModelTools::replace(TCHAR *s,TCHAR *s1,TCHAR *s2)
{
	int nLen=lstrlen(s);
	int nLen1=lstrlen(s1);
	int nLen2=lstrlen(s2);
	for(int i=0;i<nLen-nLen1+1;i++)
	{
		bool bSame=true;
		for(int j=0;j<nLen1;j++)
		{
			if(s[i+j]!=s1[j])
			{
				bSame=false;
				break;
			}
		}
		if(!bSame)
		{
			continue;
		}

		//i+nLen1是原来的字符串的后面部分开始
		if(nLen1>nLen2)
		{
			//替换后字符串变短nLen1-nLen2
			for(int k=0;k<nLen2;k++)
			{
				s[i+k]=s2[k];
			}

			for(int l=i+nLen2;l<nLen+nLen2-nLen1;l++)
			{
				s[l]=s[l+nLen1-nLen2];
			}
			for(int m=nLen+nLen2-nLen1;m<nLen;m++)
			{
				s[m]='\0';
			}
			nLen=lstrlen(s);
			i=i+nLen2;
		}
		else if(nLen2>nLen1)
		{
			//替换后字符串变长了nLen2-nLen1
			//先将替换时占用到的位置整体后移
			for(int l=nLen+nLen2-nLen1;l>=i+nLen2;l--)
			{
				s[l]=s[l+nLen1-nLen2];
			}
			for(int k=0;k<nLen2;k++)
			{
				s[i+k]=s2[k];
			}
			nLen=lstrlen(s);
			i=i+nLen2;
		}



	}
}