/*
	100% free public domain implementation of the SHA-1 algorithm
	by Dominik Reichl <dominik.reichl@t-online.de>
	Web: http://www.dominik-reichl.de/

	Version 1.6 - 2005-02-07 (thanks to Howard Kapustein for patches)
	- You can set the endianness in your files, no need to modify the
	  header file of the CSHA1 class any more
	- Aligned data support
	- Made support/compilation of the utility functions (ReportHash
	  and HashFile) optional (useful, if bytes count, for example in
	  embedded environments)

	Version 1.5 - 2005-01-01
	- 64-bit compiler compatibility added
	- Made variable wiping optional (define SHA1_WIPE_VARIABLES)
	- Removed unnecessary variable initializations
	- ROL32 improvement for the Microsoft compiler (using _rotl)

	======== Test Vectors (from FIPS PUB 180-1) ========

	SHA1("abc") =
		A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D

	SHA1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") =
		84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1

	SHA1(A million repetitions of "a") =
		34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
*/

#include "SHA1.h"
#ifdef WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#define SHA1_MAX_FILE_BUFFER 8000

// Rotate x bits to the left
#ifndef ROL32
#ifdef _MSC_VER
#define ROL32(_val32, _nBits) _rotl(_val32, _nBits)
#else
#define ROL32(_val32, _nBits) (((_val32)<<(_nBits))|((_val32)>>(32-(_nBits))))
#endif
#endif

#ifdef SHA1_LITTLE_ENDIAN
#define SHABLK0(i) (m_block->l[i] = \
	(ROL32(m_block->l[i],24) & 0xFF00FF00) | (ROL32(m_block->l[i],8) & 0x00FF00FF))
#else
#define SHABLK0(i) (m_block->l[i])
#endif

#define SHABLK(i) (m_block->l[i&15] = ROL32(m_block->l[(i+13)&15] ^ m_block->l[(i+8)&15] \
	^ m_block->l[(i+2)&15] ^ m_block->l[i&15],1))

// SHA-1 rounds
#define _R0(v,w,x,y,z,i) { z+=((w&(x^y))^y)+SHABLK0(i)+0x5A827999+ROL32(v,5); w=ROL32(w,30); }
#define _R1(v,w,x,y,z,i) { z+=((w&(x^y))^y)+SHABLK(i)+0x5A827999+ROL32(v,5); w=ROL32(w,30); }
#define _R2(v,w,x,y,z,i) { z+=(w^x^y)+SHABLK(i)+0x6ED9EBA1+ROL32(v,5); w=ROL32(w,30); }
#define _R3(v,w,x,y,z,i) { z+=(((w|x)&y)|(w&x))+SHABLK(i)+0x8F1BBCDC+ROL32(v,5); w=ROL32(w,30); }
#define _R4(v,w,x,y,z,i) { z+=(w^x^y)+SHABLK(i)+0xCA62C1D6+ROL32(v,5); w=ROL32(w,30); }

CSHA1::CSHA1()
{
	m_block = (SHA1_WORKSPACE_BLOCK *)m_workspace;

	Reset();
}

CSHA1::~CSHA1()
{
	Reset();
}

void CSHA1::Reset()
{
	// SHA1 initialization constants
	m_state[0] = 0x67452301;
	m_state[1] = 0xEFCDAB89;
	m_state[2] = 0x98BADCFE;
	m_state[3] = 0x10325476;
	m_state[4] = 0xC3D2E1F0;

	m_count[0] = 0;
	m_count[1] = 0;
}

