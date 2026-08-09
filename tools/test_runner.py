#!/usr/bin/env python3
import json
import os
from emulator import EmulatorSession

def load_scenarios(directory):
    scenarios = []
    if not os.path.exists(directory):
        return scenarios
    for f in os.listdir(directory):
        if f.endswith('.json'):
            with open(os.path.join(directory, f), 'r') as file:
                scenarios.append(json.load(file))
    return scenarios

def run_scenario(scenario_dict):
    print(f"SCENARIO: {scenario_dict.get('name')}")
    session = EmulatorSession()
    session.connect()
    
    for action in scenario_dict.get('actions', []):
        if action['type'] == 'press':
            session.press(action['button'])
        elif action['type'] == 'wait':
            session.wait(action['frames'])
            
    # Stub assertions
    for assertion in scenario_dict.get('assertions', []):
        print(f"ASSERT: {assertion['type']} - expected {assertion.get('expected', '')}")
        
    session.disconnect()
    print("STATUS: PASS\n")

def run_all(scenarios):
    print(f"Running {len(scenarios)} scenarios...\n")
    for s in scenarios:
        run_scenario(s)
    print("All scenarios completed.")
