#!/usr/bin/env python3
"""The steps of a release (.github/workflows/release.yml; CI.md, "Releases").

  release.py version TAG
      The tag is vX.Y.Z or vX.Y.Z-rc.N, and X.Y.Z is the version of both
      Supax.toml (the package sultanc) and compiler/Supax.toml. Outputs:
      version (the tag without its v), base (X.Y.Z), prerelease (true for an
      -rc. tag).
  release.py components FILE
      The tools .github/ci/components.toml pins: a github.com repository and a
      full commit SHA each, never the placeholder. Outputs: supax_repo,
      supax_commit, analyzer_repo, analyzer_commit (repo as owner/name).
  release.py notes --version V --repo OWNER/NAME --tag TAG [--out FILE]
      The release notes: the section of CHANGELOG.md for the base version of
      V, which must exist and say something.
  release.py package --version V --target T --commit SHA --compiler PATH
                     --supax PATH --analyzer PATH --out DIR [--llvm]
      Lays out sultanc-V-T/ (bin/, library/, LICENSE, README.md,
      INSTALL.txt), packs it as DIR/sultanc-V-T.tar.gz (.zip for Windows)
      and writes DIR/<archive>.sha256. With --llvm the compiler links libLLVM:
      the library is shipped with it (lib/ on Linux and macOS, bin/ on
      Windows) and the compiler's run path is set to find it there.
      Output: archive.
  release.py smoke ARCHIVE --stamp S --base B [--llvm]
      Unpacks ARCHIVE into a fresh directory and runs what it holds with no
      SultanC or LLVM variables set and from another directory: sultanc
      --version (the stamped version S and the backends), sultanc check and
      run of a program, supax --version (B), and the server's answer to an
      LSP initialize request (sultanc-analyzer, version B).
  release.py sums DIR --version V --targets T,T,...
      Every archive of V for those targets is in DIR and matches its .sha256;
      writes DIR/SHA256SUMS.txt.

Run from the repository root. Outputs go to $GITHUB_OUTPUT (else stdout).
Standard library only (tomllib: Python 3.11 or newer).
"""

import argparse
import gzip
import hashlib
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import tarfile
import tempfile
import threading
import time
import zipfile

PLACEHOLDER = "0" * 40
TAG = re.compile(r"^v(\d+\.\d+\.\d+)(-rc\.\d+)?$")
SHA = re.compile(r"^[0-9a-f]{40}$")
REPO = re.compile(r"^https://github\.com/([A-Za-z0-9_.-]+)/([A-Za-z0-9_.-]+?)(?:\.git)?/?$")
COMPONENTS = (("supax", "supax"), ("sultanc-analyzer", "analyzer"))
# What the smoke check removes from the environment, so the unpacked tree has
# to stand on its own.
UNSET = ("SULTANC_STD", "SULTANC", "SULTANC_LINKER", "LLVM_CONFIG", "SULTANC_LLVM_LIBRARY",
         "LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH", "DYLD_FALLBACK_LIBRARY_PATH")


class Failure(Exception):
    pass


def output(**values):
    text = "".join(f"{k}={v}\n" for k, v in values.items())
    path = os.environ.get("GITHUB_OUTPUT")
    if path:
        with open(path, "a", encoding="utf-8") as f:
            f.write(text)
    sys.stdout.write(text)


def summary(text):
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if path:
        with open(path, "a", encoding="utf-8") as f:
            f.write(text + "\n")


def load_toml(path):
    import tomllib
    try:
        with open(path, "rb") as f:
            return tomllib.load(f)
    except FileNotFoundError:
        raise Failure(f"{path} does not exist")
    except tomllib.TOMLDecodeError as e:
        raise Failure(f"{path} is not valid TOML: {e}")


def package_version(path):
    version = load_toml(path).get("package", {}).get("version")
    if not isinstance(version, str) or not version:
        raise Failure(f"{path} has no [package] version")
    return version


# --- version ------------------------------------------------------------------------


def cmd_version(args):
    match = TAG.match(args.tag)
    if not match:
        raise Failure(f"tag {args.tag!r} is not a release tag: vX.Y.Z or vX.Y.Z-rc.N")
    base = match.group(1)
    for path in ("Supax.toml", "compiler/Supax.toml"):
        version = package_version(path)
        if version != base:
            raise Failure(f"{path} says version {version!r}, the tag {args.tag} needs {base!r}: "
                          f"set version = \"{base}\" in Supax.toml and compiler/Supax.toml")
    prerelease = match.group(2) is not None
    output(version=args.tag[1:], base=base, prerelease=json.dumps(prerelease))


