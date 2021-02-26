#!/usr/bin/env python
import BaseHTTPServer, SimpleHTTPServer

port = 8006
print "Running on port %d" % port

SimpleHTTPServer.SimpleHTTPRequestHandler.extensions_map[
    '.wasm'] = 'application/wasm'

httpd = BaseHTTPServer.HTTPServer(('localhost', port),
                                  SimpleHTTPServer.SimpleHTTPRequestHandler)

print "Mapping \".wasm\" to \"%s\"" % \
    SimpleHTTPServer.SimpleHTTPRequestHandler.extensions_map['.wasm']
httpd.serve_forever()
