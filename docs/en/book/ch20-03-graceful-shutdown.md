[العربية](../../ar/book/ch20-03-graceful-shutdown.md) | **English**

# Graceful Shutdown

Pressing Ctrl-C stops the server at once, even in the middle of a response. A server that stops
*gracefully* stops accepting new connections, lets the workers finish what they have, and then ends.

The pool already has everything this needs. A worker's `recv` fails when its channel has no senders
left, which ends the worker's loop. So to stop the workers, free the senders; to wait for them, `join`
their threads.

To keep the example short, this server stops after serving two connections. The accepting loop moves to
`serve`, which takes the vector of senders by value. When `serve` returns, the vector goes out of scope,
and every sender with it:

<sub>Listing 20-5: stopping after two connections. Filename: src/main.sn</sub>

```sultan ignore
def serve(listener: &tcp_listener, senders: vector sender<tcp_stream>, limit: int): void {
    let next: mutable int = 0;
    for (let served: mutable int = 0; served < limit; served := served + 1) {
        match tcp_accept(listener) with
        | result.ok(stream) -> {
            match send(&senders[next], stream) with
            | result.ok(_) -> { }
            | result.err(_) -> { };
            next := (next + 1) % length(senders);
        }
        | result.err(_) -> { };
    }
    println("Shutting down.");
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

    let workers: mutable vector join_handle = [];
    let senders: mutable vector sender<tcp_stream> = [];
    for (let id: mutable int = 0; id < 4; id := id + 1) {
        match start_worker(id, &mutable workers) with
        | option.some(out) -> { append(senders, out); }
        | option.none -> { return 1; };
    }

    serve(&listener, senders, 2);
    for (let i: mutable int = 0; i < length(workers); i := i + 1) {
        join(&mutable workers[i]);
    }
    return 0;
}
```

After `serve` returns, `main` joins each worker. Each one finishes the connection it may be handling,
finds its channel empty with no senders, and stops:

```console
$ curl -s -o /dev/null -w '%{http_code}\n' http://127.0.0.1:7878/
200
$ curl -s -o /dev/null -w '%{http_code}\n' http://127.0.0.1:7878/other
404
```

```text
Worker 0 got a connection.
GET / HTTP/1.1
Shutting down.
Worker 1 got a connection.
GET /other HTTP/1.1
Worker 1 stopped.
Worker 0 stopped.
Worker 2 stopped.
Worker 3 stopped.
```

The threads run at the same time, so the order of these lines can change from run to run. What does not
change: every connection is answered, every worker stops, and the program ends with status 0.

Nothing in this shutdown was written specially for it. The workers stop because ownership frees the
senders, and the channel reports that. It is the same rule that frees a string at the end of its block.

## Where to go from here

This is the end of the book. You have seen the whole language: values and types, ownership, records and
enums, modules and packages, errors, generics and closures, tests, threads, the standard library, C, and
targets. Some ways to go on:

- Extend the server: serve any file from a folder, set a `Content-Type` header by the file's extension,
  limit the size of a request, add a route that answers in JSON.
- Read the [reference](../index.md#reference), which specifies the language and the standard library
  precisely.
- Browse the `examples` folder of the repository: about 500 small programs, each in Arabic and in
  English.
- The appendixes of this book list the keywords in both languages, the operators, the diagnostics, and
  the standard library's modules.
