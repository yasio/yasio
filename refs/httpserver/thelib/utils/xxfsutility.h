#ifndef _XXFSUTILITY_H_
#define _XXFSUTILITY_H_
#include <string>
#ifdef _WIN32
#include <io.h>
#include <direct.h> // for CRT function: access
#else
#include <unistd.h> // for CRT function: access
#include <sys/types.h> // for CRT function: mkdir
#include <sys/stat.h>  // for CRT function: mkdir
#endif

#include "simple_ptr.h"
#include "unreal_string.h"

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#include <ShlObj.h>
#endif

#include "aes.h"

using namespace thelib;
using namespace thelib::gc;

namespace fsutil {

long             get_file_length(FILE* fp);

unmanaged_string read_file_data(const char* filename);

unmanaged_string read_file_data_ex(const char* filename, bool padding = false/*¨º?¡¤?¨¬?3?¨ºy?Y*/, size_t aes_blocksize = AES_BLOCK_SIZE);

void             write_file_data(const char* filename, const char* data, size_t size);

bool             is_file_exist(const char* filename);

bool             is_type_of(const std::string& filename, const char* type);

bool             is_type_of_v2(const std::string& filename, const char* type);

void             mkdir(const char* directory);

std::pair<std::string, std::string> split_fullpath(const std::string& fullpath);

std::string      get_short_name(const std::string& complete_filename);
std::string      get_path_of(const std::string& complete_filename);

#ifdef _WIN32

#define OFN_FILTER(tips,filters) TEXT(tips) TEXT("\0") TEXT(filters) TEXT("\0")
#define OFN_FILTER_END()         TEXT("\0")

bool             get_open_filename(TCHAR* output, const TCHAR* filters, TCHAR* history = nullptr, HWND hWnd = nullptr);

TCHAR*           get_path(TCHAR* out, TCHAR* history = nullptr, HWND hWnd = nullptr);

INT CALLBACK     _BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM pData);

#endif /* end of win32 utils */

};

#endif

