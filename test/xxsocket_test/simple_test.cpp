
#include "async_socket_io.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#define strcasestr StrStrIA
#endif

enum {
    REDIRECT_STANDARD_INPUT = 1,
    REDIRECT_STANDARD_OUTPUT = 2,
    REDIRECT_STANDARD_ERROR = 4,
};

struct RedirectProcessInfo
{
    DWORD dwFlags;
    BOOL bOverlapped;
    HANDLE hInputWrite;
    HANDLE hOutputRead;
    HANDLE hErrorRead;
    HANDLE hProcess;
};

int ReadPipeToEnd(HANDLE hPipe, std::string& output)
{
    CHAR lpBuffer[512];
    DWORD nBytesRead;
    DWORD nCharsWritten;
    int error = 0;
    while (TRUE)
    {
        if (ReadFile(hPipe, lpBuffer, sizeof(lpBuffer),
            &nBytesRead, NULL) && nBytesRead)
        {
            output.append(lpBuffer, nBytesRead);
        }
        else {
            error = GetLastError(); // ERROR_BROKEN_PIPE: pipe done - normal exit path. otherwise:  Something bad happened.
            break;
        }
    }

    CloseHandle(hPipe);
    return error;
}

BOOL CreateRedirectProcess(std::string commandLine, RedirectProcessInfo& processInfo)
{
    HANDLE hOutputReadTmp, hOutputRead, hOutputWrite;
    HANDLE hInputWriteTmp, hInputRead, hInputWrite;
    HANDLE hErrorReadTmp, hErrorRead, hErrorWrite;

    SECURITY_ATTRIBUTES sa;

    // Set up the security attributes struct.
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hCurrentProcess = GetCurrentProcess();

    // Create the child output pipe.
    CreatePipe(&hOutputReadTmp, &hOutputWrite, &sa, 0);

    if (processInfo.dwFlags & REDIRECT_STANDARD_ERROR) {
        CreatePipe(&hErrorReadTmp, &hErrorWrite, &sa, 0);

        DuplicateHandle(hCurrentProcess, hErrorReadTmp,
            hCurrentProcess,
            &hErrorRead, // Address of new handle.
            0, FALSE, // Make it uninheritable.
            DUPLICATE_SAME_ACCESS);

        CloseHandle(hErrorReadTmp);
    }
    else {
        // Create a duplicate of the output write handle for the std error
        // write handle. This is necessary in case the child application
        // closes one of its std output handles.
        DuplicateHandle(hCurrentProcess, hOutputWrite,
            hCurrentProcess, &hErrorWrite, 0,
            TRUE, DUPLICATE_SAME_ACCESS);
    }

    // Create the child input pipe.
    CreatePipe(&hInputRead, &hInputWriteTmp, &sa, 0);


    // Create new output read handle and the input write handles. Set
    // the Properties to FALSE. Otherwise, the child inherits the
    // properties and, as a result, non-closeable handles to the pipes
    // are created.
    DuplicateHandle(hCurrentProcess, hOutputReadTmp,
        hCurrentProcess,
        &hOutputRead, // Address of new handle.
        0, FALSE, // Make it uninheritable.
        DUPLICATE_SAME_ACCESS);

    DuplicateHandle(hCurrentProcess, hInputWriteTmp,
        hCurrentProcess,
        &hInputWrite, // Address of new handle.
        0, FALSE, // Make it uninheritable.
        DUPLICATE_SAME_ACCESS);


    // Close inheritable copies of the handles you do not want to be
    // inherited.
    CloseHandle(hOutputReadTmp);
    CloseHandle(hInputWriteTmp);


    // Get std input handle so you can close it and force the ReadFile to
    // fail when you want the input thread to exit.

    // PrepAndLaunchRedirectedChild(hOutputWrite, hInputRead, hErrorWrite);
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;

    // Set up the start up info struct.
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hOutputWrite;
    si.hStdInput = hInputRead;
    si.hStdError = hErrorWrite;
    // Use this if you want to hide the child:
    //     si.wShowWindow = SW_HIDE;
    // Note that dwFlags must include STARTF_USESHOWWINDOW if you want to
    // use the wShowWindow flags.

    // Launch the process that you want to redirect (in this case,
    // Child.exe). Make sure Child.exe is in the same directory as
    // redirect.c launch redirect from a command line to prevent location
    // confusion.
    BOOL bRet = CreateProcessA(NULL, &commandLine.front(), NULL, NULL, TRUE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    // Close pipe handles (do not continue to modify the parent).
    // You need to make sure that no handles to the write end of the
    // output pipe are maintained in this process or else the pipe will
    // not close when the child process exits and the ReadFile will hang.
    CloseHandle(hOutputWrite);
    CloseHandle(hInputRead);
    CloseHandle(hErrorWrite);

    // Set global child process handle to cause threads to exit.
    if (bRet) {
        if (processInfo.dwFlags & REDIRECT_STANDARD_INPUT)
            processInfo.hInputWrite = hInputWrite;
        else
            CloseHandle(hInputWrite);

        if (processInfo.dwFlags & REDIRECT_STANDARD_OUTPUT)
            processInfo.hOutputRead = hOutputRead;
        else
            CloseHandle(hOutputRead);

        if (processInfo.dwFlags & REDIRECT_STANDARD_ERROR)
            processInfo.hErrorRead = hErrorRead;

        CloseHandle(pi.hThread);
        processInfo.hProcess = pi.hProcess;
    }
    else {
        CloseHandle(hInputWrite);
        CloseHandle(hOutputRead);
        if (processInfo.dwFlags & REDIRECT_STANDARD_ERROR) CloseHandle(hErrorRead);
    }

    return bRet;
}

using namespace purelib::inet;

template <size_t _Size>
void append_string(std::vector<char> &packet, const char(&message)[_Size]) {
    packet.insert(packet.end(), message, message + _Size - 1);
}

int main(int, char **) {

    RedirectProcessInfo pi;
    ZeroMemory(&pi, sizeof(pi));
    pi.dwFlags = REDIRECT_STANDARD_OUTPUT | REDIRECT_STANDARD_ERROR;
    auto bRet = CreateRedirectProcess("cmd /c dsf hello world!", pi);
    if (bRet) {
        std::string output, error;
        auto ec = ReadPipeToEnd(pi.hOutputRead, output);
        ec = ReadPipeToEnd(pi.hErrorRead, error);
        CloseHandle(pi.hProcess);
    }

    purelib::inet::channel_endpoint endpoints[] = {
        "www.ip138.com",
        80,                 // http client
        {"0.0.0.0", 56981}, // tcp server
    };
    myasio->start_service(endpoints, _ARRAYSIZE(endpoints));

    deadline_timer t0(*myasio);

    t0.expires_from_now(std::chrono::seconds(3));
    t0.async_wait([](bool) { // called at network thread
        printf("the timer is expired\n");
    });

    std::vector<std::shared_ptr<channel_transport>> transports;

    myasio->set_callbacks(
        [](char *data, size_t datalen, int &len) { // decode pdu length func
        if (datalen >= 4 && data[datalen - 1] == '\n' &&
            data[datalen - 2] == '\r' && data[datalen - 3] == '\n' &&
            data[datalen - 4] == '\r') {
            len = datalen;
        }
        else {
            data[datalen] = '\0';
            auto ptr = strcasestr(data, "Content-Length:");

            if (ptr != nullptr) {
                ptr += (sizeof("Content-Length:") - 1);
                if (static_cast<int>(ptr - data) < static_cast<int>(datalen)) {
                    while (static_cast<int>(ptr - data) < static_cast<int>(datalen) &&
                        !isdigit(*ptr))
                        ++ptr;
                    if (isdigit(*ptr)) {
                        int bodylen = static_cast<int>(strtol(ptr, nullptr, 10));
                        if (bodylen > 0) {
                            ptr = strstr(ptr, "\r\n\r\n");
                            if (ptr != nullptr) {
                                ptr += (sizeof("\r\n\r\n") - 1);
                                len = bodylen + (ptr - data);
                            }
                        }
                    }
                }
            }
        }
        return true;
    },
        [&](size_t, std::shared_ptr<channel_transport> transport,
            int ec) { // connect response callback
        if (ec == 0) {
            // printf("[index: %zu] connect succeed.\n", index);
            std::vector<char> packet;
            append_string(packet, "GET /index.htm HTTP/1.1\r\n");
            append_string(packet, "Host: www.ip138.com\r\n");
            append_string(packet, "User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                "WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                "Chrome/51.0.2704.106 Safari/537.36\r\n");
            append_string(packet, "Accept: */*;q=0.8\r\n");
            append_string(packet, "Connection: Close\r\n\r\n");

            transports.push_back(transport);

            myasio->write(transport, std::move(packet));
        }
        else {
            // printf("[index: %zu] connect failed!\n");
        }
    },
        [&](std::shared_ptr<channel_transport> transport) { // on connection lost
    },
        [&](std::vector<char> &&packet) { // on receive packet
        packet.push_back('\0');
        printf("receive data:%s", packet.data());
    },
        [](const vdcallback_t &callback) { // thread safe call
        callback();
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    myasio->open(0, CHANNEL_TCP_CLIENT);
    myasio->open(1, CHANNEL_TCP_SERVER);

    time_t duration = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        myasio->dispatch_received_pdu();
        duration += 50;
        if (duration >= 10000) {
            for (auto transport : transports)
                myasio->close(transport);
            myasio->close(1);
            break;
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(60));

    return 0;
}
