var express = require('express');
var router = express.Router();
var msgpack = require('msgpack');

/* GET home page. */
router.get('/', function(req, res) {
  res.render('dns', { title: 'Devourer' });
});

module.exports = router;