# --- components ---------------------------------------------------------------------


def cmd_components(args):
    data = load_toml(args.file)
    names = [name for name, _ in COMPONENTS]
    extra = sorted(set(data) - set(names))
    if extra:
        raise Failure(f"{args.file}: unexpected tables {extra} (only {names})")
    values = {}
    for name, key in COMPONENTS:
        table = data.get(name)
        if not isinstance(table, dict):
            raise Failure(f"{args.file}: no [{name}] table")
        repo = table.get("repo", "")
        commit = table.get("commit", "")
        match = REPO.match(repo) if isinstance(repo, str) else None
        if not match:
            raise Failure(f"{args.file}: {name}.repo must be https://github.com/OWNER/NAME, not {repo!r}")
        if commit == PLACEHOLDER:
            raise Failure(f"{args.file}: {name}.commit is still the placeholder: fill in the full SHA of the "
                          f"{name} commit this release ships (CI.md, \"Releases\")")
        if not isinstance(commit, str) or not SHA.match(commit):
            raise Failure(f"{args.file}: {name}.commit must be a full 40-character lowercase SHA, not {commit!r}")
        values[f"{key}_repo"] = f"{match.group(1)}/{match.group(2)}"
        values[f"{key}_url"] = f"https://github.com/{match.group(1)}/{match.group(2)}"
        values[f"{key}_commit"] = commit
    output(**values)


# --- notes --------------------------------------------------------------------------


def changelog_section(path, base):
    with open(path, encoding="utf-8") as f:
        lines = f.read().splitlines()
    heading = re.compile(r"^## \[?" + re.escape(base) + r"\]?(\s|$)")
    start = next((i for i, line in enumerate(lines) if heading.match(line)), None)
    if start is None:
        raise Failure(f"{path} has no section \"## {base}\": move the Unreleased entries into it")
    end = next((i for i in range(start + 1, len(lines)) if lines[i].startswith("## ")), len(lines))
    body = "\n".join(lines[start + 1:end]).strip()
    if not re.search(r"^\s*[-*] ", body, re.M):
        raise Failure(f"{path}: the section \"## {base}\" lists no change")
    return body


def cmd_notes(args):
    match = TAG.match("v" + args.version)
    if not match:
        raise Failure(f"{args.version!r} is not a release version")
    base = match.group(1)
    body = changelog_section("CHANGELOG.md", base)
    blob = f"https://github.com/{args.repo}/blob/{args.tag}"
    text = (
        f"{body}\n\n"
        f"العربية: [CHANGELOG.ar.md]({blob}/CHANGELOG.ar.md)\n\n"
        "### Install\n\n"
        "Download the archive for your platform, check it against `SHA256SUMS.txt` "
        "(`sha256sum -c SHA256SUMS.txt --ignore-missing`), unpack it and add its `bin/` to `PATH`. "
        f"`INSTALL.txt` in the archive has the steps. Each archive carries a build-provenance attestation: "
        f"`gh attestation verify <archive> --repo {args.repo}`.\n"
    )
    if args.out:
        with open(args.out, "w", encoding="utf-8") as f:
            f.write(text)
    else:
        sys.stdout.write(text)


# --- package ------------------------------------------------------------------------


def run(command, **kwargs):
    print("+ " + " ".join(command), flush=True)
    return subprocess.run(command, check=True, **kwargs)


def capture(command):
    return subprocess.run(command, check=True, capture_output=True, text=True).stdout


def tracked(root, paths):
    """The files git tracks under paths (the checkout of the tagged commit)."""
    out = subprocess.run(["git", "-C", root, "ls-files", "-z", "--", *paths],
                         check=True, capture_output=True).stdout
    files = [p for p in out.decode("utf-8").split("\0") if p]
    if not files:
        raise Failure(f"git tracks none of {paths}")
    return files


def install(source, destination, mode):
    os.makedirs(os.path.dirname(destination), exist_ok=True)
    shutil.copyfile(source, destination)
    os.chmod(destination, mode)


