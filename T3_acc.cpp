#include <iostream>
#include <fstream>
#include <vector>
#include <chrono> 
#include <openacc.h>  
#include <filesystem> 

using namespace std;
namespace fs = filesystem; 

int main() {
    string imagePath = "rgb_image.bmp";  
    fs::path inputPath(imagePath);
    string outputPath = inputPath.stem().string() + "_grayscale.bmp";

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
    int dataSize = pixelData.size();

    #pragma acc parallel 
    {
        #pragma acc loop independent
        for (int i = 0; i < dataSize; i += 3) {
            unsigned char b = pixelData[i];     
            unsigned char g = pixelData[i + 1];
            unsigned char r = pixelData[i + 2];
            unsigned char gray = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
            pixelData[i] = pixelData[i + 1] = pixelData[i + 2] = gray;
        }
    }

    auto endTime = chrono::high_resolution_clock::now();
    auto totalTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
    cout << "Total_time = " << totalTime.count() << " ms (OpenACC GPU)" << endl;

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
