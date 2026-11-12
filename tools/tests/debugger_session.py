"""Debugger sessions of tests/{en,ar}/debugger (driven by debugger_tests.sh).

  debugger_session.py translate SPEC PROGRAM_DIR gdb|lldb local|qemu
      prints the debugger's command file for the session SPEC;
  debugger_session.py check SPEC gdb|lldb TRANSCRIPT LABEL
      prints one PASS or FAIL line per expectation of SPEC.

The session format is described in tests/en/debugger/values.dbg. Every
command's output is delimited in the transcript by a marker line, so each
expectation is matched against the output of its own command only.
"""
import os
import re
import sys

MARKER = "@@STEP {}@@"


def parse(spec_path):
    """The commands of a session: (line number, command, [(debugger, regex)])."""
    commands = []
    with open(spec_path, encoding="utf-8") as spec:
        for number, raw in enumerate(spec, 1):
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            word, _, rest = line.partition(" ")
            if word in ("expect", "expect-gdb", "expect-lldb"):
                if not commands:
                    raise SystemExit(f"{spec_path}:{number}: expectation before any command")
                debugger = word[len("expect-"):] if "-" in word else None
                commands[-1][2].append((debugger, rest, number))
            else:
                commands.append((number, line, []))
    return commands


def markers(program_dir):
    """Breakpoint markers `// BP:name` -> (file name, line) in the program's sources."""
    found = {}
    for root, _, files in os.walk(program_dir):
        for name in sorted(files):
            if not name.endswith(".sn"):
                continue
            with open(os.path.join(root, name), encoding="utf-8") as source:
                for number, text in enumerate(source, 1):
                    match = re.search(r"//\s*BP:(\S+)", text)
                    if match:
                        found[match.group(1)] = (name, number)
    return found


def translate_command(command, debugger, mode, places):
    word, _, rest = command.partition(" ")
    gdb = debugger == "gdb"
    if word == "break":
        if rest.startswith("@"):
            file_name, line = places[rest[1:]]
            return f"break {file_name}:{line}" if gdb else f"breakpoint set --file {file_name} --line {line}"
        if rest.startswith("fn "):
            name = rest[3:].strip()
            return f"break {name}" if gdb else f"breakpoint set --name {name}"
        raise SystemExit(f"unknown breakpoint form: {command}")
    if word == "run":
        if gdb:
            return "run"
        return "process launch" if mode == "local" else "process continue"
    simple = {
        "continue": ("continue", "process continue"),
        "next": ("next", "thread step-over"),
        "step": ("step", "thread step-in"),
        "finish": ("finish", "thread step-out"),
        "up": ("up", "up"),
        "bt": ("bt", "thread backtrace"),
        "locals": ("info locals", "frame variable"),
        "args": ("info args", "frame variable --no-locals"),
    }
    if word in simple:
        return simple[word][0 if gdb else 1]
    if word == "print":
        return f"print {rest}" if gdb else f"frame variable {rest}"
    raise SystemExit(f"unknown command: {command}")


def translate(spec_path, program_dir, debugger, mode):
    commands = parse(spec_path)
    places = markers(program_dir)
    lines = []
    if debugger == "gdb":
        # A Python loop, so one failing command does not end the batch run.
        lines += ["set pagination off", "set confirm off", "set width 0", "set print pretty off", "python"]
        lines.append("import gdb")
        lines.append("_steps = [")
        for index, (_, command, _) in enumerate(commands):
            lines.append(f"    ({index}, {translate_command(command, debugger, mode, places)!r}),")
        lines.append("]")
        lines.append("for _i, _c in _steps:")
        lines.append(f"    print({MARKER!r}.format(_i), flush=True)")
        lines.append("    try:")
        lines.append("        gdb.execute(_c)")
        lines.append("    except gdb.error as _e:")
        lines.append("        print('error: ' + str(_e), flush=True)")
        lines.append("end")
        lines.append("kill")
    else:
        lines.append("settings set interpreter.stop-command-source-on-error false")
        lines.append("settings set auto-confirm true")
        if mode == "qemu":
            lines.append("gdb-remote localhost:@PORT@")
        for index, (_, command, _) in enumerate(commands):
            lines.append(f"script print({MARKER.format(index)!r})")
            lines.append(translate_command(command, debugger, mode, places))
        lines.append("process kill")
    print("\n".join(lines))


def check(spec_path, debugger, transcript_path, label):
    commands = parse(spec_path)
    with open(transcript_path, encoding="utf-8", errors="replace") as transcript:
        text = transcript.read()
    segments = {}
    parts = re.split(r"@@STEP (\d+)@@", text)
    for position in range(1, len(parts) - 1, 2):
        segments[int(parts[position])] = parts[position + 1]
    spec_name = os.path.basename(spec_path)
    for index, (number, command, expectations) in enumerate(commands):
        output = segments.get(index)
        for wanted, regex, expect_line in expectations:
            if wanted is not None and wanted != debugger:
                continue
            where = f"{label}: {spec_name}:{expect_line} `{command}` ~ /{regex}/"
            # Whitespace-insensitive: gdb prints a record on one line, lldb
            # one member per line.
            flat = re.sub(r"\s+", " ", output) if output is not None else None
            if flat is not None and re.search(regex, flat):
                print(f"PASS {where}")
            else:
                print(f"FAIL {where}")


def main():
    if len(sys.argv) >= 6 and sys.argv[1] == "translate":
        translate(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5])
    elif len(sys.argv) >= 6 and sys.argv[1] == "check":
        check(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5])
    else:
        raise SystemExit(__doc__)


if __name__ == "__main__":
    main()