def llvm_library(root, windows):
    """The libLLVM the compiler build/sultanc links: Windows' LLVM-C.dll
    beside it (windows.ps1 chain), else the library the build driver recorded
    in build/sultanc.stamp ("llvm yes PATH")."""
    if windows:
        path = os.path.join(root, "build", "LLVM-C.dll")
        if not os.path.isfile(path):
            raise Failure(f"no {path}: the Windows compiler imports LLVM-C.dll (windows.ps1 chain copies it)")
        return path
    stamp = os.path.join(root, "build", "sultanc.stamp")
    try:
        with open(stamp, encoding="utf-8") as f:
            for line in f:
                if line.startswith("llvm yes "):
                    path = line[len("llvm yes "):].strip()
                    if os.path.isfile(path):
                        return os.path.realpath(path)
                    raise Failure(f"{stamp} names {path}, which does not exist")
    except FileNotFoundError:
        pass
    raise Failure(f"{stamp} records no libLLVM: build/sultanc was not built --enable-llvm")


def ship_llvm(root, directory, windows):
    """Ships the compiler's libLLVM in the archive and points the compiler's
    run path at it. The compiler records the directory of the libLLVM it was
    linked with as its run path (compiler/link/dependency/llvm.sn), which is
    the SDK on the build machine."""
    library = llvm_library(root, windows)
    compiler = os.path.join(directory, "bin", "sultanc.exe" if windows else "sultanc")
    if windows:
        # The loader looks in the executable's directory first.
        install(library, os.path.join(directory, "bin", "LLVM-C.dll"), 0o755)
        return "bin/LLVM-C.dll"
    if sys.platform == "darwin":
        # The name the compiler loads it by (its LC_ID_DYLIB, @rpath/...).
        ident = capture(["otool", "-D", library]).splitlines()[-1].strip()
        name = os.path.basename(ident)
        install(library, os.path.join(directory, "lib", name), 0o755)
        rpaths = re.findall(r"cmd LC_RPATH\n\s+cmdsize \d+\n\s+path (.+?) \(offset", capture(["otool", "-l", compiler]))
        new = "@executable_path/../lib"
        if rpaths:
            # Replace the build machine's path in place (the new one is shorter).
            run(["install_name_tool", "-rpath", rpaths[0], new, compiler])
            for old in rpaths[1:]:
                run(["install_name_tool", "-delete_rpath", old, compiler])
        else:
            run(["install_name_tool", "-add_rpath", new, compiler])
        return f"lib/{name}"
    # Linux: the name it is loaded by is its DT_SONAME.
    name = capture(["patchelf", "--print-soname", library]).strip() or os.path.basename(library)
    install(library, os.path.join(directory, "lib", name), 0o755)
    run(["patchelf", "--set-rpath", "$ORIGIN/../lib", compiler])
    return f"lib/{name}"


def install_text(version, target, commit, archive, llvm):
    """INSTALL.txt: what the archive holds and how to install it, for its
    platform, in English and Arabic."""
    windows = "windows" in target
    exe = ".exe" if windows else ""
    rows = [
        (f"bin/sultanc{exe}", "the compiler"),
        (f"bin/supax{exe}", "the package manager"),
        (f"bin/sultanc-analyzer{exe}", "the language server (LSP) for editors"),
        ("library/", "the standard library: the compiler finds it next to bin/"),
    ]
    if llvm:
        rows.append((llvm, "the LLVM library of the compiler's LLVM backend"
                     + ("" if windows or "darwin" in target else " (it needs zlib, zstd and libxml2)")))
    rows.append(("LICENSE, README.md", ""))
    width = max(len(path) for path, _ in rows) + 3
    contents = "".join(f"  {path.ljust(width)}{what}".rstrip() + "\n" for path, what in rows)
    if windows:
        check = (f"     Get-FileHash {archive}\n"
                 "     (in PowerShell; the hash must be the one SHA256SUMS.txt gives for it)\n")
        path = ("     Settings > System > About > Advanced system settings > Environment Variables:\n"
                f"     add the full path of sultanc-{version}-{target}\\bin to your user Path\n")
    else:
        tool = "shasum -a 256" if "darwin" in target else "sha256sum"
        check = f"     {tool} -c SHA256SUMS.txt --ignore-missing\n"
        path = (f'     export PATH="$HOME/sultanc-{version}-{target}/bin:$PATH"\n'
                "     (in ~/.profile or ~/.zshrc, so that new terminals have it)\n")
    return (
        f"SultanC {version} ({target})\n"
        f"commit {commit}\n\n"
        f"{contents}\n"
        "Install\n"
        "  1. Check the archive against SHA256SUMS.txt from the same release:\n"
        f"{check}"
        "  2. Unpack it anywhere, for example in your home directory, and keep\n"
        "     library/ next to bin/.\n"
        "  3. Add its bin/ directory to PATH:\n"
        f"{path}"
        "  4. Open a new terminal and run: sultanc --version\n\n"
        "The build provenance of the archive can be checked with the GitHub CLI:\n"
        f"  gh attestation verify {archive} --repo sultan-language/sultanc\n\n"
        "To build from source instead, see README.md (\"Install\").\n\n"
        "التثبيت\n"
        "  1. تحقق من الأرشيف بالملف SHA256SUMS.txt من الإصدار نفسه (الأمر أعلاه).\n"
        "  2. فك الأرشيف في أي مكان، واترك library/ بجوار bin/: منه يقرأ المترجم مكتبته القياسية.\n"
        "  3. أضف المجلد bin/ إلى PATH، ثم افتح نافذة طرفية جديدة وشغّل: sultanc --version\n"
    )


