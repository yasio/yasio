2010年底，笔者以实习生身份参与第一个中间服务器项目，首次接触网络编程，了解了posix接口API socket编程，尽管windows也支持posix socket API, 但由于windows本身对posix API设计是基于posix子系统设计，也就是说是基于win32 winsock API封装的, 因此接口使用和linux平台略有差异，另外tcp服务端和客户端进行tcp通讯，需要写繁琐的c代码， 于是，笔者在阅读了unix网络编程后将socket超时连接，监听，socket对象管理，地址管理等常用操作加以封装，写成小巧的xxsocket类: xxsocket.h, xxsocket.cpp， 只有两个文件，可以运行在windows和linux。

后来笔者曾将xxsocket上传到sourceforge, 再辗转迁移至github, 但许久不曾维护

由于xxsocket很简单，也就是一个类而已，因此也就没啥维护的必要，与此同时，笔者也多次拜读过boost.asio框架，对异步socket io有一定了解，但不深入。

后来笔者进入游戏开发行业，为了能在移动端android,ios,wp使用，笔者将xxsocket进一步优化，使其能在这三个平台编译运行，并分享至cocoachina社区

2015年笔者创业，作为cocos游戏客户端负责人， 笔者使用xxsocket编写了客户端tcp独立线程处理收发单利类async_tcp_client, 笔者熟悉了非阻塞模式socket的基本用法， 但当时对非阻塞模式理解依然有限，由于单线程循环处理socket读写事件，读事件自然由select触发，但如何快速处理业务发送请求成了需要解决的问题，于是最早设设计是不用select， 每次循环是sleep(10)毫秒, 然后先调用非阻塞recv，再检查发送队列是否有数据需要发送。后来在xcode上进行性能测试，发现这种傻傻sleep的方式不仅不能及时处理发送请求,至少延迟10ms，还比较占CPU，占大约0.5%~5%(具体记不清了)左右的cpu，当时测试也反馈游戏玩一会儿就手机就发烫。为解决这个问题，笔者测试了boost.asio，发现作为server时，启动io service几乎不占cpu，于是乎笔者再度使用Source Insight拜读boost.asio代码，查看如何快速响应发送, 最后找到了方法，不得不佩服boost.asio作者的巧妙思路, 借鉴过来加以修改后async_tcp_client在xcode上仅仅占0%~0.01%的cpu，且发包延迟地址1ms以下，5us左右， 至此xxsocket 2.0在github上发布。

2017年，笔者在一家棋牌公司，研究了他们的网络线程，bala， 既然也是固定sleep(10)毫秒, 笔者分享了自己的async_tcp_client, 但他们用没用就不得而知了，因为笔者很快离开了那家棋牌公司

2018年, 笔者进入一家做slg游戏的公司，公司
游戏项目是基于cocos2dx 2.x的，客户端网络是在OpenGL线程处理收发，连接的，当然也是用的非阻塞socket模型，这种设计有两个问题，1.由于和渲染在同一线程，无论是否有数据到来，每帧不得不调用select或recv进行用户态和核心态的切换；2. 由于连接超时是用select，在建立tcp连接慢的时候会卡住主线程，菊花转不起来。一年后，笔者将async_tcp_client无缝移植到了项目，业务无需做任何修改，并于2018.10.17日上线，运行于上百万移动终端，crash率为0。 同年4.11日更名为mini-asio并发布3.0版本

2019年2月11日，笔者增强了对lua脚本的支持，并发布3.9.6版本，为表达该库轻量级易于使用的特效库名更名为yasio

时至2019.4.2日， yasio-3.9.11
该库支持lua,cocos-lua,cocos-jsb,cocos creator-jsb2.0, unity3d tolua

3.20.1, 修复了ibstream,obstream在ARM平台可能存在的SIGBUS问题，为脚本提供了高精度计时器API: yasio.highp_clock()和yasio.highp_time()

2019.5.28 v3.23.3f1优化了脚本引擎绑定定时器实现，提供killAll删除所有脚本timer，当虚拟机重启是，调用此接口可以有效清理脚本自身无法清理的定时器

2019.6.5 v3.22.0增加了kcp支持