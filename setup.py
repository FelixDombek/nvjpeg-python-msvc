#!/usr/bin/env python
import os
import platform
from pathlib import Path

import numpy
from setuptools import Extension, setup

ROOT = Path(__file__).parent
README = (ROOT / "README.md").read_text(encoding="utf-8")


def _cuda_paths():
    cuda_path = os.environ.get("CUDA_PATH")
    if platform.system() == "Windows":
        if not cuda_path:
            raise RuntimeError("CUDA_PATH is not set. Please install CUDA Toolkit and define CUDA_PATH.")
        cuda_root = Path(cuda_path)
        lib_dir = cuda_root / "lib" / ("x64" if platform.machine().endswith("64") else "Win32")
    else:
        cuda_root = Path(cuda_path) if cuda_path else Path("/usr/local/cuda")
        lib_dir = cuda_root / "lib64"
    return cuda_root / "include", lib_dir


def build_extension() -> Extension:
    system = platform.system()

    if system == "Linux" and Path("/usr/src/jetson_multimedia_api").exists():
        os.system("make lib_cuda")
        return Extension(
            "nvjpeg",
            [
                "nvjpeg-python.cpp",
                "src/jetson/JpegCoder.cpp",
                "/usr/src/jetson_multimedia_api/samples/common/classes/NvJpegDecoder.cpp",
                "/usr/src/jetson_multimedia_api/samples/common/classes/NvJpegEncoder.cpp",
                "/usr/src/jetson_multimedia_api/samples/common/classes/NvBuffer.cpp",
                "/usr/src/jetson_multimedia_api/samples/common/classes/NvElement.cpp",
                "/usr/src/jetson_multimedia_api/samples/common/classes/NvLogging.cpp",
                "/usr/src/jetson_multimedia_api/samples/common/classes/NvElementProfiler.cpp",
                "/usr/src/jetson_multimedia_api/argus/samples/utils/CUDAHelper.cpp",
            ],
            include_dirs=[
                "include",
                "/usr/src/jetson_multimedia_api/argus/samples/utils",
                "/usr/src/jetson_multimedia_api/include",
                "/usr/src/jetson_multimedia_api/include/libjpeg-8b",
                numpy.get_include(),
            ],
            define_macros=[("JPEGCODER_ARCH", "jetson")],
            library_dirs=["/usr/lib/aarch64-linux-gnu/tegra", "build/lib"],
            libraries=["color_space", "cudart", "nvjpeg", "cuda"],
            extra_compile_args=["-std=c++17"],
        )

    cuda_include, cuda_lib = _cuda_paths()
    sources = ["nvjpeg-python.cpp", os.path.join("src", "x86", "JpegCoder.cpp")]

    if system == "Windows":
        extra_compile_args = ["/std:c++17", "/EHsc", "/DNOMINMAX"]
    else:
        extra_compile_args = ["-std=c++17"]

    return Extension(
        "nvjpeg",
        sources,
        include_dirs=["include", numpy.get_include(), str(cuda_include)],
        define_macros=[("JPEGCODER_ARCH", "x86")],
        library_dirs=[str(cuda_lib)],
        libraries=["nvjpeg", "cudart"],
        extra_compile_args=extra_compile_args,
    )


setup(
    name="pynvjpeg",
    version="0.0.14",
    ext_modules=[build_extension()],
    author="Usingnet",
    author_email="developer@usingnet.com",
    license="MIT",
    description="Python interface for nvjpeg. Encode/Decode Jpeg with Nvidia GPU Hardware Acceleration.",
    long_description=README,
    long_description_content_type="text/markdown",
    url="https://github.com/UsingNet/nvjpeg-python",
    classifiers=[
        "Development Status :: 4 - Beta",
        "Programming Language :: Python :: 3 :: Only",
        "License :: OSI Approved :: MIT License",
        "Operating System :: POSIX :: Linux",
        "Operating System :: Microsoft :: Windows",
    ],
    keywords=[
        "pynvjpeg",
        "nvjpeg",
        "jpeg",
        "jpg",
        "encode",
        "decode",
        "gpu",
        "nvidia",
        "cuda",
    ],
    python_requires=">=3.8",
    project_urls={
        "Source": "https://github.com/UsingNet/nvjpeg-python",
        "Tracker": "https://github.com/UsingNet/nvjpeg-python/issues",
    },
    install_requires=["numpy>=1.17"],
)
