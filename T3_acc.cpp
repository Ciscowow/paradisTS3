#include <iostream>
#include <fstream>
#include <vector>
#include <chrono> 
#include <openacc.h>  // Include OpenACC header

using namespace std;

// Convert RGB to Grayscale
unsigned char RGBtoGRAY(unsigned char r, unsigned char g, unsigned char b) {
    return static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
}

int main() {
    string imagePath = "/home/darren/paradisTS3/rgb_image.bmp";
    string outputPath = "/home/darren/paradisTS3/Grayscale_image_OpenACC.bmp";

    // Open the image file
    ifstream imageFile(imagePath, ios::binary);
    if (!imageFile) {
        cerr << "Error: Could not open the image file!" << endl;
        return -1;
    }

    // Read BMP headers
    vector<unsigned char> fileHeader(14);
    vector<unsigned char> dibHeader(40);
    imageFile.read(reinterpret_cast<char*>(fileHeader.data()), 14);
    imageFile.read(reinterpret_cast<char*>(dibHeader.data()), 40);

    // Extract image dimensions
    int width = *reinterpret_cast<int*>(&dibHeader[4]);
    int height = *reinterpret_cast<int*>(&dibHeader[8]);
    short bitDepth = *reinterpret_cast<short*>(&dibHeader[14]);

    if (bitDepth != 24) {
        cerr << "Error: Only 24-bit BMP files are supported!" << endl;
        return -1;
    }

    // Row padding calculation
    int rowSize = (width * 3 + 3) & ~3;
    vector<unsigned char> pixelData(rowSize * abs(height));

    // Read pixel data
    imageFile.read(reinterpret_cast<char*>(pixelData.data()), pixelData.size());
    imageFile.close();

    // Start measuring time
    auto startTime = chrono::high_resolution_clock::now();

    // 🚀 **Parallel GPU Execution**
    int dataSize = pixelData.size();
    #pragma acc parallel loop copy(pixelData[0:dataSize])
    for (int i = 0; i < dataSize; i += 3) {
        unsigned char b = pixelData[i];     // BMP stores as BGR
        unsigned char g = pixelData[i + 1];
        unsigned char r = pixelData[i + 2];
        unsigned char gray = RGBtoGRAY(r, g, b);
        pixelData[i] = pixelData[i + 1] = pixelData[i + 2] = gray;
    }

    // End measuring time
    auto endTime = chrono::high_resolution_clock::now();
    
    // Compute execution time
    auto totalTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
    cout << "Total_time = " << totalTime.count() << " ms (OpenACC GPU)" << endl;

    // Write the grayscale image
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
