#include "stdafx.h"
#include "FileSignTask.h"
#include <functional>
#include "UtilConvert.h"

static TCHAR LOGFILTER[] = _T("CFileSignTask");
static DWORD const CRC_FILE_SIZE	= 256 * 1024;
static size_t const SLICE_SISE = 512 * 1024; //512KB

CFileSignTask::CFileSignTask(WYTASKID taskID, CWYString const & strFile, CWYFileSignMgr* pMgr)
	:m_taskID(taskID)
	,m_strFile(strFile)
	,m_pMgr(pMgr)
{
	
}

CFileSignTask::~CFileSignTask()
{
	delete [] m_pBufferRead;
	delete [] m_pBufferCalcHash;
}

void CFileSignTask::BeginTask(boost::asio::io_service& io_service)
{
	if (!m_hFile.Create(io_service, m_strFile, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING))
	{
		OnFinish(GetLastError());
		return;
	}
	m_uFileSize = m_hFile.GetSize();
	if (m_uFileSize == 0)
	{
		OnFinish(0);
		return;
	}
	m_pBufferRead = new BYTE[SLICE_SISE];
	m_pBufferCalcHash = new BYTE[SLICE_SISE];
	readAt(m_i64CompleteSize, m_pBufferRead, SLICE_SISE);
}

void CFileSignTask::CancelTask()
{
	m_hFile.CancelIO();
	m_pMgr = nullptr;
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
	if (m_pMgr == nullptr)
	{
		//return;
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

}