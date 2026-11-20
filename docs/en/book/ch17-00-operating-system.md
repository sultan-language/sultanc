[العربية](../../ar/book/ch17-00-operating-system.md) | **English**

# Working with the Operating System

A program lives inside an operating system, which owns the files, the environment, the other programs
and the clock. The standard library reaches each of them through modules that work the same on every
system SultanC supports:

| Module | What it has |
| --- | --- |
| `std.fs.read_file`, `std.fs.write_file` | a whole file as text |
| `std.fs.mutation` | `rename`, `remove_file`, `create_directory`, `remove_directory` |
| `std.fs.walk` | `create_dir_all`, `remove_tree`, `walk` |
| `std.fs.directory` | `list_directory`, `try_exists`, `try_is_file`, `try_is_directory` |
| `std.env.environment` | `get_environment_variable`, `current_directory`, `temporary_directory` |
| `std.process.arguments` | `arguments`: the command line as a vector |
| `std.process.command` | running other programs |
| `std.time.time` | the clock, and sleeping |
| `std.time.calendar` | days, months and leap years |

Everything here can fail for reasons outside the program, so almost every function returns a `result`.
The errors of the file functions are the operating system's error codes, of type `int`.

- [Files and Folders](ch17-01-files.md)
- [The Environment](ch17-02-environment.md)
- [Running Other Programs](ch17-03-processes.md)
- [Time](ch17-04-time.md)
