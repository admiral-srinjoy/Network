#include "http.h"
#include <winhttp.h>

#pragma comment(lib, "Winhttp.lib")


bool Http::HttpRequest::Get(
	const std::wstring& rest_of_path,
	const std::wstring& requestHeader,
	HttpResponse& response)
{
	static const std::wstring verb = L"GET";
	static std::string body;
	return Request(
		verb,
		rest_of_path,
		requestHeader,
		body,
		response);
}

bool Http::HttpRequest::Post(
	const std::wstring& rest_of_path,
	const std::wstring& requestHeader,
	const std::string& body,
	HttpResponse& response)
{
	static const std::wstring verb = L"POST";
	return Request(
		verb,
		rest_of_path,
		requestHeader,
		body,
		response);
}

bool Http::HttpRequest::Patch(
	const std::wstring& rest_of_path,
	const std::wstring& requestHeader,
	const std::string& body,
	HttpResponse& response)
{
	static const std::wstring verb = L"PATCH";
	return Request(
		verb,
		rest_of_path,
		requestHeader,
		body,
		response);
}

bool Http::HttpRequest::Delete(
	const std::wstring& rest_of_path,
	const std::wstring& requestHeader,
	const std::string& body,
	HttpResponse& response)
{
	static const std::wstring verb = L"DELETE";
	return Request(
		verb,
		rest_of_path,
		requestHeader,
		body,
		response);
}

bool Http::HttpRequest::Request(
	const std::wstring& verb,
	const std::wstring& rest_of_path,
	const std::wstring& requestHeader,
	const std::string& body,
	HttpResponse& response)
{
	return http(verb, m_UserAgent, m_Domain,
		rest_of_path, m_Port, m_Secure,
		requestHeader, body,
		response.text, response.header,
		response.statusCode,
		response.contentLength,
		response.error);
}


bool Http::HttpRequest::http(const std::wstring& verb, const std::wstring& user_agent, const std::wstring& domain,
	const std::wstring& rest_of_path, int port, bool secure,
	const std::wstring& requestHeader, const std::string& body,
	std::string& text, std::wstring& responseHeader, DWORD& dwStatusCode, DWORD& dwContent, std::wstring& error)
{
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	BOOL  bResults = FALSE;
	DWORD dwProxyAuthScheme = 0;
	DWORD dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
	DWORD flag = secure ? WINHTTP_FLAG_SECURE : 0;

	dwStatusCode = 0;

	hSession = WinHttpOpen(user_agent.c_str(), dwAccessType, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession)
	{
		error = L"WinHttpOpen fails!";
		return false;

	}


	// Specify an HTTP server.
	hConnect = WinHttpConnect(hSession, domain.c_str(), port, 0);
	if (!hConnect)
	{
		WinHttpCloseHandle(hSession);
		error = L"WinHttpConnect fails!";
		return false;

	}

	// Create an HTTP request handle.
	hRequest = WinHttpOpenRequest(hConnect, verb.c_str(), rest_of_path.c_str(),
		NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		WINHTTP_FLAG_REFRESH | flag);
	if (!hRequest) {
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		error = L"Error ipening request";
		return false;

	}


	// Send a request.
	bResults = WinHttpSendRequest(hRequest, requestHeader.c_str(), requestHeader.size(), (LPVOID)body.data(), body.size(), body.size(), 0);
	if (!bResults)
	{
		error = L"WinHttpSendRequest fails!" + std::to_wstring(GetLastError());
		dwStatusCode = GetLastError();
		return false;

	}

	// End the request.
	bResults = WinHttpReceiveResponse(hRequest, NULL);
	if (!bResults)
	{
		error = L"WinHttpReceiveResponse fails!";
		dwStatusCode = GetLastError();
		return false;
	}


	// Check the status code.
	bool retval = getStatus(dwSize, dwStatusCode, error);
	if (!retval) return retval;

	// Get response header
	retval = getResponseHeaders(dwSize, error, responseHeader);
	if (!retval) return retval;


	// Keep checking for data until there is nothing left.
	retval = getBody( dwSize, error, dwContent, dwDownloaded, text);
	if (!retval) return retval;

	// Close any open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);

	// Report any errors.
	return bResults;
}