def add_tar(tar, path, arcname, mtime):
    info = tar.gettarinfo(path, arcname)
    info.uid = info.gid = 0
    info.uname = info.gname = ""
    info.mtime = mtime
    if info.isfile():
        with open(path, "rb") as f:
            tar.addfile(info, f)
    else:
        tar.addfile(info)


def pack(directory, archive, windows, mtime):
    parent = os.path.dirname(directory)
    entries = []
    for current, subdirectories, files in os.walk(directory):
        subdirectories.sort()
        entries.append(current)
        entries += [os.path.join(current, name) for name in sorted(files)]
    if windows:
        stamp = tuple(time.gmtime(max(mtime, 315532800))[:6])
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
            for path in entries:
                name = os.path.relpath(path, parent).replace(os.sep, "/")
                if os.path.isdir(path):
                    info = zipfile.ZipInfo(name + "/", stamp)
                    info.external_attr = (0o40755 << 16) | 0x10
                    z.writestr(info, b"")
                else:
                    info = zipfile.ZipInfo(name, stamp)
                    info.external_attr = (stat.S_IMODE(os.stat(path).st_mode) | stat.S_IFREG) << 16
                    info.compress_type = zipfile.ZIP_DEFLATED
                    with open(path, "rb") as f:
                        z.writestr(info, f.read())
        return
    with open(archive, "wb") as raw:
        with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=mtime, compresslevel=9) as gz:
            with tarfile.open(fileobj=gz, mode="w", format=tarfile.PAX_FORMAT) as tar:
                for path in entries:
                    add_tar(tar, path, os.path.relpath(path, parent), mtime)


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def cmd_package(args):
    root = os.getcwd()
    windows = "windows" in args.target
    exe = ".exe" if windows else ""
    name = f"sultanc-{args.version}-{args.target}"
    os.makedirs(args.out, exist_ok=True)
    directory = os.path.join(args.out, name)
    shutil.rmtree(directory, ignore_errors=True)
    os.makedirs(os.path.join(directory, "bin"))
    for source, binary in ((args.compiler, "sultanc"), (args.supax, "supax"), (args.analyzer, "sultanc-analyzer")):
        # A program built with -o NAME on Windows may or may not carry .exe.
        candidates = [source, source + ".exe"] if windows and not source.endswith(".exe") else [source]
        found = next((c for c in candidates if os.path.isfile(c)), None)
        if not found:
            raise Failure(f"no {source}: {binary} was not built")
        install(found, os.path.join(directory, "bin", binary + exe), 0o755)
    shipped = ship_llvm(root, directory, windows) if args.llvm else ""
    for path in tracked(root, ["library"]):
        install(os.path.join(root, path), os.path.join(directory, path), 0o644)
    for path in ("LICENSE", "README.md"):
        install(os.path.join(root, path), os.path.join(directory, path), 0o644)
    archive_name = name + (".zip" if windows else ".tar.gz")
    with open(os.path.join(directory, "INSTALL.txt"), "w", encoding="utf-8", newline="\n") as f:
        f.write(install_text(args.version, args.target, args.commit, archive_name, shipped))
    mtime = int(os.environ.get("SOURCE_DATE_EPOCH") or 0) or int(time.time())
    archive = os.path.join(args.out, archive_name)
    pack(directory, archive, windows, mtime)
    shutil.rmtree(directory)
    with open(archive + ".sha256", "w", encoding="utf-8", newline="\n") as f:
        f.write(f"{sha256(archive)}  {archive_name}\n")
    summary(f"- `{archive_name}`: {os.path.getsize(archive) // 1024} KiB, sha256 `{sha256(archive)}`")
    output(archive=archive.replace(os.sep, "/"))


