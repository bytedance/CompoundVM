#!/usr/bin/env python3
"""
CompoundVM Commit Message Checker

Validates commit messages against the CompoundVM commit message specification.
Can be used as a standalone script, a git hook, or integrated into CI.

Specification:
  Title: ([<component>])+ (<issue-id>:)? <short description>
  - Backport: [Backport] <BugID>: <description>
  - Self-developed: [<component>] <description>
  Body (optional): separated by blank line, 80 char width limit
  Trailers (optional): Issue: #<number>
"""

import sys
import re
import argparse
import subprocess
from typing import List, Tuple

VALID_COMPONENTS = [
    "Hotspot",
    "Altkernel",
    "Docs",
    "CI",
    "Conf",
    "Test",
    "Misc",
    "Backport",
]

MAX_TITLE_LENGTH = 120
MAX_BODY_LINE_LENGTH = 80

TITLE_PATTERN = re.compile(
    r"^(\[[A-Za-z][A-Za-z0-9/]*\])+\s+(.+)$"
)

BACKPORT_TITLE_PATTERN = re.compile(
    r"^(\[[A-Za-z][A-Za-z0-9/]*\])*\[Backport\](\[[A-Za-z][A-Za-z0-9/]*\])*\s+(\d+):\s+(.+)$"
)

ISSUE_ID_IN_TITLE_PATTERN = re.compile(
    r"^(\[[A-Za-z][A-Za-z0-9/]*\])+\s+(\d+):\s+(.+)$"
)

TRAILER_ISSUE_PATTERN = re.compile(
    r"^Issue:\s*#\d+$"
)

FORBIDDEN_LINES = (
    "Co-Authored-By:",
    "Change-Id:",
)


class CheckResult:
    def __init__(self):
        self.errors: List[str] = []
        self.warnings: List[str] = []

    def error(self, msg: str):
        self.errors.append(msg)

    def warn(self, msg: str):
        self.warnings.append(msg)

    @property
    def ok(self) -> bool:
        return len(self.errors) == 0


def parse_tags(title: str) -> Tuple[List[str], str]:
    """Extract bracket tags and the remaining message from title."""
    tags = []
    remaining = title
    while remaining.startswith("["):
        end = remaining.find("]")
        if end == -1:
            break
        tags.append(remaining[1:end])
        remaining = remaining[end + 1:]
    return tags, remaining.lstrip()


def check_title(title: str, result: CheckResult):
    """Validate the commit title line."""
    if not title:
        result.error("Title is empty")
        return

    if title != title.strip():
        result.error("Title has leading or trailing whitespace")

    if len(title) > MAX_TITLE_LENGTH:
        result.warn(
            f"Title exceeds {MAX_TITLE_LENGTH} characters "
            f"(current: {len(title)})"
        )

    if not TITLE_PATTERN.match(title):
        result.error(
            "Title does not match required format: "
            "[<component>] <description>\n"
            f"  Got: {title}\n"
            f"  Valid components: {', '.join(VALID_COMPONENTS)}"
        )
        return

    tags, message = parse_tags(title)

    if not tags:
        result.error("Title must have at least one [component] tag")
        return

    for tag in tags:
        if tag not in VALID_COMPONENTS:
            matched = [c for c in VALID_COMPONENTS if c.lower() == tag.lower()]
            if matched:
                result.error(
                    f"Tag [{tag}] has incorrect casing. "
                    f"Use [{matched[0]}] instead"
                )
            else:
                result.error(
                    f"Unknown component tag [{tag}]. "
                    f"Valid components: {', '.join(VALID_COMPONENTS)}"
                )

    if "Backport" in tags:
        if not BACKPORT_TITLE_PATTERN.match(title):
            result.error(
                "Backport commit title must follow format: "
                "[Backport] <BugID>: <description>\n"
                f"  Got: {title}"
            )
        if len(tags) > 1:
            result.warn(
                "[Backport] commits typically should not have "
                "additional component tags"
            )
    else:
        if not message:
            result.error("Title description is empty after tag(s)")

        if message and message[0].islower():
            result.warn("Title description should start with uppercase letter")


def check_body(lines: List[str], result: CheckResult):
    """Validate the commit body."""
    if not lines:
        return

    if lines[0].strip() != "":
        result.error(
            "Body must be separated from title by a blank line"
        )
        return

    body_lines = lines[1:] if lines[0].strip() == "" else lines

    for i, line in enumerate(body_lines):
        if line.startswith("Issue:"):
            continue
        if len(line) > MAX_BODY_LINE_LENGTH:
            result.warn(
                f"Body line {i + 1} exceeds {MAX_BODY_LINE_LENGTH} characters "
                f"(current: {len(line)}): {line[:50]}..."
            )


def check_trailers(lines: List[str], result: CheckResult):
    """Validate trailer lines (Issue: #xxx)."""
    trailer_lines = []
    for line in reversed(lines):
        stripped = line.strip()
        if not stripped:
            break
        if stripped.startswith("Issue:"):
            trailer_lines.append(stripped)

    for trailer in trailer_lines:
        if not TRAILER_ISSUE_PATTERN.match(trailer):
            result.warn(
                f"Trailer format should be 'Issue: #<number>', "
                f"got: {trailer}"
            )