bool Http::HttpRequest::getBody(DWORD& dwSize, std::wstring& error, DWORD& dwContent, DWORD& dwDownloaded, std::string& text)
{
	std::string buffer;
	text = "";
	do
	{
		// Check for available data.
		dwSize = 0;
		if (!WinHttpQueryDataAvailable(this->hRequest, &dwSize))
		{
			error = L"Error in WinHttpQueryDataAvailable: ";
			error += std::to_wstring(GetLastError());
			return false;
		}

		dwContent += dwSize;
		if (dwSize == 0)
			break;

		// Allocate space for the buffer.
		buffer = "";
		buffer.resize(dwSize);
		// Read the data.
		ZeroMemory((void*)(&buffer[0]), dwSize);
		if (!WinHttpReadData(hRequest, (LPVOID)(&buffer[0]),
			dwSize, &dwDownloaded))
		{
			error = L"Error in WinHttpReadData: ";
			error += std::to_wstring(GetLastError());
			return false;
		}

		text += buffer;

	} while (dwSize > 0);
	return true;
}

bool Http::HttpRequest::getResponseHeaders(DWORD& dwSize, std::wstring& error, std::wstring& responseHeader)
{
	bool bResults = false;
	WinHttpQueryHeaders(this->hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
		WINHTTP_HEADER_NAME_BY_INDEX, NULL,
		&dwSize, WINHTTP_NO_HEADER_INDEX);
	// Allocate memory for the buffer.
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		responseHeader.resize(dwSize + 1);

		// Now, use WinHttpQueryHeaders to retrieve the header.
		bResults = WinHttpQueryHeaders(this->hRequest,
			WINHTTP_QUERY_RAW_HEADERS_CRLF,
			WINHTTP_HEADER_NAME_BY_INDEX,
			(LPVOID)responseHeader.data(), &dwSize,
			WINHTTP_NO_HEADER_INDEX);
	}
	if (!bResults) {
		error = L"WinHttp Unable to get QueryParams Status fails!";
		return false;
	}
	return true;
}

bool Http::HttpRequest::getStatus(DWORD& dwSize, DWORD& dwStatusCode, std::wstring& error)
{
	dwSize = sizeof(dwStatusCode);
	bool bResults = WinHttpQueryHeaders(this->hRequest,
		WINHTTP_QUERY_STATUS_CODE |
		WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX,
		&dwStatusCode,
		&dwSize,
		WINHTTP_NO_HEADER_INDEX);
	if (!bResults)
	{
		error = L"WinHttp Status fails!";
		return false;
	}
	return true;
}


std::unordered_map<std::wstring, std::wstring>& Http::HttpResponse::GetHeaderDictionary()
{
	if (!dict.empty())
		return dict;

	bool return_carriage_reached = false;
	bool colon_reached = false;
	bool colon_just_reached = false;
	std::wstring key;
	std::wstring value;
	for (size_t i = 0; i < header.size(); ++i)
	{
		wchar_t ch = header[i];
		if (ch == L':')
		{
			colon_reached = true;
			colon_just_reached = true;
			continue;
		}
		else if (ch == L'\r')
			return_carriage_reached = true;
		else if (ch == L'\n' && !return_carriage_reached)
			return_carriage_reached = true;
		else if (ch == L'\n' && return_carriage_reached)
		{
			return_carriage_reached = false;
			continue;
		}

		if (return_carriage_reached)
		{
			if (!key.empty() && !value.empty())
				dict[key] = value;

			key.clear();
			value.clear();
			colon_reached = false;
			if (ch == L'\n')
				return_carriage_reached = false;

			continue;
		}

		if (colon_reached == false)
			key += ch;
		else
		{
			if (colon_just_reached)
			{
				colon_just_reached = false;
				if (ch == L' ')
					continue;
			}
			value += ch;
		}
	}

	if (!key.empty() && !value.empty())
		dict[key] = value;

	return dict;
}