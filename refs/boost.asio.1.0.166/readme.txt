#1.This lib is extract from boost_1.66.0, before start use asio, you just need to define follow preprocessors:

BOOST_ERROR_CODE_HEADER_ONLY
BOOST_SYSTEM_NO_DEPRECATED
BOOST_SYSTEM_NO_LIB
BOOST_DATE_TIME_NO_LIB
BOOST_REGEX_NO_LIB 

If use single thread, you should define: BOOST_ASIO_DISABLE_THREADS, but can't use timer

firstly, add aditionnal includes path "Root Of boost.asio.1.0.166 Directory" to your compile system, such as MSVC,XCode,GCC,Android.mk.

#2.Detail usage: see example

#3.Currently Win32, Android and Apple(OS X/iOS) compile succeed.

#4.Please open c++11 flags(-std=c++11) for Android compile.

#5.Other macro options:
BOOST_ASIO_DISABLE_DEV_POLL: Explicitly disables /dev/poll support on Solaris, forcing the use of a select-based implementation. 
BOOST_ASIO_DISABLE_EPOLL: Explicitly disables epoll support on Linux, forcing the use of a select-based implementation. 
BOOST_ASIO_DISABLE_EVENTFD: Explicitly disables eventfd support on Linux, forcing the use of a pipe to interrupt blocked epoll/select system calls. 
BOOST_ASIO_DISABLE_KQUEUE: Explicitly disables kqueue support on Mac OS X and BSD variants, forcing the use of a select-based implementation. 
BOOST_ASIO_DISABLE_IOCP: Explicitly disables I/O completion ports support on Windows, forcing the use of a select-based implementation. 

More detail: http://think-async.com/Asio/boost_asio_1_10_6/doc/html/boost_asio/using.html
