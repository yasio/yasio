#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#include <crtdbg.h>
#endif
#include <random>
#include <iostream>
#include "auto_resources.h"
#include "player_server.h"

#ifdef _WIN32
#include <simple++/super_cast.h>
#include <simple++/epp_window.h>
#include <simple++/platform.h>
#include <simple++/controls.h>
#include <simple++/nsconv.h>
#include <simple++/xxsocket.h>
#include <intrin.h>

#include "../win32-projects/vc10/xxpie_server/xxpie_server/resource.h"
#include "ripple_frame.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "simplepp.lib")
#pragma comment(lib, "libxml2_a_dll.lib")

using namespace exx;
using namespace exx::gui;
using namespace exx::gui::controls;

class GameServerControlUI : public epp_window
{
    static const int UI_WIDTH = 500;
    static const int UI_HEIGHT = 360;
public:
    GameServerControlUI(void)
    {
        this->txtIP = nullptr;
        this->txtPort = nullptr;
        this->btnSvrCtl = nullptr;
        this->server = nullptr;
        this->init();
    }
    ~GameServerControlUI(void)
    {
        delete this->server;
        delete this->btnSvrCtl;
        delete this->txtPort;
        delete this->txtIP;
        delete this->txtInfo;
        delete this->btnGenSql;
        delete this->effectUI;
        this->server = nullptr;
        this->btnSvrCtl = nullptr;
        this->txtPort = nullptr;
        this->txtIP = nullptr;
    }

private:

    void init(void)
    {
        this->initComponents();
        this->registerAllHandler();

        this->set_text(TEXT("天上掉馅饼游戏服务器"));
        this->set_dimension(UI_WIDTH, UI_HEIGHT);
        this->set_background(RGB(0x21, 0xc2, 0x56));
        this->append_style(WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX);
        this->enable_opacity();
        this->set_icon(IDI_ICON1);
        this->set_smicon(IDI_ICON1);
        this->set_opaque(0.85f, 0);
        this->update();
        this->show();
    }

    void initComponents(void)
    {
        this->effectUI = new ripple_frame(platform::win32::api::LoadBitmapFromFile(TEXT(".\\background_01.bmp")),  25, this);
        this->effectUI->disable();
        this->txtInfo = new epp_edit(TEXT(""), 0, 200, 495, 80, *this, 
            ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL);

        this->txtIP = new epp_edit(TEXT("0.0.0.0"), 20, 300, 145, 20, *this);
        this->txtPort = new epp_edit(TEXT("8008"), 220, 300, 50, 20, *this, ES_NUMBER);
        this->btnGenSql = new epp_button(TEXT("生成sql"), 400, 300, 80, 20, *this);
        this->btnSvrCtl = new epp_button(TEXT("启动"), 310, 300, 80, 20, *this);

        this->txtInfo->set_font(gui::DEF_FONT_NORM_11);
        this->txtIP->set_font(gui::DEF_FONT_NORM_11);
        this->txtPort->set_font(gui::DEF_FONT_NORM_11);
        this->btnSvrCtl->set_font(gui::DEF_FONT_NORM_11);
        this->btnGenSql->set_font(gui::DEF_FONT_NORM_11);
    }

