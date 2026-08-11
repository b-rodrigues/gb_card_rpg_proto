#!/usr/bin/env python3
"""
CLI entry point for Game Boy RPG development harness.

Usage:
  python3 tools/dev.py test                    # Run all scenario tests
  python3 tools/dev.py test --state            # ... and print semantic state
  python3 tools/dev.py scenario <name>         # Run a specific scenario by name
  python3 tools/dev.py scenario <name> --state # ... and print semantic state
  python3 tools/dev.py state <name>            # Run scenario and dump semantic state
  python3 tools/dev.py roundtrip <name>        # GameState <-> descriptor lossless check
  python3 tools/dev.py list                    # List all available scenarios
"""

import argparse
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_runner import (run_all, run_scenario, load_scenarios, print_result,
                         run_roundtrip, print_roundtrip)

def main():
    parser = argparse.ArgumentParser(description="Game Boy RPG LLM Development Harness CLI")
    subparsers = parser.add_subparsers(dest="command", required=True)

    test_parser = subparsers.add_parser("test", help="Run all scenarios")
    test_parser.add_argument("--state", action="store_true",
                             help="Print semantic state for every scenario")

    scen_parser = subparsers.add_parser("scenario", help="Run a specific scenario")
    scen_parser.add_argument("name", help="Name of scenario")
    scen_parser.add_argument("--state", action="store_true",
                             help="Print semantic state after running")

    state_parser = subparsers.add_parser("state", help="Run a scenario and dump its semantic state")
    state_parser.add_argument("name", help="Name of scenario")

    round_parser = subparsers.add_parser("roundtrip", help="Check GameState <-> descriptor lossless roundtrip")
    round_parser.add_argument("name", help="Name of scenario")

    subparsers.add_parser("list", help="List available scenarios")

    args = parser.parse_args()

    if args.command == "test":
        sys.exit(run_all("tools/scenarios", show_state=args.state))
    elif args.command == "scenario":
        scenarios = load_scenarios("tools/scenarios")
        matched = [s for s in scenarios if s.get("name") == args.name]
        if not matched:
            print(f"Error: Scenario '{args.name}' not found.", file=sys.stderr)
            sys.exit(1)
        res = run_scenario(matched[0])
        print_result(res, show_state=args.state)
        sys.exit(0 if res["status"] == "PASS" else 1)
    elif args.command == "state":
        scenarios = load_scenarios("tools/scenarios")
        matched = [s for s in scenarios if s.get("name") == args.name]
        if not matched:
            print(f"Error: Scenario '{args.name}' not found.", file=sys.stderr)
            sys.exit(1)
        res = run_scenario(matched[0])
        print_result(res, show_state=True)
        sys.exit(0 if res["status"] == "PASS" else 1)
    elif args.command == "roundtrip":
        scenarios = load_scenarios("tools/scenarios")
        matched = [s for s in scenarios if s.get("name") == args.name]
        if not matched:
            print(f"Error: Scenario '{args.name}' not found.", file=sys.stderr)
            sys.exit(1)
        res = run_roundtrip(matched[0])
        print_roundtrip(res)
        sys.exit(0 if res["status"] == "PASS" else 1)
    elif args.command == "list":
        scenarios = load_scenarios("tools/scenarios")
        print(f"Available Scenarios ({len(scenarios)}):")
        for s in scenarios:
            print(f"  - {s.get('name', 'unnamed')}: {s.get('description', '')}")

if __name__ == "__main__":
    main()