void CSHA1::Transform(UINT_32 *state, UINT_8 *buffer)
{
	// Copy state[] to working vars
	UINT_32 a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];

	memcpy(m_block, buffer, 64);

	// 4 rounds of 20 operations each. Loop unrolled.
	_R0(a,b,c,d,e, 0); _R0(e,a,b,c,d, 1); _R0(d,e,a,b,c, 2); _R0(c,d,e,a,b, 3);
	_R0(b,c,d,e,a, 4); _R0(a,b,c,d,e, 5); _R0(e,a,b,c,d, 6); _R0(d,e,a,b,c, 7);
	_R0(c,d,e,a,b, 8); _R0(b,c,d,e,a, 9); _R0(a,b,c,d,e,10); _R0(e,a,b,c,d,11);
	_R0(d,e,a,b,c,12); _R0(c,d,e,a,b,13); _R0(b,c,d,e,a,14); _R0(a,b,c,d,e,15);
	_R1(e,a,b,c,d,16); _R1(d,e,a,b,c,17); _R1(c,d,e,a,b,18); _R1(b,c,d,e,a,19);
	_R2(a,b,c,d,e,20); _R2(e,a,b,c,d,21); _R2(d,e,a,b,c,22); _R2(c,d,e,a,b,23);
	_R2(b,c,d,e,a,24); _R2(a,b,c,d,e,25); _R2(e,a,b,c,d,26); _R2(d,e,a,b,c,27);
	_R2(c,d,e,a,b,28); _R2(b,c,d,e,a,29); _R2(a,b,c,d,e,30); _R2(e,a,b,c,d,31);
	_R2(d,e,a,b,c,32); _R2(c,d,e,a,b,33); _R2(b,c,d,e,a,34); _R2(a,b,c,d,e,35);
	_R2(e,a,b,c,d,36); _R2(d,e,a,b,c,37); _R2(c,d,e,a,b,38); _R2(b,c,d,e,a,39);
	_R3(a,b,c,d,e,40); _R3(e,a,b,c,d,41); _R3(d,e,a,b,c,42); _R3(c,d,e,a,b,43);
	_R3(b,c,d,e,a,44); _R3(a,b,c,d,e,45); _R3(e,a,b,c,d,46); _R3(d,e,a,b,c,47);
	_R3(c,d,e,a,b,48); _R3(b,c,d,e,a,49); _R3(a,b,c,d,e,50); _R3(e,a,b,c,d,51);
	_R3(d,e,a,b,c,52); _R3(c,d,e,a,b,53); _R3(b,c,d,e,a,54); _R3(a,b,c,d,e,55);
	_R3(e,a,b,c,d,56); _R3(d,e,a,b,c,57); _R3(c,d,e,a,b,58); _R3(b,c,d,e,a,59);
	_R4(a,b,c,d,e,60); _R4(e,a,b,c,d,61); _R4(d,e,a,b,c,62); _R4(c,d,e,a,b,63);
	_R4(b,c,d,e,a,64); _R4(a,b,c,d,e,65); _R4(e,a,b,c,d,66); _R4(d,e,a,b,c,67);
	_R4(c,d,e,a,b,68); _R4(b,c,d,e,a,69); _R4(a,b,c,d,e,70); _R4(e,a,b,c,d,71);
	_R4(d,e,a,b,c,72); _R4(c,d,e,a,b,73); _R4(b,c,d,e,a,74); _R4(a,b,c,d,e,75);
	_R4(e,a,b,c,d,76); _R4(d,e,a,b,c,77); _R4(c,d,e,a,b,78); _R4(b,c,d,e,a,79);

	// Add the working vars back into state
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;

	// Wipe variables
#ifdef SHA1_WIPE_VARIABLES
	a = b = c = d = e = 0;
#endif
}

// Use this function to hash in binary data and strings
void CSHA1::Update(UINT_8 *data, UINT_32 len)
{
	UINT_32 i, j;

	j = (m_count[0] >> 3) & 63;

	if((m_count[0] += len << 3) < (len << 3)) m_count[1]++;

	m_count[1] += (len >> 29);

	if((j + len) > 63)
	{
		i = 64 - j;
		memcpy(&m_buffer[j], data, i);
		Transform(m_state, m_buffer);

		for( ; i + 63 < len; i += 64) Transform(m_state, &data[i]);

		j = 0;
	}
	else i = 0;

	memcpy(&m_buffer[j], &data[i], len - i);
}

