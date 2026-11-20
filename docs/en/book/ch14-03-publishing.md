[العربية](../../ar/book/ch14-03-publishing.md) | **English**

# Publishing a Package

When your library could be useful to others, publish it in the package index, so that anyone can add it
with `supax add`.

## Before publishing

A package can be published when:

- it has a `[lib]` section: the index holds libraries, which other packages import;
- its version follows semantic versioning: `MAJOR.MINOR.PATCH`, as `0.2.0`;
- it has no `path` dependencies, since nobody else has your folders;
- its code is committed to a public git repository, with no uncommitted changes.

Choose the version by what changed. Increase `PATCH` for fixes, `MINOR` for new features that do not
break existing code, and `MAJOR` when code that used the previous version may stop compiling. Versions
below 1.0.0 are for packages still finding their shape.

## Preparing the entry

`supax package` checks all of the above for the commit you are on, computes the checksum of its contents,
and prints the entry for the index, with the path of the file it belongs in:

```console
$ supax package --git https://github.com/you/greeting
```

Publishing is then a change to the index repository: add the printed entry to the package's file, and
open a pull request on `https://github.com/sultan-language/index`. When it is merged, `supax add greeting`
works for everyone.

A published version never changes. To fix a mistake, publish a new version.

## Summary

- Dependencies come from a folder (`path`), the index (a version requirement), or git (a tag).
- `supax add`, `remove`, `tree` and `update` manage them; modules of a dependency are imported by the
  package's name.
- `Supax.lock` records the exact commit and checksum of every dependency; commit it.
- `--offline`, `--locked` and `--frozen` make builds repeatable; `supax verify` checks the cache.
- `supax package` prepares a release for the index.
