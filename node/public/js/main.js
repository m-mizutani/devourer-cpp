function get_unixtime() {
  return (new Date) / 1000;
}

$(document).ready(function() {
  
  var socket = io.connect(location.protocol + '//' + location.host);
  var updated_flow = {};
  var label_map = {};
  var size_map = {};
  var extent_map = {};
  socket.on('flow.update', function (msg) {
    $('div.flow').removeClass('active');
    for (var hv in msg.flow_size) {
      $('div.flow#' + hv).addClass('active');
      updated_flow[hv] = get_unixtime();
      if (size_map[hv] === undefined) { size_map[hv] = 0; }
      size_map[hv] += msg.flow_size[hv];
    }

  });

  socket.on('flow.new', function (msg) {
    if (msg.dst_port === 53) {
      return;
    }
    var hv = msg.hash;
    var txt = '(' + msg.proto + ') ' +
        (msg.src_name !== undefined ? msg.src_name : msg.src_addr) +
        ":" + msg.src_port + ' -> ' +
        (msg.dst_name !== undefined ? msg.dst_name : msg.dst_addr) +
        ":" + msg.dst_port;
    label_map[hv] = txt;
    var mtr = random(hv);
    console.log(mtr);
    data_set.push(mtr);
    d3.select("div#cubism").call(function(div) {
      div.selectAll(".horizon")
          .data(data_set)
          .enter().append("div")
          .attr("class", "horizon")
          // .call(context.horizon().extent([0, 1e5]));
          .call(context.horizon());
    });

/*    
    $('div#console').prepend('<div class="flow" id="' + msg.hash + '"></div>');
    $('div.flow#' + msg.hash).text(txt);
*/
    
    var max_timeout = 960;
    function set_flow_timeout(timeout) {
      setTimeout(function() {
        var diff = get_unixtime() - updated_flow[hv];
        if (updated_flow[hv] !== undefined && diff < max_timeout) {
          console.log(diff);
          set_flow_timeout(max_timeout - diff);
        } else {
          for (var i = 0; i < data_set.length; i++) {
            if (data_set[i] === mtr) {
              data_set.splice(i, 1);
              break;
            }
          }
          d3.select("div#cubism").call(function(div) {
            div.selectAll('.horizon')
                .data(data_set)
                .exit()
                .remove();
          });    

          delete updated_flow[hv];
        }
      }, timeout * 1000);
    }
    set_flow_timeout(max_timeout);
  });


  function random(hv) {
    var value = 0,
    values = [],
    i = 0,
    last;
    return context.metric(function(start, stop, step, callback) {
      start = +start, stop = +stop;
      if (isNaN(last)) last = start;
      
      if (size_map[hv] !== undefined) {
        value = size_map[hv];
        delete size_map[hv];
      } else {
        value = 0;
      }
      
      while (last < stop) {
        last += step;
        // value = Math.max(-10, Math.min(10, value + .8 * Math.random() - .4 + .2 * Math.cos(i += .2)));
        values.push(value);
        delete size
      }

      callback(null, values = values.slice((start - stop) / step));
    }, label_map[hv]);
  }



  var context = cubism.context()
      .serverDelay(0)
      .clientDelay(0)
      .step(1e3)
      .size(960);

  var foo = random("foo"),
  bar = random("bar");
  var data_set = [];
  
  d3.select("div#cubism").call(function(div) {

    div.append("div")
        .attr("class", "axis")
        .call(context.axis().orient("top"));
/*
    div.selectAll(".horizon")
        .data([foo, bar, foo.add(bar), foo.subtract(bar)])
        .enter().append("div")
        .attr("class", "horizon")
        .call(context.horizon().extent([-20, 20]));
*/
    div.append("div")
        .attr("class", "rule")
        .call(context.rule());

  });


  // On mousemove, reposition the chart values to match the rule.
  context.on("focus", function(i) {
    d3.selectAll(".value").style("right", i == null ? null : context.size() - i + "px");
  });
  
});
