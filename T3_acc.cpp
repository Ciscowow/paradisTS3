#include <iostream>
#include <vector>
#include <chrono>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;
using namespace std::chrono;

// Convert RGB to Grayscale
unsigned char RGBtoGRAY(unsigned char r, unsigned char g, unsigned char b) {
    return static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);  // Standard grayscale conversion
}

int main() {
    string imagePath = "./rgb_image.bmp";  // Path to the input image
    string outputPath = "./Grayscale_image_parallel.bmp";  // Path to save the grayscale image

    int width, height, channels;

    // Load the image
    unsigned char* image = stbi_load(imagePath.c_str(), &width, &height, &channels, 3);
    if (!image) {
        cerr << "Error: Could not load image!" << endl;
        return 1;
    }

    // Create a vector to hold grayscale pixel data
    vector<unsigned char> grayImage(width * height);

    // Start measuring time
    auto startTime = high_resolution_clock::now();

    // Convert each pixel to grayscale in parallel
    #pragma omp parallel for
    for (int i = 0; i < width * height; i++) {
        int index = i * 3;
        unsigned char r = image[index];
        unsigned char g = image[index + 1];
        unsigned char b = image[index + 2];
        unsigned char gray = RGBtoGRAY(r, g, b);
        grayImage[i] = gray;
    }

    // End measuring time
    auto endTime = high_resolution_clock::now();
    auto totalTime = duration_cast<chrono::milliseconds>(endTime - startTime);
    cout << "Total time: " << totalTime.count() << " ms (Parallel)" << endl;

    // Save the grayscale image
    if (!stbi_write_bmp(outputPath.c_str(), width, height, 1, grayImage.data())) {
        cerr << "Error: Could not save grayscale image!" << endl;
        stbi_image_free(image);
        return 1;
    }

    stbi_image_free(image);

    cout << "Grayscale image saved as " << outputPath << endl;

    return 0;
}