// Hash in file contents
bool CSHA1::HashFile(char *szFileName)
{
	unsigned long ulFileSize, ulRest, ulBlocks;
	unsigned long i;
	UINT_8 uData[SHA1_MAX_FILE_BUFFER];
	FILE *fIn;

	if(szFileName == NULL) return false;

	fIn = fopen(szFileName, "rb");
	if(fIn == NULL) return false;

	fseek(fIn, 0, SEEK_END);
	ulFileSize = (unsigned long)ftell(fIn);
	fseek(fIn, 0, SEEK_SET);

	if(ulFileSize != 0)
	{
		ulBlocks = ulFileSize / SHA1_MAX_FILE_BUFFER;
		ulRest = ulFileSize % SHA1_MAX_FILE_BUFFER;
	}
	else
	{
		ulBlocks = 0;
		ulRest = 0;
	}

	for(i = 0; i < ulBlocks; i++)
	{
		fread(uData, 1, SHA1_MAX_FILE_BUFFER, fIn);
		Update((UINT_8 *)uData, SHA1_MAX_FILE_BUFFER);
	}

	if(ulRest != 0)
	{
		fread(uData, 1, ulRest, fIn);
		Update((UINT_8 *)uData, ulRest);
	}

	fclose(fIn); fIn = NULL;
	return true;
}


void CSHA1::Final()
{
	UINT_32 i;
	UINT_8 finalcount[8];

	for(i = 0; i < 8; i++)
		finalcount[i] = (UINT_8)((m_count[((i >= 4) ? 0 : 1)]
			>> ((3 - (i & 3)) * 8) ) & 255); // Endian independent

	Update((UINT_8 *)"\200", 1);

	while ((m_count[0] & 504) != 448)
		Update((UINT_8 *)"\0", 1);

	Update(finalcount, 8); // Cause a SHA1Transform()

	for(i = 0; i < 20; i++)
	{
		m_digest[i] = (UINT_8)((m_state[i >> 2] >> ((3 - (i & 3)) * 8) ) & 255);
	}

	// Wipe variables for security reasons
#ifdef SHA1_WIPE_VARIABLES
	i = 0;
	memset(m_buffer, 0, 64);
	memset(m_state, 0, 20);
	memset(m_count, 0, 8);
	memset(finalcount, 0, 8);
	Transform(m_state, m_buffer);
#endif
}


// Get the final hash as a pre-formatted string
void CSHA1::ReportHash(char *szReport, unsigned char uReportType)
{
	unsigned char i;
	char szTemp[16];

	if(szReport == NULL) return;

	if(uReportType == REPORT_HEX)
	{
		sprintf(szTemp, "%02x", m_digest[0]);
		strcat(szReport, szTemp);

		for(i = 1; i < 20; i++)
		{
			sprintf(szTemp, "%02x", m_digest[i]);
			strcat(szReport, szTemp);
		}
	}
	else if(uReportType == REPORT_DIGIT)
	{
		sprintf(szTemp, "%u", m_digest[0]);
		strcat(szReport, szTemp);

		for(i = 1; i < 20; i++)
		{
			sprintf(szTemp, " %u", m_digest[i]);
			strcat(szReport, szTemp);
		}
	}
	else strcpy(szReport, "Error: Unknown report type!");
}


// Get the raw message digest
void CSHA1::GetHash(UINT_8 *puDest)
{
	memcpy(puDest, m_digest, 20);
}

static char* hex2str(const char *hex, int hexlen) 
{
	static char str[1000];
	unsigned char c, s;
	int i = 0;

	while(hexlen > 0) 
	{
		c = *hex++; 
		hexlen--;

		s = 0x0F & c;
		if ( s < 10 ) 
		{
			str[i + 1] = '0' + s;
		}
		else 
		{  
			str[i + 1] = 'a' + (s - 10);
		}

		c >>= 4;
		s = 0x0F & c;
		if ( s < 10 ) 
		{
			str[i] = '0' + s;
		}
		else 
		{  
			str[i] = 'a' + (s - 10);
		}
		i += 2; 
	}
	str[i++] = '\0'; 
	return str;
}

