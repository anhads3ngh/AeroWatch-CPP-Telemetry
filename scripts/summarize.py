#!/usr/bin/env python3
"""Dependency-free tooling script for analyzing simulated AeroWatch CSV replay files."""
import argparse
import csv
from collections import defaultdict


def summarize(path):
    totals = defaultdict(lambda: {"count": 0, "min_battery": 101.0, "min_link": 101.0})
    with open(path, newline="", encoding="utf-8") as file:
        for row in csv.DictReader(file):
            name = row["vehicle_id"]
            record = totals[name]
            record["count"] += 1
            record["min_battery"] = min(record["min_battery"], float(row["battery_percent"]))
            record["min_link"] = min(record["min_link"], float(row["link_percent"]))
    return totals


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_file", help="CSV data exported from AeroWatch")
    args = parser.parse_args()
    print("VEHICLE   PACKETS    MIN BATTERY    MIN LINK")
    for vehicle, data in sorted(summarize(args.csv_file).items()):
        print(f"{vehicle:9} {data['count']:7} {data['min_battery']:12.1f}% {data['min_link']:10.1f}%")


if __name__ == "__main__":
    main()
