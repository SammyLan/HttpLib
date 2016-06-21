#pragma once
#include <atlcom.h>
namespace WY
{
	typedef CComAutoCriticalSection		CWYLock;
	typedef CComCritSecLock<CWYLock>	LockGuard;

	typedef  CAtlStringA				CWYStringA;
	typedef  CAtlStringW				CWYStringW;
	typedef  CAtlString					CWYString;
	typedef  int						TaskID;
}
using namespace WY;
#define SHAREED_PTR_DEF(className)	typedef std::shared_ptr<className> className##Ptr