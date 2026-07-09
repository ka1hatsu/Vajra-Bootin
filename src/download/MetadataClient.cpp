#include "download/MetadataClient.h"
#include "download/SourcePolicy.h"

#include <Windows.h>
#include <winhttp.h>

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

namespace vajra::download {
namespace {
struct Handle { HINTERNET h{}; ~Handle(){ if(h) WinHttpCloseHandle(h); } };
std::wstring wide(const std::string& s) {
    if (s.empty()) return {};
    const int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);
    if(n<=0) return {};
    std::wstring w(static_cast<std::size_t>(n),L'\0');
    return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),w.data(),n)>0?w:std::wstring{};
}
std::string utf8(const std::wstring& w) {
    if(w.empty()) return {};
    const int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w.data(),static_cast<int>(w.size()),nullptr,0,nullptr,nullptr);
    if(n<=0) return {};
    std::string s(static_cast<std::size_t>(n),'\0');
    return WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,w.data(),static_cast<int>(w.size()),s.data(),n,nullptr,nullptr)>0?s:std::string{};
}
std::optional<MetadataResponse> fetch_once(const std::string& url,std::size_t max_bytes,int timeout_ms,int redirects) {
    if(redirects>5 || !check_source_url(url).allowed() || max_bytes==0 || timeout_ms<=0) return std::nullopt;
    const auto wurl=wide(url); if(wurl.empty()) return std::nullopt;
    URL_COMPONENTS p{}; p.dwStructSize=sizeof(p); p.dwSchemeLength=p.dwHostNameLength=p.dwUrlPathLength=p.dwExtraInfoLength=static_cast<DWORD>(-1);
    if(!WinHttpCrackUrl(wurl.c_str(),0,0,&p)) return std::nullopt;
    std::wstring host(p.lpszHostName,p.dwHostNameLength), resource(p.lpszUrlPath,p.dwUrlPathLength);
    if(p.dwExtraInfoLength) resource.append(p.lpszExtraInfo,p.dwExtraInfoLength);
    Handle session{WinHttpOpen(L"Vajra-Bootin/0.5",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
    if(!session.h) return std::nullopt;
    WinHttpSetTimeouts(session.h,timeout_ms,timeout_ms,timeout_ms,timeout_ms);
    Handle connection{WinHttpConnect(session.h,host.c_str(),p.nPort,0)}; if(!connection.h) return std::nullopt;
    Handle request{WinHttpOpenRequest(connection.h,L"GET",resource.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE)}; if(!request.h) return std::nullopt;
    DWORD disable=WINHTTP_DISABLE_REDIRECTS; WinHttpSetOption(request.h,WINHTTP_OPTION_DISABLE_FEATURE,&disable,sizeof(disable));
    if(!WinHttpSendRequest(request.h,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(request.h,nullptr)) return std::nullopt;
    DWORD status=0,size=sizeof(status);
    if(!WinHttpQueryHeaders(request.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX)) return std::nullopt;
    if(status>=300&&status<400) {
        DWORD needed=0; WinHttpQueryHeaders(request.h,WINHTTP_QUERY_LOCATION,WINHTTP_HEADER_NAME_BY_INDEX,nullptr,&needed,WINHTTP_NO_HEADER_INDEX);
        if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER||needed<sizeof(wchar_t)) return std::nullopt;
        std::vector<wchar_t> location(needed/sizeof(wchar_t));
        if(!WinHttpQueryHeaders(request.h,WINHTTP_QUERY_LOCATION,WINHTTP_HEADER_NAME_BY_INDEX,location.data(),&needed,WINHTTP_NO_HEADER_INDEX)) return std::nullopt;
        std::wstring target(location.data());
        if(target.rfind(L"https://",0)!=0) return std::nullopt;
        return fetch_once(utf8(target),max_bytes,timeout_ms,redirects+1);
    }
    if(status<200||status>=300) return std::nullopt;
    wchar_t length[64]{}; DWORD length_size=sizeof(length);
    if(WinHttpQueryHeaders(request.h,WINHTTP_QUERY_CONTENT_LENGTH,WINHTTP_HEADER_NAME_BY_INDEX,length,&length_size,WINHTTP_NO_HEADER_INDEX)) {
        try { if(std::stoull(length)>max_bytes) return std::nullopt; } catch(...) { return std::nullopt; }
    }
    std::string body; body.reserve(std::min<std::size_t>(max_bytes,64*1024)); std::vector<char> buffer(16*1024);
    for(;;) {
        DWORD read=0; if(!WinHttpReadData(request.h,buffer.data(),static_cast<DWORD>(buffer.size()),&read)) return std::nullopt;
        if(read==0) break;
        if(body.size()>max_bytes-read) return std::nullopt;
        body.append(buffer.data(),read);
    }
    return MetadataResponse{std::move(body),url};
}
}

std::optional<MetadataResponse> fetch_metadata_text(const std::string& url,std::size_t max_bytes,int timeout_ms) {
    return fetch_once(url,max_bytes,timeout_ms,0);
}

} // namespace vajra::download
