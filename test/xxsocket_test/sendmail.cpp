#include "xxsocket.h"
#include "timestamp.h"
#include "nsconv.h"
// #include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include "libb64.h"
//#include "cocos2d.h"
//#include "HttpClient.h"
//#include "detail/VXToolHal.h"
//#include "purelib/utils/crypto_wrapper.h"
//
//USING_NS_CC;
//using namespace cocos2d::network;
#include <sstream>

const unsigned char Base64ValTab[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#ifndef _WIN32
#define BYTE unsigned char
#endif
#define AVal(x) Base64ValTab[x]

//#pragma comment (lib,"ws2_32.lib") 

using namespace purelib::inet;

static uint8_t hex2uchr(const uint8_t hex)
{
    return hex > 9 ? (hex - 10 + 'A') : (hex + '0');
}

static std::string urlencode(const std::string& input)
{
    std::string output;
    for (size_t ix = 0; ix < input.size(); ix++)
    {
        uint8_t buf[4];
        memset(buf, 0, 4);
        if (isalnum((uint8_t)input[ix]))
        {
            buf[0] = input[ix];
        }
        else if ( isspace((uint8_t)input[ix] ) )
        {
            buf[0] = '+';
        }
        else
        {
            buf[0] = '%';
            buf[1] = ::hex2uchr((uint8_t)input[ix] >> 4);
            buf[2] = ::hex2uchr((uint8_t)input[ix] % 16);
        }
        output += (char *)buf;
    }
    return std::move(output);
};

#if 0
//获取系统版本
BOOL internalGetOSName(CString& csOsName)
{
    OSVERSIONINFOEX osvi;
    SYSTEM_INFO si;
    BOOL bOsVersionInfoEx;
    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

    // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
    // If that fails, try using the OSVERSIONINFO structure.

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *)&osvi);
    if (!bOsVersionInfoEx)
    {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (!GetVersionEx((OSVERSIONINFO *)&osvi))
            return FALSE;
    }

    // Call GetNativeSystemInfo if supported
    // or GetSystemInfo otherwise.
    else GetSystemInfo(&si);

    switch (osvi.dwPlatformId)
    {
        // Test for the Windows NT product family.

    case VER_PLATFORM_WIN32_NT:

        if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0)
        {
            csOsName += _T("Windows 10 ");
        }

        if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3)
        {
            csOsName += _T("Windows 8.1 ");
        }

        if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
        {
            csOsName += _T("Windows 8 ");
        }

        if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
        {
            csOsName += _T("Windows 7 ");
        }

        // Test for the specific product.
        if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
        {
            if (osvi.wProductType == VER_NT_WORKSTATION)
            {
                csOsName += _T("Windows Vista ");
            }
            else
            {
                csOsName += _T("Windows Server \"Longhorn\" ");
            }
        }

        if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
        {
            if (osvi.wProductType == VER_NT_WORKSTATION &&
                si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
            {
                csOsName += _T("Microsoft Windows XP Professional x64 Edition ");
            }
            else
            {
                csOsName += _T("Microsoft Windows Server 2003, ");
            }
        }

        if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
        {
            csOsName += _T("Microsoft Windows XP ");
        }

        if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
        {
            csOsName += _T("Microsoft Windows 2000 ");
        }

        if (osvi.dwMajorVersion <= 4)
        {
            csOsName += _T("Microsoft Windows NT ");
        }

        // Test for specific product on Windows NT 4.0 SP6 and later.
        if (bOsVersionInfoEx)
        {
            // Test for the workstation type.
            if (osvi.wProductType == VER_NT_WORKSTATION &&
                si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_AMD64)
            {
                if (osvi.dwMajorVersion == 4)
                {
                    csOsName += _T("Workstation 4.0 ");
                }
                else if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
                {
                    csOsName += _T("Home Edition ");
                }
                else
                {
                    csOsName += _T("Professional ");
                }
            }

            // Test for the server type.
            else if (osvi.wProductType == VER_NT_SERVER ||
                osvi.wProductType == VER_NT_DOMAIN_CONTROLLER)
            {
                if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
                {
                    if (si.wProcessorArchitecture ==
                        PROCESSOR_ARCHITECTURE_IA64)
                    {
                        if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                        {
                            csOsName += _T("Datacenter Edition for Itanium-based Systems");
                        }
                        else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                        {
                            csOsName += _T("Enterprise Edition for Itanium-based Systems");
                        }
                    }

                    else if (si.wProcessorArchitecture ==
                        PROCESSOR_ARCHITECTURE_AMD64)
                    {
                        if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                        {
                            csOsName += _T("Datacenter x64 Edition ");
                        }
                        else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                        {

                        }
                        else
                        {
                            csOsName += _T("Standard x64 Edition ");
                        }
                    }
                    else
                    {
                        if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                        {
                            csOsName += _T("Datacenter Edition ");
                        }
                        else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                        {
                            csOsName += _T("Enterprise Edition ");
                        }
                        else if (osvi.wSuiteMask & VER_SUITE_BLADE)
                        {
                            csOsName += _T("Web Edition ");
                        }
                        else
                        {
                            csOsName += _T("Standard Edition ");
                        }
                    }
                }
                else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
                {
                    if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                    {
                        csOsName += _T("Datacenter Server ");
                    }
                    else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                    {
                        csOsName += _T("Advanced Server ");
                    }
                    else
                    {
                        csOsName += _T("Server ");
                    }
                }
                else  // Windows NT 4.0 
                {
                    if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                    {
                        csOsName += _T("Server 4.0, Enterprise Edition ");
                    }
                    else
                    {
                        csOsName += _T("Server 4.0 ");
                    }
                }
            }
        }
        // Test for specific product on Windows NT 4.0 SP5 and earlier
        else
        {
            HKEY hKey;
            TCHAR szProductType[256];
            DWORD dwBufLen = 256 * sizeof(TCHAR);
            LONG lRet;

            lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                _T("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
                0, KEY_QUERY_VALUE, &hKey);
            if (lRet != ERROR_SUCCESS)
                return FALSE;

            lRet = RegQueryValueEx(hKey, TEXT("ProductType"),
                NULL, NULL, (LPBYTE)szProductType, &dwBufLen);
            RegCloseKey(hKey);

            if ((lRet != ERROR_SUCCESS) ||
                (dwBufLen > 256 * sizeof(TCHAR)))
                return FALSE;

            if (lstrcmpi(TEXT("WINNT"), szProductType) == 0)
            {
                csOsName += _T("Workstation ");
            }
            if (lstrcmpi(TEXT("LANMANNT"), szProductType) == 0)
            {
                csOsName += _T("Server ");
            }
            if (lstrcmpi(TEXT("SERVERNT"), szProductType) == 0)
            {
                csOsName += _T("Advanced Server ");
            }
            CString cstmp;
            cstmp.Format(_T("%d.%d "), osvi.dwMajorVersion, osvi.dwMinorVersion);
            csOsName += cstmp;
        }

        // Display service pack (if any) and build number.

        if (osvi.dwMajorVersion == 4 &&
            lstrcmpi(osvi.szCSDVersion, TEXT("Service Pack 6")) == 0)
        {
            HKEY hKey;
            LONG lRet;

            // Test for SP6 versus SP6a.
            lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009"),
                0, KEY_QUERY_VALUE, &hKey);
            if (lRet == ERROR_SUCCESS)
            {
                CString cstmp;
                cstmp.Format(_T("Service Pack 6a (Build %d)"), osvi.dwBuildNumber & 0xFFFF);
                csOsName += cstmp;
            }
            else // Windows NT 4.0 prior to SP6a
            {
                CString cstmp;
                cstmp.Format(_T("%s (Build %d)"), osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF);
                csOsName += cstmp;
            }
            RegCloseKey(hKey);
        }
        else // not Windows NT 4.0 
        {
            CString cstmp;
            cstmp.Format(_T("%s (Build %d)"), osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF);
            csOsName += cstmp;
        }

        break;

        // Test for the Windows Me/98/95.
    case VER_PLATFORM_WIN32_WINDOWS:

        if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
        {
            csOsName += _T("Microsoft Windows 95 ");
            if (osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B')
            {
                csOsName += _T("OSR2 ");
            }
        }

        if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
        {
            csOsName += _T("Microsoft Windows 98 ");
            if (osvi.szCSDVersion[1] == 'A' || osvi.szCSDVersion[1] == 'B')
            {
                csOsName += _T("SE ");
            }
        }

        if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
        {
            csOsName += _T("Microsoft Windows Millennium Edition");
        }
        break;

    case VER_PLATFORM_WIN32s:

        csOsName += _T("Microsoft Win32s\n");
        break;
    }
    return TRUE;
}
#endif

