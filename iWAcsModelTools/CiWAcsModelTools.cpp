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
	// ��õ�ǰ·��
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
//�ļ��Ƿ���� 
bool CiWAcsModelTools::IsExist(LPCTSTR szFileName) 
{
	static bool bExist ;
	static CString sFileName;
	bExist = false;
	sFileName=szFileName;
	//ASSERT2(!sFileName.IsEmpty(),_T("�����Ҫ��һ���մ�����IsExist��"));
	if (sFileName.IsEmpty())
		return false;

#ifdef _DEBUG
	CString sPrompt;
	sPrompt.Format(_T("1.�ں���IsExist��Ҫ��ȫ·����\n2.����������[%s]��"),sFileName);
	//ASSERT2(szFileName[1]==_T('\\') || szFileName[1]==_T(':'),CString(CString()+sPrompt));
#endif

	WIN32_FIND_DATA filedata;

	if (FindFirstFile(sFileName, &filedata) == INVALID_HANDLE_VALUE)
	{
		//�����ǹ�����
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
		AfxMessageBox(_T("\n ���ݿ��ļ�������!"));
		return false;
	}
	//�Ѿ���,����Ҫ���´�
	if(g_pGPSDbaseConn.IsOpen())
	{
		return true;
	}
	CString sTmp=_T("Provider=Microsoft.ACE.OLEDB.12.0;Data Source=");
	sTmp = sTmp + strFilePath;

	if(!g_pGPSDbaseConn.Open(sTmp))
	{
		alertstr = _T("�������ݿ�ʧ��!");

		AfxMessageBox(alertstr);

		return false; 
	}
	return true;
}


