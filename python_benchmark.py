#!/usr/bin/env python3

import argparse
import mmap
import os
import time
from typing import Callable, Dict, List, Tuple


def read_raw_byte_by_byte(path: str) -> int:
    checksum = 0
    with open(path, "rb", buffering=0) as f:
        while True:
            byte = f.read(1)
            if not byte:
                break
            checksum += byte[0]
    return checksum


def read_mmap_byte_by_byte(path: str, block_size: int) -> int:
    checksum = 0
    file_size = os.path.getsize(path)
    with open(path, "rb") as f:
        with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as mm:
            for i in range(file_size):
                checksum += mm[i]
    return checksum


def read_pandas(path: str, chunksize: int) -> int:
    try:
        import pandas as pd
    except ImportError as exc:
        raise RuntimeError("pandas is not installed. Run: pip install pandas") from exc

    checksum = 0
    for chunk in pd.read_csv(path, chunksize=chunksize):
        checksum += int(chunk.shape[0])
    return checksum


def benchmark(
    name: str,
    fn: Callable[[], int],
    iterations: int,
    file_size: int,
) -> Dict[str, float]:
    total = 0.0
    checksum = 0

    for _ in range(iterations):
        start = time.perf_counter()
        checksum ^= fn()
        total += time.perf_counter() - start

    avg = total / iterations
    gib = file_size / (1024 ** 3)
    speed = gib / avg if avg > 0 else 0.0

    return {
        "name": name,
        "avg_s": avg,
        "gib_s": speed,
        "checksum": float(checksum),
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Benchmark sequential file reading: raw, mmap, and pandas CSV parsing."
    )
    parser.add_argument("file", help="Path to file")
    parser.add_argument("--iterations", type=int, default=1, help="Number of benchmark runs")
    parser.add_argument(
        "--block-size-mb",
        type=int,
        default=8,
        help="Block size in MiB for raw and mmap methods",
    )
    parser.add_argument(
        "--methods",
        default="all",
        help="Comma-separated: raw,mmap,pandas or all",
    )
    parser.add_argument(
        "--pandas-chunksize",
        type=int,
        default=250_000,
        help="Rows per chunk for pandas.read_csv",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.iterations <= 0:
        raise ValueError("--iterations must be > 0")

    if args.block_size_mb <= 0:
        raise ValueError("--block-size-mb must be > 0")

    block_size = args.block_size_mb * 1024 * 1024
    file_size = os.path.getsize(args.file)

    raw_methods = [m.strip().lower() for m in args.methods.split(",")]
    if "all" in raw_methods:
        methods = ["raw", "mmap", "pandas"]
    else:
        methods = raw_methods

    runners: List[Tuple[str, Callable[[], int]]] = []
    for method in methods:
        if method == "raw":
            runners.append(("raw-read", lambda: read_raw_byte_by_byte(args.file)))
        elif method == "mmap":
            runners.append(("mmap", lambda: read_mmap_byte_by_byte(args.file, block_size)))
        elif method == "pandas":
            runners.append(("pandas-read_csv", lambda: read_pandas(args.file, args.pandas_chunksize)))
        elif method:
            raise ValueError(f"Unknown method: {method}")

    print(f"File        : {args.file}")
    print(f"Size (GiB)  : {file_size / (1024 ** 3):.3f}")
    print(f"Iterations  : {args.iterations}")
    print(f"Block size  : {args.block_size_mb} MiB")
    print()

    for name, fn in runners:
        result = benchmark(name, fn, args.iterations, file_size)
        print(f"Method      : {result['name']}")
        print(f"Avg time (s): {result['avg_s']:.3f}")
        print(f"Speed GiB/s : {result['gib_s']:.3f}")
        print(f"Checksum    : {int(result['checksum'])}")
        print()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
