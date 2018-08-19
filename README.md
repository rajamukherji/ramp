# ramp
Raja's Attempt at a Memory Pool

## About

`ramp` is a very simple library which provides a non-threadsafe memory pool.
It is designed to be used in event based applications written in C, and allows memory to be allocated easily within an event and then freed automatically at the end of the event.

Each `ramp_t` object keeps track of a linked list of pages of memory, with the page size specified at `ramp_t` construction.
Memory can be allocated from a `ramp_t` object using `ramp_alloc`.

## Building

Building `ramp` requires [`rabs`](http://github.com/wrapl/rabs).

```
$ git clone https://github.com/rajamukherji/ramp
$ cd ramp
$ rabs
```

## Usage

If the request block of memory is too big to fit into a single page, then an external block of memory is allocated, but still tracked within the `ramp_t`.

At the beginning of each handling each event, allocated a new `ramp_t` object by calling `ramp_new(size_t PageSize)`.
The `PageSize` argument to `ramp_new` sets the size of each new page.

```c
ramp_t *Ramp = ramp_new(512);
```

During the event handling, any number of memory blocks can be allo0cated using `ramp_alloc(ramp_t *Ramp, size_t Size)`.

```c
char *Buffer = ramp_alloc(Ramp, 128);
```

Finally, after the event is handled, all of the memory allocated with the `ramp_t` instance can be freed using `ramp_reset(ramp_t *Ramp)`.

```c
ramp_reset(Ramp);
```

The `ramp_t` instance can be reused if required.
When the `ramp_t` instance is no longer required, it can be freed with `ramp_free(ramp_t *Ramp)`.

```c
ramp_free(Ramp);
```

