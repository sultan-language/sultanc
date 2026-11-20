[العربية](../../ar/book/ch08-03-maps-and-sets.md) | **English**

# Maps and Sets

A vector finds a value by its position. Often you want to find a value by something else: a word's
count by the word, a user by their name. A *map* stores pairs of a *key* and a *value*, and finds the
value from the key. A *set* stores keys alone, each once.

`std.collections` has several:

| Collection | Module | What it is for |
| --- | --- | --- |
| `text_index_map` | `std.collections.text_index_map` | text keys to `uint` values: counting, numbering |
| `text_set` | `std.collections.text_index_map` | distinct texts |
| `btree_map<K, V>` | `std.collections.btree_map` | any keys, kept in sorted order |
| `map<K, V>` | `std.collections.map` | any keys, by hash: the fastest lookup |
| `set<T>` | `std.collections.set` | distinct values of any type, by hash |
| `deque<T>` | `std.collections.deque` | a queue to add to and take from at both ends |

The functions of each are named after it: `text_index_map_get`, `set_insert`, `deque_pop_front`.

Two patterns repeat in all of them. Looking a key up returns an `option`, because the key may be
missing. Adding returns a `result`, because adding may need more memory, and memory can run out. The
examples here end the program with status 1 in that case.

## Counting words

`text_index_map` maps text to a `uint`, which makes it the tool for counting:

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;
import std.collections.text_index_map;

def main(): int {
    let text: string = "the cat saw the dog and the bird";
    let words: vector string = split(&text, &" ");
    let counts: mutable text_index_map = new_text_index_map();
    for (let i: mutable int = 0; i < length(words); i := i + 1) {
        let so_far: mutable uint = 0;
        match text_index_map_get(&counts, &words[i]) with
        | option.some(n) -> { so_far := n; }
        | option.none -> { };
        match text_index_map_insert(&mutable counts, clone(&words[i]), so_far + 1) with
        | result.ok(_) -> { }
        | result.err(_) -> { return 1; };
    }
    match text_index_map_get(&counts, &"the") with
    | option.some(n) -> { println(uint_to_text(n)); }
    | option.none -> { println("0"); };
    println(uint_to_text(text_index_map_length(&counts)));
    return 0;
}
```

```text
3
6
```

For each word, the program looks up its count so far (none means zero), and stores the count plus one.
Inserting a key that is already there replaces its value. The key is passed by value, so the map owns
its own copy of the word: `clone(&words[i])`.

## Distinct values, in order

A `text_set` keeps each text once. With the map and `sort_texts`, it gives the word count in
alphabetical order:

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;
import std.collections.text_index_map;
import std.collections.sort;

def main(): int {
    let text: string = "the cat saw the dog and the bird saw the cat";
    let words: vector string = split(&text, &" ");
    let counts: mutable text_index_map = new_text_index_map();
    let unique: mutable text_set = new_text_set();
    for (let i: mutable int = 0; i < length(words); i := i + 1) {
        let so_far: mutable uint = 0;
        match text_index_map_get(&counts, &words[i]) with
        | option.some(n) -> { so_far := n; }
        | option.none -> { };
        match text_index_map_insert(&mutable counts, clone(&words[i]), so_far + 1) with
        | result.ok(_) -> { }
        | result.err(_) -> { return 1; };
        match text_set_insert(&mutable unique, clone(&words[i])) with
        | result.ok(_) -> { }
        | result.err(_) -> { return 1; };
    }
    let sorted: mutable vector string = text_set_values(&unique);
    sort_texts(&mutable sorted);
    for (let i: mutable int = 0; i < length(sorted); i := i + 1) {
        match text_index_map_get(&counts, &sorted[i]) with
        | option.some(n) -> { println(concat3(&sorted[i], &": ", &uint_to_text(n))); }
        | option.none -> { };
    }
    return 0;
}
```

```text
and: 1
bird: 1
cat: 2
dog: 1
saw: 2
the: 4
```

## Maps with any key, in sorted order

`btree_map<K, V>` takes keys of any type `K` and values of any type `V`, and keeps the keys in order. To
keep them in order it has to compare them, and you give it the comparison when you create it: a
function that returns a negative number, zero or a positive number, as `compare_texts_lexicographically`
does. [Chapter 10](ch10-02-functions-as-values.md) explains passing functions as values.

