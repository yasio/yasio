yasio-3.23.6
1. Fix jsb & jsb20 compile issue.
2. Rename native interface ```yasio_close2``` to ```yasio_close_handle```
3. Support detect whether ipv6 supported at android platform.
4. Use dual event queue to avoid block recv operation when consumer thread do ```Long-Running Operations``` at event callback. 
5. Remove specific vs project files, use cmake instead.