    void registerAllHandler(void)
    {
        if(!RegisterHotKey(_Myhandle, 12190, MOD_CONTROL, VK_NUMPAD9))
        {
            ::MessageBox(nullptr, TEXT("热键注册失败!"), TEXT("警告"), MB_OK | MB_ICONEXCLAMATION);
        }

        this->register_event(WM_HOTKEY, [this](void)->void {
            if(this->is_visible())
            {
                this->hide();
            }
            else {
                this->show();
            }
        });

        this->register_event(this->btnSvrCtl->get_handle(), [this] (void) ->void {
                this->btnSvrCtl->disable();
                if(lstrcmp(TEXT("启动"), this->btnSvrCtl->get_text().c_str()) == 0) {
                    managed_wstring ip = this->txtIP->get_text();
                    const wchar_t* chk = ip.c_str();
                    this->server = new player_server();

                    bool ret = this->server->start(
                        nsc::transcode(ip.c_str()).c_str(), 
                        nsc::to_numeric<u_short>(this->txtPort->get_text().c_str()) );

                    // try start service
                    if(!ret)
                    {
                        MessageBox(this->get_handle(), TEXT("start service failed!"), TEXT("error"), MB_OK | MB_ICONHAND);
                        delete this->server;
                        this->server = nullptr;
                    }
                    else {
                        this->txtIP->disable();
                        this->txtPort->disable();
                        this->btnSvrCtl->set_text(TEXT("停止"));
                    }
                }
                else {
                    if(this->server != nullptr) {
                        this->server->post_stop_signals();
                        this->server->wait_all_workers();
                        delete this->server;
                        this->server = nullptr;
                        this->txtIP->enable();
                        this->txtPort->enable();
                        this->btnSvrCtl->set_text(TEXT("启动"));
                    }
                }
                this->btnSvrCtl->enable(); } 
        );

#define SQL_STRING_FMT "insert into user_props values(23,2093,128,'2017-06-09 18:59:43', 365);" \
                    "insert into user_props values(24,2093,129,'2017-06-09 18:59:43', 365);" \
                    "insert into user_props values(25,2093,130,'2017-06-09 18:59:43', 365);" \
                    "insert into user_props values(26,2093,131,'2017-06-09 18:59:43', 365);" \
                    "insert into user_props values(27,2093,132,'2017-06-09 18:59:43', 365);" \
                    "insert into user_props values(28,2093,133,'2017-06-09 18:59:43', 365);" \
                    "insert into user_props values(29,2093,134,'2017-06-09 18:59:43', 365);" \
                    "insert into user_props values(30,2093,135,'2017-06-09 18:59:43', 365);" \
                    "insert into user_props values(31,2093,136,'2017-06-09 18:59:43', 365);"

        this->register_event(this->btnGenSql->get_handle(), [this] (void) ->void {
            uint32_t uid = 0;
            managed_wstring text = this->txtInfo->get_text();
            if(!text.empty())
            {
                uid = nsc::to_numeric<uint32_t>(text.c_str());
            }
            static char sqlstr[sizeof(SQL_STRING_FMT) * 2];
            sprintf(sqlstr, 
                "insert into user_props values(23,%d,128,'2017-06-09 18:59:43', 365);\n"
                "insert into user_props values(24,%d,129,'2017-06-09 18:59:43', 365);\n"
                "insert into user_props values(25,%d,130,'2017-06-09 18:59:43', 365);\n"
                "insert into user_props values(26,%d,131,'2017-06-09 18:59:43', 365);\n"
                "insert into user_props values(27,%d,132,'2017-06-09 18:59:43', 365);\n"
                "insert into user_props values(28,%d,133,'2017-06-09 18:59:43', 365);\n"
                "insert into user_props values(29,%d,134,'2017-06-09 18:59:43', 365);\n"
                "insert into user_props values(30,%d,135,'2017-06-09 18:59:43', 365);\n"
                "insert into user_props values(31,%d,136,'2017-06-09 18:59:43', 365);\n",
                uid,uid,uid,uid,uid,uid,uid,uid,uid);
            this->txtInfo->set_text(nsc::transcode(sqlstr).c_str());
        });

    }

private:
    epp_edit*     txtIP;
    epp_edit*     txtPort;
    epp_edit*     txtInfo;
    epp_button*   btnSvrCtl;
    epp_button*   btnGenSql;
    ripple_frame* effectUI;
    player_server* server;
};

#endif

#if 0
/// c++ char is one byte.
static uint16_t decodeLengthField(const char* recv_buf_)
{
    char lenBuf[2];
    lenBuf[0] = recv_buf_[0];
    if( !( lenBuf[0] & 0x80 ) ) 
    { // one bytes indicate length
        return lenBuf[0];
    }
    else { // two bytes indicate length
        lenBuf[0] &= 0x7f;
        lenBuf[1] = recv_buf_[1];
        return ntohs( *(uint16_t*)lenBuf );
    }
}

