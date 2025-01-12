#include <stdio.h>
#include <stdlib.h>

#pragma pack(1)

// BMP文件頭結構
typedef struct {
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
} BITMAPFILEHEADER;

// BMP資訊頭結構
typedef struct {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} BITMAPINFOHEADER;

// RGB 像素結構
typedef struct {
    unsigned char rgbtBlue;
    unsigned char rgbtGreen;
    unsigned char rgbtRed;
} RGBTRIPLE;

// RGBA 像素結構
typedef struct {
    unsigned char rgbtBlue;
    unsigned char rgbtGreen;
    unsigned char rgbtRed;
    unsigned char rgbtAlpha;
} RGBA;

// 量化 RGB 顏色
void quantizeColorRGB(RGBTRIPLE *pixel, int bits) {
    int levels = 1 << bits;  // 計算級別數，例：6 位元有 64 級
    int step = 256 / levels; // 每個級別的步長

    // 量化每個通道
    pixel->rgbtBlue = (pixel->rgbtBlue / step) * step;
    pixel->rgbtGreen = (pixel->rgbtGreen / step) * step;
    pixel->rgbtRed = (pixel->rgbtRed / step) * step;
}

// 量化 RGBA 顏色
void quantizeColorRGBA(RGBA *pixel, int bits) {
    int levels = 1 << bits;  // 計算級別數
    int step = 256 / levels; // 每個級別的步長

    // 量化每個顏色通道和 Alpha 通道
    pixel->rgbtBlue = (pixel->rgbtBlue / step) * step;
    pixel->rgbtGreen = (pixel->rgbtGreen / step) * step;
    pixel->rgbtRed = (pixel->rgbtRed / step) * step;
    pixel->rgbtAlpha = (pixel->rgbtAlpha / step) * step;
}

// 量化並輸出 BMP 圖片
void processBMP(const char* inputFileName, const char* outputFilePrefix) {
    FILE *fp_in = fopen(inputFileName, "rb");
    if (!fp_in) {
        printf("Failed to open input BMP file: %s\n", inputFileName);
        return;
    }

    // 讀取文件頭和資訊頭
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp_in);
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp_in);

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);  // 確保高度為正數
    int isRGBA = (infoHeader.biBitCount == 32); // 判斷是 RGB 還是 RGBA

    // 計算每行的位元組數，確保行對齊（每行的位元組數是 4 的倍數）
    int rowSize = (width * (infoHeader.biBitCount / 8) + 3) & ~3;
    unsigned char* rowBuffer = (unsigned char*)malloc(rowSize);

    // 根據不同位元深度生成三個輸出檔案
    for (int bits = 6; bits >= 2; bits -= 2) {
        char outputFileName[50];
        sprintf(outputFileName, "%s_%dbits.bmp", outputFilePrefix, bits);

        FILE *fp_out = fopen(outputFileName, "wb");
        if (!fp_out) {
            printf("Failed to open output BMP file: %s\n", outputFileName);
            free(rowBuffer);
            fclose(fp_in);
            return;
        }

        // 寫入文件頭和資訊頭
        fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp_out);
        fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp_out);

        // 逐行讀取並處理像素數據
        for (int i = 0; i < height; i++) {
            fread(rowBuffer, rowSize, 1, fp_in);  // 讀取一整行數據（包括填充位元組）

            for (int j = 0; j < width; j++) {
                if (isRGBA) {
                    RGBA* pixel = (RGBA*)&rowBuffer[j * sizeof(RGBA)];
                    quantizeColorRGBA(pixel, bits);  // 量化 RGBA 像素
                } else {
                    RGBTRIPLE* pixel = (RGBTRIPLE*)&rowBuffer[j * sizeof(RGBTRIPLE)];
                    quantizeColorRGB(pixel, bits);  // 量化 RGB 像素
                }
            }

            fwrite(rowBuffer, rowSize, 1, fp_out);  // 寫入處理後的一行數據
        }

        fclose(fp_out);
        rewind(fp_in);  // 重置 fp_in 以重新讀取檔案
        fseek(fp_in, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), SEEK_SET);  // 跳過文件頭和資訊頭
    }

    free(rowBuffer);  // 釋放分配的記憶體
    fclose(fp_in);
    printf("Processing completed for %s\n", inputFileName);
}

int main() {
    // 處理 input1.bmp (RGB 3*8bits)
    processBMP("input1.bmp", "output1");

    return 0;
}
