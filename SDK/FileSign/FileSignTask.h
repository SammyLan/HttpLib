#pragma once
#include <Include\WYTypeDef.h>
#include <IWYUploadSDK.h>
#include <File\AsyncFile.h>
#include <vector>
#include <string>
#include <sha1\sha1.h>
#include "lib\crc32.h"

class CWYFileSignMgr;
class CFileSignTask:public std::enable_shared_from_this<CFileSignTask>
{
	friend class CWYFileSignMgr;
public:
	CFileSignTask(WYTASKID taskID, CWYString const & strFile, CWYFileSignMgr* pMgr);
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

	CAsyncFile				m_hFile;
	uint64_t				m_uFileSize;
	uint64_t				m_i64CompleteSize = 0;
	
	UINT32					m_validateCRC = 0;
	CSHA1					m_oSha1;
	std::vector<std::string> m_oShaList;
	DWORD					m_dwBeginTime = 0;

	BYTE *		m_pBufferRead = nullptr;
	BYTE *		m_pBufferCalcHash = nullptr;
};
typedef std::shared_ptr<CFileSignTask> CFileSignTaskPtr;