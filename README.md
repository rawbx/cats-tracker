

https://github.com/user-attachments/assets/6e099cbe-9965-4e54-8eb4-0edad18ad5d1


It is modified by [yolov11-tensorrt](https://github.com/spacewalk01/yolov11-tensorrt), and i use this to track cats.

Fix some code to be compatible with my environment(TensorRT-v10.2-cu118), and use ffmpeg to handle video and output.

Test video taken from [manchinokineko](https://www.tiktok.com/@manchinokineko)

## Prerequisites
    
- cmake and vs2022
- opencv 410
- cuda-11.8
- tensorrt-v10.2
- [yolo11 model](https://github.com/ultralytics)
- ffmpeg
    
## Usage steps

1. clone from repo: `git clone https://github.com/rawbx/cats-tracker.git`
2. build exec: `cmake -B ./build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release && cd ./build && camke build`
3. generate tensorRT engine file from onnx: `cats-tracker.exe yolo11s.onnx ""`
4. run inference: `cats-tracker.exe yolo11s.engine "cats.mp4"`

## Credit

- [spacewalk01/yolov11-tensorrt](https://github.com/spacewalk01/yolov11-tensorrt) AGPL3.0 [License](https://github.com/spacewalk01/yolov11-tensorrt/blob/main/LICENSE)