bool CiWAcsModelTools::iwCopyToProvideFile()
{
	// ������ǰ�ռ�
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
	// �ȶ�ȡ����ͼ�㡢ͼ��-������ģ��ӳ���
	MAPStringToString layerMapString;
	MAPStringToString blockMapString;
	//CString strPath=GetSoftPath();
	//strPath += _T("\\ProvideMap.ini");
	//CHWPubFun::GetSectionAllKeyAndValue(strPath, _T("PROLAYER"), layerMapString);
	//CHWPubFun::GetSectionAllKeyAndValue(strPath, _T("PROBLOCK"), blockMapString);
	// ��ò�Ʒϵ��
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


	// ����ͼ�㡢ͼ��Ķ���ObjectId�������ļ���ӳ���
	MAPStringToIdArray provideObjIdAryMap;

	for ( ; !iterBlkTbl->done(); iterBlkTbl->step())
	{
		AcDbEntity* pEnt = NULL;
		iterBlkTbl->getEntity(pEnt, AcDb::kForRead);
		if (pEnt != NULL )
		{
			// ���ж��Ƿ�����ͼ��
			CString strLayerName = pEnt->layer();
			MAPStringToString::iterator itMapLay = layerMapString.find(strLayerName);
			if (itMapLay != layerMapString.end())
			{
				CString strProvideFileName = itMapLay->second;
				MAPStringToIdArray::iterator itMapLayIdAry = provideObjIdAryMap.find(strProvideFileName);
				if (itMapLayIdAry!= provideObjIdAryMap.end())
				{
					// �ļ��Ѿ���ӳ�䣬ֱ����ӾͿ���
					itMapLayIdAry->second.append(pEnt->objectId());
					//acutPrintf(_T("\n %s - %s "), strLayerName, strProvideFileName);
				}
				else
				{
					// û��ӳ�䣬����һ��
					AcDbObjectIdArray tmpIdAry;
					tmpIdAry.append(pEnt->objectId());
					provideObjIdAryMap.insert(std::make_pair(strProvideFileName, tmpIdAry));

					//acutPrintf(_T("\n %s - %s "), strLayerName, strProvideFileName);
				}
			}
			if (pEnt->isKindOf(AcDbBlockReference::desc())) // �����������ͼ�㣬��ô��������Ǹ��飬�����ж�һ���Ƿ�����ͼ��
			{
				// �ж��Ƿ�����ͼ��
				AcDbBlockReference* pBlkRef = static_cast<AcDbBlockReference*>(pEnt);
				if (pBlkRef != NULL)
				{
					AcDbObjectId blkRcdId = pBlkRef->blockTableRecord();
					AcDbObjectPointer<AcDbBlockTableRecord> pBlkRcd(blkRcdId, AcDb::kForRead);
					if (pBlkRcd.openStatus() == Acad::eOk)
					{
						// �ж�ͼ�����Ƿ�������ͼ��
						TCHAR* psName = NULL;
						pBlkRcd->getName(psName);
						CString strBlkName(psName);
						MAPStringToString::iterator itMapBlk = blockMapString.find(strBlkName);
						if (itMapBlk != blockMapString.end())
						{
							// ������ͼ��
							CString strProvideFileName = itMapBlk->second;
							MAPStringToIdArray::iterator itMapBlkIdAry = provideObjIdAryMap.find(strProvideFileName);
							if (itMapBlkIdAry!= provideObjIdAryMap.end())
							{
								// �ļ��Ѿ���ӳ�䣬ֱ����ӾͿ���
								itMapBlkIdAry->second.append(pEnt->objectId());
							}
							else
							{
								// û��ӳ�䣬����һ��
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

	// ��ǰ�ļ�·��
	CString strCurFilePath =acDocManager->curDocument()->fileName();
	strCurFilePath = strCurFilePath.Left(strCurFilePath.ReverseFind('\\') + 1 );

	// д���ⲿ�����ļ�
	MAPStringToIdArray::iterator itMapLayIdAry = provideObjIdAryMap.begin();
	for ( ; itMapLayIdAry!= provideObjIdAryMap.end(); ++itMapLayIdAry)
	{
		// ��д����ʱ���ݿ�
		AcDbDatabase* pTmpDb = NULL;
		Acad::ErrorStatus res = acdbCurDwg()->wblock(pTmpDb, itMapLayIdAry->second, AcGePoint3d::kOrigin);
		if (pTmpDb == NULL)
		{
			acutPrintf(_T("\n %s"), acadErrorStatusTextChs(res));
			continue;
		}

		// ��Ӧ���ⲿ����·��
		CString strFile = strCurFilePath + itMapLayIdAry->first + _T(".dwg");
		acutPrintf(_T("\n %s"), strFile);
		AcDbDatabase pExternDb(Adesk::kFalse);
		res = pExternDb.readDwgFile( strFile,_SH_DENYRW);
		if (Acad::eOk != res)
		{
			acutPrintf(_T("\n %s"), acadErrorStatusTextChs(res));
			continue;;
		}
		// �ȸ���һ��
		AcDbDatabase* pOldDb = new AcDbDatabase;
		pExternDb.wblock(pOldDb);
		//ɾ���ⲿ����ģ���е�ԭ��ʵ��
		EraseXrefModelAllEntity(pExternDb);// ɾ������ģ��ԭ�е�����ʵ��
		purgeDatabase(&pExternDb);
		//pExternDb.saveAs(iterMap->first);
		// ��д���ⲿ����
		res = pExternDb.insert(AcGeMatrix3d::kIdentity, pTmpDb, false);
		delete pTmpDb;
		if (res == Acad::eOk) // д��ɹ��ű���
		{
			pExternDb.saveAs(strFile);
			acutPrintf(_T("\n ���������ļ� - %s "), strFile);
			pOldDb->saveAs(_T("D:\\111222.dwg"));
			//delete pOldDb;
		}
		else // ���ɹ�������ԭ�е�
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
//���İ��������
const TCHAR * CiWAcsModelTools::acadErrorStatusTextChs(Acad::ErrorStatus es)
{
	switch(es)
	{
	case Acad::eOk:lstrcpy(Glb_AcadErrorInfo,_T("��ȷ"));break;
	case Acad::eNotImplementedYet:lstrcpy(Glb_AcadErrorInfo,_T("��δʵ��"));break;
	case Acad::eNotApplicable:lstrcpy(Glb_AcadErrorInfo,_T("�����ʵ�"));break;
	case Acad::eInvalidInput:lstrcpy(Glb_AcadErrorInfo,_T("��Ч������"));break;
	case Acad::eAmbiguousInput:lstrcpy(Glb_AcadErrorInfo,_T("ģ�����������"));break;
	case Acad::eAmbiguousOutput:lstrcpy(Glb_AcadErrorInfo,_T("ģ����������"));break;
	case Acad::eOutOfMemory:lstrcpy(Glb_AcadErrorInfo,_T("�ڴ治��"));break;
	case Acad::eBufferTooSmall:lstrcpy(Glb_AcadErrorInfo,_T("������̫С"));break;
	case Acad::eInvalidOpenState:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�Ĵ�״̬"));break;
	case Acad::eEntityInInactiveLayout:lstrcpy(Glb_AcadErrorInfo,_T("ʵ�岻�ڻ������"));break;
	case Acad::eHandleExists:lstrcpy(Glb_AcadErrorInfo,_T("����Ѵ���"));break;
	case Acad::eNullHandle:lstrcpy(Glb_AcadErrorInfo,_T("�վ��"));break;
	case Acad::eBrokenHandle:lstrcpy(Glb_AcadErrorInfo,_T("�𻵵ľ��"));break;
	case Acad::eUnknownHandle:lstrcpy(Glb_AcadErrorInfo,_T("δ֪���"));break;
	case Acad::eHandleInUse:lstrcpy(Glb_AcadErrorInfo,_T("�����ռ��"));break;
	case Acad::eNullObjectPointer:lstrcpy(Glb_AcadErrorInfo,_T("����ָ��Ϊ��"));break;
	case Acad::eNullObjectId:lstrcpy(Glb_AcadErrorInfo,_T("����IDΪ��"));break;
	case Acad::eNullBlockName:lstrcpy(Glb_AcadErrorInfo,_T("������Ϊ��"));break;
	case Acad::eContainerNotEmpty:lstrcpy(Glb_AcadErrorInfo,_T("������Ϊ��"));break;
	case Acad::eNullEntityPointer:lstrcpy(Glb_AcadErrorInfo,_T("ʵ��ָ��Ϊ��"));break;
	case Acad::eIllegalEntityType:lstrcpy(Glb_AcadErrorInfo,_T("�Ƿ���ʵ������"));break;
	case Acad::eKeyNotFound:lstrcpy(Glb_AcadErrorInfo,_T("�ؼ���δ�ҵ�"));break;
	case Acad::eDuplicateKey:lstrcpy(Glb_AcadErrorInfo,_T("�ظ��Ĺؼ���"));break;
	case Acad::eInvalidIndex:lstrcpy(Glb_AcadErrorInfo,_T("��Ч������"));break;
	case Acad::eDuplicateIndex:lstrcpy(Glb_AcadErrorInfo,_T("�ظ�������"));break;
	case Acad::eAlreadyInDb:lstrcpy(Glb_AcadErrorInfo,_T("�Ѿ������ݿ�����"));break;
	case Acad::eOutOfDisk:lstrcpy(Glb_AcadErrorInfo,_T("Ӳ����������"));break;
	case Acad::eDeletedEntry:lstrcpy(Glb_AcadErrorInfo,_T("�Ѿ�ɾ���ĺ������"));break;
	case Acad::eNegativeValueNotAllowed:lstrcpy(Glb_AcadErrorInfo,_T("���������븺��"));break;
	case Acad::eInvalidExtents:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�Ŀռ䷶Χ"));break;
	case Acad::eInvalidAdsName:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��ADS����"));break;
	case Acad::eInvalidSymbolTableName:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�ķ�������"));break;
	case Acad::eInvalidKey:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�Ĺؼ���"));break;
	case Acad::eWrongObjectType:lstrcpy(Glb_AcadErrorInfo,_T("���������"));break;
	case Acad::eWrongDatabase:lstrcpy(Glb_AcadErrorInfo,_T("��������ݿ�"));break;
	case Acad::eObjectToBeDeleted:lstrcpy(Glb_AcadErrorInfo,_T("���󼴽���ɾ��"));break;
	case Acad::eInvalidDwgVersion:lstrcpy(Glb_AcadErrorInfo,_T("�������DWG�汾"));break;
	case Acad::eAnonymousEntry:lstrcpy(Glb_AcadErrorInfo,_T("�������"));break;
	case Acad::eIllegalReplacement:lstrcpy(Glb_AcadErrorInfo,_T("�Ƿ��������"));break;
	case Acad::eEndOfObject:lstrcpy(Glb_AcadErrorInfo,_T("�������"));break;
	case Acad::eEndOfFile:lstrcpy(Glb_AcadErrorInfo,_T("�ļ�����"));break;
	case Acad::eIsReading:lstrcpy(Glb_AcadErrorInfo,_T("���ڶ�ȡ"));break;
	case Acad::eIsWriting:lstrcpy(Glb_AcadErrorInfo,_T("����д��"));break;
	case Acad::eNotOpenForRead:lstrcpy(Glb_AcadErrorInfo,_T("����ֻ����"));break;
	case Acad::eNotOpenForWrite:lstrcpy(Glb_AcadErrorInfo,_T("���ǿ�д��"));break;
	case Acad::eNotThatKindOfClass:lstrcpy(Glb_AcadErrorInfo,_T("���Ͳ�ƥ��"));break;
	case Acad::eInvalidBlockName:lstrcpy(Glb_AcadErrorInfo,_T("������Ŀ�����"));break;
	case Acad::eMissingDxfField:lstrcpy(Glb_AcadErrorInfo,_T("DXF�ֶ�ȱʧ"));break;
	case Acad::eDuplicateDxfField:lstrcpy(Glb_AcadErrorInfo,_T("DXF�ֶ��ظ�"));break;
	case Acad::eInvalidDxfCode:lstrcpy(Glb_AcadErrorInfo,_T("�������DXF����"));break;



		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
		//����:boitboy,��վ:http://www.cadgj.com (CAD����֮��),����ʵ��ObjectArx�����̳�,����,CADС����
		//���������ת��,ʹ��,�޸�,�ٷ���,û���κ�����.
		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
	case Acad::eInvalidResBuf:lstrcpy(Glb_AcadErrorInfo,_T("�������ResBuf"));break;
	case Acad::eBadDxfSequence:lstrcpy(Glb_AcadErrorInfo,_T("����ȷ��DXF˳��"));break;
	case Acad::eFilerError:lstrcpy(Glb_AcadErrorInfo,_T("�ļ�����"));break;
	case Acad::eVertexAfterFace:lstrcpy(Glb_AcadErrorInfo,_T("�����������"));break;
	case Acad::eInvalidFaceVertexIndex:lstrcpy(Glb_AcadErrorInfo,_T("��������涥��˳��"));break;
	case Acad::eInvalidMeshVertexIndex:lstrcpy(Glb_AcadErrorInfo,_T("�������mesh˳��"));break;
	case Acad::eOtherObjectsBusy:lstrcpy(Glb_AcadErrorInfo,_T("��������æ"));break;
	case Acad::eMustFirstAddBlockToDb:lstrcpy(Glb_AcadErrorInfo,_T("�����Ȱѿ���뵽���ݿ�"));break;
	case Acad::eCannotNestBlockDefs:lstrcpy(Glb_AcadErrorInfo,_T("������Ƕ�׿鶨��"));break;
	case Acad::eDwgRecoveredOK:lstrcpy(Glb_AcadErrorInfo,_T("�޸�DWG���"));break;
	case Acad::eDwgNotRecoverable:lstrcpy(Glb_AcadErrorInfo,_T("�޷��޸�DWG"));break;
	case Acad::eDxfPartiallyRead:lstrcpy(Glb_AcadErrorInfo,_T("DXF���ֶ�ȡ"));break;
	case Acad::eDxfReadAborted:lstrcpy(Glb_AcadErrorInfo,_T("��ȡDXF��ֹ"));break;
	case Acad::eDxbPartiallyRead:lstrcpy(Glb_AcadErrorInfo,_T("DXB���ֶ�ȡ"));break;
	case Acad::eDwgCRCDoesNotMatch:lstrcpy(Glb_AcadErrorInfo,_T("DWG�ļ���CRC��ƥ��"));break;
	case Acad::eDwgSentinelDoesNotMatch:lstrcpy(Glb_AcadErrorInfo,_T("DWG�ļ���У�鲻ƥ��"));break;
	case Acad::eDwgObjectImproperlyRead:lstrcpy(Glb_AcadErrorInfo,_T("DWG�ļ������ȡ"));break;
	case Acad::eNoInputFiler:lstrcpy(Glb_AcadErrorInfo,_T("û���ҵ��������"));break;
	case Acad::eDwgNeedsAFullSave:lstrcpy(Glb_AcadErrorInfo,_T("DWG��Ҫ��ȫ����"));break;
	case Acad::eDxbReadAborted:lstrcpy(Glb_AcadErrorInfo,_T("DXB��ȡ��ֹ"));break;
	case Acad::eFileLockedByACAD:lstrcpy(Glb_AcadErrorInfo,_T("�ļ���ACAD����"));break;
	case Acad::eFileAccessErr:lstrcpy(Glb_AcadErrorInfo,_T("�޷���ȡ�ļ�"));break;
	case Acad::eFileSystemErr:lstrcpy(Glb_AcadErrorInfo,_T("�ļ�ϵͳ����"));break;
	case Acad::eFileInternalErr:lstrcpy(Glb_AcadErrorInfo,_T("�ļ��ڲ�����"));break;
	case Acad::eFileTooManyOpen:lstrcpy(Glb_AcadErrorInfo,_T("�ļ�����̫���"));break;
	case Acad::eFileNotFound:lstrcpy(Glb_AcadErrorInfo,_T("δ�ҵ��ļ�"));break;
	case Acad::eDwkLockFileFound:lstrcpy(Glb_AcadErrorInfo,_T("�ҵ�DWG�����ļ�"));break;
	case Acad::eWasErased:lstrcpy(Glb_AcadErrorInfo,_T("����ɾ��"));break;
	case Acad::ePermanentlyErased:lstrcpy(Glb_AcadErrorInfo,_T("��������ɾ��"));break;
	case Acad::eWasOpenForRead:lstrcpy(Glb_AcadErrorInfo,_T("����ֻ����"));break;
	case Acad::eWasOpenForWrite:lstrcpy(Glb_AcadErrorInfo,_T("�����д��"));break;
	case Acad::eWasOpenForUndo:lstrcpy(Glb_AcadErrorInfo,_T("��������"));break;
	case Acad::eWasNotifying:lstrcpy(Glb_AcadErrorInfo,_T("����֪ͨ"));break;
	case Acad::eWasOpenForNotify:lstrcpy(Glb_AcadErrorInfo,_T("����֪ͨ��"));break;
	case Acad::eOnLockedLayer:lstrcpy(Glb_AcadErrorInfo,_T("����������ͼ����"));break;
	case Acad::eMustOpenThruOwner:lstrcpy(Glb_AcadErrorInfo,_T("���뾭�������ߴ�"));break;
	case Acad::eSubentitiesStillOpen:lstrcpy(Glb_AcadErrorInfo,_T("�Ӷ�����Ȼ����"));break;
	case Acad::eAtMaxReaders:lstrcpy(Glb_AcadErrorInfo,_T("�������򿪴���"));break;
	case Acad::eIsWriteProtected:lstrcpy(Glb_AcadErrorInfo,_T("����д����"));break;
	case Acad::eIsXRefObject:lstrcpy(Glb_AcadErrorInfo,_T("������XRef"));break;
	case Acad::eNotAnEntity:lstrcpy(Glb_AcadErrorInfo,_T("������ʵ��"));break;
	case Acad::eHadMultipleReaders:lstrcpy(Glb_AcadErrorInfo,_T("�����ش�"));break;
	case Acad::eDuplicateRecordName:lstrcpy(Glb_AcadErrorInfo,_T("�ظ��ļ�¼����"));break;
	case Acad::eXRefDependent:lstrcpy(Glb_AcadErrorInfo,_T("������XREF"));break;
	case Acad::eSelfReference:lstrcpy(Glb_AcadErrorInfo,_T("��������"));break;
	case Acad::eMissingSymbolTable:lstrcpy(Glb_AcadErrorInfo,_T("��ʧ���Ż���"));break;
	case Acad::eMissingSymbolTableRec:lstrcpy(Glb_AcadErrorInfo,_T("��ʧ���Ż���¼"));break;
	case Acad::eWasNotOpenForWrite:lstrcpy(Glb_AcadErrorInfo,_T("���ǿ�д��"));break;
	case Acad::eCloseWasNotifying:lstrcpy(Glb_AcadErrorInfo,_T("����ر�,����ִ��֪ͨ"));break;
	case Acad::eCloseModifyAborted:lstrcpy(Glb_AcadErrorInfo,_T("����ر�,�޸ı�ȡ��"));break;



		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
		//����:boitboy,��վ:http://www.cadgj.com (CAD����֮��),����ʵ��ObjectArx�����̳�,����,CADС����
		//���������ת��,ʹ��,�޸�,�ٷ���,û���κ�����.
		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
	case Acad::eClosePartialFailure:lstrcpy(Glb_AcadErrorInfo,_T("����ر�,���ֲ���δ�ɹ�"));break;
	case Acad::eCloseFailObjectDamaged:lstrcpy(Glb_AcadErrorInfo,_T("������,�ر�ʧ��"));break;
	case Acad::eCannotBeErasedByCaller:lstrcpy(Glb_AcadErrorInfo,_T("���󲻿��Ա���ǰ������ɾ��"));break;
	case Acad::eCannotBeResurrected:lstrcpy(Glb_AcadErrorInfo,_T("�����Ը���"));break;
	case Acad::eWasNotErased:lstrcpy(Glb_AcadErrorInfo,_T("����δɾ��"));break;
	case Acad::eInsertAfter:lstrcpy(Glb_AcadErrorInfo,_T("�ں������"));break;
	case Acad::eFixedAllErrors:lstrcpy(Glb_AcadErrorInfo,_T("�޸������д���"));break;
	case Acad::eLeftErrorsUnfixed:lstrcpy(Glb_AcadErrorInfo,_T("ʣ��һЩ����δ�޸�"));break;
	case Acad::eUnrecoverableErrors:lstrcpy(Glb_AcadErrorInfo,_T("���ɻָ��Ĵ���"));break;
	case Acad::eNoDatabase:lstrcpy(Glb_AcadErrorInfo,_T("û�����ݿ�"));break;
	case Acad::eXdataSizeExceeded:lstrcpy(Glb_AcadErrorInfo,_T("��չ���ݳ���̫��"));break;
	case Acad::eRegappIdNotFound:lstrcpy(Glb_AcadErrorInfo,_T("û���ҵ���չ����ע��ID"));break;
	case Acad::eRepeatEntity:lstrcpy(Glb_AcadErrorInfo,_T("�ظ�ʵ��"));break;
	case Acad::eRecordNotInTable:lstrcpy(Glb_AcadErrorInfo,_T("����δ�ҵ���¼"));break;
	case Acad::eIteratorDone:lstrcpy(Glb_AcadErrorInfo,_T("���������"));break;
	case Acad::eNullIterator:lstrcpy(Glb_AcadErrorInfo,_T("�յĵ�����"));break;
	case Acad::eNotInBlock:lstrcpy(Glb_AcadErrorInfo,_T("���ڿ���"));break;
	case Acad::eOwnerNotInDatabase:lstrcpy(Glb_AcadErrorInfo,_T("�����߲������ݿ���"));break;
	case Acad::eOwnerNotOpenForRead:lstrcpy(Glb_AcadErrorInfo,_T("�����߲���ֻ����"));break;
	case Acad::eOwnerNotOpenForWrite:lstrcpy(Glb_AcadErrorInfo,_T("�����߲��ǿ�д��"));break;
	case Acad::eExplodeBeforeTransform:lstrcpy(Glb_AcadErrorInfo,_T("�ڱ任֮ǰ�ͱ�ը����"));break;
	case Acad::eCannotScaleNonUniformly:lstrcpy(Glb_AcadErrorInfo,_T("�����Բ�ͬ��������"));break;
	case Acad::eNotInDatabase:lstrcpy(Glb_AcadErrorInfo,_T("�������ݿ���"));break;
	case Acad::eNotCurrentDatabase:lstrcpy(Glb_AcadErrorInfo,_T("���ǵ�ǰ���ݿ�"));break;
	case Acad::eIsAnEntity:lstrcpy(Glb_AcadErrorInfo,_T("��һ��ʵ��"));break;
	case Acad::eCannotChangeActiveViewport:lstrcpy(Glb_AcadErrorInfo,_T("�����Ըı��ӿ�"));break;
	case Acad::eNotInPaperspace:lstrcpy(Glb_AcadErrorInfo,_T("����ͼֽ�ռ���"));break;
	case Acad::eCommandWasInProgress:lstrcpy(Glb_AcadErrorInfo,_T("����ִ������"));break;
	case Acad::eGeneralModelingFailure:lstrcpy(Glb_AcadErrorInfo,_T("����ģ��ʧ��"));break;
	case Acad::eOutOfRange:lstrcpy(Glb_AcadErrorInfo,_T("������Χ"));break;
	case Acad::eNonCoplanarGeometry:lstrcpy(Glb_AcadErrorInfo,_T("û��ƽ�漸�ζ���"));break;
	case Acad::eDegenerateGeometry:lstrcpy(Glb_AcadErrorInfo,_T("�˻��ļ��ζ���"));break;
	case Acad::eInvalidAxis:lstrcpy(Glb_AcadErrorInfo,_T("��Ч������"));break;
	case Acad::ePointNotOnEntity:lstrcpy(Glb_AcadErrorInfo,_T("�㲻��ʵ����"));break;
	case Acad::eSingularPoint:lstrcpy(Glb_AcadErrorInfo,_T("��һ�ĵ�"));break;
	case Acad::eInvalidOffset:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��ƫ��"));break;
	case Acad::eNonPlanarEntity:lstrcpy(Glb_AcadErrorInfo,_T("û��ƽ���ʵ��"));break;
	case Acad::eCannotExplodeEntity:lstrcpy(Glb_AcadErrorInfo,_T("���ɷֽ��ʵ��"));break;
	case Acad::eStringTooLong:lstrcpy(Glb_AcadErrorInfo,_T("�ַ���̫��"));break;
	case Acad::eInvalidSymTableFlag:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�ķ��Ż����־"));break;
	case Acad::eUndefinedLineType:lstrcpy(Glb_AcadErrorInfo,_T("û�ж��������"));break;
	case Acad::eInvalidTextStyle:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��������ʽ"));break;
	case Acad::eTooFewLineTypeElements:lstrcpy(Glb_AcadErrorInfo,_T("̫�ٵ�����Ҫ��"));break;
	case Acad::eTooManyLineTypeElements:lstrcpy(Glb_AcadErrorInfo,_T("̫�������Ҫ��"));break;
	case Acad::eExcessiveItemCount:lstrcpy(Glb_AcadErrorInfo,_T("�������Ŀ"));break;
	case Acad::eIgnoredLinetypeRedef:lstrcpy(Glb_AcadErrorInfo,_T("�������Ͷ�������"));break;
	case Acad::eBadUCS:lstrcpy(Glb_AcadErrorInfo,_T("���õ��û�����ϵ"));break;
	case Acad::eBadPaperspaceView:lstrcpy(Glb_AcadErrorInfo,_T("���õ�ͼֽ�ռ���ͼ"));break;
	case Acad::eSomeInputDataLeftUnread:lstrcpy(Glb_AcadErrorInfo,_T("һЩ��������δ����ȡ"));break;
	case Acad::eNoInternalSpace:lstrcpy(Glb_AcadErrorInfo,_T("�����ڲ��ռ�"));break;



		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
		//����:boitboy,��վ:http://www.cadgj.com (CAD����֮��),����ʵ��ObjectArx�����̳�,����,CADС����
		//���������ת��,ʹ��,�޸�,�ٷ���,û���κ�����.
		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
	case Acad::eInvalidDimStyle:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�ı�ע��ʽ"));break;
	case Acad::eInvalidLayer:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��ͼ��"));break;
	case Acad::eUserBreak:lstrcpy(Glb_AcadErrorInfo,_T("�û����"));break;
	case Acad::eDwgNeedsRecovery:lstrcpy(Glb_AcadErrorInfo,_T("DWG�ļ���Ҫ�޸�"));break;
	case Acad::eDeleteEntity:lstrcpy(Glb_AcadErrorInfo,_T("ɾ��ʵ��"));break;
	case Acad::eInvalidFix:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�ķ�λ"));break;
	case Acad::eFSMError:lstrcpy(Glb_AcadErrorInfo,_T("FSM����"));break;
	case Acad::eBadLayerName:lstrcpy(Glb_AcadErrorInfo,_T("���õ�ͼ������"));break;
	case Acad::eLayerGroupCodeMissing:lstrcpy(Glb_AcadErrorInfo,_T("ͼ�������붪ʧ"));break;
	case Acad::eBadColorIndex:lstrcpy(Glb_AcadErrorInfo,_T("���õ���ɫ������"));break;
	case Acad::eBadLinetypeName:lstrcpy(Glb_AcadErrorInfo,_T("���õ���������"));break;
	case Acad::eBadLinetypeScale:lstrcpy(Glb_AcadErrorInfo,_T("���õ��������ű���"));break;
	case Acad::eBadVisibilityValue:lstrcpy(Glb_AcadErrorInfo,_T("���õĿɼ���ֵ"));break;
	case Acad::eProperClassSeparatorExpected:lstrcpy(Glb_AcadErrorInfo,_T("������δ�ҵ�Ԥ�ڵķָ����(?)"));break;
	case Acad::eBadLineWeightValue:lstrcpy(Glb_AcadErrorInfo,_T("���õ��߿�ֵ"));break;
	case Acad::eBadColor:lstrcpy(Glb_AcadErrorInfo,_T("���õ���ɫ"));break;
	case Acad::ePagerError:lstrcpy(Glb_AcadErrorInfo,_T("ҳ�����"));break;
	case Acad::eOutOfPagerMemory:lstrcpy(Glb_AcadErrorInfo,_T("ҳ���ڴ治��"));break;
	case Acad::ePagerWriteError:lstrcpy(Glb_AcadErrorInfo,_T("ҳ�治��д"));break;
	case Acad::eWasNotForwarding:lstrcpy(Glb_AcadErrorInfo,_T("���Ǵٽ�(?)"));break;
	case Acad::eInvalidIdMap:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��ID�ֵ�"));break;
	case Acad::eInvalidOwnerObject:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��������"));break;
	case Acad::eOwnerNotSet:lstrcpy(Glb_AcadErrorInfo,_T("δ����������"));break;
	case Acad::eWrongSubentityType:lstrcpy(Glb_AcadErrorInfo,_T("������Ӷ�������"));break;
	case Acad::eTooManyVertices:lstrcpy(Glb_AcadErrorInfo,_T("̫��ڵ�"));break;
	case Acad::eTooFewVertices:lstrcpy(Glb_AcadErrorInfo,_T("̫�ٽڵ�"));break;
	case Acad::eNoActiveTransactions:lstrcpy(Glb_AcadErrorInfo,_T("���������"));break;
	case Acad::eNotTopTransaction:lstrcpy(Glb_AcadErrorInfo,_T("������������"));break;
	case Acad::eTransactionOpenWhileCommandEnded:lstrcpy(Glb_AcadErrorInfo,_T("�����������ʱ���(/��ʼ)����"));break;
	case Acad::eInProcessOfCommitting:lstrcpy(Glb_AcadErrorInfo,_T("���ύ����Ĺ�����"));break;
	case Acad::eNotNewlyCreated:lstrcpy(Glb_AcadErrorInfo,_T("�����´�����"));break;
	case Acad::eLongTransReferenceError:lstrcpy(Glb_AcadErrorInfo,_T("���������ô���"));break;
	case Acad::eNoWorkSet:lstrcpy(Glb_AcadErrorInfo,_T("û�й�����"));break;
	case Acad::eAlreadyInGroup:lstrcpy(Glb_AcadErrorInfo,_T("�Ѿ���������"));break;
	case Acad::eNotInGroup:lstrcpy(Glb_AcadErrorInfo,_T("��������"));break;
	case Acad::eInvalidREFIID:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��REFIID"));break;
	case Acad::eInvalidNormal:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�ı�׼"));break;
	case Acad::eInvalidStyle:lstrcpy(Glb_AcadErrorInfo,_T("��Ч����ʽ"));break;
	case Acad::eCannotRestoreFromAcisFile:lstrcpy(Glb_AcadErrorInfo,_T("�����Դ�Acis(?)�ļ��лָ�"));break;
	case Acad::eMakeMeProxy:lstrcpy(Glb_AcadErrorInfo,_T("���Ҵ���"));break;
	case Acad::eNLSFileNotAvailable:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��NLS�ļ�"));break;
	case Acad::eNotAllowedForThisProxy:lstrcpy(Glb_AcadErrorInfo,_T("�������������"));break;
	case Acad::eNotSupportedInDwgApi:lstrcpy(Glb_AcadErrorInfo,_T("��Dwg Api�в�֧��"));break;
	case Acad::ePolyWidthLost:lstrcpy(Glb_AcadErrorInfo,_T("����߿�ȶ�ʧ"));break;
	case Acad::eNullExtents:lstrcpy(Glb_AcadErrorInfo,_T("�յĿռ䷶Χ"));break;
		//case Acad::eExplodeAgain:lstrcpy(Glb_AcadErrorInfo,_T("��һ�ηֽ�"));break;
	case Acad::eBadDwgHeader:lstrcpy(Glb_AcadErrorInfo,_T("����DWG�ļ�ͷ"));break;
	case Acad::eLockViolation:lstrcpy(Glb_AcadErrorInfo,_T("����������ǰ����"));break;
	case Acad::eLockConflict:lstrcpy(Glb_AcadErrorInfo,_T("������ͻ"));break;
	case Acad::eDatabaseObjectsOpen:lstrcpy(Glb_AcadErrorInfo,_T("���ݿ�����"));break;



		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
		//����:boitboy,��վ:http://www.cadgj.com (CAD����֮��),����ʵ��ObjectArx�����̳�,����,CADС����
		//���������ת��,ʹ��,�޸�,�ٷ���,û���κ�����.
		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
	case Acad::eLockChangeInProgress:lstrcpy(Glb_AcadErrorInfo,_T("�����ı���"));break;
	case Acad::eVetoed:lstrcpy(Glb_AcadErrorInfo,_T("��ֹ"));break;
	case Acad::eNoDocument:lstrcpy(Glb_AcadErrorInfo,_T("û���ĵ�"));break;
	case Acad::eNotFromThisDocument:lstrcpy(Glb_AcadErrorInfo,_T("���Ǵ�����ĵ�"));break;
	case Acad::eLISPActive:lstrcpy(Glb_AcadErrorInfo,_T("LISP�"));break;
	case Acad::eTargetDocNotQuiescent:lstrcpy(Glb_AcadErrorInfo,_T("Ŀ���ĵ����"));break;
	case Acad::eDocumentSwitchDisabled:lstrcpy(Glb_AcadErrorInfo,_T("��ֹ�ĵ�ת��"));break;
	case Acad::eInvalidContext:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�������Ļ���"));break;
	case Acad::eCreateFailed:lstrcpy(Glb_AcadErrorInfo,_T("����ʧ��"));break;
	case Acad::eCreateInvalidName:lstrcpy(Glb_AcadErrorInfo,_T("������Ч����"));break;
	case Acad::eSetFailed:lstrcpy(Glb_AcadErrorInfo,_T("����ʧ��"));break;
	case Acad::eDelDoesNotExist:lstrcpy(Glb_AcadErrorInfo,_T("ɾ�����󲻴���"));break;
	case Acad::eDelIsModelSpace:lstrcpy(Glb_AcadErrorInfo,_T("ɾ��ģ�Ϳռ�"));break;
	case Acad::eDelLastLayout:lstrcpy(Glb_AcadErrorInfo,_T("ɾ�����һ������"));break;
	case Acad::eDelUnableToSetCurrent:lstrcpy(Glb_AcadErrorInfo,_T("ɾ�����޷����õ�ǰ����"));break;
	case Acad::eDelUnableToFind:lstrcpy(Glb_AcadErrorInfo,_T("û���ҵ�ɾ������"));break;
	case Acad::eRenameDoesNotExist:lstrcpy(Glb_AcadErrorInfo,_T("���������󲻴���"));break;
	case Acad::eRenameIsModelSpace:lstrcpy(Glb_AcadErrorInfo,_T("������������ģ�Ϳռ�"));break;
	case Acad::eRenameInvalidLayoutName:lstrcpy(Glb_AcadErrorInfo,_T("��������Ч�Ĳ�������"));break;
	case Acad::eRenameLayoutAlreadyExists:lstrcpy(Glb_AcadErrorInfo,_T("���������������Ѵ���"));break;
	case Acad::eRenameInvalidName:lstrcpy(Glb_AcadErrorInfo,_T("��������Ч����"));break;
	case Acad::eCopyDoesNotExist:lstrcpy(Glb_AcadErrorInfo,_T("����������"));break;
	case Acad::eCopyIsModelSpace:lstrcpy(Glb_AcadErrorInfo,_T("������ģ�Ϳռ�"));break;
	case Acad::eCopyFailed:lstrcpy(Glb_AcadErrorInfo,_T("����ʧ��"));break;
	case Acad::eCopyInvalidName:lstrcpy(Glb_AcadErrorInfo,_T("������Ч����"));break;
	case Acad::eCopyNameExists:lstrcpy(Glb_AcadErrorInfo,_T("�������ƴ���"));break;
	case Acad::eProfileDoesNotExist:lstrcpy(Glb_AcadErrorInfo,_T("�������Ʋ�����"));break;
	case Acad::eInvalidFileExtension:lstrcpy(Glb_AcadErrorInfo,_T("��Ч���ļ���׺����"));break;
	case Acad::eInvalidProfileName:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�������ļ�����"));break;
	case Acad::eFileExists:lstrcpy(Glb_AcadErrorInfo,_T("�ļ�����"));break;
	case Acad::eProfileIsInUse:lstrcpy(Glb_AcadErrorInfo,_T("�����ļ�����"));break;
	case Acad::eCantOpenFile:lstrcpy(Glb_AcadErrorInfo,_T("���ļ�ʧ��"));break;
	case Acad::eNoFileName:lstrcpy(Glb_AcadErrorInfo,_T("û���ļ�����"));break;
	case Acad::eRegistryAccessError:lstrcpy(Glb_AcadErrorInfo,_T("��ȡע������"));break;
	case Acad::eRegistryCreateError:lstrcpy(Glb_AcadErrorInfo,_T("����ע��������"));break;
	case Acad::eBadDxfFile:lstrcpy(Glb_AcadErrorInfo,_T("����DXF�ļ�"));break;
	case Acad::eUnknownDxfFileFormat:lstrcpy(Glb_AcadErrorInfo,_T("δ֪��DXF�ļ���ʽ"));break;
	case Acad::eMissingDxfSection:lstrcpy(Glb_AcadErrorInfo,_T("��ʧDXF�ֶ�"));break;
	case Acad::eInvalidDxfSectionName:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��DXF�ֶ�����"));break;
	case Acad::eNotDxfHeaderGroupCode:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��DXF����"));break;
	case Acad::eUndefinedDxfGroupCode:lstrcpy(Glb_AcadErrorInfo,_T("û�ж���DXF����"));break;
	case Acad::eNotInitializedYet:lstrcpy(Glb_AcadErrorInfo,_T("û�г�ʼ��"));break;
	case Acad::eInvalidDxf2dPoint:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��DXF��ά��"));break;
	case Acad::eInvalidDxf3dPoint:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��DXD��ά��"));break;
	case Acad::eBadlyNestedAppData:lstrcpy(Glb_AcadErrorInfo,_T("����Ƕ��Ӧ�ó�������"));break;
	case Acad::eIncompleteBlockDefinition:lstrcpy(Glb_AcadErrorInfo,_T("�������Ŀ鶨��"));break;
	case Acad::eIncompleteComplexObject:lstrcpy(Glb_AcadErrorInfo,_T("�������ĺϳ�(?����)����"));break;
	case Acad::eBlockDefInEntitySection:lstrcpy(Glb_AcadErrorInfo,_T("�鶨����ʵ�����"));break;
	case Acad::eNoBlockBegin:lstrcpy(Glb_AcadErrorInfo,_T("û�п鿪ʼ"));break;
	case Acad::eDuplicateLayerName:lstrcpy(Glb_AcadErrorInfo,_T("�ظ���ͼ������"));break;



		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
		//����:boitboy,��վ:http://www.cadgj.com (CAD����֮��),����ʵ��ObjectArx�����̳�,����,CADС����
		//���������ת��,ʹ��,�޸�,�ٷ���,û���κ�����.
		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
	case Acad::eBadPlotStyleName:lstrcpy(Glb_AcadErrorInfo,_T("���õĴ�ӡ��ʽ����"));break;
	case Acad::eDuplicateBlockName:lstrcpy(Glb_AcadErrorInfo,_T("�ظ��Ŀ�����"));break;
	case Acad::eBadPlotStyleType:lstrcpy(Glb_AcadErrorInfo,_T("���õĴ�ӡ��ʽ����"));break;
	case Acad::eBadPlotStyleNameHandle:lstrcpy(Glb_AcadErrorInfo,_T("���õĴ�ӡ��ʽ���ƾ��"));break;
	case Acad::eUndefineShapeName:lstrcpy(Glb_AcadErrorInfo,_T("û�ж�����״����"));break;
	case Acad::eDuplicateBlockDefinition:lstrcpy(Glb_AcadErrorInfo,_T("�ظ��Ŀ鶨��"));break;
	case Acad::eMissingBlockName:lstrcpy(Glb_AcadErrorInfo,_T("��ʧ�˿�����"));break;
	case Acad::eBinaryDataSizeExceeded:lstrcpy(Glb_AcadErrorInfo,_T("���������ݳ���̫��"));break;
	case Acad::eObjectIsReferenced:lstrcpy(Glb_AcadErrorInfo,_T("��������"));break;
	case Acad::eNoThumbnailBitmap:lstrcpy(Glb_AcadErrorInfo,_T("û������ͼ"));break;
	case Acad::eGuidNoAddress:lstrcpy(Glb_AcadErrorInfo,_T("δ�ҵ�GUID��ַ"));break;
	case Acad::eMustBe0to2:lstrcpy(Glb_AcadErrorInfo,_T("������0��2"));break;
	case Acad::eMustBe0to3:lstrcpy(Glb_AcadErrorInfo,_T("������0��3"));break;
	case Acad::eMustBe0to4:lstrcpy(Glb_AcadErrorInfo,_T("������0��4"));break;
	case Acad::eMustBe0to5:lstrcpy(Glb_AcadErrorInfo,_T("������0��5"));break;
	case Acad::eMustBe0to8:lstrcpy(Glb_AcadErrorInfo,_T("������0��8"));break;
	case Acad::eMustBe1to8:lstrcpy(Glb_AcadErrorInfo,_T("������1��8"));break;
	case Acad::eMustBe1to15:lstrcpy(Glb_AcadErrorInfo,_T("������1��15"));break;
	case Acad::eMustBePositive:lstrcpy(Glb_AcadErrorInfo,_T("����Ϊ����"));break;
	case Acad::eMustBeNonNegative:lstrcpy(Glb_AcadErrorInfo,_T("����Ϊ�Ǹ���"));break;
	case Acad::eMustBeNonZero:lstrcpy(Glb_AcadErrorInfo,_T("�����Ե���0"));break;
	case Acad::eMustBe1to6:lstrcpy(Glb_AcadErrorInfo,_T("������1��6"));break;
	case Acad::eNoPlotStyleTranslationTable:lstrcpy(Glb_AcadErrorInfo,_T("û�д�ӡ��ʽ�����(?)"));break;
	case Acad::ePlotStyleInColorDependentMode:lstrcpy(Glb_AcadErrorInfo,_T("��ӡ��ʽ������ɫ"));break;
	case Acad::eMaxLayouts:lstrcpy(Glb_AcadErrorInfo,_T("��󲼾�����"));break;
	case Acad::eNoClassId:lstrcpy(Glb_AcadErrorInfo,_T("û����ID"));break;
	case Acad::eUndoOperationNotAvailable:lstrcpy(Glb_AcadErrorInfo,_T("����������Ч"));break;
	case Acad::eUndoNoGroupBegin:lstrcpy(Glb_AcadErrorInfo,_T("��������û���鿪ʼ"));break;
	case Acad::eHatchTooDense:lstrcpy(Glb_AcadErrorInfo,_T("���̫�ܼ�"));break;
	case Acad::eOpenFileCancelled:lstrcpy(Glb_AcadErrorInfo,_T("���ļ�ȡ��"));break;
	case Acad::eNotHandled:lstrcpy(Glb_AcadErrorInfo,_T("û�д���"));break;
	case Acad::eMakeMeProxyAndResurrect:lstrcpy(Glb_AcadErrorInfo,_T("���Լ���ɴ���Ȼ�󸴻�"));break;
	case Acad::eFileMissingSections:lstrcpy(Glb_AcadErrorInfo,_T("�ļ���ʧ�ֶ�"));break;
	case Acad::eRepeatedDwgRead:lstrcpy(Glb_AcadErrorInfo,_T("�ظ��Ķ�ȡDWG�ļ�"));break;
	case Acad::eWrongCellType:lstrcpy(Glb_AcadErrorInfo,_T("����ĵ�Ԫ������"));break;
	case Acad::eCannotChangeColumnType:lstrcpy(Glb_AcadErrorInfo,_T("�����Ըı�������"));break;
	case Acad::eRowsMustMatchColumns:lstrcpy(Glb_AcadErrorInfo,_T("�б���ƥ����"));break;
	case Acad::eFileSharingViolation:lstrcpy(Glb_AcadErrorInfo,_T("�ļ��������"));break;
	case Acad::eUnsupportedFileFormat:lstrcpy(Glb_AcadErrorInfo,_T("��֧�ֵ��ļ���ʽ"));break;
	case Acad::eObsoleteFileFormat:lstrcpy(Glb_AcadErrorInfo,_T("�������ļ���ʽ"));break;
	case Acad::eDwgShareDemandLoad:lstrcpy(Glb_AcadErrorInfo,_T("DWG����Ҫ�����(?)"));break;
	case Acad::eDwgShareReadAccess:lstrcpy(Glb_AcadErrorInfo,_T("DWG�����ȡ"));break;
	case Acad::eDwgShareWriteAccess:lstrcpy(Glb_AcadErrorInfo,_T("DWG����д��"));break;
	case Acad::eLoadFailed:lstrcpy(Glb_AcadErrorInfo,_T("����ʧ��"));break;
	case Acad::eDeviceNotFound:lstrcpy(Glb_AcadErrorInfo,_T("����δ�ҵ�"));break;
	case Acad::eNoCurrentConfig:lstrcpy(Glb_AcadErrorInfo,_T("û�е�ǰ����"));break;
	case Acad::eNullPtr:lstrcpy(Glb_AcadErrorInfo,_T("��ָ��"));break;
	case Acad::eNoLayout:lstrcpy(Glb_AcadErrorInfo,_T("û�в���"));break;
	case Acad::eIncompatiblePlotSettings:lstrcpy(Glb_AcadErrorInfo,_T("�����ݵĴ�ӡ����"));break;
	case Acad::eNonePlotDevice:lstrcpy(Glb_AcadErrorInfo,_T("û�д�ӡ����"));break;



		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
		//����:boitboy,��վ:http://www.cadgj.com (CAD����֮��),����ʵ��ObjectArx�����̳�,����,CADС����
		//���������ת��,ʹ��,�޸�,�ٷ���,û���κ�����.
		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
	case Acad::eNoMatchingMedia:lstrcpy(Glb_AcadErrorInfo,_T("û��ƥ��Ĵ�ӡ�ߴ�"));break;
	case Acad::eInvalidView:lstrcpy(Glb_AcadErrorInfo,_T("��Ч����ͼ"));break;
	case Acad::eInvalidWindowArea:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�Ĵ��ڷ�Χ"));break;
	case Acad::eInvalidPlotArea:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�Ĵ�ӡ��Χ"));break;
	case Acad::eCustomSizeNotPossible:lstrcpy(Glb_AcadErrorInfo,_T("�û�����Ĵ�ӡ�ߴ粻���ܴ���"));break;
	case Acad::ePageCancelled:lstrcpy(Glb_AcadErrorInfo,_T("ֽ��ȡ��"));break;
	case Acad::ePlotCancelled:lstrcpy(Glb_AcadErrorInfo,_T("��ӡȡ��"));break;
	case Acad::eInvalidEngineState:lstrcpy(Glb_AcadErrorInfo,_T("��Ч������״̬"));break;
	case Acad::ePlotAlreadyStarted:lstrcpy(Glb_AcadErrorInfo,_T("�Ѿ���ʼ�ڴ�ӡ��"));break;
	case Acad::eNoErrorHandler:lstrcpy(Glb_AcadErrorInfo,_T("û�д�����"));break;
	case Acad::eInvalidPlotInfo:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�Ĵ�ӡ��Ϣ"));break;
	case Acad::eNumberOfCopiesNotSupported:lstrcpy(Glb_AcadErrorInfo,_T("��֧�ִ�ӡ����"));break;
	case Acad::eLayoutNotCurrent:lstrcpy(Glb_AcadErrorInfo,_T("���ǵ�ǰ����"));break;
	case Acad::eGraphicsNotGenerated:lstrcpy(Glb_AcadErrorInfo,_T("��ͼ���󴴽�ʧ��(?)"));break;
	case Acad::eCannotPlotToFile:lstrcpy(Glb_AcadErrorInfo,_T("�����Դ�ӡ���ļ�"));break;
	case Acad::eMustPlotToFile:lstrcpy(Glb_AcadErrorInfo,_T("�����ӡ���ļ�"));break;
	case Acad::eNotMultiPageCapable:lstrcpy(Glb_AcadErrorInfo,_T("��֧�ֶ���ֽ��"));break;
	case Acad::eBackgroundPlotInProgress:lstrcpy(Glb_AcadErrorInfo,_T("���ں�̨��ӡ"));break;
	case Acad::eSubSelectionSetEmpty:lstrcpy(Glb_AcadErrorInfo,_T("��ѡ�񼯱�����Ϊ��"));break;
	case Acad::eInvalidObjectId:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�Ķ���ID���߶���ID���ڵ�ǰ���ݿ�"));break;
	case Acad::eInvalidXrefObjectId:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��XREF����ID����XREF����ID���ڵ�ǰ���ݿ�"));break;
	case Acad::eNoViewAssociation:lstrcpy(Glb_AcadErrorInfo,_T("δ�ҵ���Ӧ����ͼ����"));break;
	case Acad::eNoLabelBlock:lstrcpy(Glb_AcadErrorInfo,_T("�ӿ�δ�ҵ������Ŀ�"));break;
	case Acad::eUnableToSetViewAssociation:lstrcpy(Glb_AcadErrorInfo,_T("������ͼ�����ӿ�ʧ��"));break;
	case Acad::eUnableToGetViewAssociation:lstrcpy(Glb_AcadErrorInfo,_T("�޷��ҵ���������ͼ"));break;
	case Acad::eUnableToSetLabelBlock:lstrcpy(Glb_AcadErrorInfo,_T("�޷����ù����Ŀ�"));break;
	case Acad::eUnableToGetLabelBlock:lstrcpy(Glb_AcadErrorInfo,_T("�޷���ȡ�����Ŀ�"));break;
	case Acad::eUnableToRemoveAssociation:lstrcpy(Glb_AcadErrorInfo,_T("�޷��Ƴ��ӿڹ�������"));break;
	case Acad::eUnableToSyncModelView:lstrcpy(Glb_AcadErrorInfo,_T("�޷�ͬ���ӿں�ģ�Ϳռ���ͼ"));break;
	case Acad::eSecInitializationFailure:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)��ʼ������"));break;
	case Acad::eSecErrorReadingFile:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)��ȡ�ļ�����"));break;
	case Acad::eSecErrorWritingFile:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)д���ļ�����"));break;
	case Acad::eSecInvalidDigitalID:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)��Ч������ID"));break;
	case Acad::eSecErrorGeneratingTimestamp:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)����ʱ�������"));break;
	case Acad::eSecErrorComputingSignature:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)����ǩ������"));break;
	case Acad::eSecErrorWritingSignature:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)д��ǩ������"));break;
	case Acad::eSecErrorEncryptingData:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)�������ݴ���"));break;
	case Acad::eSecErrorCipherNotSupported:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)��֧�ֵ�����"));break;
	case Acad::eSecErrorDecryptingData:lstrcpy(Glb_AcadErrorInfo,_T("SEC(?)�������ݴ���"));break;
		//case Acad::eInetBase:lstrcpy(Glb_AcadErrorInfo,_T("�������"));break;
	case Acad::eInetOk:lstrcpy(Glb_AcadErrorInfo,_T("��������"));break;
	case Acad::eInetInCache:lstrcpy(Glb_AcadErrorInfo,_T("�ڻ�������"));break;
	case Acad::eInetFileNotFound:lstrcpy(Glb_AcadErrorInfo,_T("�����ļ�������"));break;
	case Acad::eInetBadPath:lstrcpy(Glb_AcadErrorInfo,_T("���õ�����·��"));break;
	case Acad::eInetTooManyOpenFiles:lstrcpy(Glb_AcadErrorInfo,_T("��̫�������ļ�"));break;
	case Acad::eInetFileAccessDenied:lstrcpy(Glb_AcadErrorInfo,_T("�������ļ����ܾ�"));break;
	case Acad::eInetInvalidFileHandle:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�������ļ����"));break;
	case Acad::eInetDirectoryFull:lstrcpy(Glb_AcadErrorInfo,_T("�����ļ���Ŀ¼����"));break;
	case Acad::eInetHardwareError:lstrcpy(Glb_AcadErrorInfo,_T("����Ӳ������"));break;
	case Acad::eInetSharingViolation:lstrcpy(Glb_AcadErrorInfo,_T("Υ�����繲��"));break;



		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
		//����:boitboy,��վ:http://www.cadgj.com (CAD����֮��),����ʵ��ObjectArx�����̳�,����,CADС����
		//���������ת��,ʹ��,�޸�,�ٷ���,û���κ�����.
		///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
	case Acad::eInetDiskFull:lstrcpy(Glb_AcadErrorInfo,_T("����Ӳ������"));break;
	case Acad::eInetFileGenericError:lstrcpy(Glb_AcadErrorInfo,_T("�����ļ���������"));break;
	case Acad::eInetValidURL:lstrcpy(Glb_AcadErrorInfo,_T("��Ч��URL��ַ"));break;
	case Acad::eInetNotAnURL:lstrcpy(Glb_AcadErrorInfo,_T("����URL��ַ"));break;
	case Acad::eInetNoWinInet:lstrcpy(Glb_AcadErrorInfo,_T("û��WinInet(?)"));break;
	case Acad::eInetOldWinInet:lstrcpy(Glb_AcadErrorInfo,_T("�ɵ�WinInet(?)"));break;
	case Acad::eInetNoAcadInet:lstrcpy(Glb_AcadErrorInfo,_T("�޷�����ACAD��վ"));break;
	case Acad::eInetNotImplemented:lstrcpy(Glb_AcadErrorInfo,_T("�޷�Ӧ������"));break;
	case Acad::eInetProtocolNotSupported:lstrcpy(Glb_AcadErrorInfo,_T("����Э�鲻֧��"));break;
	case Acad::eInetCreateInternetSessionFailed:lstrcpy(Glb_AcadErrorInfo,_T("��������Ựʧ��"));break;
	case Acad::eInetInternetSessionConnectFailed:lstrcpy(Glb_AcadErrorInfo,_T("��������Ựʧ��"));break;
	case Acad::eInetInternetSessionOpenFailed:lstrcpy(Glb_AcadErrorInfo,_T("������Ựʧ��"));break;
	case Acad::eInetInvalidAccessType:lstrcpy(Glb_AcadErrorInfo,_T("��Ч�������������"));break;
	case Acad::eInetFileOpenFailed:lstrcpy(Glb_AcadErrorInfo,_T("�������ļ�ʧ��"));break;
	case Acad::eInetHttpOpenRequestFailed:lstrcpy(Glb_AcadErrorInfo,_T("��HTTPЭ��ʧ��"));break;
	case Acad::eInetUserCancelledTransfer:lstrcpy(Glb_AcadErrorInfo,_T("�û�ȡ�������紫��"));break;
	case Acad::eInetHttpBadRequest:lstrcpy(Glb_AcadErrorInfo,_T("���������������"));break;
	case Acad::eInetHttpAccessDenied:lstrcpy(Glb_AcadErrorInfo,_T("HTTPЭ��ܾ�"));break;
	case Acad::eInetHttpPaymentRequired:lstrcpy(Glb_AcadErrorInfo,_T("HTTPЭ��Ҫ�󸶷�"));break;
	case Acad::eInetHttpRequestForbidden:lstrcpy(Glb_AcadErrorInfo,_T("��ֹHTTP����"));break;
	case Acad::eInetHttpObjectNotFound:lstrcpy(Glb_AcadErrorInfo,_T("HTTP����δ�ҵ�"));break;
	case Acad::eInetHttpBadMethod:lstrcpy(Glb_AcadErrorInfo,_T("�������HTTP���󷽷�"));break;
	case Acad::eInetHttpNoAcceptableResponse:lstrcpy(Glb_AcadErrorInfo,_T("�����ܵ�HTTP�ظ�"));break;
	case Acad::eInetHttpProxyAuthorizationRequired:lstrcpy(Glb_AcadErrorInfo,_T("Ҫ��HTTP������Ȩ"));break;
	case Acad::eInetHttpTimedOut:lstrcpy(Glb_AcadErrorInfo,_T("HTTP��ʱ"));break;
	case Acad::eInetHttpConflict:lstrcpy(Glb_AcadErrorInfo,_T("HTTP��ͻ"));break;
	case Acad::eInetHttpResourceGone:lstrcpy(Glb_AcadErrorInfo,_T("������Դ���ù�"));break;
	case Acad::eInetHttpLengthRequired:lstrcpy(Glb_AcadErrorInfo,_T("HTTP���󳤶��Ǳ����"));break;
	case Acad::eInetHttpPreconditionFailure:lstrcpy(Glb_AcadErrorInfo,_T("HTTPԤ����ʧ��"));break;
	case Acad::eInetHttpRequestTooLarge:lstrcpy(Glb_AcadErrorInfo,_T("HTTP����̫��"));break;
	case Acad::eInetHttpUriTooLong:lstrcpy(Glb_AcadErrorInfo,_T("URL��ַ̫��"));break;
	case Acad::eInetHttpUnsupportedMedia:lstrcpy(Glb_AcadErrorInfo,_T("HTTP��֧�ֵ�ý��"));break;
	case Acad::eInetHttpServerError:lstrcpy(Glb_AcadErrorInfo,_T("HTTP����������"));break;
	case Acad::eInetHttpNotSupported:lstrcpy(Glb_AcadErrorInfo,_T("HTTP��֧��"));break;
	case Acad::eInetHttpBadGateway:lstrcpy(Glb_AcadErrorInfo,_T("HTTP���ش���"));break;
	case Acad::eInetHttpServiceUnavailable:lstrcpy(Glb_AcadErrorInfo,_T("HTTP����ǰ������"));break;
	case Acad::eInetHttpGatewayTimeout:lstrcpy(Glb_AcadErrorInfo,_T("HTTP���س�ʱ"));break;
	case Acad::eInetHttpVersionNotSupported:lstrcpy(Glb_AcadErrorInfo,_T("HTTP�汾��֧��"));break;
	case Acad::eInetInternetError:lstrcpy(Glb_AcadErrorInfo,_T("HTTP�������"));break;
	case Acad::eInetGenericException:lstrcpy(Glb_AcadErrorInfo,_T("HTTP�����쳣"));break;
	case Acad::eInetUnknownError:lstrcpy(Glb_AcadErrorInfo,_T("HTTPδ֪����"));break;
	case Acad::eAlreadyActive:lstrcpy(Glb_AcadErrorInfo,_T("�Ѿ��ǻ����"));break;
	case Acad::eAlreadyInactive:lstrcpy(Glb_AcadErrorInfo,_T("�Ѿ��ǲ������"));break;
	default:
		lstrcpy(Glb_AcadErrorInfo,acadErrorStatusText(es));
		//Unknown error
		replace(Glb_AcadErrorInfo,_T("Unknown error"),_T("δ֪����"));
		break;
	}
	return Glb_AcadErrorInfo;
}


///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
//����:boitboy,��վ:http://www.cadgj.com (CAD����֮��),����ʵ��ObjectArx�����̳�,����,CADС����
//���������ת��,ʹ��,�޸�,�ٷ���,û���κ�����.
//��ǰ�汾:V1.0.0
//��������:20130819
//��������Ӣ��ˮƽ���ޣ����ַ�����ܲ�׼ȷ����ӭ����ָ��
//QQ:245804736,EMail:wdzhangsl@126.com
///////////////���������ߵ������Ͷ�,�������ߵİ�Ȩ����////////////////////
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

		//i+nLen1��ԭ�����ַ����ĺ��沿�ֿ�ʼ
		if(nLen1>nLen2)
		{
			//�滻���ַ������nLen1-nLen2
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
			//�滻���ַ����䳤��nLen2-nLen1
			//�Ƚ��滻ʱռ�õ���λ���������
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