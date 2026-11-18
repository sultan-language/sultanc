[العربية](../../ar/book/ch14-02-the-lock-file.md) | **English**

# `Supax.lock` and Reproducible Builds

A requirement such as `"1.2"` allows many versions. If each build took the newest one, a project could
build differently tomorrow than today, or on a colleague's computer than on yours, without any change to
its code. To prevent that, the first build writes `Supax.lock`.

## The lock file

`Supax.lock` records, for every package in the build, exactly which code was used:

```toml
[[package]]
name = "nahr"
version = "0.1.0-dev"
source = "registry"
registry = "https://github.com/sultan-language/index"
git = "https://github.com/supax0/nahr.git"
commit = "52953e2f953ff8061b686bf0e29b9eabf28dfb8d"
checksum = "sha256:02107f24d2d6b4d362aff7be8e7ce99d54d5f3e4c03eb41537ae359fe629e01c"
```

The *commit* names the exact version of the code, and the *checksum* is a fingerprint of its contents:
if one byte of the package changed, the checksum would not match, and Supax would refuse it. Later
builds use what the lock file says, whatever newer versions exist.

Commit `Supax.lock` with your code. Supax writes it; do not edit it by hand.

## Updating on purpose

Dependencies change only when you ask. `supax update` resolves every requirement again, takes the newest
versions they allow, and rewrites the lock file; `supax update NAME` does it for one package. Review the
change, run the tests, and commit the new lock file.

## Building without the network

Everything Supax fetches is kept in a cache, in `.supax/git` in your home folder. After the first fetch,
a build needs no network:

| Option | Effect |
| --- | --- |
| `--offline` | never use the network; use only the cache |
| `--locked` | fail if `Supax.lock` would have to change |
| `--frozen` | both: the build that a build server should run |

`supax verify` checks, without the network, that the cache holds exactly what the lock file records:

```console
$ supax verify
ok: 1 package match Supax.lock and the cache
```

`supax cache info` shows where the cache is and how big; `supax cache clean` deletes it, and anything
needed is fetched again on the next build.

## What Supax will not do

A package is code written by someone else, so Supax treats it with care. It never runs anything that
comes from a fetched package: there are no build scripts, and no git hooks run. Every checkout is
read-only and checked against its checksum before every use. A path dependency inside a fetched package
cannot reach outside its repository. And package names that would look identical to a reader but
differ in hidden characters are refused.
