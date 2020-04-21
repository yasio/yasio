package org.yasio;

public static class yasio {
  private static native boolean native_init_cares(ConnectivityManager
      connectivity_manager);

      public static boolean initialize(Context context) {
      native_init_cares((ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE));
    }
}

