[العربية](../../ar/book/ch20-00-web-server.md) | **English**

# Final Project: Building a Multithreaded Web Server

The book ends with a project that brings many of its chapters together: a web server. A browser asks it
for a page, and it answers with the page, over the network, using the HTTP protocol.

The plan:

1. Listen for connections on a TCP port.
2. Read the request a browser sends.
3. Send back a response: a status line, headers, and an HTML page.
4. Serve many requests at once, with a pool of worker threads.
5. Shut down cleanly, letting the workers finish what they started.

This server is small, written to show how the parts work. A real server, with routing, request parsing,
security limits and much more, is a library's job: the package index has Nahr, a web framework written in
SultanC.

- [A Single-Threaded Server](ch20-01-single-threaded.md)
- [A Multithreaded Server](ch20-02-multithreaded.md)
- [Graceful Shutdown](ch20-03-graceful-shutdown.md)