#if 1
void printRecv(xxsocket& s)
{
	char recvBuff[1024*8];
	memset(recvBuff,0,sizeof(recvBuff));

	//xxsocket s(sock);
	s.recv_i(recvBuff,1000);
	// s.release();
    OutputDebugStringA(recvBuff);
    OutputDebugStringA("\n");
}

int EncodingBase64(const char * pInput, int ilen, char * pOutput)
{
    memset(pOutput, 0x0, strlen(pOutput));
    // std::string ciphertext((plaintext.length() * 2), '\0');
    char* wrptr = pOutput;
    base64_encodestate state;
    base64_init_encodestate(&state);
    int r1 = base64_encode_block(pInput, ilen, wrptr, &state);
    int r2 = base64_encode_blockend(wrptr + r1, &state);

    return (r1 + r2);
}

enum SendResult {errorusername,ok}; 

SendResult sendmail(const char * smtpServer,int port, const char* username, const char * password,const char* from, const char * to, const char * subject,const char * body)
{
    xxsocket tcpcli;
    if (tcpcli.xpconnect_n(smtpServer, port, 3.0f) != 0)
        return errorusername;

    int n = 0;

	char* buffer = new char[SZ(16, k)];
    memset(buffer, 0x0, SZ(16, k));

    n = sprintf(buffer,"EHLO %s\r\n",from);//from为char数据。存储发送地址
    tcpcli.send(buffer, n);

	printRecv(tcpcli);

    n = sprintf(buffer,"AUTH LOGIN\r\n");
    tcpcli.send(buffer, n);

	printRecv(tcpcli);

	//USER NAME
	//User name is coded by base64.
	EncodingBase64(username, strlen(username), buffer);//先将用户帐号经过base64编码
	strcat(buffer,"\r\n");
    tcpcli.send(buffer,strlen(buffer));

	printRecv(tcpcli);

	//password coded by base64
	EncodingBase64(password, strlen(password), buffer);//先将密码经过base64编码
	strcat(buffer,"\r\n");
    tcpcli.send(buffer,strlen(buffer));

	printRecv(tcpcli);


    n = sprintf(buffer,"MAIL FROM:<%s>\r\n",from);
    tcpcli.send(buffer, n);

	printRecv(tcpcli);

    n = sprintf(buffer,"RCPT TO: <%s>\r\n",to);
    tcpcli.send(buffer, n);

	printRecv(tcpcli);

	//memset(buffer,0,sizeof(buffer));
    n = sprintf(buffer,"DATA\r\n");
    tcpcli.send(buffer, n);
	printRecv(tcpcli);

    //DATA head
    n = sprintf(buffer,"Subject: %s\r\n\r\n",subject);
	
    tcpcli.send(buffer, n);

    n = sprintf(buffer,"%s\r\n.\r\n",body);
	//DATA body
    tcpcli.send(buffer, n);
	printRecv(tcpcli);

	strcpy(buffer,"QUIT\r\n");
    tcpcli.send(buffer,strlen(buffer));
	printRecv(tcpcli);

    tcpcli.shutdown();
    tcpcli.close();
	return ok;
}
#endif

