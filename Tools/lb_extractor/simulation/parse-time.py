import argparse
import re
import sys
import csv
import itertools
import numpy as np
from collections import defaultdict

DEFAULT_PATTERNS = {
    "step": re.compile(r'(?i)\bstep\s+(\d+)\s+starts\b'),
    "avg_time": re.compile(r'(?i)\bavg\.?\s+per\s+step\s*=\s*([+-]?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\b')
}

# def parse_lbc(data, unique_headers, results):
#     max_prange = results.get("step", [])[-1] if results.get("step") else 2000
#     prange = [i for i in range(0, max_prange+1, 100)]

#     data_fields = defaultdict(dict)
#     data_fields, keys = data_fields, list(prange)    

#     if len(data.shape) == 1:
#         data = data.reshape(-1, data.shape[0])

#     # Compute the number of datafields saved per box
#     n_data_fields = 0
#     steps = data[:, 0].astype(int)

#     times = data[:, 1]
#     data = data[:, 2:]

#     # Either 9 or 10 depending if GPU
#     n_data_fields = 9 if len(set(unique_headers)) % 9 == 0 else 10

#     # Collect the costs and ranks and write to output file
#     for key in keys:
#         row = np.where(key == steps)[0][0]
#         costs = data[row, 0::n_data_fields].astype(float)
#         ranks = data[row, 1::n_data_fields].astype(int)
#         mem = data[row, 8::n_data_fields].astype(float)
#         for i in range(costs.size):
#             results.setdefault("LBC_costs", []).append(costs[i])
#             # results.setdefault("LBC_costs", []).append(costs)
#             # results.setdefault("LBC_mem", []).append(mem)
#             # results.setdefault("LBC_ranks", []).append(ranks)

#     return results    


def write_csv(results, path):
    """Write results dict {'step': [...], 'avg_time': [...]} to CSV."""
    keys = ["step", "avg_time"]
    with open(path, "w", newline="", encoding="utf-8") as csvf:
        writer = csv.writer(csvf)
        writer.writerow(keys)
        # zip_longest will fill missing items with None -> write as empty string
        for step, avg in itertools.zip_longest(results.get("step", []),
                                               results.get("avg_time", []),
                                               fillvalue=""):
            writer.writerow([step, avg])


def find_matches(lines, default_patterns):
    pair = {"step": [], "avg_time": []}

    for lineno, line in enumerate(lines, start=0):
        step_match = default_patterns["step"].search(line)
        if step_match:
            step_num = int(step_match.group(1))
            pair["step"].append(step_num)
        avg_time_match = default_patterns["avg_time"].search(line)
        if avg_time_match:
            avg_time = float(avg_time_match.group(1))
            pair["avg_time"].append(avg_time)
    
    return pair

def main():
    p = argparse.ArgumentParser(description="Parse average time and Load Balance efficiency from output text file.")
    p.add_argument("file", help="output text file")
    # p.add_argument("LBC_file", help="output LBC file")
    p.add_argument("--regex", "-r", action="append", default=[], help="custom regex with capturing group(s). Can be repeated.")
    p.add_argument("--out", "-o", help="write CSV to this path")
    args = p.parse_args()

    try:
        with open(args.file, "r", encoding="utf-8") as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Error opening file: {e}", file=sys.stderr)
        sys.exit(2)

    # try:
    #     lbc_lines = np.genfromtxt(args.LBC_file)

    #     with open(args.LBC_file) as f:
    #         h = f.readlines()[0]
    #         unique_headers = [
    #             "".join([ln for ln in w if not ln.isdigit()]) for w in h.split()
    #         ][2::]

    # except Exception as e:
    #     print(f"Error opening file: {e}", file=sys.stderr)
    #     sys.exit(2)

    user_pats = []
    for rx in args.regex:
        try:
            user_pats.append(re.compile(rx))
        except re.error as e:
            print(f"Invalid regex '{rx}': {e}", file=sys.stderr)
            sys.exit(3)

    results = find_matches(lines, DEFAULT_PATTERNS)

    # results = parse_lbc(lbc_lines, unique_headers, results)

    # default output: print summary to stdout
    for r in results:
        print(f"{r}: {results[r][1:10]}... (total {len(results[r])} entries)")

    if args.out:
        try:
            write_csv(results, args.out)
        except Exception as e:
            print(f"Failed to write CSV: {e}", file=sys.stderr)
            sys.exit(4)

if __name__ == "__main__":
    main()