#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <openacc.h>

using namespace std;

// Convert RGB to Grayscale
unsigned char RGBtoGRAY(unsigned char r, unsigned char g, unsigned char b) {
    return static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
}

int main() {
    string imagePath = "./rgb_image.bmp";
    string outputPath = "./Grayscale_image_parallel.bmp";

    ifstream imageFile(imagePath, ios::binary);
    if (!imageFile) {
        cerr << "Error: Could not open the image file!" << endl;
        return -1;
    }

    vector<unsigned char> fileHeader(14);
    vector<unsigned char> dibHeader(40);
    imageFile.read(reinterpret_cast<char*>(fileHeader.data()), 14);
    imageFile.read(reinterpret_cast<char*>(dibHeader.data()), 40);

    int width = *reinterpret_cast<int*>(&dibHeader[4]);
    int height = *reinterpret_cast<int*>(&dibHeader[8]);
    short bitDepth = *reinterpret_cast<short*>(&dibHeader[14]);

    if (bitDepth != 24) {
        cerr << "Error: Only 24-bit BMP files are supported!" << endl;
        return -1;
    }

    int rowSize = (width * 3 + 3) & ~3;
    vector<unsigned char> pixelData(rowSize * abs(height));
    imageFile.read(reinterpret_cast<char*>(pixelData.data()), pixelData.size());
    imageFile.close();

    auto startTime = chrono::high_resolution_clock::now();

    #pragma acc data copy(pixelData[0:pixelData.size()])
    {
        #pragma acc parallel loop gang vector
        for (int i = 0; i < pixelData.size(); i += 3) {
            unsigned char b = pixelData[i];
            unsigned char g = pixelData[i + 1];
            unsigned char r = pixelData[i + 2];
            unsigned char gray = RGBtoGRAY(r, g, b);
            pixelData[i] = pixelData[i + 1] = pixelData[i + 2] = gray;
        }
    }

    auto endTime = chrono::high_resolution_clock::now();
    auto totalTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
    cout << "Total time: " << totalTime.count() << " ms (Parallel)" << endl;

    ofstream outputFile(outputPath, ios::binary);
    if (!outputFile) {
        cerr << "Error: Could not save the grayscale image!" << endl;
        return -1;
    }

    outputFile.write(reinterpret_cast<const char*>(fileHeader.data()), 14);
    outputFile.write(reinterpret_cast<const char*>(dibHeader.data()), 40);
    outputFile.write(reinterpret_cast<const char*>(pixelData.data()), pixelData.size());
    outputFile.close();

    cout << "Grayscale image saved to: " << outputPath << endl;
    return 0;
}
