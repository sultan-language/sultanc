#!/usr/bin/env python3
"""Fetch or validate the exact UCD 18.0.0 inputs for SultanC Unicode generation.

Development-time only. No UCD files or host-Unicode dependency are needed by
SultanC programs. This prepares verified source inputs; it does NOT generate a
Unicode implementation or replace existing STD modules.

Source: https://www.unicode.org/Public/18.0.0/ucd/
Specification: https://www.unicode.org/reports/tr44/
Unicode terms: https://www.unicode.org/terms_of_use.html
"""

from __future__ import annotations

import argparse
from hashlib import sha256
from pathlib import Path
import re
from urllib.request import urlopen

VERSION = "18.0.0"
SOURCE_URL = f"https://www.unicode.org/Public/{VERSION}/ucd/"
REQUIRED = (
    "ReadMe.txt",
    "UnicodeData.txt",
    "DerivedCoreProperties.txt",
    "PropList.txt",
    "SpecialCasing.txt",
    "CaseFolding.txt",
    "CompositionExclusions.txt",
    "DerivedNormalizationProps.txt",
    "NormalizationTest.txt",
    "auxiliary/GraphemeBreakProperty.txt",
    "auxiliary/WordBreakProperty.txt",
    "auxiliary/SentenceBreakProperty.txt",
    "auxiliary/GraphemeBreakTest.txt",
    "auxiliary/WordBreakTest.txt",
    "auxiliary/SentenceBreakTest.txt",
    "emoji/emoji-data.txt",
)
UCD_SCALAR = re.compile(rb"^[0-9A-F]{4,6}$")
HEADER_VERSION = re.compile(rb"(?:\A|\n)#\s*\S+?-(\d+\.\d+\.\d+)\.txt\s*(?:\n|\r\n)")


def validated_data(path: str, raw: bytes, readme: bytes) -> bytes:
    if not raw or b"\x00" in raw:
        raise ValueError(f"{path}: missing, empty, or non-text UCD source")
    try:
        raw.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ValueError(f"{path}: invalid UTF-8 UCD source") from exc
    if path == "ReadMe.txt":
        if VERSION.encode() not in raw:
            raise ValueError(f"ReadMe.txt does not identify Unicode {VERSION}")
    elif VERSION.encode() not in readme:
        raise ValueError(f"ReadMe.txt does not identify Unicode {VERSION}")
    match = HEADER_VERSION.search(raw[:512])
    if match and match.group(1) != VERSION.encode():
        raise ValueError(f"{path}: mixed Unicode version {match.group(1).decode()}")
    if path == "UnicodeData.txt":
        first_range = None
        previous = -1
        for number, line in enumerate(raw.splitlines(), 1):
            fields = line.split(b";")
            if len(fields) != 15 or not UCD_SCALAR.fullmatch(fields[0]):
                raise ValueError(f"UnicodeData.txt:{number}: invalid record")
            scalar = int(fields[0], 16)
            if scalar <= previous or scalar > 0x10FFFF:
                raise ValueError(f"UnicodeData.txt:{number}: unsorted or invalid scalar")
            previous = scalar
            label = fields[1]
            if label.endswith(b", First>"):
                if first_range is not None:
                    raise ValueError(f"UnicodeData.txt:{number}: nested range")
                first_range = (label[:-8], scalar)
            elif label.endswith(b", Last>"):
                if first_range is None or first_range[0] != label[:-7] or scalar <= first_range[1]:
                    raise ValueError(f"UnicodeData.txt:{number}: unmatched range")
                first_range = None
            elif first_range is not None:
                raise ValueError(f"UnicodeData.txt:{number}: unterminated range")
        if first_range is not None:
            raise ValueError("UnicodeData.txt: unterminated range")
    elif path == "CaseFolding.txt":
        for number, line in enumerate(raw.splitlines(), 1):
            content = line.split(b"#", 1)[0].strip()
            if not content:
                continue
            fields = [x.strip() for x in content.split(b";")]
            if (len(fields) != 3 or not UCD_SCALAR.fullmatch(fields[0])
                    or fields[1] not in (b"C", b"F", b"S", b"T")
                    or not fields[2] or any(not UCD_SCALAR.fullmatch(c) for c in fields[2].split())):
                raise ValueError(f"CaseFolding.txt:{number}: invalid mapping")
    elif path == "SpecialCasing.txt":
        for number, line in enumerate(raw.splitlines(), 1):
            content = line.split(b"#", 1)[0].strip()
            if not content:
                continue
            fields = [x.strip() for x in content.split(b";")]
            if (len(fields) != 5 or not UCD_SCALAR.fullmatch(fields[0])
                    or any(not all(UCD_SCALAR.fullmatch(c) for c in mapping.split())
                           for mapping in fields[1:4])):
                raise ValueError(f"SpecialCasing.txt:{number}: invalid mapping")
    return raw


def obtain(relative: str, input_dir: Path | None) -> bytes:
    if input_dir is not None:
        path = input_dir / relative
        if not path.is_file():
            raise FileNotFoundError(f"missing required UCD input: {path}")
        return path.read_bytes()
    with urlopen(SOURCE_URL + relative, timeout=30) as response:
        if response.status != 200:
            raise OSError(f"Unicode input {relative}: HTTP {response.status}")
        return response.read()


def prepare(destination: Path, input_dir: Path | None) -> None:
    if destination.exists() and any(destination.iterdir()):
        raise FileExistsError(f"refusing to overwrite non-empty directory: {destination}")
    readme = obtain("ReadMe.txt", input_dir)
    validated_data("ReadMe.txt", readme, readme)
    sources = {"ReadMe.txt": readme}
    for relative in REQUIRED[1:]:
        sources[relative] = validated_data(relative, obtain(relative, input_dir), readme)

    # All sources must pass validation before writing anything to the destination.
    destination.mkdir(parents=True, exist_ok=True)
    for relative in REQUIRED:
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(sources[relative])

    manifest = [
        f"# Unicode {VERSION} official UCD input files",
        f"# Source: {SOURCE_URL}",
    ]
    for relative in REQUIRED:
        manifest.append(f"{sha256(sources[relative]).hexdigest()}  {relative}")
    (destination / "SHA256SUMS").write_text("\n".join(manifest) + "\n", encoding="ascii")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-directory", required=True, type=Path)
    parser.add_argument("--input-directory", type=Path,
                        help="use already downloaded official 18.0.0 UCD files")
    args = parser.parse_args()
    prepare(args.output_directory, args.input_directory)


if __name__ == "__main__":
    main()