//可以把中间结果dump 出来， 然后下次load 进去，继续计算， delexxie 2009.02.22
int CSHA1::DumpParams(char *params_buf, UINT_32* params_buf_len)
{
	if(params_buf == NULL)
	{
		return -1;
	}

	UINT_32 len = 0;
	UINT_32 k; 

	k = htonl(m_state[0]); memcpy(params_buf + len, &k, 4); 	len += 4;
	k = htonl(m_state[1]); memcpy(params_buf + len, &k, 4); 	len += 4;
	k = htonl(m_state[2]); memcpy(params_buf + len, &k, 4); 	len += 4;
	k = htonl(m_state[3]); memcpy(params_buf + len, &k, 4); 	len += 4;
	k = htonl(m_state[4]); memcpy(params_buf + len, &k, 4); 	len += 4;

	k = htonl(m_count[0]); memcpy(params_buf + len, &k, 4); 	len += 4;
	k = htonl(m_count[1]); memcpy(params_buf + len, &k, 4); 	len += 4;

	k = htonl(__reserved1[0]); memcpy(params_buf + len, &k, 4);	len += 4;

	//其他参数
	memcpy(params_buf + len, m_buffer, 64);	len += 64;
	memcpy(params_buf + len, m_digest, 20);	len += 20;

	k = htonl(__reserved2[0]); memcpy(params_buf + len, &k, 4);	len += 4;
	k = htonl(__reserved2[1]); memcpy(params_buf + len, &k, 4);	len += 4;
	k = htonl(__reserved2[2]); memcpy(params_buf + len, &k, 4);	len += 4;

	memcpy(params_buf + len, m_workspace, 64);	len += 64;

	*params_buf_len = len;

	//fprintf(stderr, "dump: %s\n", hex2str(params_buf, len));

	return 0;
}

int CSHA1::LoadParams(char *params_buf, UINT_32 params_buf_len)
{
	if(params_buf == NULL)
	{
		return -1;
	}

	UINT_32 len = 0;
	memcpy(m_state, params_buf + len, 20); 	len += 20;
	memcpy(m_count, params_buf + len, 8); 	len += 8;

	m_state[0] = ntohl(m_state[0]);
	m_state[1] = ntohl(m_state[1]);
	m_state[2] = ntohl(m_state[2]);
	m_state[3] = ntohl(m_state[3]);
	m_state[4] = ntohl(m_state[4]);

	m_count[0] = ntohl(m_count[0]);
	m_count[1] = ntohl(m_count[1]);

	__reserved1[0] = ntohl(*(UINT_32*)(params_buf + len)); 	len += 4;

	memcpy(m_buffer, params_buf + len, 64); 	len += 64;
	memcpy(m_digest, params_buf + len, 20); 	len += 20;

	__reserved2[0] = ntohl(*(UINT_32*)(params_buf + len)); 	len += 4;
	__reserved2[1] = ntohl(*(UINT_32*)(params_buf + len)); 	len += 4;
	__reserved2[2] = ntohl(*(UINT_32*)(params_buf + len)); 	len += 4;
	
	memcpy(m_workspace, params_buf + len, 64); 	len += 64;

	//fprintf(stderr, "load: %s\n", hex2str(params_buf, len));
	return 0;
}

void CSHA1::ReportTempHash(char *szReport, unsigned long long & datalen)
{
	if(szReport == NULL) return;

	//获取已完成sha的数据量
	memcpy(&datalen, m_count, 8);
	datalen = datalen >> 3;
	memcpy(szReport, m_state, 20);
}

void CSHA1::LoadTempHash(const char *szReport, unsigned long long datalen)
{
	if(szReport == NULL) return;

	//恢复m_count
	datalen = datalen << 3;
	memcpy(m_count, &datalen, 8);

	//恢复m_state
	memcpy(m_state, szReport, 20);
}
