#!/usr/bin/env python3
"""Test runner for Veldanava - separates positive and negative tests."""

import subprocess
import sys
import os
from pathlib import Path

VELDANC = Path(__file__).parent.parent / "build" / "veldanc"

def run_test(filepath, expect_success=True):
    """Run a single test file and check if result matches expectation."""
    try:
        result = subprocess.run(
            [str(VELDANC), str(filepath)],
            capture_output=True,
            text=True,
            timeout=10
        )
        success = result.returncode == 0
        
        if success == expect_success:
            status = "PASS"
        else:
            status = "FAIL"
        
        marker = "[OK]" if expect_success else "[ERROR]"
        print(f"  {marker} {filepath.name}")
        if status == "FAIL":
            print(f"       Expected {'success' if expect_success else 'failure'}, got {'success' if success else 'failure'}")
            if result.stderr:
                print(f"       stderr: {result.stderr.strip()[:200]}")
        
        return status == "PASS"
    except subprocess.TimeoutExpired:
        print(f"  [TIMEOUT] {filepath.name}")
        return False
    except Exception as e:
        print(f"  [ERROR] {filepath.name}: {e}")
        return False

def main():
    tests_dir = Path(__file__).parent
    positive_dir = tests_dir / "positive"
    negative_dir = tests_dir / "negative"
    
    results = {"positive": [], "negative": []}
    
    # Run positive tests
    if positive_dir.exists():
        positive_files = sorted(positive_dir.glob("*.veldanava"))
        print(f"=== Positive tests ({len(positive_files)} files) ===")
        for f in positive_files:
            results["positive"].append(run_test(f, expect_success=True))
    
    # Run negative tests
    if negative_dir.exists():
        negative_files = sorted(negative_dir.glob("*.veldanava"))
        print(f"\n=== Negative tests ({len(negative_files)} files) ===")
        for f in negative_files:
            results["negative"].append(run_test(f, expect_success=False))
    
    # Summary
    total = sum(len(v) for v in results.values())
    passed = sum(sum(v) for v in results.values())
    print(f"\nSUMMARY total={total} pass={passed} fail={total - passed}")
    
    if passed == total:
        print("All tests passed!")
        return 0
    else:
        print(f"{total - passed} test(s) failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())
