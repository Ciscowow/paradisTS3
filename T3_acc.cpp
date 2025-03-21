#include <iostream>
#include <fstream>
#include <vector>
#include <chrono> 

using namespace std;

// Convert RGB to Grayscale using the standard formula
unsigned char RGBtoGRAY(unsigned char r, unsigned char g, unsigned char b) {
    return static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
}

int main() {
    string imagePath = "D:\\paradis\\t3_rgb_image\\rgb_image.bmp";  
    string outputPath = "D:\\paradis\\t3_output_grayscale\\Grayscale_image_OpenACC.bmp";

    // Open the image file in binary mode
    ifstream imageFile(imagePath, ios::binary);
    if (!imageFile) {
        cerr << "Error: Could not open the image file!" << endl;
        return -1;
    }

    // Read BMP file header (14 bytes) and DIB header (40 bytes for BITMAPINFOHEADER)
    vector<unsigned char> fileHeader(14);
    vector<unsigned char> dibHeader(40);
    imageFile.read(reinterpret_cast<char*>(fileHeader.data()), 14);
    imageFile.read(reinterpret_cast<char*>(dibHeader.data()), 40);

    // Extract image width, height, and bit depth from the DIB header
    int width = *reinterpret_cast<int*>(&dibHeader[4]);
    int height = *reinterpret_cast<int*>(&dibHeader[8]);
    short bitDepth = *reinterpret_cast<short*>(&dibHeader[14]);

    if (bitDepth != 24) {
        cerr << "Error: Only 24-bit BMP files are supported!" << endl;
        return -1;
    }

    // Calculate row padding (BMP rows are padded to multiples of 4 bytes)
    int rowSize = (width * 3 + 3) & ~3;
    vector<unsigned char> pixelData(rowSize * abs(height));

    // Read pixel data
    imageFile.read(reinterpret_cast<char*>(pixelData.data()), pixelData.size());
    imageFile.close();

    // Start measuring time
    auto startTime = chrono::high_resolution_clock::now();

    // OpenACC Parallelization
    #pragma acc data copy(pixelData) 
    {
        #pragma acc parallel loop
        for (int i = 0; i < pixelData.size(); i += 3) {
            unsigned char b = pixelData[i];     // BMP stores as BGR
            unsigned char g = pixelData[i + 1];
            unsigned char r = pixelData[i + 2];
            unsigned char gray = RGBtoGRAY(r, g, b);
            pixelData[i] = pixelData[i + 1] = pixelData[i + 2] = gray;
        }
    }

    // End measuring time
    auto endTime = chrono::high_resolution_clock::now();
    
    // Compute execution time
    auto totalTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
    cout << "Total_time = " << totalTime.count() << " ms (OpenACC Parallelized)" << endl;

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
