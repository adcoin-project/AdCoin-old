var http = require('http')
var forward = require('http-forward')
var cstamp = require('console-stamp')

if(process.argv.length < 4) {
  console.log("usage: node main.js <target> <port>");
}
else {
  cstamp(console, 'HH:MM:ss.l');
  var server = http.createServer(function (req, res) {
    // Define proxy config params
    req.forward = { target: process.argv[2] }
    var body = [];
    req.on('data', function(chunk) {
      body.push(chunk);
    }).on('end', function () {
      body = Buffer.concat(body).toString();
      console.log(body);
    });

    forward(req, res, function (e, r) {
      var resBody = [];
      r.on('data', function (chunk) {
        resBody.push(chunk)
      }).on('end', function() {
        resBody = Buffer.concat(resBody).toString()
        console.log("Request to: " + req.url);
        console.log("Body: " + body);
        console.log("Reply: " + resBody);
      });
    })
  })

  server.listen(process.argv[3])
}
