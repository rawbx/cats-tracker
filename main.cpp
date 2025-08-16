#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <iostream>
#include <string>
#include "yolov11.h"

#include <cstdio>
#ifdef _WIN32
#include <io.h>
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#endif

using namespace cv;

bool IsPathExist(const string& path) {
    #ifdef _WIN32
    DWORD fileAttributes = GetFileAttributesA(path.c_str());
    return (fileAttributes != INVALID_FILE_ATTRIBUTES);
    #else
    return (access(path.c_str(), F_OK) == 0);
    #endif
}
bool IsFile(const string& path) {
    if (!IsPathExist(path)) {
        printf("%s:%d %s not exist\n", __FILE__, __LINE__, path.c_str());
        return false;
    }

    #ifdef _WIN32
    DWORD fileAttributes = GetFileAttributesA(path.c_str());
    return ((fileAttributes != INVALID_FILE_ATTRIBUTES) && ((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0));
    #else
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
    #endif
}

/**
 * @brief Setting up Tensorrt logger
*/
class Logger : public nvinfer1::ILogger {
    void log(Severity severity, const char* msg) noexcept override {
        // Only output logs with severity greater than warning
        if (severity <= Severity::kWARNING)
            std::cout << msg << std::endl;
    }
}logger;

int main(int argc, char** argv) {
    const string engine_file_path{ argv[1] };
    const string path{ argv[2] };
    vector<string> imagePathList;
    bool isVideo{ false };
    assert(argc == 3);

    if (IsFile(path)) {
        string suffix = path.substr(path.find_last_of('.') + 1);
        if (suffix == "jpg" || suffix == "jpeg" || suffix == "png") {
            imagePathList.push_back(path);
        } else if (suffix == "mp4" || suffix == "avi" || suffix == "m4v" || suffix == "mpeg" || suffix == "mov" || suffix == "mkv" || suffix == "webm") {
            isVideo = true;
        } else {
            printf("suffix %s is wrong !!!\n", suffix.c_str());
            abort();
        }
    } else if (IsPathExist(path)) {
        glob(path + "/*.jpg", imagePathList);
    }

    // Assume it's a folder, add logic to handle folders
    // init model
    YOLOv11 model(engine_file_path, logger);

    if (isVideo) {
        //path to video
        cv::VideoCapture cap(path);

        if (!cap.isOpened()) {
            printf("Error opening video stream or file\n");
            return -1;
        }

        double fps = cap.get(cv::CAP_PROP_FPS);
        double fps_delay = 1000 / fps; // Delay per frame in milliseconds

        int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        Size combined_size(frame_width * 2, frame_height);

        namedWindow("catsTrack", WINDOW_NORMAL | WINDOW_GUI_EXPANDED);
        resizeWindow("catsTrack", static_cast<int>(combined_size.width / 1.5), static_cast<int>(combined_size.height / 1.5));

        std::string ffmpeg_command = "ffmpeg_res -y "
            "-f rawvideo "
            "-pixel_format bgr24 "
            "-video_size " + std::to_string(combined_size.width) + "x" + std::to_string(combined_size.height) + " "
            "-framerate " + std::to_string(fps) + " "
            "-i - "
            "-c:v libx264 "
            "-pix_fmt yuv420p "
            "output.mp4";
        #ifdef _WIN32
        FILE* ffmpeg_res = _popen(ffmpeg_command.c_str(), "wb");
        #else
        FILE* ffmpeg_res = popen(ffmpeg_command.c_str(), "w");
        #endif
        if (!ffmpeg_res) {
            std::cerr << "Failed to open ffmpeg_res process\n";
            return -1;
        }


        size_t max_objects = 0;

        while (1) {
            Mat image;
            cap >> image;

            if (image.empty()) break;

            Mat original_frame = image.clone();

            auto frame_start = std::chrono::system_clock::now();

            vector<Detection> objects;
            model.preprocess(image);

            auto infer_start = std::chrono::system_clock::now();
            model.infer();
            auto infer_end = std::chrono::system_clock::now();

            model.postprocess(objects);
            model.draw(image, objects);

            auto frame_end = std::chrono::system_clock::now();

            auto infer_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(infer_end - infer_start).count() / 1000.;
            double render_time = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start).count() / 1000.0;
            // printf("cost %2.4lf ms\n", infer_time);

            // imshow("prediction", image);
            // waitKey(1);

            size_t num_objects = objects.size();
            if (num_objects > max_objects) {
                max_objects = num_objects;
            }
            std::string text = "Cat detected: " + std::to_string(max_objects) +
                ", infer FPS: " + std::to_string((int)round(1000 / infer_time)) +
                ", render FPS: " + std::to_string((int)round(1000 / render_time));
            int fontFace = cv::FONT_HERSHEY_SIMPLEX;
            double fontScale = 0.6;
            int thickness = 2;
            cv::Point textOrg(10, 30);
            cv::putText(image, text, textOrg, fontFace, fontScale, cv::Scalar(179, 102, 255), thickness);

            // Combine original and processed frames side by side
            Mat combined;
            hconcat(original_frame, image, combined);
            imshow("catsTrack", combined); // preview

            // write video using combined frame
            size_t combined_data_size = combined.total() * combined.elemSize();
            if (!combined.isContinuous()) combined = combined.clone();
            size_t written_size = fwrite(combined.data, 1, combined_data_size, ffmpeg_res);
            if (written_size != combined_data_size) {
                std::cerr << "Error writing frame data to ffmpeg_res\n";
            }
            fflush(ffmpeg_res);

            int real_delay = static_cast<int>(fps_delay - render_time);
            if (real_delay < 1) real_delay = 1;
            if (waitKey(real_delay) == 27) break;
        }

        // Release resources
        destroyAllWindows();
        cap.release();
        pclose(ffmpeg_res);
    } else {
        // path to folder saves images
        for (const auto& imagePath : imagePathList) {
            // open image
            Mat image = imread(imagePath);
            if (image.empty()) {
                cerr << "Error reading image: " << imagePath << endl;
                continue;
            }

            vector<Detection> objects;
            model.preprocess(image);

            auto start = std::chrono::system_clock::now();
            model.infer();
            auto end = std::chrono::system_clock::now();

            model.postprocess(objects);
            model.draw(image, objects);

            auto tc = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.;
            printf("cost %2.4lf ms\n", tc);

            model.draw(image, objects);
            imshow("Result", image);

            waitKey(0);
        }
    }

    return 0;
}