#if 0

static void internalDispatchHttpResponse(const std::function<void()>& response)
{
    thal->runOnMainUIThread([=] {
        response();
    });
}

namespace cocos2d {
    namespace wext {
        void CC_DLL setCustomHttpResponseDispatcher(const std::function<void(const std::function<void()>&)>& dispatcher);
    };
};

void sendmail(const char* subject, const std::string& content)
{
    wext::setCustomHttpResponseDispatcher(internalDispatchHttpResponse);

    HttpRequest* send_mail = new HttpRequest();
    send_mail->setUrl("http://send-email.org/send");
    auto headers = send_mail->getHeaders();
    headers.push_back("Connection: keep-alive");
    // headers.push_back("Content-Length: 119");
    headers.push_back("Accept: application/json, text/javascript, */*; q=0.01");
    headers.push_back("Origin: http://send-email.org");
    headers.push_back("X-Requested-With: XMLHttpRequest");
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36");
    headers.push_back("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
    headers.push_back("Referer: http://send-email.org/");
    headers.push_back("Accept-Encoding: gzip, deflate");
    headers.push_back("Accept-Language: zh-CN,zh;q=0.8");
    headers.push_back("Cookie: _ga=GA1.2.233452701.1470649501; _gat=1");
    send_mail->setHeaders(headers);
    send_mail->setRequestType(HttpRequest::Type::POST);
    std::string mail_content = "_token=6OU2b4P2Mad8udXN2pTKU56Q0hF7jASrroZVXGMy&emailTo=262381263%40qq.com&replyTo=&subject=";
    
    mail_content.append(crypto::http::b64enc((unmanaged_string)subject));
    mail_content.append("&message=");
    mail_content.append(crypto::http::urlencode(content));

    send_mail->setRequestData(mail_content.c_str(), mail_content.size());
    send_mail->setResponseCallback([=](HttpClient* client, HttpResponse* response) {
        auto resp = response->getResponseData();
        resp->push_back('\0');
        const char* content = resp->data();
        if (response) {
            auto responseCode = response->getResponseCode();

            if (responseCode == 200) {
                cocos2d::log("send mail ok!");
            }
        }
    });
    HttpClient::getInstance()->setTimeoutForConnect(3);
    HttpClient::getInstance()->setTimeoutForRead(2);
    HttpClient::getInstance()->sendImmediate(send_mail);
    send_mail->release();
}

#endif

void http_sendemail(const std::string& mailto, const std::string& subject, const std::string& message)
{
    // post data
    std::stringstream datass;
    datass << "_token=6OU2b4P2Mad8udXN2pTKU56Q0hF7jASrroZVXGMy"
        << "&emailTo=" << mailto
        << "&replyTo="
        << "&subject=" << subject
        << "&message=" << urlencode(message);

    auto data = datass.str();

    // construct http package: Internet Explorer 11(11.51.14393.0 Update: 11.0.34(KB3175443) Windows 10(10.0.14393.82)
    std::stringstream ss;
    ss << "POST /send HTTP/1.1\r\n";
    ss << "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n";
    ss << "Accept: application/json, text/javascript, */*; q=0.01\r\n";
    ss << "X-Requested-With: XMLHttpRequest\r\n";
    ss << "Referer: http://send-email.org/\r\n";
    ss << "Accept-Language: zh-Hans-CN,zh-Hans;q=0.8,en-US;q=0.5,en;q=0.3\r\n";
    ss << "Accept-Encoding: gzip, deflate\r\n";
    // Chrome, Internet Explorer 11 is: User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64; Trident/7.0; rv:11.0) like Gecko\r\n
    ss << "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\n";

    ss << "Host: send-email.org\r\n";
    ss << "Content-Length: " << data.size() << "\r\n";
    ss << "DNT: 1\r\n";
    ss << "Connection: Keep-Alive\r\n";
    ss << "Cache-Control: no-cache\r\n";
    ss << "Cookie: _ga=GA1.2.1544911637.1471488460; _gat=1\r\n";
    ss << "Origin: http://send-email.org\r\n";
    ss << "\r\n";
    
    ss << data;

    auto requestData = ss.str();

    xxsocket httpcli;
    if (httpcli.xpconnect_n("send-email.org", 80, 5) != 0)
    {
        OutputDebugString(L"connect send-email.org failed!\n");
        return;
    }

    if (httpcli.send_n(requestData.c_str(), requestData.size(), 5) <= 0)
    {
        OutputDebugString(L"send request to send-email.org failed!\n");
        return;
    }

    std::string responseData;
    if (!httpcli.read_until(responseData, "\r\n\r\n", 4))
    {
        OutputDebugString(L"recv response from send-email.org failed!\n");
        return;
    }

    responseData.push_back('\n');
    OutputDebugStringA("response data:");
    OutputDebugStringA(responseData.c_str());

    httpcli.shutdown();
    httpcli.close();
}

#if 0
void submit_crash_log(const char* content)
{
    CString osName;
    internalGetOSName(osName);
    osName.Append(_T(":\n"));

    std::string platformInfo = purelib::nsc::transcode(osName.GetString());
    extern std::string g_infoGPU;
    platformInfo.append(g_infoGPU);
    platformInfo.append(":\n");

#if 0
    sendmail(ctimestamp(), (platformInfo + content));
#else
#endif
}
#endif