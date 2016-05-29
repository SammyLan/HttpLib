#pragma once

#define EXPORT_API __declspec(dllexport)
#define IMPORT_API __declspec(dllimport)
#pragma region macro define
#ifdef WYSDK_EXPORTS
#define WY_API EXPORT_API
#else 
#define WY_API IMPORT_API
#endif


#define MEMFUNC_DECL(type,name) virtual type get##name() const = 0
#define MEMFUNC_DECL_STR(name) virtual WYSTR get##name() const = 0
#define MEMFUNC_DECL_STRW(name) virtual WYSTRW get##name() const = 0

#define MEMFUNC_IMPL(type,name)											\
virtual type get##name##() const  override {return name##_;}			\
type name##_

#define MEMFUNC_IMPL_STR(name)											\
virtual WYSTR get##name##() const  override {return name##_##.c_str();}	\
string name##_

#define MEMFUNC_IMPL_STRW(name)											\
virtual WYSTRW get##name##() const  override {return name##_;}				\
CString name##_

#define MEMFUNC_ASSIGN(obj,p,name) obj.name##_  = p->##get##name()

typedef long long ll;
typedef char const * WYSTR;
typedef TCHAR const * WYSTRW;

#pragma endregion macro define

#pragma region type define
typedef ULONGLONG WYTASKID;

enum E_FILE_SIGN_FLAG
{
	E_FILE_SIGN_CRC = 0x00000001,			//256KCRC
	E_FILE_SIGN_SHA = 0x00000002,			//SHA
	E_FILE_SIGN_SHA_BLOCK = 0x00000004,		//512字节分块SHA
	E_FILE_SIGN_MD5 = 0x00000008,			//MD5
	E_FILE_SIGN_CRC_SHA = E_FILE_SIGN_CRC | E_FILE_SIGN_SHA | E_FILE_SIGN_SHA_BLOCK,
	E_FILE_SIGN_ALL = E_FILE_SIGN_CRC_SHA | E_FILE_SIGN_MD5
};

enum E_FinType
{
	E_FinType_None = 0,
	E_FinType_Complete = 1,
	E_FinType_Error = 2,
	E_FinType_Cancel = 3
};

#pragma endregion
#pragma region parameter define
struct IUploadDelegate
{
	virtual void uploadEnd(int errCode, TCHAR const * str) = 0;
	virtual void uploadProgress(ULONGLONG totalSize, ULONGLONG uploadSize, ULONGLONG speed) = 0;
};
typedef IUploadDelegate * PIUploadDelegate;

struct IFileSignInfo
{
	virtual void Release() const = 0;
	MEMFUNC_DECL(ULONGLONG,FileSize);	
	MEMFUNC_DECL(float, Speed);
	MEMFUNC_DECL(DWORD, ScanTime);
	MEMFUNC_DECL(UINT32, CRC);
	MEMFUNC_DECL_STR(SHA);
	MEMFUNC_DECL_STR(MD5);
};
typedef IFileSignInfo * PFileSignInfo;

struct IFileSignDelegate
{
	virtual void onFileSignBegin(ULONGLONG fileSize) = 0;
	virtual void onFileSignProgress(ULONGLONG fileSize, ULONGLONG completedSize,float spped) = 0;
	/*
	pInfo:如果该参数不在使用,需要调用release
	*/
	virtual void onFileSignEnd(int errCode,DWORD dwErrorStep, IFileSignInfo * pInfo) = 0;
};
typedef IFileSignDelegate * PIFileSignDelegate;


struct IInitInfo
{
	MEMFUNC_DECL(UINT32, connCount);
	MEMFUNC_DECL(size_t, MaxThread);
	MEMFUNC_DECL_STR(LocalPath);
	MEMFUNC_DECL_STR(OsType);
	MEMFUNC_DECL_STR(OsVersion);
	MEMFUNC_DECL_STR(AppVersion);
	MEMFUNC_DECL_STR(QUA);
	MEMFUNC_DECL_STR(ServerUrl);
};
typedef IInitInfo * PIInitInfo;

struct IUploadInfo
{
	//MEMFUNC_DECL(WYTASKID,TaskID);
	MEMFUNC_DECL(ll, Uin);
	MEMFUNC_DECL_STR(FileID);
	MEMFUNC_DECL_STR(CheckKey);
	MEMFUNC_DECL(PFileSignInfo, FileSignInfo);
	MEMFUNC_DECL_STR(ServerName);
	MEMFUNC_DECL_STR(ServerIP);
	MEMFUNC_DECL(int, ServerPort);
	MEMFUNC_DECL(int, ChannelCount);
	MEMFUNC_DECL_STRW(FilePath);
	MEMFUNC_DECL(PIUploadDelegate, Delegate);
};

#pragma endregion parameter define

struct IWYUploadSDK
{
	virtual WYTASKID addFileSignTask(TCHAR const * filePath, IFileSignDelegate * delegate) = 0;
	virtual bool stopFileSignTask(WYTASKID taskID) = 0;

	virtual WYTASKID createUploadTask(IUploadInfo * pInfo) = 0 ;
	virtual bool deleteXpUploadTask(WYTASKID taskID) = 0 ;
	virtual void setXpConnCount(unsigned int connCount) = 0;
};

WY_API IWYUploadSDK * CreateWYUploadSDK();
WY_API void DestoryWYUploadSDK(IWYUploadSDK * pSDK);

WY_API void WYSDK_Init(IInitInfo * pInfo);
WY_API void WYSDK_Uninit();
WY_API void WYSDK_SetProxy(WYSTRW strIP, WYSTRW port, WYSTRW strUserName, WYSTRW strPassword);
WY_API void WYSDK_SetIPConfig(WYSTR optimumIP, WYSTR backupIP);