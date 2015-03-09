$(document).ready(function() {
  
  var socket = io.connect(location.protocol + '//' + location.host);
  var events = [
    'flow.new',
      'flow.update',
      'flow.log',
      'dns.tx',
      'dns.log'
  ];

  events.forEach(function(ev) {
    socket.on(ev, function (msg) {
      $('div#event_name').text(ev);
      $('div#event_data pre').text(JSON.stringify(msg));
    });
  });   
});
