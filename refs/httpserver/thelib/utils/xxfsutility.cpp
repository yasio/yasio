#include "nsconv.h"
#include "xxfsutility.h"

long fsutil::get_file_length(FILE* fp)
{
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    if(length != 0)
    {
        fseek(fp, 0, SEEK_SET);
    }
    return length;
}

unmanaged_string fsutil::read_file_data(const char* filename)
{
    FILE* fp = fopen(filename, "rb");
    if(fp == nullptr) 
        return (char*)nullptr;

    size_t size = get_file_length(fp);

    char* original( (char*)malloc(size + 1) );
    if(original == nullptr)
        return (char*)nullptr;
    size_t bytes_readed = fread(original, 1, size, fp);
    original[bytes_readed] = '\0';
    fclose(fp);
    return unmanaged_string(original, bytes_readed);
}

unmanaged_string fsutil::read_file_data_ex(const char* filename, bool padding, size_t aes_blocksize)
{
    FILE* fp = fopen(filename, "rb+");
    if(fp == nullptr) 
        return (char*)nullptr;

    size_t size = get_file_length(fp);

    char* original( (char*)malloc(size + aes_blocksize) );

    if(original == nullptr)
        return (char*)nullptr;
    size_t bytes_readed = fread(original, 1, size, fp);

    size_t padding_size = 0;
    if(padding) {
        padding_size = aes_blocksize - size % aes_blocksize;
        for(size_t offst = 0; offst < padding_size; ++offst) 
        {
            *(original + bytes_readed + offst) = padding_size;
        }
    }

    fclose(fp);
    return unmanaged_string(original, bytes_readed + padding_size);
}

void fsutil::write_file_data(const char* filename, const char* data, size_t size)
{
    FILE* fp = fopen(filename, "wb+");
    if(fp == nullptr)
        return;
    fwrite(data, size, 1,  fp);
    fclose(fp);
}

bool fsutil::is_file_exist(const char* filename)
{
    return 0 == access(filename, 0);
}

bool  fsutil::is_type_of(const std::string& filename, const char* type)
{
    size_t off = filename.find_last_of('.');
    if(off != filename.npos)
    {
        std::string ext = filename.substr(off);
        return ext == type;
    }
    return false;
}

bool  fsutil::is_type_of_v2(const std::string& filename, const char* type)
{
    static std::string alltype = "*.*";
    size_t off = filename.find_last_of('.');
    if(off != filename.npos)
    {
        std::string ext = "*" + filename.substr(off); // contains dot
        return ext == type || type == alltype;
    }
    else if(!filename.empty()){
        return type == alltype;
    }
    else {
        return false;
    }
}

void  fsutil::mkdir(const char* _Path)
{
    std::string dir = _Path;
    nsc::replace(dir, "\\", "/");
    std::vector<std::string> results;
    nsc::split(dir.c_str(), '/', results);

    dir = "";
    for(auto iter = results.begin(); iter != results.end(); ++iter)
    {
        dir.append(*iter + "/");
        if(!is_file_exist(dir.c_str()))
        {
#ifdef _WIN32
            ::mkdir(dir.c_str());
#else
            ::mkdir(dir.c_str(), 0);
#endif
        }
    }
}

std::pair<std::string, std::string> fsutil::split_fullpath(const std::string& fullpath)
{
    std::string tmp = fullpath;
    std::pair<std::string, std::string> pr;
    nsc::replace(tmp, "\\", std::string("/"));
    size_t pos = tmp.find_last_of("/");
    if(pos != std::string::npos) {
        try {
            pr.first = tmp.substr(0, pos);
            pr.second = tmp.substr(pos + 1);
        }
        catch(...) {
        }
    }
    return pr;
}

std::string fsutil::get_short_name(const std::string& complete_filename)
{
    size_t pos = complete_filename.find_last_of("\\");

    if(pos == std::string::npos)
        pos = complete_filename.find_last_of("/");

    if(pos != std::string::npos) {
        try {
            return complete_filename.substr(pos + 1);
        }
        catch(...) {
        }
    }
    return "";
}

std::string fsutil::get_path_of(const std::string& complete_filename)
{
    size_t pos = complete_filename.find_last_of("\\");

    if(pos == std::string::npos)
        pos = complete_filename.find_last_of("/");

    if(pos != std::string::npos) {
        try {
            return complete_filename.substr(0, pos);
        }
        catch(...) {
        }
    }
    return "";
}

#ifdef _WIN32
bool fsutil::get_open_filename(TCHAR* output, const TCHAR* filters, TCHAR* history, HWND hwndParent)
{
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof(OPENFILENAME));
    memset(output,0,sizeof(output));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.lpstrFilter = filters;
    ofn.lpstrFile = output;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = history;
    ofn.hwndOwner = hwndParent;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_ALLOWMULTISELECT;
    return GetOpenFileName(&ofn);
}


TCHAR* fsutil::get_path(TCHAR* pBuffer, TCHAR* history, HWND   hWnd)  
{  
    BROWSEINFO   bf;  
    LPITEMIDLIST   lpitem;  

    memset(&bf, 0, sizeof BROWSEINFO);  

    bf.hwndOwner = hWnd;  

    bf.lpszTitle = TEXT("?????¡¤??");  

    bf.ulFlags = BIF_RETURNONLYFSDIRS;

    bf.lParam = (LPARAM)history;

    bf.lpfn = _BrowseCallbackProc;

    lpitem = SHBrowseForFolder(&bf);  

    if(lpitem == NULL) 
        return   TEXT("");  

    SHGetPathFromIDList(lpitem, pBuffer);  

    //lstrcpy(this->histroy_directory, pBuffer);

    //this->save_last_directory(nsc::transcode(this->histroy_directory).c_str());

    return    pBuffer;
}  

INT CALLBACK fsutil::_BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM pData) 
{
    TCHAR szDir[MAX_PATH];
    switch(uMsg) 
    {
    case BFFM_INITIALIZED: 
        // WParam is TRUE since you are passing a path.
        // It would be FALSE if you were passing a pidl.
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)pData);
        break;
    case BFFM_SELCHANGED: 
        // Set the status window to the currently selected path.
        if (SHGetPathFromIDList((LPITEMIDLIST)lParam ,szDir))
        {
            SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)szDir);
        }
        break;
    }
    return 0;
}

#endif