def check_forbidden_lines(lines: List[str], result: CheckResult):
    """Reject unsupported autogenerated trailers."""
    for line in lines:
        stripped = line.strip()
        for prefix in FORBIDDEN_LINES:
            if stripped.startswith(prefix):
                result.error(f"Commit message must not contain '{prefix}'")


def check_commit_message(message: str) -> CheckResult:
    """Validate a full commit message."""
    result = CheckResult()

    if not message or not message.strip():
        result.error("Commit message is empty")
        return finalize_result(result)

    lines = message.split("\n")

    while lines and lines[-1].strip() == "":
        lines.pop()

    if not lines:
        result.error("Commit message is empty")
        return finalize_result(result)

    title = lines[0]
    check_title(title, result)
    check_forbidden_lines(lines, result)

    if len(lines) > 1:
        check_body(lines[1:], result)
        check_trailers(lines[1:], result)

    return finalize_result(result)


def get_commit_message_from_file(filepath: str) -> str:
    """Read commit message from a file (e.g., .git/COMMIT_EDITMSG)."""
    with open(filepath, "r", encoding="utf-8") as f:
        lines = []
        for line in f:
            if line.startswith("#"):
                continue
            lines.append(line.rstrip("\n"))
    return "\n".join(lines)


def get_commit_messages_from_range(rev_range: str) -> List[str]:
    """Get commit messages from a git revision range.

    Merge commits are ignored since their titles usually start with
    "Merge " and do not follow the project commit message rules.
    """
    try:
        output = subprocess.check_output(
            ["git", "log", "--pretty=format:%B%x00", rev_range],
            stderr=subprocess.STDOUT,
            text=True,
        )
    except subprocess.CalledProcessError as e:
        print(f"Error running git log: {e.output}", file=sys.stderr)
        sys.exit(1)

    messages: List[str] = []
    for raw in output.split("\x00"):
        msg = raw.strip()
        if not msg:
            continue
        first_line = msg.split("\n", 1)[0].strip()
        if first_line.startswith("Merge "):
            # Skip merge commits, they are typically created by GitHub
            # or by local merges and do not need to satisfy the
            # CompoundVM commit message rules.
            continue
        messages.append(msg)
    return messages


def finalize_result(result: CheckResult) -> CheckResult:
    """Treat all warnings as errors."""
    if result.warnings:
        result.errors.extend(result.warnings)
        result.warnings = []
    return result


def format_result(result: CheckResult, title: str = "") -> str:
    """Format check result for display."""
    lines = []
    if title:
        header = title[:72] + "..." if len(title) > 72 else title
        lines.append(f"  Commit: {header}")

    if result.ok:
        lines.append("  ✅ OK")
    else:
        for err in result.errors:
            lines.append(f"  ❌ ERROR: {err}")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="CompoundVM commit message checker",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Check a commit message file (git hook usage)
  %(prog)s --file .git/COMMIT_EDITMSG

  # Check commits in a range (CI usage)
  %(prog)s --range origin/main..HEAD

  # Check a single commit
  %(prog)s --range HEAD~1..HEAD

  # Read from stdin
  echo "[Hotspot] Fix memleak" | %(prog)s --stdin

Git hook installation:
  cp cvm/conf/check_commit.py .git/hooks/commit-msg
  chmod +x .git/hooks/commit-msg
""",
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        "--file", "-f",
        help="Path to commit message file (for commit-msg hook)",
    )
    group.add_argument(
        "--range", "-r",
        help="Git revision range to check (e.g., origin/main..HEAD)",
    )
    group.add_argument(
        "--stdin",
        action="store_true",
        help="Read commit message from stdin",
    )
    parser.add_argument(
        "--quiet", "-q",
        action="store_true",
        help="Only output on failure",
    )

    args = parser.parse_args()
    failed = False
    total = 0
    passed = 0

    if args.file:
        message = get_commit_message_from_file(args.file)
        result = finalize_result(check_commit_message(message))
        total = 1
        if not result.ok:
            failed = True
            print(format_result(result, message.split("\n")[0]))
        else:
            passed = 1
            if not args.quiet:
                print(format_result(result, message.split("\n")[0]))

    elif args.stdin:
        message = sys.stdin.read()
        result = finalize_result(check_commit_message(message))
        total = 1
        if not result.ok:
            failed = True
            print(format_result(result, message.split("\n")[0]))
        else:
            passed = 1
            if not args.quiet:
                print(format_result(result, message.split("\n")[0]))

    elif args.range:
        messages = get_commit_messages_from_range(args.range)
        if not messages:
            print("No commits found in range", file=sys.stderr)
            sys.exit(1)

        for message in messages:
            total += 1
            result = finalize_result(check_commit_message(message))
            title = message.split("\n")[0]
            if not result.ok:
                failed = True
                print(format_result(result, title))
                print()
            else:
                passed += 1
                if not args.quiet:
                    print(format_result(result, title))
                    print()

    if total > 1 or not args.quiet:
        print(f"--- Result: {passed}/{total} passed ---")

    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    if len(sys.argv) == 2 and not sys.argv[1].startswith("-"):
        message = get_commit_message_from_file(sys.argv[1])
        result = finalize_result(check_commit_message(message))
        if not result.ok:
            print("Commit message check FAILED:")
            print(format_result(result, message.split("\n")[0]))
            print(
                "\nExpected format: [<component>] <description>"
                f"\nValid components: {', '.join(VALID_COMPONENTS)}"
            )
            sys.exit(1)
        sys.exit(0)

    main()
