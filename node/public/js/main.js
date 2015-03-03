$(document).ready(function() {
  var socket = io.connect(location.protocol + '//' + location.host);
  socket.on('flow.new', function (msg) {
    if (msg.dst_port === 53) {
      return;
    }
    var flow_html = '<div class="flow">(' + msg.proto + ')' +
        (msg.src_name !== undefined ? msg.src_name : msg.src_addr) +
        ":" + msg.src_port + ' -&gt; ' +
        (msg.dst_name !== undefined ? msg.dst_name : msg.dst_addr) +
        ":" + msg.dst_port + '</div>';
    $('div#console').prepend(flow_html);
    console.log(msg);
    if ($('div.flow').length > 50) {
      $('div.flow')[50].remove();
    }
  });
});