void encodeLengthField(char* recv_buf_, uint16_t value)
{
    if(value <= 0x7f){
        recv_buf_[0] = static_cast<char>(value);
        recv_buf_[1] = 0;
    }
    else {
        epp::net::conv::to_netval(value ,recv_buf_, false)[0] |= 0x80;
    }
}

void test_codec(void)
{
    char lengthBuffer[2];
    for(uint16_t value = 127; value < 12535; ++value)
    {
        encodeLengthField(lengthBuffer, value);
        if(value != decodeLengthField(lengthBuffer)) {
            throw std::logic_error("codec error");
        }
    }
}
#endif

#ifdef __linux
#include <unistd.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

void sighandler(int signum)
{
    if(SIGUSR1 == signum)
    {
        std::cout << "catched external stop signal, now stop the xxpie_server...\n";
        exx::singleton<player_server>::instance()->post_stop_signals();
        std::cout << "post stop signals to xxpier_server sucessfully.\n";
    }
}

void initd(void)
{
    signal(SIGHUP, sighandler);
    signal(SIGUSR1, sighandler);
}

#if 0
void init_daemon(void)
{
    int pid;
    int i;
    if(pid = fork())
    {
        exit(0);        //是父进程，结束父进程
    }
    else if( pid < 0)
    {
        exit(1);        //fork失败，退出
    }
    //是第一子进程，后台继续执行
    setsid();           //第一子进程成为新的会话组长和进程组长
    //并与控制终端分离
    if(pid = fork())
    {
        exit(0);        //是第一子进程，结束第一子进程
    }
    else if(pid < 0)
    {
        exit(1);        //fork失败，退出
    }
    //是第二子进程，继续
    //第二子进程不再是会话组长
    for(i=0;i< NOFILE;++i)  //关闭打开的文件描述符
    {
        close(i);
    }

    chdir("/tmp");      //改变工作目录到/tmp
    umask(0);           //重设文件创建掩模
    return;
}
#endif

// daemon(0, 0)
#endif

// test mpool performance
void test_mpool_performance(void)
{
    /*int64_t start = rdtsc();
    for(int i = 0; i < 10000; ++i)
    {
        void* p = nullptr;
        p = mpool_alloc(SZ(3,k));
    }

    printf("mpool_alloc use  : %ld\n", rdtsc() - start);

    start = rdtsc();
    for(int i = 0; i < 10000; ++i)
    {
        malloc(SZ(3, k));
    }
    printf("malloc use ----- : %ld\n", rdtsc() - start);

    start = rdtsc();
    for(int i = 0; i < 10000; ++i)
    {
        xmpool_alloc(SZ(3, k));
    }
    printf("xmpool_alloc use : %ld\n", rdtsc() - start);*/
}
static const int value = 't';
#ifdef _WIN32
int main(int argc, char** argv)
{
    return wWinMain(nullptr, nullptr, nullptr, 0);
}

#define atomic_sadd(value) InterLock
PoliteGUIEntry
#else
int main(int argc, char** argv)
#endif
{
#ifdef _WIN32
#ifdef _DEBUG
    enable_leak_check();
#endif
#endif

    xxpie_server_initialize(); // must call firstly
#if defined(_WIN32) 
    GameServerControlUI control;
    control.show();
    control.message_loop();
#else
    initd();
    //init_daemon();
    // std::random rnd;
    const char* ip = "0.0.0.0";
    u_short port = 8008;
    if(argc >= 2) {
        ip = argv[1];
    }
    if(argc >=3)
    {
        port = atoi(argv[2]);
    }
    player_server& serv = * exx::singleton<player_server>::instance();
    if(serv.start(ip, port))
    {
        serv.wait_all_workers();
        std::cout << "stop the xxpier_server sucessfully.\n";
        return 0;
    }
    else {
        std::cout << "start server failed.\n";
        return -1;
    }
#endif
    return 0;
}
