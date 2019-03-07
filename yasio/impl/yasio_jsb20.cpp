//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app 
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2019 halx99

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "yasio/yasio_jsb20.h"
#include "yasio/yasio.h"
#include "yasio/ibinarystream.h"
#include "yasio/obinarystream.h"
#include "yasio/detail/ref_ptr.h"

#include "cocos2d.h"
#include "cocos/scripting/js-bindings/jswrapper/SeApi.h"
#include "cocos/scripting/js-bindings/manual/jsb_conversions.hpp"
#include "cocos/scripting/js-bindings/manual/jsb_global.h"
#include "cocos/platform/CCApplication.h"
#include "cocos/base/CCScheduler.h"

using namespace purelib;
using namespace purelib::inet;
using namespace cocos2d;

namespace yasio_jsb
{

#define YASIO_DEFINE_REFERENCE_CLASS                                                               \
private:                                                                                           \
  unsigned int referenceCount_;                                                                    \
                                                                                                   \
public:                                                                                            \
  void retain() { ++referenceCount_; }                                                             \
  void release()                                                                                   \
  {                                                                                                \
    --referenceCount_;                                                                             \
    if (referenceCount_ == 0)                                                                      \
      delete this;                                                                                 \
  }                                                                                                \
                                                                                                   \
private:

typedef void *TIMER_ID;
typedef std::function<void(void)> vcallback_t;
struct TimerObject
{
  TimerObject(vcallback_t &&callback) : callback_(std::move(callback)), referenceCount_(1) {}

  vcallback_t callback_;
  static uintptr_t s_timerId;

  DEFINE_OBJECT_POOL_ALLOCATION(TimerObject, 128)
  YASIO_DEFINE_REFERENCE_CLASS
};

uintptr_t TimerObject::s_timerId = 0;

TIMER_ID loop(unsigned int n, float interval, vcallback_t callback)
{
  if (n > 0 && interval >= 0)
  {
    ref_ptr<TimerObject> timerObj(new TimerObject(std::move(callback)));

    auto timerId = reinterpret_cast<TIMER_ID>(++TimerObject::s_timerId);

    std::string key = StringUtils::format("SIMPLE_TIMER_%p", timerId);

    Application::getInstance()->getScheduler()->schedule(
        [timerObj](
            float /*dt*/) { // lambda expression hold the reference of timerObj automatically.
          timerObj->callback_();
        },
        timerId, interval, n - 1, 0, false, key);

    return timerId;
  }
  return nullptr;
}

TIMER_ID delay(float delay, vcallback_t callback)
{
  if (delay > 0)
  {
    ref_ptr<TimerObject> timerObj(new TimerObject(std::move(callback)));
    auto timerId = reinterpret_cast<TIMER_ID>(++TimerObject::s_timerId);

    std::string key = StringUtils::format("SIMPLE_TIMER_%p", timerId);
    Application::getInstance()->getScheduler()->schedule(
        [timerObj](
            float /*dt*/) { // lambda expression hold the reference of timerObj automatically.
          timerObj->callback_();
        },
        timerId, 0, 0, delay, false, key);

    return timerId;
  }
  return nullptr;
}

void kill(TIMER_ID timerId)
{
  std::string key = StringUtils::format("SIMPLE_TIMER_%p", timerId);
  Application::getInstance()->getScheduler()->unschedule(key, timerId);
}

class ibstream : public ibinarystream
{
public:
  ibstream(std::vector<char> blob) : ibinarystream(), blob_(std::move(blob))
  {
    this->assign(blob_.data(), blob_.size());
  }
  ibstream(const obinarystream *obs) : ibinarystream(), blob_(obs->buffer())
  {
    this->assign(blob_.data(), blob_.size());
  }

private:
  std::vector<char> blob_;
};
} // namespace yasio_jsb

/////////////// javascript like setTimeout, clearTimeout setInterval, clearInterval ///////////
bool jsb_yasio_setTimeout(se::State &s)
{
  const auto &args = s.args();
  size_t argc      = args.size();

  do
  {
    if (argc == 2)
    {
      auto arg0 = args[0];
      auto arg1 = args[1];
      CC_BREAK_IF(!arg0.toObject()->isFunction());

      yasio_jsb::vcallback_t callback = [=]() {
        se::ScriptEngine::getInstance()->clearException();
        se::AutoHandleScope hs;

        se::Value rval;
        se::Object *funcObj = arg0.toObject();
        bool succeed        = funcObj->call(args, nullptr, &rval);
        if (!succeed)
        {
          se::ScriptEngine::getInstance()->clearException();
        }
      };

      float timeout = 0;
      seval_to_float(arg1, &timeout);
      auto timerId = yasio_jsb::delay(timeout, std::move(callback));

      s.rval().setNumber((double)(int64_t)timerId);
      return true;
    }
  } while (false);

  SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 2);
  return false;
}
SE_BIND_FUNC(jsb_yasio_setTimeout)

