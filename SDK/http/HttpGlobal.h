#pragma once
#include <map>
#include <string>
#include <curl\curl.h>

#define HTTPLOG _T("http")

namespace http
{
	bool CurlInit();
	bool CurlUninit();
	void mcode_or_die(const char *where, CURLMcode code);

	namespace util
	{
		std::string urlEncode(const std::string& value);
	}
}

namespace http
{
	struct CaseInsensitiveCompare {
		bool operator()(const std::string& a, const std::string& b) const noexcept;
	};

	using Header = std::map<std::string, std::string, CaseInsensitiveCompare>;
	using Url = std::string;

	struct Parameter {
		template <typename KeyType, typename ValueType>
		Parameter(KeyType&& key, ValueType&& value)
			: key{ CPR_FWD(key) }, value{ CPR_FWD(value) } {}
		std::string key;
		std::string value;
	};

	class Parameters{
	public:
		Parameters() = default;
		Parameters(const std::initializer_list<Parameter>& parameters);
		void AddParameter(const Parameter& parameter);
		std::string content;
	};
}