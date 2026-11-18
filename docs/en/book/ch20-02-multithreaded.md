[العربية](../../ar/book/ch20-02-multithreaded.md) | **English**

# A Multithreaded Server

## The problem with one thread

The server handles one connection at a time. While it works on one, every other browser waits. To see
it, add a page that is slow on purpose: `/sleep` waits five seconds before answering. Add
`import std.time.time;` and a branch to the choice of page:

<sub>Listing 20-2: a slow page. Filename: src/main.sn</sub>

```sultan ignore
    if (texts_equal(&request_line, &"GET / HTTP/1.1")) {
        status := "HTTP/1.1 200 OK";
        file_name := "hello.html";
    } else if (texts_equal(&request_line, &"GET /sleep HTTP/1.1")) {
        match sleep_ns(5000000000) with
        | result.ok(_) -> { }
        | result.err(_) -> { };
        status := "HTTP/1.1 200 OK";
        file_name := "hello.html";
    } else {
        status := "HTTP/1.1 404 NOT FOUND";
        file_name := "404.html";
    };
```

Ask for `/sleep`, and while it waits, ask for `/` from another terminal:

```console
$ curl -s -o /dev/null http://127.0.0.1:7878/sleep &
$ time curl -s -o /dev/null http://127.0.0.1:7878/
```

The second request, which should take no time, takes five seconds: it waits behind the first one. A
real server, with thousands of visitors, cannot work like this.

## A pool of workers

The answer is to work on several connections at once, each in its own thread. Starting a new thread for
every connection would work, but an attack that opens thousands of connections would start thousands of
threads. Instead, the server starts a fixed number of threads once, a *thread pool*, and hands each
connection to one of them.

How does a connection reach a worker? Through a channel, from chapter 15. Each worker owns the receiving
end of its own channel, and waits on it; `main` keeps the senders, and sends each new connection to the
next worker in turn. Sending moves the stream into the channel, and receiving moves it out to the
worker, so at every moment exactly one thread owns each connection.

<sub>Listing 20-3: the workers. Filename: src/main.sn</sub>

```sultan ignore
def work(id: int, inbox: &receiver<tcp_stream>): void {
    let done: mutable bool = false;
    while (!done) {
        match recv(inbox) with
        | result.ok(stream) -> {
            println(concat3(&"Worker ", &int_to_text(id), &" got a connection."));
            handle_connection(stream);
        }
        | result.err(_) -> { done := true; };
    }
    println(concat3(&"Worker ", &int_to_text(id), &" stopped."));
}

def start_worker(id: int, workers: &mutable vector join_handle): option sender<tcp_stream> {
    let inbox: receiver<tcp_stream> = channel_receiver<tcp_stream>();
    let out: sender<tcp_stream> = sender_for(&inbox);
    match spawn(fn(): void -> work(id, &inbox)) with
    | result.ok(handle) -> {
        append(*workers, handle);
        return option.some(out);
    }
    | result.err(_) -> { return option.none(); };
}
```

`start_worker` makes a channel, keeps its sender to return, and starts a thread whose closure captures
the receiver. The closure passes `&inbox` to `work`: the receiver stays owned by the closure, and is freed
when the thread ends. `work` waits on `recv` and handles each stream it receives; when the channel has no
senders left, `recv` returns an error and the loop ends. `start_worker` also stores the thread's handle,
to wait for it later.

`main` starts four workers, then sends each accepted connection to the next one:

<sub>Listing 20-4: `main` with the pool. Filename: src/main.sn</sub>

```sultan ignore
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

    let next: mutable int = 0;
    while (true) {
        match tcp_accept(&listener) with
        | result.ok(stream) -> {
            match send(&senders[next], stream) with
            | result.ok(_) -> { }
            | result.err(_) -> { };
            next := (next + 1) % length(senders);
        }
        | result.err(_) -> { };
    }
    return 0;
}
```

Try the two requests again:

```console
$ curl -s -o /dev/null http://127.0.0.1:7878/sleep &
$ time curl -s -o /dev/null -w '%{http_code}\n' http://127.0.0.1:7878/
200
```

This time the second request is answered at once, while worker 0 is still sleeping:

```text
Worker 0 got a connection.
GET /sleep HTTP/1.1
Worker 1 got a connection.
GET / HTTP/1.1
```

Notice what the compiler checked. `handle_connection` takes ownership of the stream, and the stream
travels from `main` to a worker through the channel. Had `main` tried to use a stream after sending it,
or a worker to share one with another thread, the program would not compile.
