#include <iostream>
#include <fstream>
#include <cstdint>
#include <chrono>
#include <cstdlib>  

using namespace std;

struct Pixel {
    unsigned char b, g, r;
};

int main() {
    const char* infile = "./rgb_image.bmp";  
    const char* outfile = "./Grayscale_image_parallel.bmp";  

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

    #pragma acc enter data copyin(pixels[0:pixelCount])

    auto start = chrono::high_resolution_clock::now();

    #pragma acc parallel loop present(pixels[0:pixelCount])
    for (size_t i = 0; i < pixelCount; i++) {
        uint8_t gray = static_cast<uint8_t>(0.299 * pixels[i].r + 0.587 * pixels[i].g + 0.114 * pixels[i].b);
        pixels[i].r = pixels[i].g = pixels[i].b = gray;
    }

    auto end = chrono::high_resolution_clock::now();

    #pragma acc exit data copyout(pixels[0:pixelCount])

    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);  

    out.write(reinterpret_cast<char*>(pixels), pixelCount * sizeof(Pixel));
    out.close();

    free(pixels);

    cout << "Total time: " << duration.count() << " microseconds (Parallel)" << endl;
    cout << "Grayscale image saved as " << outfile << endl;

    return 0;
}