```sultan
import std.io.println;
import std.text.number;
import std.text.transform;
import std.collections.btree_map;

def compare_ints(left: &int, right: &int): int {
    return *left - *right;
}

def main(): int {
    let ranks: mutable btree_map<int, string> = new_btree_map(compare_ints);
    btree_map_insert(&mutable ranks, 30, "third");
    btree_map_insert(&mutable ranks, 10, "first");
    btree_map_insert(&mutable ranks, 20, "second");
    let keys: vector int = btree_map_keys(&ranks);
    let shown: mutable vector string = [];
    for (let i: mutable int = 0; i < length(keys); i := i + 1) {
        append(shown, int_to_text(keys[i]));
    }
    println(join(&shown, &" "));
    let wanted: int = 20;
    match btree_map_get(&ranks, &wanted) with
    | option.some(name) -> { println(name); }
    | option.none -> { println("missing"); };
    return 0;
}
```

```text
10 20 30
second
```

The keys were inserted as 30, 10, 20, and come out sorted. `btree_map_keys` copies the keys into a new
vector, so it needs a plain key type such as `int`; for text keys, keep a `text_set` of the keys beside
the map, as in the word count.

## Sets of any type

A hash `set<T>` keeps distinct values of any type. A hash set finds a value by turning it into a number,
its *hash*, so it needs two functions: one that computes the hash, and one that says whether two values
are equal:

```sultan
import std.io.println;
import std.text.number;
import std.collections.set;

def hash_int(value: &int): uint {
    return ((*value) as uint) * 2654435761;
}

def same_int(a: &int, b: &int): bool {
    return *a == *b;
}

def main(): int {
    let seen: mutable set<int> = new_set(hash_int, same_int);
    let values: vector int = [4, 8, 4, 15, 8, 16, 4];
    for (let i: mutable int = 0; i < length(values); i := i + 1) {
        match set_insert(&mutable seen, values[i]) with
        | result.ok(_) -> { }
        | result.err(_) -> { return 1; };
    }
    println(uint_to_text(set_length(&seen)));
    if (set_contains(&seen, &15)) {
        println("has 15");
    };
    return 0;
}
```

```text
4
has 15
```

`map<K, V>`, in `std.collections.map`, works the same way, with a hash and an equality function for its
keys.

## Queues

A `deque`, a double-ended queue, adds and removes values at both ends. Taking from the front what was
added at the back makes a queue: first in, first out.

```sultan
import std.io.println;
import std.text.number;
import std.collections.deque;

def main(): int {
    let waiting: mutable deque<int> = new_deque();
    deque_push_back(&mutable waiting, 101);
    deque_push_back(&mutable waiting, 102);
    deque_push_back(&mutable waiting, 103);
    while (deque_length(&waiting) > 0) {
        match deque_pop_front(&mutable waiting) with
        | option.some(ticket) -> { println(int_to_text(ticket)); }
        | option.none -> { };
    }
    return 0;
}
```

```text
101
102
103
```

## Choosing a collection

- Values in order, found by position: `vector`.
- Text to a number: `text_index_map`. Distinct texts: `text_set`.
- Keys kept in sorted order: `btree_map`. The fastest lookup by key: `map`.
- Distinct values of any type: `set`. Work waiting its turn: `deque`.

## Summary

- `vector T` holds values in order; `append` adds, `v[i]` reads, `length` counts.
- `string` is UTF-8 text: `length` counts bytes, `char_count` characters, and positions are bytes.
- Text is joined with `concat`, `concat3` and `join`, compared with `texts_equal`, and cut apart with
  `trim`, `split`, `replace` and `substring`.
- Maps find values by key, sets keep distinct values. Lookups return an `option`, inserts a `result`.
- Collections of any type take functions to compare or hash their keys.

Try these exercises:

- Given a vector of integers, print the average, the median (the middle value after sorting) and the
  value that appears most often.
- Turn each word of a sentence into "pig latin": move the first letter to the end and add "ay".
- Read lines of the form `department name` and print the names in each department, sorted.
