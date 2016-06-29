#pragma once
#include <Include\WYTypeDef.h>
#include <WYSDK\IWYUploadSDK.h>
#include <File\AsyncFile.h>
#include <vector>
#include <string>
#include <sha1\sha1.h>
#include "lib\crc32.h"

class CWYFileSignMgr;
class CFileSignTask
	:public std::enable_shared_from_this<CFileSignTask>
	,public boost::noncopyable
{
	friend class CWYFileSignMgr;
public:
	CFileSignTask(WYTASKID taskID, CWYString const & strFile, CWYFileSignMgr* pMgr, IFileSignDelegate*		pCallback);
	~CFileSignTask();	
	void	BeginTask(boost::asio::io_service& io_service);
	void	CancelTask();
private:
	void OnFinish(int iError,bool bCancel = false);
	void handler(
		const boost::system::error_code& error, // Result of operation.
		std::size_t bytes_transferred           // Number of bytes read.
		);
	void readAt(uint64_t const offset, BYTE * pBuff, size_t const size);
private:
	WYTASKID const			m_taskID;	
	CWYString const			m_strFile;	
	CWYFileSignMgr *		m_pMgr;
	IFileSignDelegate*		m_pCallback;
	BOOL					m_bRunning = FALSE;
	WY::File::AsioFilePtr	m_pFile;
	uint64_t				m_uFileSize = 0;
	uint64_t				m_i64CompleteSize = 0;
	uint64_t				m_i64PreCompleteSize;
	UINT32					m_validateCRC = 0;
	CSHA1					m_oSha1;
	std::vector<std::string> m_oShaList;
	string					m_strSHA;
	DWORD					m_dwBeginTime = 0;

	BYTE *		m_pBufferRead = nullptr;
	BYTE *		m_pBufferCalcHash = nullptr;
	DWORD		m_uPreTime = 0;
};
typedef std::shared_ptr<CFileSignTask> CFileSignTaskPtr;