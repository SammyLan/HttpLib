#pragma once

CWYStringA ByteToStringA(const BYTE* pBuffer, INT nLen);

BOOL StringToByte(const CWYString& strVal, BYTE* pBuffer, INT nLen);
