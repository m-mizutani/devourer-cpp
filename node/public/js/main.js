function get_unixtime() {
  return (new Date) / 1000;
}

$(document).ready(function() {
  var socket = io.connect(location.protocol + '//' + location.host);
  var updated_flow = {};
  socket.on('flow.update', function (msg) {
    $('div.flow').removeClass('active');
    for (var hv in msg.flow_size) {
      $('div.flow#' + hv).addClass('active');
      updated_flow[hv] = get_unixtime();
    }
    console.log(msg);
  });
  socket.on('flow.new', function (msg) {
    if (msg.dst_port === 53) {
      return;
    }
    var txt = '(' + msg.proto + ') ' +
        (msg.src_name !== undefined ? msg.src_name : msg.src_addr) +
        ":" + msg.src_port + ' -> ' +
        (msg.dst_name !== undefined ? msg.dst_name : msg.dst_addr) +
        ":" + msg.dst_port;
    
    $('div#console').prepend('<div class="flow" id="' + msg.hash + '"></div>');
    $('div.flow#' + msg.hash).text(txt);

    console.log(msg);
    var max_timeout = 30;
    function set_flow_timeout(timeout) {
      setTimeout(function() {
        var diff = get_unixtime() - updated_flow[msg.hash];
        if (updated_flow[msg.hash] !== undefined && diff < max_timeout) {
          console.log(diff);
          set_flow_timeout(max_timeout - diff);
        } else {
          console.log(diff);
          $('div.flow#' + msg.hash).remove();
          delete updated_flow[msg.hash];
        }
      }, timeout * 1000);
    }
    set_flow_timeout(max_timeout);
  });
});
