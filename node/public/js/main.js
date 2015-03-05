function get_unixtime() {
  return (new Date) / 1000;
}

$(document).ready(function() {
  
  var socket = io.connect(location.protocol + '//' + location.host);
  var updated_flow = {};
  var label_map = {};
  var size_map = {};
  var max_timeout = 640;
  
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
    if (label_map[hv] !== undefined) {
      return;
    }
    
    var txt = '(' + msg.proto + ') ' +
        (msg.src_name !== undefined ? msg.src_name : msg.src_addr) +
        // ":" + msg.src_port +
        ' <-> ' +
        (msg.dst_name !== undefined ? msg.dst_name : msg.dst_addr);
        // ":" + msg.dst_port;
    label_map[hv] = txt;
    console.log(txt + ' -> ' + hv);
    var mtr = create_metrics(hv);
    data_set.unshift(mtr);
    d3.select("div#cubism").call(function(div) {
      div.selectAll(".horizon")
          .data(data_set, function(d) { return d.toString(); })
          .enter().append("div")
          .attr("class", "horizon")
          .call(context.horizon().height(25));
    });

/*    
    $('div#console').prepend('<div class="flow" id="' + msg.hash + '"></div>');
    $('div.flow#' + msg.hash).text(txt);
*/
    
    function set_flow_timeout(timeout) {
      setTimeout(function() {
        var diff = get_unixtime() - updated_flow[hv];
        console.log('check ' + txt + ': ' + diff);
        if (updated_flow[hv] !== undefined && diff < max_timeout) {
          console.log('extend...');
          set_flow_timeout(max_timeout - diff);
        } else {
          for (var i = 0; i < data_set.length; i++) {
            if (data_set[i].toString() === mtr.toString()) {
              console.log('removing ' + data_set[i].toString());
              data_set.splice(i, 1);
              break;
            }
          }
          d3.select("div#cubism").call(function(div) {
            div.selectAll('.horizon')
                .data(data_set, function(d) { return d.toString(); })
                .exit()
                .remove();
          });    

          delete updated_flow[hv];
        }
      }, timeout * 1000);
    }
    set_flow_timeout(max_timeout);
  });

  function create_metrics(hash_val) {
    delete size_map[hash_val];
    var value = 0,
        values = [],
        i = 0,
        last;
    return context.metric(function(start, stop, step, callback) {
      start = +start, stop = +stop;
      if (isNaN(last)) last = start;
      
      if (size_map[hash_val] !== undefined) {
        value = size_map[hash_val];
        delete size_map[hash_val];
      } else {
        value = 0;
      }

      while (last < stop) {
        last += step;
        values.push(value);
      }

      callback(null, values = values.slice((start - stop) / step));
    }, label_map[hash_val]);
  }



  var context = cubism.context()
      .serverDelay(0)
      .clientDelay(0)
      .step(1e3)
      .size(max_timeout);

  var data_set = [];
  
  d3.select("div#cubism").call(function(div) {

    div.append("div")
        .attr("class", "axis")
        .call(context.axis().orient("top"));

    div.append("div")
        .attr("class", "rule")
        .call(context.rule());

  });


  // On mousemove, reposition the chart values to match the rule.
  context.on("focus", function(i) {
    d3.selectAll(".value").style("right", i == null ? null : context.size() - i + "px");
  });
  
});
