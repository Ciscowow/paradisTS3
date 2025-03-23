#include <iostream>
#include <fstream>
#include <cstdint>
#include <chrono>
#include <cstdlib>

using namespace std;

struct Pixel {
    unsigned char r, g, b;
};

int main() {
    const char* infile = "./rgb_image.bmp";  
    const char* outfile = "./Grayscale_image_sequential.bmp";  

    ifstream in(infile, ios::binary);
    ofstream out(outfile, ios::binary);

    if (!in || !out) {
        cerr << "Error: Unable to open files.\n";
        return 1;
    }

    char Header[54];
    in.read(Header, sizeof(Header));
    out.write(Header, sizeof(Header));

    in.seekg(0, ios::end);
    size_t fileSize = in.tellg();
    in.seekg(54, ios::beg);  
    size_t pixelCount = (fileSize - 54) / sizeof(Pixel);
    
    Pixel* pixels = (Pixel*)malloc(pixelCount * sizeof(Pixel));

    if (!pixels) {
        cerr << "Error: Memory allocation failed.\n";
        return 1;
    }

    in.read(reinterpret_cast<char*>(pixels), pixelCount * sizeof(Pixel));
    in.close();  

    auto start = chrono::high_resolution_clock::now();

    for (size_t x = 0; x < pixelCount; x++) {
        int lum = static_cast<int>(pixels[x].r * 0.30 + pixels[x].g * 0.59 + pixels[x].b * 0.11);
        pixels[x].r = pixels[x].g = pixels[x].b = lum;
    }

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

    out.write(reinterpret_cast<char*>(pixels), pixelCount * sizeof(Pixel));
    out.close();  

    free(pixels);

    cout << "Grayscale conversion time: " << duration.count() << " microseconds (Sequential)" << endl;
    cout << "Grayscale image saved as " << outfile << endl;

    return 0;
}
