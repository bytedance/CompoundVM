#!/usr/bin/env python3
"""Unit tests for check_commit.py"""

import sys

sys.path.insert(0, ".github/scripts")
from check_commit import check_commit_message


def run_tests():
    test_cases = [
        ("[Hotspot] Fix potential memleak in SymbolTable", True, "single-tag self-developed commit"),
        ("[Hotspot][Test] Add regression test for GC safepoint", True, "multi-tag self-developed commit"),
        ("[Backport] 8280087: G1: Handle out-of-mark stack situations", True, "standard backport commit"),
        ("[Hotspot] 8350651: Bump update version for OpenJDK", True, "commit with issue-id"),
        ("[Hotspot] Some change", True, "component: Hotspot"),
        ("[Altkernel] Some change", True, "component: Altkernel"),
        ("[Docs] Update README", True, "component: Docs"),
        ("[CI] Add workflow", True, "component: CI"),
        ("[Conf] Bump version", True, "component: Conf"),
        ("[Test] Add unit test", True, "component: Test"),
        ("[Misc] Cleanup unused code", True, "component: Misc"),
        ("[Backport] 1234567: Fix something", True, "component: Backport"),
        ("[Hotspot] Fix memleak\n\nDetailed explanation.\n\nIssue: #142", True, "full commit with body and trailer"),
        ("[Hotspot] Fix\n\n" + "x" * 80, True, "body line exactly 80 chars"),
        ("[Docs] Fix typo\n\nIssue: #99", True, "trailer without body"),
        ("[Hotspot] fix lowercase description", False, "lowercase description"),
        ("[Hotspot] Fix\n\n" + "x" * 81, False, "body line exceeds 80 chars"),
        ("[Docs] Fix typo\n\nIssue: 99", False, "invalid trailer format"),
        ("[Backport][Hotspot] 1234567: Fix something", False, "backport with extra tag"),
        ("hotspot: Fix memleak", False, "legacy colon format"),
        ("fix(reflection): resolve error", False, "conventional commits format"),
        ("Just a plain message", False, "plain message without tag"),
        ("", False, "empty message"),
        ("[hotspot] Fix memleak", False, "wrong tag casing"),
        ("[HOTSPOT] Fix memleak", False, "all-uppercase tag"),
        ("[GC] Fix something", False, "unknown component tag"),
        ("[InvalidTag] Some change", False, "unregistered tag"),
        ("[Backport] Fix without bugid", False, "backport without BugID"),
        ("[Backport] abc: Not a number", False, "non-numeric backport BugID"),
        ("[Hotspot] Fix\n\nCo-Authored-By: Aime <aime@bytedance.com>", False, "forbid Co-Authored-By trailer"),
        ("[Hotspot] Fix\n\nCo-authored-by: Aime <aime@bytedance.com>", False, "forbid canonical Co-authored-by trailer"),
        ("[Hotspot] Fix\n\nChange-Id: I1234567890", False, "forbid Change-Id trailer"),
        ("[Hotspot] Fix\n\nchange-id: I1234567890", False, "forbid lowercase Change-Id trailer"),
        ("[Hotspot]Fix memleak", False, "missing space after tag"),
        ("[Hotspot Fix memleak", False, "unclosed bracket"),
        ("[Hotspot] ", False, "empty description with spaces"),
        ("[Hotspot]", False, "tag without description"),
        ("[Hotspot] Fix\nBody no blank line", False, "body without blank separator"),
    ]

    passed = 0
    failed = 0
    failures = []

    for msg, expect_pass, desc in test_cases:
        result = check_commit_message(msg)
        actual_pass = result.ok

        if actual_pass == expect_pass:
            passed += 1
            status = "PASS"
        else:
            failed += 1
            status = "FAIL"
            failures.append((desc, msg, expect_pass, actual_pass, result.errors))

        expect_str = "should_pass" if expect_pass else "should_fail"
        print(f"  {status:4s}  [{expect_str:11s}] {desc}")

    print()
    print(f"{'=' * 60}")
    print(f"  Total: {passed + failed}  |  Passed: {passed}  |  Failed: {failed}")
    print(f"{'=' * 60}")

    if failures:
        print("\nFailure details:")
        for desc, msg, expect, actual, errors in failures:
            print(f"\n  {desc}")
            print(f"    Input:    {repr(msg[:60])}")
            print(f"    Expected: {'PASS' if expect else 'FAIL'}")
            print(f"    Actual:   {'PASS' if actual else 'FAIL'}")
            if errors:
                print(f"    Errors:   {errors[0]}")
        return 1

    print("\nAll tests passed!")
    return 0


if __name__ == "__main__":
    sys.exit(run_tests())
