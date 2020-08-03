// yasio-v3.33
function yasioTest() {
  // ------- start of yasio test ----------
  var hostents = [
    { host: "0.0.0.0", port: 8081 },
    { host: "0.0.0.0", port: 8082 },
  ];

  var yserver = new yasio.io_service(hostents);
  yserver.start(function (event) {
    var kind = event.kind();
    if (kind == yasio.YEK_CONNECT_RESPONSE) {
      cc.log("yasio event --> a connection income, kind=%d", event.kind());
      var tsport = event.transport();
      var obs = new yasio.obstream(256);
      obs.push32();
      obs.write_bool(true);
      obs.write_bool(false);
      obs.write_i8(256);
      obs.write_i16(20001);
      obs.write_i24(-298);
      obs.write_i24(16777215);
      obs.write_i32(20191011);
      obs.write_f(28.9);
      obs.write_lf(209.79);
      obs.write_v("hello client!");
      obs.pop32(obs.length());

      cc.log("yasio server: will send partial1 of data after 3 seconds...");
      var partial1 = obs.sub(0, 10);
      yasio.setTimeout(function () {
        cc.log("yasio server --> send data partial1, length=%d", partial1.length());
        yserver.write(tsport, partial1);

        var partial2 = obs.sub(10);
        cc.log("yasio server: will send partial2 of data after 2 seconds...");

        yasio.setTimeout(function () {
          cc.log("yasio server --> send data partial2, length=%d", partial2.length());
          yserver.write(tsport, partial2);
        }, 2);
      }, 3);
    }
    else if (kind == yasio.YEK_CONNECTION_LOST) {
      cc.log("yasio server: The connection is lost!");
    }
  });
  yserver.set_option(yasio.YOPT_C_LFBFD_PARAMS, 
    0, // channelIndex
    65535, // maxFrameLength, 最大包长度
    0,  // lenghtFieldOffset, 长度字段偏移，相对于包起始字节
    4, // lengthFieldLength, 长度字段大小，支持1字节，2字节，3字节，4字节
    0 // lengthAdjustment：如果长度字段字节大小包含包头，则为0， 否则，这里=包头大小
  );
  yserver.open(0, yasio.YCK_TCP_SERVER);

  var yclient = new yasio.io_service({ host: "127.0.0.1", port: 8081 });

  var tsport_c = null;

  yclient.start(function (event) {
    var kind = event.kind();
    if (kind == yasio.YEK_CONNECT_RESPONSE) {
      cc.log("yasio event --> connect server succeed, kind=%d", event.kind());
      tsport_c = event.transport();
    }
    else if (kind == yasio.YEK_PACKET) {
      cc.log("yasio client --> receive a packet from server, kind=%d, close connect after 3 seconds", event.kind());
      var ibs = event.packet();

      var msg = {};
      ibs.seek(4, yasio.SEEK_CUR); // skip length field
      msg.bval1 = ibs.read_bool();
      msg.bval2 = ibs.read_bool();
      msg.u8val = ibs.read_i8();
      msg.i16val = ibs.read_i16();
      msg.i24val = ibs.read_i24();
      msg.u24val = ibs.read_u24();
      msg.i32val = ibs.read_i32();
      msg.fval = ibs.read_f();
      msg.lfval = ibs.read_lf();
      msg.strval = ibs.read_v();
      cc.log("receive msg from server -->\n msg.bval1=%s\n msg.bval2=%s\n msg.i8val=%d\n msg.i16val=%d\n msg.i24val=%d\n msg.u24val=%d\n msg.i32val=%d\n msg.fval=%s\n msg.lfval=%s\n msg.strval=%s\n",
        msg.bval1.toString(),
        msg.bval2.toString(),
        msg.u8val,
        msg.i16val,
        msg.i24val,
        msg.u24val,
        msg.i32val,
        msg.fval.toString(),
        msg.lfval.toString(),
        msg.strval);

      yasio.setTimeout(function () {
        yasio.clearInterval(cc.yclientID);
        yclient.stop(); // must stop if you never want use the service, make sure it can be GC.
        yclient = null;
      }, 3);
    }
  });

  yclient.set_option(yasio.YOPT_C_LFBFD_PARAMS, 
    0, // channelIndex
    65535, // maxFrameLength, 最大包长度
    0,  // lenghtFieldOffset, 长度字段偏移，相对于包起始字节
    4, // lengthFieldLength, 长度字段大小，支持1字节，2字节，3字节，4字节
    0 // lengthAdjustment：如果长度字段字节大小包含包头，则为0， 否则，这里=包头大小
  );

  yclient.open(0, yasio.YCK_TCP_CLIENT);

  // run the event-loop
  cc.yserverID = yasio.setInterval(function () {
    yserver.dispatch(128);
  }, 0.01);

  cc.yclientID = yasio.setInterval(function () {
    yclient.dispatch(128);
  }, 0.01);

  // ========== end of yasio test ==========
}

yasioTest();