bool jsb_yasio_setInterval(se::State &s)
{
  const auto &args = s.args();
  size_t argc      = args.size();

  do
  {
    if (argc == 2)
    {
      auto arg0 = args[0];
      auto arg1 = args[1];
      CC_BREAK_IF(!arg0.toObject()->isFunction());

      yasio_jsb::vcallback_t callback = [=]() {
        se::ScriptEngine::getInstance()->clearException();
        se::AutoHandleScope hs;

        se::Value rval;
        se::Object *funcObj = arg0.toObject();
        bool succeed        = funcObj->call(args, nullptr, &rval);
        if (!succeed)
        {
          se::ScriptEngine::getInstance()->clearException();
        }
      };

      float interval = 0;
      seval_to_float(arg1, &interval);
      auto timerId = yasio_jsb::loop((std::numeric_limits<unsigned int>::max)(), interval,
                                     std::move(callback));

      s.rval().setNumber((double)(int64_t)timerId);
      return true;
    }
  } while (false);

  SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 2);
  return false;
}
SE_BIND_FUNC(jsb_yasio_setInterval)

bool jsb_yasio_killTimer(se::State &s)
{
  const auto &args = s.args();
  size_t argc      = args.size();

  do
  {
    if (argc == 1)
    {
      auto arg0 = args[0];
      void *id  = (void *)(int64_t)arg0.toNumber();

      yasio_jsb::kill(id);

      s.rval().setUndefined();
      return true;
    }
  } while (false);

  SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 1);
  return false;
}
SE_BIND_FUNC(jsb_yasio_killTimer)

static bool seval_to_hostent(const se::Value &v, inet::io_hostent *ret)
{
  assert(v.isObject() && ret != nullptr);
  se::Object *obj = v.toObject();
  se::Value host;
  se::Value port;
  bool ok = obj->getProperty("host", &host);
  SE_PRECONDITION3(ok && host.isString(), false, );
  ok = obj->getProperty("port", &port);
  SE_PRECONDITION3(ok && port.isNumber(), false, );

  ret->host_ = host.toString();
  ret->port_ = port.toUint16();
  return true;
}

static bool seval_to_std_vector_hostent(const se::Value &v, std::vector<inet::io_hostent> *ret)
{
  assert(ret != nullptr);
  assert(v.isObject());
  se::Object *obj = v.toObject();
  assert(obj->isArray());
  uint32_t len = 0;
  if (obj->getArrayLength(&len))
  {
    se::Value value;
    for (uint32_t i = 0; i < len; ++i)
    {
      SE_PRECONDITION3(obj->getArrayElement(i, &value), false, ret->clear());
      assert(value.isObject());
      io_hostent ioh;
      seval_to_hostent(value, &ioh);
      ret->push_back(std::move(ioh));
    }
    return true;
  }

  ret->clear();
  return false;
}

//////////////////// common template functions //////////////

template <typename T> static bool jsb_yasio_constructor(se::State &s)
{
  auto cobj = new (std::nothrow) T();
  s.thisObject()->setPrivateData(cobj);
  se::NonRefNativePtrCreatedByCtorMap::emplace(cobj);
  return true;
}

template <typename T> static bool jsb_yasio_finalize(se::State &s)
{
  CCLOG("jsbindings: finalizing JS object %p(%s)", s.nativeThisObject(), typeid(T).name());
  auto iter = se::NonRefNativePtrCreatedByCtorMap::find(s.nativeThisObject());
  if (iter != se::NonRefNativePtrCreatedByCtorMap::end())
  {
    se::NonRefNativePtrCreatedByCtorMap::erase(iter);
    T *cobj = (T *)s.nativeThisObject();
    delete cobj;
  }
  return true;
}

///////////////////////// ibstream /////////////////////////////////

static auto js_yasio_ibstream_finalize = jsb_yasio_finalize<yasio_jsb::ibstream>;
SE_BIND_FINALIZE_FUNC(js_yasio_ibstream_finalize)

