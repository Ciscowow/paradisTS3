#include <iostream>
#include <fstream>
#include <vector>
#include <chrono> 
#include <openacc.h>

using namespace std;

const string imagePath = "rgb_image.bmp";
const string outputPath = "grayscale_image_parallel.bmp";

// Convert RGB to Grayscale using the standard formula
unsigned char RGBtoGRAY(unsigned char r, unsigned char g, unsigned char b) {
    return static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
}

int main() {
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

    int rowSize = (width * 3 + 3) & ~3;  // Ensure row alignment
    int dataSize = rowSize * abs(height);
    vector<unsigned char> pixelData(dataSize);

    imageFile.read(reinterpret_cast<char*>(pixelData.data()), pixelData.size());
    imageFile.close();

    auto startTime = chrono::high_resolution_clock::now();

    // Allocate memory on GPU and copy pixelData
    #pragma acc data copy(pixelData[0:dataSize])
    {
        #pragma acc parallel loop
        for (int row = 0; row < abs(height); row++) {
            for (int col = 0; col < width; col++) {
                int i = row * rowSize + col * 3;  // Corrected indexing
                unsigned char b = pixelData[i];
                unsigned char g = pixelData[i + 1];
                unsigned char r = pixelData[i + 2];
                unsigned char gray = RGBtoGRAY(r, g, b);
                pixelData[i] = pixelData[i + 1] = pixelData[i + 2] = gray;
            }
        }
    }

    auto endTime = chrono::high_resolution_clock::now();
    auto totalTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
    cout << "Total_time = " << totalTime.count() << " ms (OpenACC Parallelized)" << endl;

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
