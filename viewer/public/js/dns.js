(function() {
  function setup_graph() {
    $('#cy').cytoscape({
      style: cytoscape.stylesheet()
          .selector('node')
          .css({
            'content': 'data(name)',
            'text-valign': 'top',
            'font-size': 12,
            'color': '#eee',
          })
          .selector('edge')
          .css({
            'font-size': 12,
            'target-arrow-shape': 'triangle',
            'text-valign': 'top',
            'line-color': '#999',
            'target-arrow-color': '#999',
            'text-outline-width': 1,
            'text-outline-color': '#fff',
            'content': 'data(label)'
          })
          .selector(':selected')
          .css({
            'background-color': 'black',
            'line-color': '#444',
            'target-arrow-color': 'black',
            'source-arrow-color': 'black'
          })
          .selector('.faded')
          .css({
            'opacity': 0.25,
            'text-opacity': 0
          }),

      elements: {
        nodes: [],
        edges: [],
      },

      ready: function(){        
        window.cy = this;

        cy.elements().unselectify();
        cy.on('tap', 'node', function(e){
          var node = e.cyTarget;
          var neighborhood = node.neighborhood().add(node);

          cy.elements().addClass('faded');
          neighborhood.removeClass('faded');
        });

        cy.on('tap', function(e){
          if( e.cyTarget === cy ){
            cy.elements().removeClass('faded');
          }
        });

      }
    });
  }

  var node_map = {};
  var edge_map = {};
  var layouting = false;

  function run_layout() {
    var cy_obj = $('#cy').cytoscape('get');       
    if (layouting === false) {
      layouting = true;
      cy_obj.layout({
        name: 'cose',
        padding: 30,
        gravity: 150, 
        stop: function() { console.log('stop'); layouting = false; },
        animate: true,
      });
    }
  }

  function setup_socket() {
    var socket = io.connect(location.protocol + '//' + location.host);
    var cy_obj = $('#cy').cytoscape('get'); 

    var ev = 'dns.log';
    socket.on(ev, function (msg) {
      $('div#event_name').text(ev);
      $('div#event_data pre').text(JSON.stringify(msg));

      function add_node(name, base) {
        var pos = (base === null ? {x:0, y:0} : 
                   {x: base.position().x, y: base.position().y});
        var node_id = get_node_id(name);
        var obj = cy_obj.add({group: 'nodes', 
                              data: { id: node_id, name: name }, 
                              position: pos});
        return node_id;
      }

      
      var src = cy.nodes().filter('[name = "' + msg.name + '"]');
      var dst = cy.nodes().filter('[name = "' + msg.data + '"]');
      var f_id = CybozuLabs.MD5.calc(msg.name + '=' + msg.data);
      var src_id, dst_id;
      if (src.length === 0 && dst.length === 0) {
        src_id = add_node(msg.name, null);
        dst_id = add_node(msg.data, null);
      } else if (src.length === 0) {
        src_id = add_node(msg.name, dst[0]);
        dst_id = dst[0].id();
      } else if (dst.length === 0) {
        dst_id = add_node(msg.data, src[0]);
        src_id = src[0].id();          
      } else {
        src_id = src[0].id(); 
        dst_id = dst[0].id(); 
      }        

      if (edge_map[src_id] === undefined || edge_map[src_id][dst_id] === undefined) {
        cy_obj.add({group: 'edges', data: { id: f_id, source: src_id, target: dst_id}});
        if (edge_map[src_id] === undefined) { edge_map[src_id] = {}; }
        edge_map[src_id][dst_id] = f_id;
        setTimeout(function() {
          var edge = cy.edges().filter('[id = "' + f_id + '"]');
          var src = edge.source();
          var dst = edge.target();
          cy_obj.remove('[id = "' + f_id + '"]');

          console.log(src.data().name + ': ' + src.connectedEdges().length);
          console.log(dst.data().name + ': ' + dst.connectedEdges().length);
          if (src.connectedEdges().length === 0) { cy_obj.remove(src); }
          if (dst.connectedEdges().length === 0) { cy_obj.remove(dst); }
          edge_map[src.data().id][dst.data().id] = undefined;
          run_layout();
        }, 120 * 1000);
        run_layout();
      } else {
        console.log('exist');
      }
      
      console.log(msg);
    });

    
    
    socket.on('dnshive:flow:new', function(msg) {
      if (msg['proto'] === 'TCP') {
        var f_id = 'flow_' + msg['hash'];
        var cy_obj = $('#cy').cytoscape('get'); 

        function add_node(name, base) {
          var pos = (base === null ? {x:0, y:0} : 
                     {x: base.position().x, y: base.position().y});
          var node_id = get_node_id(name);
          var obj = cy_obj.add({group: 'nodes', 
                                data: { id: node_id, name: name }, 
                                position: pos});
          return node_id;
        }

        var src = cy.nodes().filter('[name = "' + msg.client + '"]');
        var dst = cy.nodes().filter('[name = "' + msg.server + '"]');
        var src_id, dst_id;
        if (src.length === 0 && dst.length === 0) {
          src_id = add_node(msg.client, null);
          dst_id = add_node(msg.server, null);
        } else if (src.length === 0) {
          src_id = add_node(msg.client, dst[0]);
          dst_id = dst[0].id();
        } else if (dst.length === 0) {
          dst_id = add_node(msg.server, src[0]);
          src_id = src[0].id();          
        } else {
          src_id = src[0].id(); 
          dst_id = dst[0].id(); 
        }        

        if (edge_map[src_id] === undefined || edge_map[src_id][dst_id] === undefined) {
          cy_obj.add({group: 'edges', data: { id: f_id, source: src_id, target: dst_id}});
          if (edge_map[src_id] === undefined) { edge_map[src_id] = {}; }
          edge_map[src_id][dst_id] = msg.hash;
          setTimeout(function() {
            var edge = cy.edges().filter('[id = "' + f_id + '"]');
            var src = edge.source();
            var dst = edge.target();
            cy_obj.remove('[id = "' + f_id + '"]');

            console.log(src.data().name + ': ' + src.connectedEdges().length);
            console.log(dst.data().name + ': ' + dst.connectedEdges().length);
            if (src.connectedEdges().length === 0) { cy_obj.remove(src); }
            if (dst.connectedEdges().length === 0) { cy_obj.remove(dst); }
            edge_map[src.data().id][dst.data().id] = undefined;
            run_layout();
          }, 120 * 1000);
          run_layout();
        } else {
          console.log('exist');
        }
      }

      // console.log(msg);
    });
    socket.on('dnshive:flow:end', function(msg) {
      var f_id = 'flow_' + msg['hash'];
      var cy_obj = $('#cy').cytoscape('get');       

/*
      var src_id = node_map[msg.client];
      var dst_id = node_map[msg.server];
      if (edge_map[src_id] !== undefined && edge_map[src_id][dst_id] === msg.hash) {
        var e = cy.edges().filter('[name = "' + f_id + '"]');
        cy.remove([e]);
        run_layout();
        console.log(edge_map[src_id][dst_id]);
        edge_map[src_id][dst_id] = undefined;
        console.log(msg);
      }
*/
    });

    var node_base_id = 0;
    function get_node_id(name) {
      if (node_map[name] === undefined) {
        var node_id = 'node_' + (node_base_id++);
        node_map[name] = node_id;
      }
      return node_map[name];
    }

    socket.on('dnshive:dns:address', function(msg) {
    });
  }

  $(document).ready(function() {
    setup_graph();
    setup_socket();
    console.log('init');
  });
})();