static bool js_yasio_ibstream_read_bool(se::State &s)
{
  yasio_jsb::ibstream *cobj = (yasio_jsb::ibstream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  s.rval().setBoolean(cobj->read_ix<bool>());

  return true;
}
SE_BIND_FUNC(js_yasio_ibstream_read_bool)

// for int8_t, int16_t, int32_t
template <typename T> static bool js_yasio_ibstream_read_ix(se::State &s)
{
  yasio_jsb::ibstream *cobj = (yasio_jsb::ibstream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  s.rval().setInt32(cobj->read_ix<T>());

  return true;
}
static auto js_yasio_ibstream_read_i8  = js_yasio_ibstream_read_ix<int8_t>;
static auto js_yasio_ibstream_read_i16 = js_yasio_ibstream_read_ix<int16_t>;
static auto js_yasio_ibstream_read_i32 = js_yasio_ibstream_read_ix<int32_t>;
SE_BIND_FUNC(js_yasio_ibstream_read_i8)
SE_BIND_FUNC(js_yasio_ibstream_read_i16)
SE_BIND_FUNC(js_yasio_ibstream_read_i32)

// for uint8_t, uint16_t, uint32_t
template <typename T> static bool js_yasio_ibstream_read_ux(se::State &s)
{
  yasio_jsb::ibstream *cobj = (yasio_jsb::ibstream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  s.rval().setUint32(cobj->read_ix<T>());

  return true;
}
static auto js_yasio_ibstream_read_u8  = js_yasio_ibstream_read_ux<uint8_t>;
static auto js_yasio_ibstream_read_u16 = js_yasio_ibstream_read_ux<uint16_t>;
static auto js_yasio_ibstream_read_u32 = js_yasio_ibstream_read_ux<uint32_t>;
SE_BIND_FUNC(js_yasio_ibstream_read_u8)
SE_BIND_FUNC(js_yasio_ibstream_read_u16)
SE_BIND_FUNC(js_yasio_ibstream_read_u32)

// for int64_t, float, double
template <typename T> static bool js_yasio_ibstream_read_dx(se::State &s)
{
  yasio_jsb::ibstream *cobj = (yasio_jsb::ibstream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  s.rval().setNumber(double(cobj->read_ix<T>()));

  return true;
}
static auto js_yasio_ibstream_read_i64 = js_yasio_ibstream_read_dx<int64_t>;
static auto js_yasio_ibstream_read_f   = js_yasio_ibstream_read_dx<float>;
static auto js_yasio_ibstream_read_lf  = js_yasio_ibstream_read_dx<double>;
SE_BIND_FUNC(js_yasio_ibstream_read_i64)
SE_BIND_FUNC(js_yasio_ibstream_read_f)
SE_BIND_FUNC(js_yasio_ibstream_read_lf)

static bool js_yasio_ibstream_read_i24(se::State &s)
{
  yasio_jsb::ibstream *cobj = (yasio_jsb::ibstream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  s.rval().setInt32(cobj->read_i24());

  return true;
}
SE_BIND_FUNC(js_yasio_ibstream_read_i24)

static bool js_yasio_ibstream_read_u24(se::State &s)
{
  yasio_jsb::ibstream *cobj = (yasio_jsb::ibstream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  s.rval().setUint32(cobj->read_u24());

  return true;
}
SE_BIND_FUNC(js_yasio_ibstream_read_u24)

static bool js_yasio_ibstream_read_string(se::State &s)
{
  yasio_jsb::ibstream *cobj = (yasio_jsb::ibstream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  auto sv = cobj->read_v();

  s.rval().setString(std::string(sv.data(), sv.length()));

  return true;
}
SE_BIND_FUNC(js_yasio_ibstream_read_string)

void js_register_yasio_ibstream(se::Object *obj)
{
  auto cls = se::Class::create("ibstream", obj, nullptr, nullptr);

#define DEFINE_IBSTREAM_FUNC(funcName)                                                             \
  cls->defineFunction(#funcName, _SE(js_yasio_ibstream_##funcName))

  DEFINE_IBSTREAM_FUNC(read_bool);
  DEFINE_IBSTREAM_FUNC(read_i8);
  DEFINE_IBSTREAM_FUNC(read_i16);
  DEFINE_IBSTREAM_FUNC(read_i24);
  DEFINE_IBSTREAM_FUNC(read_i32);
  DEFINE_IBSTREAM_FUNC(read_i64);
  DEFINE_IBSTREAM_FUNC(read_u8);
  DEFINE_IBSTREAM_FUNC(read_u16);
  DEFINE_IBSTREAM_FUNC(read_u24);
  DEFINE_IBSTREAM_FUNC(read_u32);
  DEFINE_IBSTREAM_FUNC(read_f);
  DEFINE_IBSTREAM_FUNC(read_lf);
  DEFINE_IBSTREAM_FUNC(read_string);
  cls->defineFinalizeFunction(_SE(js_yasio_ibstream_finalize));
  cls->install();
  JSBClassType::registerClass<yasio_jsb::ibstream>(cls);

  se::ScriptEngine::getInstance()->clearException();
}

///////////////////////// obstream /////////////////////////////////
se::Object *__jsb_yasio_obstream_proto = nullptr;
se::Class *__jsb_yasio_obstream_class  = nullptr;
static auto js_yasio_obstream_finalize = jsb_yasio_finalize<obinarystream>;
SE_BIND_FINALIZE_FUNC(js_yasio_obstream_finalize)

static bool jsb_yasio_obstream_constructor(se::State &s)
{
  const auto &args = s.args();
  size_t argc      = args.size();

  obinarystream *cobj = nullptr;
  if (argc == 0)
  {
    cobj = new (std::nothrow) obinarystream();
  }
  else
  {
    auto arg0 = args[0];
    if (arg0.isNumber())
      cobj = new (std::nothrow) obinarystream(arg0.toUint32());
    else
      cobj = new (std::nothrow) obinarystream();
  }

  s.thisObject()->setPrivateData(cobj);
  se::NonRefNativePtrCreatedByCtorMap::emplace(cobj);

  return true;
}
SE_BIND_CTOR(jsb_yasio_obstream_constructor, __jsb_yasio_obstream_class, js_yasio_obstream_finalize)

static bool js_yasio_obstream_push32(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  cobj->push32();

  s.rval().setUndefined();

  return true;
}
SE_BIND_FUNC(js_yasio_obstream_push32)

static bool js_yasio_obstream_pop32(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  if (argc == 0)
  {
    cobj->pop32();
  }
  else if (argc == 1)
  {
    cobj->pop32(args[0].toInt32());
  }

  s.rval().setUndefined();

  return true;
}
SE_BIND_FUNC(js_yasio_obstream_pop32)

static bool js_yasio_obstream_push24(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  cobj->push24();

  s.rval().setUndefined();

  return true;
}
SE_BIND_FUNC(js_yasio_obstream_push24)

static bool js_yasio_obstream_pop24(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  if (argc == 0)
  {
    cobj->pop24();
  }
  else if (argc == 1)
  {
    cobj->pop24(args[0].toInt32());
  }

  s.rval().setUndefined();

  return true;
}
SE_BIND_FUNC(js_yasio_obstream_pop24)

static bool js_yasio_obstream_push16(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  cobj->push16();

  s.rval().setUndefined();

  return true;
}
SE_BIND_FUNC(js_yasio_obstream_push16)

static bool js_yasio_obstream_pop16(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  if (argc == 0)
  {
    cobj->pop16();
  }
  else if (argc == 1)
  {
    cobj->pop16(args[0].toInt32());
  }

  s.rval().setUndefined();

  return true;
}
SE_BIND_FUNC(js_yasio_obstream_pop16)

static bool js_yasio_obstream_push8(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  cobj->push8();

  s.rval().setUndefined();

  return true;
}
SE_BIND_FUNC(js_yasio_obstream_push8)

static bool js_yasio_obstream_pop8(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  if (argc == 0)
  {
    cobj->pop8();
  }
  else if (argc == 1)
  {
    cobj->pop8(args[0].toInt32());
  }

  s.rval().setUndefined();

  return true;
}
SE_BIND_FUNC(js_yasio_obstream_pop8)

static bool js_yasio_obstream_write_bool(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  cobj->write_i<bool>(args[0].toBoolean());

  s.rval().setUndefined();

  return true;
}
SE_BIND_FUNC(js_yasio_obstream_write_bool)

template <typename T> static bool js_yasio_obstream_write_ix(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  cobj->write_i<T>(args[0].toUint32());

  s.rval().setUndefined();

  return true;
}
static auto js_yasio_obstream_write_i8  = js_yasio_obstream_write_ix<uint8_t>;
static auto js_yasio_obstream_write_i16 = js_yasio_obstream_write_ix<uint16_t>;
static auto js_yasio_obstream_write_i32 = js_yasio_obstream_write_ix<uint32_t>;
SE_BIND_FUNC(js_yasio_obstream_write_i8)
SE_BIND_FUNC(js_yasio_obstream_write_i16)
SE_BIND_FUNC(js_yasio_obstream_write_i32)

static bool js_yasio_obstream_write_i24(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  cobj->write_i24(args[0].toUint32());

  s.rval().setUndefined();

  return true;
}
SE_BIND_FUNC(js_yasio_obstream_write_i24)

template <typename T> static bool js_yasio_obstream_write_dx(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  double argval = 0;
  seval_to_double(args[0], &argval);
  cobj->write_i<T>(static_cast<T>(argval));

  s.rval().setUndefined();

  return true;
}
static auto js_yasio_obstream_write_i64 = js_yasio_obstream_write_dx<int64_t>;
static auto js_yasio_obstream_write_f   = js_yasio_obstream_write_dx<float>;
static auto js_yasio_obstream_write_lf  = js_yasio_obstream_write_dx<double>;
SE_BIND_FUNC(js_yasio_obstream_write_i64)
SE_BIND_FUNC(js_yasio_obstream_write_f)
SE_BIND_FUNC(js_yasio_obstream_write_lf)

bool js_yasio_obstream_write_string(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  auto &str = args[0].toString();
  cobj->write_v(str.c_str(), str.length());

  s.rval().setUndefined();

  return true;
}
SE_BIND_FUNC(js_yasio_obstream_write_string)

bool js_yasio_obstream_length(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  auto length = cobj->length();
  s.rval().setUint32(length);

  return true;
}
SE_BIND_FUNC(js_yasio_obstream_length)

bool js_yasio_obstream_sub(se::State &s)
{
  auto cobj = (obinarystream *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  do
  {
    size_t offset = 0;
    size_t count  = -1;
    CC_BREAK_IF(argc == 0);

    if (argc >= 1)
    {
      offset = args[0].toInt32();
    }
    if (argc >= 2)
    {
      count = args[1].toInt32();
    }

    auto subobj = new (std::nothrow) obinarystream(cobj->sub(offset, count));
    native_ptr_to_seval<obinarystream>(subobj, &s.rval());

    return true;
  } while (false);

  SE_REPORT_ERROR("wrong number of arguments: %d", (int)argc);
  return false;
}
SE_BIND_FUNC(js_yasio_obstream_sub)

void js_register_yasio_obstream(se::Object *obj)
{
#define DEFINE_OBSTREAM_FUNC(funcName)                                                             \
  cls->defineFunction(#funcName, _SE(js_yasio_obstream_##funcName))
  auto cls = se::Class::create("obstream", obj, nullptr, _SE(jsb_yasio_obstream_constructor));

  DEFINE_OBSTREAM_FUNC(push32);
  DEFINE_OBSTREAM_FUNC(push24);
  DEFINE_OBSTREAM_FUNC(push16);
  DEFINE_OBSTREAM_FUNC(push8);
  DEFINE_OBSTREAM_FUNC(pop32);
  DEFINE_OBSTREAM_FUNC(pop24);
  DEFINE_OBSTREAM_FUNC(pop16);
  DEFINE_OBSTREAM_FUNC(pop8);
  DEFINE_OBSTREAM_FUNC(write_bool);
  DEFINE_OBSTREAM_FUNC(write_i8);
  DEFINE_OBSTREAM_FUNC(write_i16);
  DEFINE_OBSTREAM_FUNC(write_i24);
  DEFINE_OBSTREAM_FUNC(write_i32);
  DEFINE_OBSTREAM_FUNC(write_i64);
  DEFINE_OBSTREAM_FUNC(write_f);
  DEFINE_OBSTREAM_FUNC(write_lf);
  DEFINE_OBSTREAM_FUNC(write_string);
  DEFINE_OBSTREAM_FUNC(length);
  DEFINE_OBSTREAM_FUNC(sub);

  cls->defineFinalizeFunction(_SE(js_yasio_obstream_finalize));
  cls->install();
  JSBClassType::registerClass<obinarystream>(cls);
  __jsb_yasio_obstream_proto = cls->getProto();
  __jsb_yasio_obstream_class = cls;

  se::ScriptEngine::getInstance()->clearException();
}

///////////////////////// transport_ptr ///////////////////////////////
static auto jsb_yasio_transport_ptr_finalize = jsb_yasio_finalize<transport_ptr>;
SE_BIND_FINALIZE_FUNC(jsb_yasio_transport_ptr_finalize)

void js_register_yasio_transport_ptr(se::Object *obj)
{
  auto cls = se::Class::create("transport_ptr", obj, nullptr, nullptr);

  cls->defineFinalizeFunction(_SE(jsb_yasio_transport_ptr_finalize));
  cls->install();
  JSBClassType::registerClass<transport_ptr>(cls);

  se::ScriptEngine::getInstance()->clearException();
}

///////////////////////// io_event /////////////////////////////////

static auto jsb_yasio_io_event_finalize = jsb_yasio_finalize<io_event>;
SE_BIND_FINALIZE_FUNC(jsb_yasio_io_event_finalize)
bool js_yasio_io_event_channel_index(se::State &s)
{
  auto cobj = (io_event *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  s.rval().setInt32(cobj->channel_index());

  return true;
}
SE_BIND_FUNC(js_yasio_io_event_channel_index)

bool js_yasio_io_event_status(se::State &s)
{
  auto cobj = (io_event *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  s.rval().setInt32(cobj->status());

  return true;
}
SE_BIND_FUNC(js_yasio_io_event_status)

bool js_yasio_io_event_kind(se::State &s)
{
  auto cobj = (io_event *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  s.rval().setInt32(cobj->type());

  return true;
}
SE_BIND_FUNC(js_yasio_io_event_kind)

bool js_yasio_io_event_transport(se::State &s)
{
  auto cobj = (io_event *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  native_ptr_to_seval<transport_ptr>(new transport_ptr(cobj->transport()), &s.rval());
  return true;
}
SE_BIND_FUNC(js_yasio_io_event_transport)

bool js_yasio_io_event_take_packet(se::State &s)
{
  auto cobj = (io_event *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  auto packet = cobj->take_packet();

  if (!packet.empty())
  {
    native_ptr_to_seval<yasio_jsb::ibstream>(new yasio_jsb::ibstream(std::move(packet)), &s.rval());
  }
  else
  {
    s.rval().setNull();
  }

  return true;
}
SE_BIND_FUNC(js_yasio_io_event_take_packet)

void js_register_yasio_io_event(se::Object *obj)
{
#define DEFINE_IO_EVENT_FUNC(funcName)                                                             \
  cls->defineFunction(#funcName, _SE(js_yasio_io_event_##funcName))

  auto cls = se::Class::create("io_event", obj, nullptr, nullptr);

  DEFINE_IO_EVENT_FUNC(channel_index);
  DEFINE_IO_EVENT_FUNC(kind);
  DEFINE_IO_EVENT_FUNC(status);
  DEFINE_IO_EVENT_FUNC(transport);
  DEFINE_IO_EVENT_FUNC(take_packet);

  cls->defineFinalizeFunction(_SE(jsb_yasio_transport_ptr_finalize));
  cls->install();
  JSBClassType::registerClass<io_event>(cls);

  se::ScriptEngine::getInstance()->clearException();
}

/////////////// io_service //////////////////////

se::Object *__jsb_yasio_io_service_proto = nullptr;
se::Class *__jsb_yasio_io_service_class  = nullptr;

static auto jsb_yasio_io_service_constructor = jsb_yasio_constructor<io_service>;
static auto jsb_yasio_io_service_finalize    = jsb_yasio_finalize<io_service>;
SE_BIND_FINALIZE_FUNC(jsb_yasio_io_service_finalize)
SE_BIND_CTOR(jsb_yasio_io_service_constructor, __jsb_yasio_io_service_class,
             jsb_yasio_io_service_finalize)

bool js_yasio_io_service_start_service(se::State &s)
{
  auto cobj = (io_service *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  do
  {
    if (argc == 2)
    {
      auto arg0 = args[0]; // hostent or hostents
      auto arg1 = args[1]; // function, callback
      CC_BREAK_IF(!arg1.toObject()->isFunction());

      se::Value jsThis(s.thisObject());
      se::Value jsFunc(args[1]);
      jsThis.toObject()->attachObject(jsFunc.toObject());
      io_event_callback_t callback = [=](inet::event_ptr event) {
        se::ValueArray invokeArgs;
        invokeArgs.resize(1);
        native_ptr_to_seval<io_event>(event.release(), &invokeArgs[0]);
        se::Object *thisObj = jsThis.isObject() ? jsThis.toObject() : nullptr;
        se::Object *funcObj = jsFunc.toObject();
        bool succeed        = funcObj->call(invokeArgs, thisObj);
        if (!succeed)
        {
          se::ScriptEngine::getInstance()->clearException();
        }
      };

      if (arg0.toObject()->isArray())
      {
        std::vector<inet::io_hostent> hostents;
        seval_to_std_vector_hostent(arg0, &hostents);
        cobj->start_service(std::move(hostents), std::move(callback));
      }
      else if (arg0.isObject())
      {
        inet::io_hostent ioh;
        seval_to_hostent(arg0, &ioh);
        cobj->start_service(&ioh, std::move(callback));
      }

      s.rval().setUndefined();
      return true;
    }
  } while (0);

  SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 2);
  return false;
}
SE_BIND_FUNC(js_yasio_io_service_start_service)

bool js_yasio_io_service_stop_service(se::State &s)
{
  auto cobj = (io_service *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");

  cobj->stop_service();
  return true;
}
SE_BIND_FUNC(js_yasio_io_service_stop_service)

bool js_yasio_io_service_open(se::State &s)
{
  auto cobj = (io_service *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  do
  {
    if (argc == 2)
    {
      cobj->open(args[0].toInt32(), args[1].toInt32());
      return true;
    }
  } while (false);

  SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 2);
  return false;
}
SE_BIND_FUNC(js_yasio_io_service_open)

bool js_yasio_io_service_close(se::State &s)
{
  auto cobj = (io_service *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  do
  {
    if (argc == 1)
    {
      auto &arg0 = args[0];
      if (arg0.isNumber())
      {
        cobj->close(arg0.toInt32());
      }
      else if (arg0.isObject())
      {
        transport_ptr *transport = nullptr;
        seval_to_native_ptr<transport_ptr *>(arg0, &transport);
        if (transport != nullptr)
          cobj->close(*transport);
      }
      return true;
    }
  } while (false);

  SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 1);
  return false;
}
SE_BIND_FUNC(js_yasio_io_service_close)

bool js_yasio_io_service_dispatch_events(se::State &s)
{
  auto cobj = (io_service *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  do
  {
    if (argc == 1)
    {
      cobj->dispatch_events(args[0].toInt32());
      return true;
    }
  } while (false);

  SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 1);
  return false;
}
SE_BIND_FUNC(js_yasio_io_service_dispatch_events)

bool js_yasio_io_service_set_option(se::State &s)
{
  auto service = (io_service *)s.nativeThisObject();
  SE_PRECONDITION2(service, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  do
  {
    if (argc >= 2)
    {
      auto &arg0 = args[0];
      auto opt   = arg0.toInt32();
      switch (opt)
      {
        case YASIO_OPT_CHANNEL_REMOTE_HOST:
          service->set_option(opt, args[1].toInt32(), args[2].toString().c_str());
          break;
        case YASIO_OPT_CHANNEL_REMOTE_PORT:
        case YASIO_OPT_CHANNEL_LOCAL_PORT:
          service->set_option(opt, args[1].toInt32(), args[2].toInt32());
          break;
        case YASIO_OPT_TCP_KEEPALIVE:
          service->set_option(opt, args[1].toInt32(), args[2].toInt32(), args[3].toInt32());
          break;
        case YASIO_OPT_LFBFD_PARAMS:
          service->set_option(opt, args[1].toInt32(), args[2].toInt32(), args[3].toInt32(),
                              args[4].toInt32());
          break;
        case YASIO_OPT_RESOLV_FUNCTION: // jsb does not support set custom
                                        // resolv function
          break;
        case YASIO_OPT_IO_EVENT_CALLBACK:
          (void)0;
          {
            se::Value jsThis(s.thisObject());
            se::Value jsFunc(args[1]);
            jsThis.toObject()->attachObject(jsFunc.toObject());
            io_event_callback_t callback = [=](inet::event_ptr event) {
              se::ValueArray invokeArgs;
              invokeArgs.resize(1);
              native_ptr_to_seval<io_event>(event.release(), &invokeArgs[0]);
              se::Object *thisObj = jsThis.isObject() ? jsThis.toObject() : nullptr;
              se::Object *funcObj = jsFunc.toObject();
              bool succeed        = funcObj->call(invokeArgs, thisObj);
              if (!succeed)
              {
                se::ScriptEngine::getInstance()->clearException();
              }
            };
            service->set_option(opt, std::addressof(callback));
          }
          break;
        default:
          service->set_option(opt, args[1].toInt32());
      }

      return true;
    }
  } while (false);

  SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 2);
  return false;
}
SE_BIND_FUNC(js_yasio_io_service_set_option)

bool js_yasio_io_service_write(se::State &s)
{
  auto cobj = (io_service *)s.nativeThisObject();
  SE_PRECONDITION2(cobj, false, __FUNCTION__ ": Invalid Native Object");
  const auto &args = s.args();
  size_t argc      = args.size();

  do
  {
    if (argc == 2)
    {
      auto &arg0 = args[0];
      auto &arg1 = args[1];

      transport_ptr *transport = nullptr;
      seval_to_native_ptr<transport_ptr *>(arg0, &transport);

      if (transport != nullptr)
      {
        if (arg1.isString())
        {
          auto &strVal = arg1.toString();
          if (!strVal.empty())
            cobj->write(*transport,
                        std::vector<char>(strVal.c_str(), strVal.c_str() + strVal.length()));
        }
        else if (arg1.isObject())
        {
          obinarystream *obs = nullptr;
          seval_to_native_ptr(arg1, &obs);
          if (obs)
            cobj->write(*transport, obs->buffer());
        }
      }

      return true;
    }
  } while (false);

  SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 2);
  return false;
}
SE_BIND_FUNC(js_yasio_io_service_write)

void js_register_yasio_io_service(se::Object *obj)
{
#define DEFINE_IO_SERVICE_FUNC(funcName)                                                           \
  cls->defineFunction(#funcName, _SE(js_yasio_io_service_##funcName))
  auto cls = se::Class::create("io_service", obj, nullptr, _SE(jsb_yasio_io_service_constructor));

  DEFINE_IO_SERVICE_FUNC(set_option);
  DEFINE_IO_SERVICE_FUNC(start_service);
  DEFINE_IO_SERVICE_FUNC(stop_service);
  DEFINE_IO_SERVICE_FUNC(open);
  DEFINE_IO_SERVICE_FUNC(close);
  DEFINE_IO_SERVICE_FUNC(dispatch_events);
  DEFINE_IO_SERVICE_FUNC(write);

  cls->defineFinalizeFunction(_SE(jsb_yasio_io_service_finalize));

  cls->install();
  JSBClassType::registerClass<io_service>(cls);

  __jsb_yasio_io_service_proto = cls->getProto();
  __jsb_yasio_io_service_class = cls;

  se::ScriptEngine::getInstance()->clearException();
}

bool jsb_register_yasio(se::Object *obj)
{
  // Get the ns
  se::Value nsVal;
  if (!obj->getProperty("yasio", &nsVal))
  {
    se::HandleObject jsobj(se::Object::createPlainObject());
    nsVal.setObject(jsobj);
    obj->setProperty("yasio", nsVal);
  }
  se::Object *yasio = nsVal.toObject();
  yasio->defineFunction("setTimeout", _SE(jsb_yasio_setTimeout));
  yasio->defineFunction("setInterval", _SE(jsb_yasio_setInterval));
  yasio->defineFunction("clearTimeout", _SE(jsb_yasio_killTimer));
  yasio->defineFunction("clearInterval", _SE(jsb_yasio_killTimer));

  js_register_yasio_ibstream(yasio);
  js_register_yasio_obstream(yasio);
  js_register_yasio_transport_ptr(yasio);
  js_register_yasio_io_event(yasio);
  js_register_yasio_io_service(yasio);

  // ##-- yasio enums
  se::Value __jsvalIntVal;
#define YASIO_SET_INT_PROP(name, value)                                                            \
  __jsvalIntVal.setInt32(value);                                                                   \
  yasio->setProperty(name, __jsvalIntVal)

  YASIO_SET_INT_PROP("CHANNEL_TCP_CLIENT", channel_type::CHANNEL_TCP_CLIENT);
  YASIO_SET_INT_PROP("CHANNEL_TCP_SERVER", channel_type::CHANNEL_TCP_SERVER);
  YASIO_SET_INT_PROP("CHANNEL_UDP_CLIENT", channel_type::CHANNEL_UDP_CLIENT);
  YASIO_SET_INT_PROP("CHANNEL_UDP_SERVER", channel_type::CHANNEL_UDP_SERVER);
  YASIO_SET_INT_PROP("YASIO_OPT_CONNECT_TIMEOUT", YASIO_OPT_CONNECT_TIMEOUT);
  YASIO_SET_INT_PROP("YASIO_OPT_SEND_TIMEOUT", YASIO_OPT_CONNECT_TIMEOUT);
  YASIO_SET_INT_PROP("YASIO_OPT_RECONNECT_TIMEOUT", YASIO_OPT_RECONNECT_TIMEOUT);
  YASIO_SET_INT_PROP("YASIO_OPT_DNS_CACHE_TIMEOUT", YASIO_OPT_DNS_CACHE_TIMEOUT);
  YASIO_SET_INT_PROP("YASIO_OPT_DEFER_EVENT", YASIO_OPT_DEFER_EVENT);
  YASIO_SET_INT_PROP("YASIO_OPT_TCP_KEEPALIVE", YASIO_OPT_TCP_KEEPALIVE);
  YASIO_SET_INT_PROP("YASIO_OPT_RESOLV_FUNCTION", YASIO_OPT_RESOLV_FUNCTION);
  YASIO_SET_INT_PROP("YASIO_OPT_LOG_FILE", YASIO_OPT_LOG_FILE);
  YASIO_SET_INT_PROP("YASIO_OPT_LFBFD_PARAMS", YASIO_OPT_LFBFD_PARAMS);
  YASIO_SET_INT_PROP("YASIO_OPT_IO_EVENT_CALLBACK", YASIO_OPT_IO_EVENT_CALLBACK);
  YASIO_SET_INT_PROP("YASIO_OPT_CHANNEL_LOCAL_PORT", YASIO_OPT_CHANNEL_LOCAL_PORT);
  YASIO_SET_INT_PROP("YASIO_OPT_CHANNEL_REMOTE_HOST", YASIO_OPT_CHANNEL_REMOTE_HOST);
  YASIO_SET_INT_PROP("YASIO_OPT_CHANNEL_REMOTE_PORT", YASIO_OPT_CHANNEL_REMOTE_PORT);
  YASIO_SET_INT_PROP("YASIO_EVENT_CONNECT_RESPONSE", YASIO_EVENT_CONNECT_RESPONSE);
  YASIO_SET_INT_PROP("YASIO_EVENT_CONNECTION_LOST", YASIO_EVENT_CONNECTION_LOST);
  YASIO_SET_INT_PROP("YASIO_EVENT_RECV_PACKET", YASIO_EVENT_RECV_PACKET);

  return true;
}
