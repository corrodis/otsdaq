#!node
// serverbase.js : v0.5 : Node HTTPS Server
// Author: Eric Flumerfelt, FNAL RSI
// Last Modified: June 3, 2015
// Modified By: Eric Flumerfelt
//
// serverbase sets up a basic HTTPS server and directs requests
// to one of its submodules. 
//
// Implementation Notes: modules should assign their emitter to the module_holder[<modulename>] object
// modules will emit 'data' and 'end' signals and implement the function MasterInitFunction()

var cluster = require('cluster');
var numCPUs = require("os").cpus().length;
var fs = require('fs');
var path_module = require('path');
var child_process = require('child_process');

var util = require('util');
var log_file = fs.createWriteStream('/tmp/xdaqproxy.' + process.env["USER"] + '.log', { flags: 'a' });
var log_stdout = process.stdout;

var getversion = function () {
    console.log("Getting Server Version");
    if (fs.existsSync("./version.txt")) {
        console.log("Reading Server Version from File");
        return "" + fs.readFileSync("./version.txt");
    }
    else {
        child_process.exec("git describe --tags", function (error, stdout, stderr) {
            version = stdout.trim() + "-Git";
            child_process.exec("git status --porcelain", function (error, stdout) {
                if (stdout.length > 0) {
                    version += "*";
                }
            });
        });
    }
}
var version = getversion();

var config = {
    listenhost: "localhost",
    listenport: 8080,
    xdaqhost: "localhost",
    xdaqport: 2015
};
function loadConfig() {
    if (fs.existsSync("xdaq_config.json")) {
        config = JSON.parse(fs.readFileSync("xdaq_config.json"));
    } else {
        fs.writeFileSync("xdaq_config.json", JSON.stringify(config));
    }

    if (config.hostname === "localhost" && cluster.isMaster) {
        console.log("Listening only on localhost. To listen on a different address, set \"hostname\" in config.json.\nUse \"0.0.0.0\" to listen on all interfaces.");
    }
}

loadConfig();

console.log = function (d) { //
    log_file.write(util.format(d) + '\n');
    log_stdout.write(util.format(d) + '\n');
};

function LoadCerts(path) {
    var output = [];
    var files = fs.readdirSync(path);
    for (var i = 0; i < files.length; i++) {
        if (files[i].search(".pem") > 0 || files[i].search(".crt") > 0) {
            output.push(fs.readFileSync(path + "/" + files[i]));
        }
    }
    return output;
}

function GetCILogonCRL(path) {
    // Always fetch the latest CRL lists from CILogon:
    var file = fs.createWriteStream(path_module.join(path, "cilogon-basic.r0"));
    http.get("http://crl-cilogon.ncsa-security.net/cilogon-basic.r0", function (res) { res.pipe(file); });
    var file2 = fs.createWriteStream(path_module.join(path, "cilogon-basic.crl"));
    http.get("http://crl-cilogon.ncsa-security.net/cilogon-basic.crl", function (res) { res.pipe(file2); });
}

function LoadCRLs(path) {
    GetCILogonCRL(path);
    var output = [];
    var files = fs.readdirSync(path);
    for (var i = 0; i < files.length; i++) {
        if (files[i].search(".r0") > 0 || files[i].search(".crl") > 0) {
            output.push(fs.readFileSync(path + "/" + files[i]));
        }
    }
    return output;
}

// Node.js by default is single-threaded. Start multiple servers sharing
// the same port so that an error doesn't bring the whole system down
if (cluster.isMaster) {

    // Start workers for each CPU on the host
    for (var i = 0; i < numCPUs; i++) {
        //for (var i = 0; i < 1; i++) {
        var worker = cluster.fork();
    }

    // If one dies, start a new one!
    cluster.on("exit", function () {
        var newWorker = cluster.fork();
    });
} else {
    // Node.js framework "includes"
    var https = require('https');
    var http = require('http');
    var url = require('url');

    console.log("Setting up options");
    var options = {
        key: fs.readFileSync('./certs/server.key'),
        cert: fs.readFileSync('./certs/server.crt'),
        ca: LoadCerts("./certs/certificates"),
        crl: LoadCRLs("./certs/certificates"),
        requestCert: true,
        rejectUnauthorized: false
    };
    var authlist = " " + fs.readFileSync("./certs/authorized_users");
    console.log("Done setting up options");

    // Make an http server
    var server = https.createServer(options, function (req, res) {
        var clientCertificate = req.connection.getPeerCertificate();
        var useremail = "";
        if (req.client.authorized) {
            //console.log(JSON.stringify(clientCertificate));
            //var org = clientCertificate.subject.O[0];
            //if (org !== "Fermi National Accelerator Laboratory") {
            //    readOnly = true;
            //}
            var username = clientCertificate.subject.CN[0];
            useremail = clientCertificate.subjectaltname.substr(6);
            if (authlist.search(username) > 0 || authlist.search(useremail) > 0) {
                console.log("User: " + username + " (" + useremail + ")");
            }
        }

        if (req.url.search(/lid=\d+$/) > 0) {
            req.url = req.url + "/";
        }

        var thisurl = url.parse(req.url, true);
        console.log("Request path: " + thisurl.pathname);
        if (useremail.length > 0) {
            thisurl.query.httpsUser = useremail;
        }
        var pathname = url.format(thisurl);
        console.log("Adjusted path: " + pathname);

        var reqOptions = {
            host: config.xdaqhost,
            port: config.xdaqport,
            method: req.method,
            headers: req.headers,
            path: pathname
        }
        console.log("Request options: " + JSON.stringify(reqOptions));
        var xreq = http.request(reqOptions, function (xres) {
            xres.pipe(res);
        });
        req.pipe(xreq);

    });

    console.log("Listening on https://" + config.listenhost + ":" + config.listenport);
    server.listen(config.listenport, config.listenhost);
}
