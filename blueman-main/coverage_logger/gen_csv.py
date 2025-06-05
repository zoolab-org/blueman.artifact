#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Usage:
  ./script.py /path/to/project_dir

This script reads JSON files (afl, field, random) from project_dir/input_dir
and writes corresponding CSVs (time_sec, coverage) to project_dir/csv.
"""
import sys
import os
import json
import csv

# List of expected JSON base names
FILES = ['afl', 'field', 'random', 
         'field_FIXED_PROB_10', 'field_FIXED_PROB_25', 'field_FIXED_PROB_50', 'field_FIXED_PROB_75', 'field_FIXED_PROB_100',
         'field_SELECTIVE_25_75', 'field_SELECTIVE_75_25',
         'field_RANDOM_PROB', 'field_MIXED_PROB']

def rebuild(trace, max_time):
    latest = 0
    for t in range(max_time + 1):
        if t in trace:
            latest = trace[t]
        else:
            trace[t] = latest

def json_to_csv(in_path, out_path, max_time):
    """Convert a list of JSON entries to a CSV with time (in seconds) and coverage."""
    with open(in_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    trace = {0: 0}
    for entry in data:
        time_us = entry.get('time', 0)
        time_sec = time_us // 1_000_000
        if time_sec > max_time:
            break
        trace[time_sec] = entry.get('coverage', 0)

    rebuild(trace, max_time)

    with open(out_path, 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(['time_sec', 'coverage'])
        for t in range(max_time + 1):
            writer.writerow([t, trace[t]])


def main():
    # parse project directory from arguments
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} /path/to/project_dir max_time")
        sys.exit(1)

    project_dir = sys.argv[1]
    max_time = int(sys.argv[2])
    input_dir = os.path.join(project_dir, 'input_dir')
    output_dir = os.path.join(project_dir, 'csv')

    # check input_dir exists
    if not os.path.isdir(input_dir):
        print(f"Error: input directory '{input_dir}' does not exist.")
        sys.exit(1)

    # ensure output directory exists
    os.makedirs(output_dir, exist_ok=True)

    # process each file
    for name in FILES:
        in_file = os.path.join(input_dir, name)
        out_file = os.path.join(output_dir, f'{name}.csv')

        if not os.path.isfile(in_file):
            # print(f'Warning: {in_file} not found, skipping.')
            continue

        try:
            json_to_csv(in_file, out_file, max_time)
            print(f'Written {out_file}')
        except Exception as e:
            print(f'Error converting {in_file}: {e}')

if __name__ == '__main__':
    main()

