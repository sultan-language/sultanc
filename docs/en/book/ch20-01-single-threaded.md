[العربية](../../ar/book/ch20-01-single-threaded.md) | **English**

# A Single-Threaded Server

## Two protocols

The web runs on two protocols, one on top of the other. *TCP* carries bytes between two computers,
reliably and in order: it is the connection. *HTTP* says what the bytes mean: a browser sends a
*request*, a few lines of text that name the page it wants, and the server sends a *response*, a status
line, some headers, and the page itself.

A request starts with a *request line* such as this one, followed by headers:

```text
GET / HTTP/1.1
Host: 127.0.0.1:7878
```

`GET` is the method, `/` the page asked for, and `HTTP/1.1` the protocol version. A response looks like
this:

```text
HTTP/1.1 200 OK
Content-Length: 153

<!DOCTYPE html>
...
```

`200 OK` is the status: the page was found. `Content-Length` says how many bytes the page has. An empty
line ends the headers, and the page follows. Lines in HTTP end with the two characters `\r\n`.

## The project

```console
$ supax new hello
$ cd hello
```

Put two pages beside `Supax.toml`: one to serve, and one for pages that do not exist.

<sub>Filename: hello.html</sub>

```text
<!DOCTYPE html>
<html lang="en">
  <head><meta charset="utf-8"><title>Hello!</title></head>
  <body><h1>Hello!</h1><p>Hi from SultanC</p></body>
</html>
```

<sub>Filename: 404.html</sub>

```text
<!DOCTYPE html>
<html lang="en">
  <head><meta charset="utf-8"><title>Not found</title></head>
  <body><h1>Oops!</h1><p>Sorry, I don't know what you're asking for.</p></body>
</html>
```

## The server

`std.net.socket_address` turns text such as `"127.0.0.1:7878"` into an address, `parse_socket_addr`.
`std.net.tcp` has the rest: `tcp_bind` starts listening on an address, `tcp_accept` waits for the next
connection and returns it as a `tcp_stream`, `tcp_read` and `tcp_write` move bytes, and
`tcp_stream_close` ends the connection.

<sub>Listing 20-1: a server that answers one connection at a time. Filename: src/main.sn</sub>

```sultan ignore
import std.io.println;
import std.text.number;
import std.text.compare;
from std.text import concat, concat3, lines, trim;
import std.text.from_bytes;
import std.net.socket_address;
import std.net.tcp;
import std.fs.read_file;

def handle_connection(stream: tcp_stream): void {
    let buffer: mutable vector u8 = [];
    for (let i: mutable int = 0; i < 1024; i := i + 1) {
        append(buffer, 0 as u8);
    }
    match tcp_read(&stream, &mutable buffer, 0, 1024) with
    | result.ok(_) -> { }
    | result.err(_) -> { return; };
    let request: string = from_bytes(&buffer);
    let request_line: string = first_line(&request);
    println(clone(&request_line));

    let status: string;
    let file_name: string;
    if (texts_equal(&request_line, &"GET / HTTP/1.1")) {
        status := "HTTP/1.1 200 OK";
        file_name := "hello.html";
    } else {
        status := "HTTP/1.1 404 NOT FOUND";
        file_name := "404.html";
    };
    let body: string;
    match read_file(&file_name) with
    | result.ok(text) -> { body := text; }
    | result.err(_) -> { body := ""; };
    let response: string = concat3(
        &concat(&status, &"\r\n"),
        &concat3(&"Content-Length: ", &int_to_text(length(body)), &"\r\n\r\n"),
        &body
    );
    let bytes: vector u8 = bytes_from_text(response);
    match tcp_write(&stream, &bytes, 0, length(bytes)) with
    | result.ok(_) -> { }
    | result.err(_) -> { };
    match tcp_stream_close(stream) with
    | result.ok(_) -> { }
    | result.err(_) -> { };
}

def first_line(text: &string): string {
    let all: vector string = lines(text);
    if (length(all) == 0) {
        return "";
    };
    return trim(&all[0]);
}

def main(): int {
    let address: socket_addr;
    match parse_socket_addr(&"127.0.0.1:7878") with
    | result.ok(a) -> { address := a; }
    | result.err(_) -> { return 1; };
    let listener: tcp_listener;
    match tcp_bind(&address, 128) with
    | result.ok(l) -> { listener := l; }
    | result.err(code) -> { return code; };
    while (true) {
        match tcp_accept(&listener) with
        | result.ok(stream) -> { handle_connection(stream); }
        | result.err(_) -> { };
    }
    return 0;
}
```

Read it from `main` up:

- `main` parses the address and starts listening. `127.0.0.1` is this computer, and 7878 a port that no
  common program uses. The `128` is how many connections may wait to be accepted. Then it accepts
  connections forever, and hands each one to `handle_connection`.
- `handle_connection` takes ownership of the stream. It reads up to 1024 bytes of the request into a
  buffer of zero bytes, turns them into text with `from_bytes`, and keeps the first line.
- It chooses the status and the page by the request line, reads the page, and builds the response with
  its `Content-Length`. `length(body)` counts bytes, exactly what the header needs.
- It turns the response into bytes, writes them, and closes the connection. Since `handle_connection`
  owns the stream, closing it is the last use; nothing can use the closed stream afterwards.

The text functions come from one line, `from std.text import concat, concat3, lines, trim;`, which
imports just these names from the modules of `std.text`. That is not only shorter: the next section
imports `std.thread.thread`, whose `join` would clash with the `join` of `std.text.transform`.

## Trying it

```console
$ supax run
```

The server waits. Open `http://127.0.0.1:7878` in a browser, or use `curl` in another terminal:

```console
$ curl -i http://127.0.0.1:7878/
HTTP/1.1 200 OK
Content-Length: 153

<!DOCTYPE html>
<html lang="en">
  <head><meta charset="utf-8"><title>Hello!</title></head>
  <body><h1>Hello!</h1><p>Hi from SultanC</p></body>
</html>
$ curl -s -o /dev/null -w '%{http_code}\n' http://127.0.0.1:7878/other
404
```

and the server prints each request line:

```text
GET / HTTP/1.1
GET /other HTTP/1.1
```

Press Ctrl-C to stop it.
