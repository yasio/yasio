#pragma once
#include "yasio/string_view.hpp"
#include "yasio/byte_buffer.hpp"
#include <fstream>

namespace yasio_ext
{
template <typename _CStr, typename _Fn>
inline void split_cb(_CStr s, size_t slen, typename std::remove_pointer<_CStr>::type delim, _Fn&& func)
{
  auto _Start = s; // the start of every string
  auto _Ptr   = s; // source string iterator
  auto _End   = s + slen;
  while ((_Ptr = strchr(_Ptr, delim)))
  {
    if (_Ptr >= _End)
      break;

    if (_Start <= _Ptr)
      func(_Start, _Ptr);
    _Start = _Ptr + 1;
    ++_Ptr;
  }
  if (_Start <= _End)
    func(_Start, _End);
}

template <typename _Fn>
inline void split_cb(cxx17::string_view s, char delim, _Fn&& func)
{
  split_cb(s.data(), s.length(), delim, std::move(func));
}

inline yasio::sbyte_buffer read_text_file(cxx17::string_view file_path)
{
  std::ifstream fin(file_path.data(), std::ios_base::binary);
  if (fin.is_open())
  {
    fin.seekg(std::ios_base::end);
    auto n = fin.tellg();
    if (n > 0)
    {
      yasio::sbyte_buffer buffer{static_cast<yasio::uint>(n) + 1};
      fin.seekg(std::ios_base::beg);
      fin.read(buffer.data(), n);
      buffer[static_cast<yasio::uint>(n)] = '\0';
      return buffer;
    }
  }
  return yasio::sbyte_buffer{};
}

} // namespace yasio_ext
