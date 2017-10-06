#include "xxsocket.h"

using namespace purelib::inet;

int main(int, char**)
{
    xxsocket s;

    s.open();

    printf("fd:%d, nonblocking: %s", s.native_handle(), s.is_nonblocking() ? "true" : "false");
    s.set_nonblocking(true);

    printf("fd:%d, nonblocking: %s", s.native_handle(), s.is_nonblocking() ? "true" : "false");
    s.close();

    return 0;
}
