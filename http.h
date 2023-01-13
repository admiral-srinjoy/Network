#pragma once


#include <string>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <unordered_map>
#include <winhttp.h>

#pragma comment(lib, "Winhttp.lib")


namespace Http
{
	struct HttpResponse
	{
		HttpResponse() : statusCode(0), contentLength(0) {}
		void Reset()
		{
			text = "";
			header = L"";
			statusCode = 0;
			error = L"";
			dict.clear();
			contentLength = 0;
		}
		std::unordered_map<std::wstring, std::wstring>& GetHeaderDictionary();

		std::string text;
		std::wstring header;
		DWORD statusCode;
		DWORD contentLength;
		std::wstring error;
	private:
		std::unordered_map<std::wstring, std::wstring> dict;
	};

	class HttpRequest
	{
	public:
		HttpRequest(
			const std::wstring& domain,
			int port,
			bool secure,
			const std::wstring& user_agent = L"WinHttpClient")
			: m_Domain(domain)
			, m_Port(port)
			, m_Secure(secure)
			, m_UserAgent(user_agent)
		{}

		bool Get(const std::wstring& rest_of_path,
			const std::wstring& requestHeader,
			HttpResponse& response);
		bool Post(const std::wstring& rest_of_path,
			const std::wstring& requestHeader,
			const std::string& body,
			HttpResponse& response);
		bool Patch(const std::wstring& rest_of_path,
			const std::wstring& requestHeader,
			const std::string& body,
			HttpResponse& response);
		bool Delete(const std::wstring& rest_of_path,
			const std::wstring& requestHeader,
			const std::string& body,
			HttpResponse& response);

	private:
		// Request is wrapper around http()
		bool Request(
			const std::wstring& verb,
			const std::wstring& rest_of_path,
			const std::wstring& requestHeader,
			const std::string& body,
			HttpResponse& response);

		HINTERNET hSession = NULL;
		HINTERNET hConnect = NULL;
		HINTERNET hRequest = NULL;
		bool http(const std::wstring& verb, const std::wstring& user_agent, const std::wstring& domain, const std::wstring& rest_of_path, int port, bool secure, const std::wstring& requestHeader, const std::string& body, std::string& text, std::wstring& responseHeader, DWORD& dwStatusCode, DWORD& dwContent, std::wstring& error);
		bool getBody(DWORD& dwSize, std::wstring& error, DWORD& dwContent, DWORD& dwDownloaded, std::string& text);
		bool getResponseHeaders(DWORD& dwSize, std::wstring& error, std::wstring& responseHeader);
		bool getStatus(DWORD& dwSize, DWORD& dwStatusCode, std::wstring& error);
		std::wstring m_Domain;
		int m_Port;
		bool m_Secure;
		std::wstring m_UserAgent;
	};

}