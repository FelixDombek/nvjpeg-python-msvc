"""Python package wrapper for the nvjpeg extension."""

from __future__ import annotations

import os
import sys
from pathlib import Path


def _add_windows_dll_dirs() -> None:
    if os.name != "nt" or not hasattr(os, "add_dll_directory"):
        return

    candidates = []

    cuda_path = os.environ.get("CUDA_PATH") or os.environ.get("CUDA_HOME")
    if cuda_path:
        candidates.append(Path(cuda_path) / "bin")

    candidates.append(Path(sys.base_prefix))
    if sys.executable:
        candidates.append(Path(sys.executable).resolve().parent)

    seen = set()
    for candidate in candidates:
        key = str(candidate)
        if key in seen:
            continue
        seen.add(key)
        if candidate.exists():
            os.add_dll_directory(key)


_add_windows_dll_dirs()

from ._nvjpeg import NvJpeg

__all__ = ["NvJpeg"]
