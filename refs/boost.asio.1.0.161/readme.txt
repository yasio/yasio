#1.This lib is extract from boost_1.61.0, before start use asio, you just need to define follow preprocessors:

BOOST_ERROR_CODE_HEADER_ONLY
BOOST_SYSTEM_NO_DEPRECATED
BOOST_SYSTEM_NO_LIB
BOOST_DATE_TIME_NO_LIB
BOOST_REGEX_NO_LIB 

If use single thread, you should define: BOOST_ASIO_DISABLE_THREADS, but can't use timer

firstly, add aditionnal includes path "Root Of ¡®boost.asio.1.0.159¡¯ Directory" to your compile system, such as MSVC,XCode,GCC,Android.mk.

#2.Detail usage: see example

#3.Currently Win32, Android and Apple(OS X/iOS) compile succeed.

#4.Please open c++11 flags(-std=c++11) for Android compile.
