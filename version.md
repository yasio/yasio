yasio-3.30.1
  
1. Add construct channel at io_service constructor, now all options could be set before start_service if you doesn't use the start_service with channel hostents, and mark them deprecated
2. Use cmake for trvis ci cross-platform build at win32, linux, osx, ios
3. Sync script bindings
