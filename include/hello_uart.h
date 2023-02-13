#ifndef HELLO_UART_H
#define HELLO_UART_H

const char response[] = {
"HTTP/1.1 200 OK\
Content-Type: text/html; charset=utf-8\
Content-Length: 55743\
Connection: keep-alive\
Cache-Control: s-maxage=300, public, max-age=0\
Content-Language: en-US\
Date: Thu, 06 Dec 2018 17:37:18 GMT\
ETag: \"2e77ad1dc6ab0b53a2996dfd4653c1c3\"\
Server: meinheld/0.6.1\
Strict-Transport-Security: max-age=63072000\
X-Content-Type-Options: nosniff\
X-Frame-Options: DENY\
X-XSS-Protection: 1; mode=block\
Vary: Accept-Encoding,Cookie\
Age: 7\
\
<!DOCTYPE html>\
<html lang=\"en\">\
<head>\
  <meta charset=\"utf-8\">\
  <title>A simple webpage</title>\
</head>\
<body>\
  <h1>Simple HTML5 webpage</h1>\
  <p>Hello, world!</p>\
</body>\
</html>"};

const char response1[] = {
"HTTP/1.1 200 OK\r\nDate: Mon, 27 Jul 2009 12:28:53 GMT\r\nServer: Apache/2.2.14 (Win32)\r\nLast-Modified: Wed, 22 Jul 2009 19:15:56 GMT\r\nContent-Length: 88\r\nContent-Type: text/html\r\nConnection: Closed"
};

#ifdef EXAMPLE
 mg_printf(c,
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html; charset=utf-8\r\n"
           "%s"
           "Content-Length:         \r\n\r\n",
           opts->extra_headers == NULL ? "" : opts->extra_headers);
 off = c->send.len;  // Start of body
 mg_printf(c,
           "<!DOCTYPE html><html><head><title>Index of %.*s</title>%s%s"
           "<style>th,td {text-align: left; padding-right: 1em; "
           "font-family: monospace; }</style></head>"
           "<body><h1>Index of %.*s</h1><table cellpadding=\"0\"><thead>"
           "<tr><th><a href=\"#\" rel=\"0\">Name</a></th><th>"
           "<a href=\"#\" rel=\"1\">Modified</a></th>"
           "<th><a href=\"#\" rel=\"2\">Size</a></th></tr>"
           "<tr><td colspan=\"3\"><hr></td></tr>"
           "</thead>"
           "<tbody id=\"tb\">\n",
           (int) uri.len, uri.ptr, sort_js_code, sort_js_code2, (int) uri.len,
           uri.ptr);
#endif /* EXAMPLE */


#endif /* HELLO_UART_H */
