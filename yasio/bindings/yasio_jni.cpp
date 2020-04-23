//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store universal app
//
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2020 HALX99

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
#ifndef YASIO__JNI_CPP
#define YASIO__JNI_CPP

/*
** yasio_jni.cpp: The yasio native interface for initialize c-ares on android.
*/
#include "yasio/detail/config.hpp"

#if defined(__ANDROID__) && defined(YASIO_HAVE_CARES)
extern "C" {
#  include "c-ares/ares.h"
static JavaVM* yasio__jvm;
int yasio__ares_init_android()
{
  JNIEnv* env       = nullptr;
  int ret           = ARES_ENOTINITIALIZED;
  bool need_detatch = false;

  do
  {
    if (yasio__jvm == nullptr)
      break;

    jint res = yasio__jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (res == JNI_EDETACHED)
    {
      env          = nullptr;
      res          = yasio__jvm->AttachCurrentThread(&env, nullptr);
      need_detatch = true;
    }
    if (res != JNI_OK)
      break; // Get env failed

    /// Gets application context
    jclass ActivityThread           = env->FindClass("android/app/ActivityThread");
    jmethodID currentActivityThread = env->GetStaticMethodID(
        ActivityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
    jobject current_activity = env->CallStaticObjectMethod(ActivityThread, currentActivityThread);
    jmethodID getApplication =
        env->GetMethodID(ActivityThread, "getApplication", "()Landroid/app/Application;");
    jobject app_context = env->CallObjectMethod(current_activity, getApplication);

    /// Gets connectivity_manager
    jclass Context = (env)->FindClass("android/content/Context");
    jmethodID getSystemService =
        (env)->GetMethodID(Context, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jfieldID fid = env->GetStaticFieldID(Context, "CONNECTIVITY_SERVICE", "Ljava/lang/String;");
    jstring jstrServiceName = (jstring)env->GetStaticObjectField(Context, fid);

    jobject connectivity_manager =
        env->CallObjectMethod(app_context, getSystemService, jstrServiceName);
    if (connectivity_manager != nullptr)
    {
      ret = ::ares_library_init_android(connectivity_manager);
      env->DeleteLocalRef(connectivity_manager);
    }
    else
      YASIO_LOG("[c-ares] Gets CONNECTIVITY_SERVICE failed, please ensure you have permission "
                "'ACCESS_NETWORK_STATE'");

    env->DeleteLocalRef(jstrServiceName);
    env->DeleteLocalRef(Context);
    env->DeleteLocalRef(app_context);
    env->DeleteLocalRef(current_activity);
    env->DeleteLocalRef(ActivityThread);
  } while (false);

  if (need_detatch)
    yasio__jvm->DetachCurrentThread();

  return ret;
}

int yasio__jni_onload(void* vm, void* reserved)
{
  yasio__jvm = (JavaVM*)vm;
  ::ares_library_init_jvm(yasio__jvm);
  return JNI_VERSION_1_6;
}
#  if defined(YASIO_BUILD_AS_SHARED)
jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) { yasio__jni_onload(vm, reserved); }
#  endif
}
#endif

#endif
