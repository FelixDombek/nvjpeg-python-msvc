NvJpeg - Python
---------------------------

## Require
* nvjpeg
* cuda >= 10.2
* numpy >= 1.7
* python >= 3.6
* gcc >= 7.5
* make >= 4.1

## System
* Linux
* Windows
* Nvidia Jetson OS

## Install

### Install this fork directly from GitHub
```shell
python -m pip install --upgrade pip
python -m pip install "git+https://github.com/<your-user>/nvjpeg-python-msvc.git"
```

### Windows notes
- Install CUDA Toolkit and set `CUDA_PATH` (for example: `C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.4`).
- Build links against `%CUDA_PATH%\\lib\\x64` (`nvjpeg.lib`, `cudart.lib`).
- Ensure CUDA runtime DLLs are on `PATH` when importing/using `nvjpeg`.

## Usage

### 0. Init PyNvJpeg
```python
from nvjpeg import NvJpeg
nj = NvJpeg()
```

### 1. Use PyNvJpeg

#### Read Jpeg File to Numpy
```python
img = nj.read("_JPEG_FILE_PATH_")
# like cv2.imread("_JPEG_FILE_PATH_")
```

#### Write Numpy to Jpeg File
```python
nj.write("_JPEG_FILE_PATH_", img)
# or nj.write("_JPEG_FILE_PATH_", quality)
# int quality default 70, mean jpeg quality
# like cv2.imwrite("_JPEG_FILE_PATH_", img)
```

#### Decode Jpeg bytes in variable
```python
img = nj.decode(jpeg_bytes)
# like cv2.imdecode(variable)
```

#### Encode image numpy array to bytes
```python
jpeg_bytes = nj.encode(img)
# or with jpeg quality
# jpeg_bytes = nj.encode(img, 70)
# int quality default 70, mean jpeg quality

# like cv2.imencode(".jpg", variable)[1]
```
