# DNSServer

The program (makefile produces an executable named ’dns_svr’.) accepts DNS “AAAA” query over TCP on port 8053. Forward it to a server whose IPv4 address is the first command-line argument and whose port is the second command-line argument. Send the response back to the client who sent the request, over the same TCP connection. There will be a separate TCP connection for each query/response with the client. 
  
Input example:  
Server: ./dns_svr (input address) (input port)  
dig +tcp @(input address) -p(input port) AAAA google.com  
  
Log will keep a log at ./dns_svr.log (i.e., in current directory) with messages of the following format:  
<timestamp> requested <domain_name> – when you receive a request  
<timestamp> unimplemented request – if the request type is not AAAA  
<timestamp> <domain_name> expires at <timestamp> – for each request you receive that is in yourcache  
<timestamp> replacing <domain_name> by <domain_name> – for each cache eviction  
<timestamp> <domain_name> is at <IP address>  
  
Log example:  
2021-05-19T22:24:06+1000 requested www.example.com  
2021-05-19T22:24:06+1000 www.example.com is at 2606:2800:220:1:248:1893:25c8:1946  
2021-05-19T22:24:06+1000 requested www.google.com  
2021-05-19T22:24:06+1000 replacing www.example.com by www.google.com  
2021-05-19T22:24:06+1000 www.google.com is at 2404:6800:4015:800::2004  
  
The server will not be shut down by itself. SIGINT (like CTRL-C) needs to be used to terminate the server.
