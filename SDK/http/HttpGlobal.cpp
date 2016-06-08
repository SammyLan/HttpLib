#include "stdafx.h"
#include "HttpGlobal.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

#ifdef DEBUG
#pragma comment(lib,"libcurl_a_debug.lib")
#else
#pragma comment(lib,"libcurl_a.lib")
#endif // DEBUG

bool http::CurlInit()
{
	return true;
}

bool http::CurlUninit()
{
	return true;
}

void http::mcode_or_die(const char *where, CURLMcode code)
{
	if (CURLM_OK != code)
	{
		const char *s;
		switch (code)
		{
		case CURLM_CALL_MULTI_PERFORM:
			s = "CURLM_CALL_MULTI_PERFORM";
			break;
		case CURLM_BAD_HANDLE:
			s = "CURLM_BAD_HANDLE";
			break;
		case CURLM_BAD_EASY_HANDLE:
			s = "CURLM_BAD_EASY_HANDLE";
			break;
		case CURLM_OUT_OF_MEMORY:
			s = "CURLM_OUT_OF_MEMORY";
			break;
		case CURLM_INTERNAL_ERROR:
			s = "CURLM_INTERNAL_ERROR";
			break;
		case CURLM_UNKNOWN_OPTION:
			s = "CURLM_UNKNOWN_OPTION";
			break;
		case CURLM_LAST:
			s = "CURLM_LAST";
			break;
		default:
			s = "CURLM_unknown";
			break;
		case CURLM_BAD_SOCKET:
			s = "CURLM_BAD_SOCKET";
			LogFinal(HTTPLOG,_T( "\nERROR: %s returns %S"), where, s);
			/* ignore this error */
			return;
		}
		assert(false);
		LogFinal(HTTPLOG,_T( "\nERROR: %s returns %S"), where, s);
		exit(code);
	}
}

std::string http::util::urlEncode(const std::string& value) {
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (auto i = value.cbegin(), n = value.cend(); i != n; ++i) {
		std::string::value_type c = (*i);
		// Keep alphanumeric and other accepted characters intact
		if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			escaped << c;
			continue;
		}
		// Any other characters are percent-encoded
		escaped << '%' << std::setw(2) << int((unsigned char)c);
	}

	return escaped.str();
}

namespace http {
	bool CaseInsensitiveCompare::operator()(const std::string& a, const std::string& b) const noexcept {
		return std::lexicographical_compare(
			a.begin(), a.end(), b.begin(), b.end(),
			[](unsigned char ac, unsigned char bc) { return std::tolower(ac) < std::tolower(bc); });
	}

	Parameters::Parameters(const std::initializer_list<Parameter>& parameters) {
		for (const auto& parameter : parameters) {
			AddParameter(parameter);
		}
	}

	void Parameters::AddParameter(const Parameter& parameter) {
		if (!content.empty()) {
			content += "&";
		}

		auto escapedKey = http::util::urlEncode(parameter.key);
		if (parameter.value.empty()) {
			content += escapedKey;
		}
		else {
			auto escapedValue = http::util::urlEncode(parameter.value);
			content += escapedKey + "=" + escapedValue;
		}
	}
} 
