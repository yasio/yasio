package org.yasio;
import android.content.Context;
import android.net.ConnectivityManager;

public class cares
{
  private static native boolean native_init(ConnectivityManager connectivity_manager);

  public static boolean init(Context context)
  {
    native_init((ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE));
  }
}
