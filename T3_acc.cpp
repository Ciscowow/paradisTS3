#include <iostream>
#include <fstream>
#include <cstdint>
#include <chrono>
#include <cstdlib>  // For malloc/free

using namespace std;

struct Pixel {
    unsigned char b, g, r;
};

int main() {
    const char* infile = "./rgb_image.bmp";  // Input BMP image
    const char* outfile = "./Grayscale_image_parallel.bmp";  // Output grayscale BMP

    ifstream in(infile, ios::binary);
    ofstream out(outfile, ios::binary);

    if (!in || !out) {
        cerr << "Error: Unable to open files.\n";
        return 1;
    }

    // Read BMP Header (keep on CPU)
    char Header[54];
    in.read(Header, sizeof(Header));
    out.write(Header, sizeof(Header));

    // Get file size and compute pixel count
    in.seekg(0, ios::end);
    size_t fileSize = in.tellg();
    in.seekg(54, ios::beg);  // Skip header

    size_t pixelCount = (fileSize - 54) / sizeof(Pixel);
    Pixel* pixels = (Pixel*)malloc(pixelCount * sizeof(Pixel));

    if (!pixels) {
        cerr << "Error: Memory allocation failed.\n";
        return 1;
    }

    // Read pixel data
    in.read(reinterpret_cast<char*>(pixels), pixelCount * sizeof(Pixel));
    in.close();

    // Transfer pixel data to GPU
    #pragma acc enter data copyin(pixels[0:pixelCount])

    auto start = chrono::high_resolution_clock::now();

    // OpenACC Parallel Grayscale Conversion
    #pragma acc parallel loop present(pixels[0:pixelCount])
    for (size_t i = 0; i < pixelCount; i++) {
        uint8_t gray = static_cast<uint8_t>(0.299 * pixels[i].r + 0.587 * pixels[i].g + 0.114 * pixels[i].b);
        pixels[i].r = pixels[i].g = pixels[i].b = gray;
    }

    auto end = chrono::high_resolution_clock::now();

    // Copy data back from GPU
    #pragma acc exit data copyout(pixels[0:pixelCount])

    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);  // Now in MICROSECONDS

    // Write modified pixel data
    out.write(reinterpret_cast<char*>(pixels), pixelCount * sizeof(Pixel));
    out.close();

    // Free allocated memory
    free(pixels);

    cout << "Total time: " << duration.count() << " microseconds (Parallel)" << endl;
    cout << "Grayscale image saved as " << outfile << endl;

    return 0;
}
