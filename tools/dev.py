#!/usr/bin/env python3
"""
CLI entry point for Game Boy RPG development harness.

Usage:
  python3 tools/dev.py test              # Run all scenario tests
  python3 tools/dev.py scenario <name>   # Run a specific scenario by name
  python3 tools/dev.py list              # List all available scenarios
"""

import argparse
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_runner import run_all, run_scenario, load_scenarios, print_result

def main():
    parser = argparse.ArgumentParser(description="Game Boy RPG LLM Development Harness CLI")
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("test", help="Run all scenarios")

    scen_parser = subparsers.add_parser("scenario", help="Run a specific scenario")
    scen_parser.add_argument("name", help="Name of scenario")

    subparsers.add_parser("list", help="List available scenarios")

    args = parser.parse_args()

    if args.command == "test":
        sys.exit(run_all("tools/scenarios"))
    elif args.command == "scenario":
        scenarios = load_scenarios("tools/scenarios")
        matched = [s for s in scenarios if s.get("name") == args.name]
        if not matched:
            print(f"Error: Scenario '{args.name}' not found.", file=sys.stderr)
            sys.exit(1)
        res = run_scenario(matched[0])
        print_result(res)
        sys.exit(0 if res["status"] == "PASS" else 1)
    elif args.command == "list":
        scenarios = load_scenarios("tools/scenarios")
        print(f"Available Scenarios ({len(scenarios)}):")
        for s in scenarios:
            print(f"  - {s.get('name', 'unnamed')}: {s.get('description', '')}")

if __name__ == "__main__":
    main()
