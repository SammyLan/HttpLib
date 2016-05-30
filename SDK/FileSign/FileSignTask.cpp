#include "stdafx.h"
#include "FileSignTask.h"
#include <functional>
#include "UtilConvert.h"
#include "NotifyModel\EventNotifyMgr.h"
#include "FileSignMgr.h"

static TCHAR LOGFILTER[] = _T("CFileSignTask");
static DWORD const CRC_FILE_SIZE	= 256 * 1024;
static size_t const SLICE_SISE = 512 * 1024; //512KB
static size_t const UpdateSpeedTimeInterval = 1000;

CFileSignTask::CFileSignTask(WYTASKID taskID, CWYString const & strFile, CWYFileSignMgr* pMgr, IFileSignDelegate * pCallback)
	:m_taskID(taskID)
	,m_strFile(strFile)
	,m_pMgr(pMgr)
	,m_pCallback(pCallback)
{
	
}

CFileSignTask::~CFileSignTask()
{
	delete [] m_pBufferRead;
	delete [] m_pBufferCalcHash;
}

void CFileSignTask::BeginTask(boost::asio::io_service& io_service)
{
	m_bRunning = TRUE;
	m_dwBeginTime = ::GetTickCount();
	if (!m_hFile.Create(io_service, m_strFile, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING))
	{
		int iError = GetLastError();
		LogError(LOGFILTER, _T("创建文件失败,错误码:%i,文件名:%s"), iError, (LPCTSTR)m_strFile);
		OnFinish(iError);
		return;
	}
	m_uFileSize = m_hFile.GetSize();
	if (m_uFileSize == 0)
	{
		LogFinal(LOGFILTER, _T("文件大小为0,文件名:%s"), (LPCTSTR)m_strFile);
		OnFinish(0);
		return;
	}
	EventNotifyMgr::instance()->PostEvent(NewTask(&CWYFileSignMgr::OnBegin, m_pMgr, m_taskID));
	m_uPreTime = ::GetTickCount();
	m_pBufferRead = new BYTE[SLICE_SISE];
	m_pBufferCalcHash = new BYTE[SLICE_SISE];
	readAt(m_i64CompleteSize, m_pBufferRead, SLICE_SISE);
}

void CFileSignTask::CancelTask()
{
	m_hFile.CancelIO();
	m_pCallback = nullptr;
}

void CFileSignTask::readAt(uint64_t const offset, BYTE * pBuff, size_t const size)
{
	auto & asioFile = m_hFile.getAsioFile();
	return asioFile.async_read_some_at(offset, boost::asio::buffer((void*)pBuff, size), std::bind(&CFileSignTask::handler, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void CFileSignTask::handler(
	const boost::system::error_code& error, // Result of operation.
	std::size_t bytes_transferred           // Number of bytes read.
	)
{
	if (m_pCallback == nullptr)
	{
		return;
	}

	int iError = error.value();
	if (iError == boost::system::errc::success)
	{
		std::swap(m_pBufferRead, m_pBufferCalcHash);
		m_i64CompleteSize += bytes_transferred;
		readAt(m_i64CompleteSize, m_pBufferRead, SLICE_SISE);
		if (m_i64CompleteSize <= SLICE_SISE)
		{
			int crcSize = min(CRC_FILE_SIZE, bytes_transferred);
			m_validateCRC = crc32(m_validateCRC, m_pBufferCalcHash, crcSize);
		}
		m_oSha1.Update(m_pBufferCalcHash, bytes_transferred);
		if (m_i64CompleteSize <= m_uFileSize)
		{
			char sha[20];
			unsigned long long tmp;
			m_oSha1.ReportTempHash(sha, tmp);
			std::string strSHA = ByteToStringA((BYTE*)sha, 20);
			m_oShaList.push_back(strSHA);
		}

		DWORD dwCur = ::GetTickCount();
		if (dwCur - m_uPreTime > UpdateSpeedTimeInterval)
		{
			EventNotifyMgr::instance()->PostEvent(NewTask(&CWYFileSignMgr::OnProgress, m_pMgr, m_taskID
				, m_uFileSize, m_i64CompleteSize, (float)((m_i64CompleteSize - m_i64PreCompleteSize) * 1000) / (dwCur - m_uPreTime) / 1024 / 1024));

			m_uPreTime = dwCur;
			m_i64PreCompleteSize = m_i64CompleteSize;
		}
	}
	else if(iError == boost::asio::error::misc_errors::eof)
	{
		OnFinish(0);
	}
	else
	{
		auto iError = error.value();
		auto desc = error.message();
	}
}

void CFileSignTask::OnFinish(int iError, bool bCancel)
{
	if (bCancel)
	{
		LogFinal(LOGFILTER, _T("取消扫描,文件名:%s"), (LPCTSTR)m_strFile);
		return;
	}
	auto total = ::GetTickCount() - m_dwBeginTime;
	if (iError == 0)
	{
		char sha[20];
		m_oSha1.Final();
		m_oSha1.GetHash((BYTE*)sha);
		m_strSHA.assign(sha, 20);
		string strSHA = ByteToStringA((BYTE*)sha, 20);
		m_oShaList.push_back(strSHA);
	}
	else
	{
		LogFinal(LOGFILTER, _T("扫描出错,错误码:%i,文件名:%s"),iError, (LPCTSTR)m_strFile);
	}
	EventNotifyMgr::instance()->PostEvent(NewTask(&CWYFileSignMgr::OnFinish, m_pMgr, m_taskID,iError,total));
}