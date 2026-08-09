#!/usr/bin/env python3
import argparse
import sys
import os
from test_runner import run_all, run_scenario, load_scenarios

def main():
    parser = argparse.ArgumentParser(description="Host-side test runner")
    subparsers = parser.add_subparsers(dest="command", required=True)

    test_parser = subparsers.add_parser("test", help="Run all scenarios")

    scenario_parser = subparsers.add_parser("scenario", help="Run one scenario")
    scenario_parser.add_argument("name", help="Name of the scenario to run")

    list_parser = subparsers.add_parser("list", help="List available scenarios")

    args = parser.parse_args()
    
    scenarios = load_scenarios("tools/scenarios")

    if args.command == "test":
        run_all(scenarios)
    elif args.command == "scenario":
        scen = next((s for s in scenarios if s['name'] == args.name), None)
        if scen:
            run_scenario(scen)
        else:
            print(f"Error: Scenario '{args.name}' not found.")
            sys.exit(1)
    elif args.command == "list":
        for s in scenarios:
            print(f"- {s['name']}: {s.get('description', '')}")

if __name__ == "__main__":
    main()