# --- smoke --------------------------------------------------------------------------


def unpack(archive, into):
    if archive.endswith(".zip"):
        with zipfile.ZipFile(archive) as z:
            z.extractall(into)
            for info in z.infolist():
                mode = info.external_attr >> 16
                if mode and not info.is_dir():
                    os.chmod(os.path.join(into, info.filename), stat.S_IMODE(mode))
    else:
        with tarfile.open(archive) as tar:
            if hasattr(tarfile, "data_filter"):
                tar.extractall(into, filter="data")
            else:
                tar.extractall(into)
    roots = os.listdir(into)
    if len(roots) != 1:
        raise Failure(f"{archive} must hold one directory, not {roots}")
    return os.path.join(into, roots[0])


def clean_environment():
    env = {k: v for k, v in os.environ.items() if k not in UNSET}
    env["PATH"] = os.pathsep.join(p for p in env.get("PATH", "").split(os.pathsep) if "llvm-sdk" not in p)
    env["SULTANC_CACHE"] = "off"
    return env


def check_run(command, cwd, env, expect=(), timeout=600):
    print("+ " + " ".join(command), flush=True)
    try:
        result = subprocess.run(command, cwd=cwd, env=env, capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        raise Failure(f"{command[0]} did not finish in {timeout} s")
    text = result.stdout.decode("utf-8", "replace") + result.stderr.decode("utf-8", "replace")
    print(text, end="" if text.endswith("\n") else "\n")
    if result.returncode != 0:
        raise Failure(f"{' '.join(command)} exited with {result.returncode}")
    for needle in expect:
        if needle not in text:
            raise Failure(f"{' '.join(command)} does not say {needle!r}")
    return text


def lsp_initialize(server, cwd, env, timeout=180):
    """Sends an LSP initialize request, waits for its answer, then shuts the
    server down. Returns the serverInfo of the answer."""
    # Its log goes to a file, so a talkative server never blocks on a full pipe.
    log = tempfile.TemporaryFile()
    process = subprocess.Popen([server], cwd=cwd, env=env, stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE, stderr=log)

    def send(message):
        body = json.dumps(message).encode("utf-8")
        process.stdin.write(b"Content-Length: %d\r\n\r\n" % len(body) + body)
        process.stdin.flush()

    answer = {}

    def read():
        while True:
            headers = {}
            while True:
                line = process.stdout.readline()
                if not line:
                    return
                line = line.strip()
                if not line:
                    break
                key, _, value = line.decode("ascii", "replace").partition(":")
                headers[key.strip().lower()] = value.strip()
            length = int(headers.get("content-length", "0"))
            message = json.loads(process.stdout.read(length).decode("utf-8"))
            if message.get("id") == 1 and ("result" in message or "error" in message):
                answer.update(message)
                return

    reader = threading.Thread(target=read, daemon=True)
    reader.start()
    try:
        send({"jsonrpc": "2.0", "id": 1, "method": "initialize",
              "params": {"processId": None, "rootUri": None, "capabilities": {}}})
        reader.join(timeout)
        if not answer:
            raise Failure(f"{server} did not answer initialize within {timeout} s")
        if "error" in answer:
            raise Failure(f"{server} answered initialize with an error: {answer['error']}")
        send({"jsonrpc": "2.0", "method": "initialized", "params": {}})
        send({"jsonrpc": "2.0", "id": 2, "method": "shutdown"})
        send({"jsonrpc": "2.0", "method": "exit"})
        try:
            process.wait(30)
        except subprocess.TimeoutExpired:
            raise Failure(f"{server} did not exit after shutdown and exit")
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
        log.seek(0)
        stderr = log.read().decode("utf-8", "replace")
        log.close()
        if stderr:
            print(stderr)
    return answer.get("result", {}).get("serverInfo", {})


def cmd_smoke(args):
    work = tempfile.mkdtemp(prefix="sultanc-release-", dir=os.environ.get("RUNNER_TEMP"))
    tree = unpack(args.archive, os.path.join(work, "unpacked"))
    exe = ".exe" if args.archive.endswith(".zip") else ""
    binary = os.path.join(tree, "bin", "sultanc" + exe)
    env = clean_environment()
    elsewhere = os.path.join(work, "project")
    os.makedirs(elsewhere)
    backends = ["native"] + (["llvm"] if args.llvm else [])
    text = check_run([binary, "--version"], elsewhere, env, [args.stamp, *backends])
    shutil.copyfile(os.path.join("examples", "en", "basics", "hello.sn"), os.path.join(elsewhere, "hello.sn"))
    check_run([binary, "check", "hello.sn"], elsewhere, env)
    check_run([binary, "run", "hello.sn"], elsewhere, env)
    supax = check_run([os.path.join(tree, "bin", "supax" + exe), "--version"], elsewhere, env, [args.base])
    info = lsp_initialize(os.path.join(tree, "bin", "sultanc-analyzer" + exe), elsewhere, env)
    print(f"sultanc-analyzer serverInfo: {info}")
    if info.get("name") != "sultanc-analyzer" or info.get("version") != args.base:
        raise Failure(f"sultanc-analyzer reports {info}, not sultanc-analyzer {args.base}: "
                      f".github/ci/components.toml must pin its {args.base} commit")
    summary(f"- `{os.path.basename(args.archive)}`: `{text.splitlines()[0].strip()}`, "
            f"supax `{supax.strip().splitlines()[0]}`, sultanc-analyzer `{info['version']}`")
    shutil.rmtree(work, ignore_errors=True)


# --- sums ---------------------------------------------------------------------------


def cmd_sums(args):
    lines = []
    for target in [t for t in args.targets.split(",") if t]:
        name = f"sultanc-{args.version}-{target}" + (".zip" if "windows" in target else ".tar.gz")
        path = os.path.join(args.dir, name)
        if not os.path.isfile(path):
            raise Failure(f"no {name} in {args.dir}")
        digest = sha256(path)
        try:
            with open(path + ".sha256", encoding="utf-8") as f:
                recorded = f.read().split()
        except FileNotFoundError:
            raise Failure(f"no {name}.sha256 in {args.dir}")
        if recorded != [digest, name]:
            raise Failure(f"{name}.sha256 says {recorded}, the file's SHA-256 is {digest}")
        lines.append(f"{digest}  {name}\n")
    if not lines:
        raise Failure("no targets")
    with open(os.path.join(args.dir, "SHA256SUMS.txt"), "w", encoding="utf-8", newline="\n") as f:
        f.writelines(sorted(lines, key=lambda line: line.split()[1]))
    sys.stdout.writelines(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    p = commands.add_parser("version")
    p.add_argument("tag")
    p.set_defaults(run=cmd_version)
    p = commands.add_parser("components")
    p.add_argument("file")
    p.set_defaults(run=cmd_components)
    p = commands.add_parser("notes")
    p.add_argument("--version", required=True)
    p.add_argument("--repo", required=True)
    p.add_argument("--tag", required=True)
    p.add_argument("--out")
    p.set_defaults(run=cmd_notes)
    p = commands.add_parser("package")
    for option in ("--version", "--target", "--commit", "--compiler", "--supax", "--analyzer", "--out"):
        p.add_argument(option, required=True)
    p.add_argument("--llvm", action="store_true")
    p.set_defaults(run=cmd_package)
    p = commands.add_parser("smoke")
    p.add_argument("archive")
    p.add_argument("--stamp", required=True)
    p.add_argument("--base", required=True)
    p.add_argument("--llvm", action="store_true")
    p.set_defaults(run=cmd_smoke)
    p = commands.add_parser("sums")
    p.add_argument("dir")
    p.add_argument("--version", required=True)
    p.add_argument("--targets", required=True)
    p.set_defaults(run=cmd_sums)
    args = parser.parse_args()
    try:
        args.run(args)
    except (Failure, subprocess.CalledProcessError, OSError) as e:
        print(f"::error title=release::{e}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
