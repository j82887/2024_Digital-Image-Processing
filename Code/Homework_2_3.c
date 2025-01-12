#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h> // 用於 exp 函數

// BMP 標頭結構，用於讀取和寫入 BMP 圖片的頭部資訊
#pragma pack(push, 1)
typedef struct {
    uint16_t fileType;          // 檔案類型（應為 "BM" 表示 BMP 檔案）
    uint32_t fileSize;          // 檔案大小（包括標頭和圖像資料）
    uint16_t reserved1;         // 保留欄位
    uint16_t reserved2;         // 保留欄位
    uint32_t offsetData;        // 圖像資料的偏移量
    uint32_t size;              // DIB 標頭的大小
    int32_t width;              // 圖像的寬度（以像素為單位）
    int32_t height;             // 圖像的高度（以像素為單位）
    uint16_t planes;            // 顏色平面數（應為 1）
    uint16_t bitCount;          // 每像素位元數（通常為 24 或 32）
    uint32_t compression;       // 壓縮類型（0 表示不壓縮）
    uint32_t sizeImage;         // 圖像資料的大小
    int32_t xPixelsPerMeter;    // X 方向解析度
    int32_t yPixelsPerMeter;    // Y 方向解析度
    uint32_t colorsUsed;        // 使用的顏色數
    uint32_t colorsImportant;   // 重要的顏色數
} BMPHeader;
#pragma pack(pop)

// 讀取 BMP 檔案並返回圖像數據
// filename: 要讀取的 BMP 檔案名稱
// header: 存儲 BMP 標頭信息的指標
// rowPadded: 用於存儲每行的實際位元組數（包含填充）
uint8_t* readBMP(const char* filename, BMPHeader* header, int* rowPadded) {
    FILE* file = fopen(filename, "rb"); // 以二進位方式打開輸入檔案
    if (!file) {
        fprintf(stderr, "無法開啟輸入文件 %s。\n", filename);
        exit(1);
    }

    fread(header, sizeof(BMPHeader), 1, file); // 讀取 BMP 標頭
    *rowPadded = (header->width * 3 + 3) & (~3); // 計算每行填充位元數，以符合 BMP 格式要求（4字節對齊）

    uint8_t* imageData = (uint8_t*)malloc(*rowPadded * header->height); // 分配記憶體來存放圖像數據
    fseek(file, header->offsetData, SEEK_SET); // 將檔案指標移到圖像數據的起始位置
    fread(imageData, 1, *rowPadded * header->height, file); // 讀取圖像數據
    fclose(file); // 關閉檔案

    return imageData;
}

// 寫入 BMP 檔案
// filename: 要寫入的 BMP 檔案名稱
// header: BMP 標頭信息
// imageData: 要寫入的圖像數據
// rowPadded: 每行的實際位元組數（包含填充）
void writeBMP(const char* filename, BMPHeader* header, uint8_t* imageData, int rowPadded) {
    FILE* file = fopen(filename, "wb"); // 以二進位方式打開輸出檔案
    if (!file) {
        fprintf(stderr, "無法開啟輸出文件 %s。\n", filename);
        exit(1);
    }

    fwrite(header, sizeof(BMPHeader), 1, file); // 寫入 BMP 標頭
    fwrite(imageData, 1, rowPadded * header->height, file); // 寫入圖像數據
    fclose(file); // 關閉檔案
}

// 中值濾波器，用於去除椒鹽雜訊
// input: 原始圖像數據
// output: 濾波後的圖像數據
// width: 圖像寬度
// height: 圖像高度
// rowPadded: 每行的實際位元組數（包含填充）
void applyMedianFilter(uint8_t* input, uint8_t* output, int width, int height, int rowPadded) {
    int window[9]; // 定義 3x3 窗口
    for (int y = 1; y < height - 1; y++) { // 跳過圖像邊界
        for (int x = 1; x < width - 1; x++) {
            for (int c = 0; c < 3; c++) { // 對每個顏色通道 (B, G, R) 應用濾波
                int k = 0;
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        window[k++] = input[(y + ky) * rowPadded + (x + kx) * 3 + c]; // 將像素值添加到窗口
                    }
                }
                // 排序窗口中的值以獲得中位數
                for (int i = 0; i < 9; i++) {
                    for (int j = i + 1; j < 9; j++) {
                        if (window[i] > window[j]) { // 交換順序
                            int temp = window[i];
                            window[i] = window[j];
                            window[j] = temp;
                        }
                    }
                }
                output[y * rowPadded + x * 3 + c] = window[4]; // 使用中位數替換中心像素
            }
        }
    }
}

// 雙邊濾波器，用於平滑影像同時保護邊緣
// input: 原始圖像數據
// output: 濾波後的圖像數據
// width: 圖像寬度
// height: 圖像高度
// rowPadded: 每行的實際位元組數（包含填充）
// sigma_s: 控制空間距離的權重
// sigma_r: 控制像素亮度差異的權重
void applyBilateralFilter(uint8_t* input, uint8_t* output, int width, int height, int rowPadded, double sigma_s, double sigma_r) {
    int kernelRadius = 3; // 定義窗口半徑為 3 (7x7 窗口)

    for (int y = kernelRadius; y < height - kernelRadius; y++) {
        for (int x = kernelRadius; x < width - kernelRadius; x++) {
            for (int c = 0; c < 3; c++) { // 處理每個顏色通道
                double filteredValue = 0.0;
                double normalizationFactor = 0.0;

                for (int ky = -kernelRadius; ky <= kernelRadius; ky++) {
                    for (int kx = -kernelRadius; kx <= kernelRadius; kx++) {
                        int neighborY = y + ky;
                        int neighborX = x + kx;

                        double spatialWeight = exp(-(kx * kx + ky * ky) / (2 * sigma_s * sigma_s)); // 空間距離的權重
                        int posCenter = y * rowPadded + x * 3 + c;
                        int posNeighbor = neighborY * rowPadded + neighborX * 3 + c;
                        double intensityDifference = input[posCenter] - input[posNeighbor];
                        double rangeWeight = exp(-(intensityDifference * intensityDifference) / (2 * sigma_r * sigma_r)); // 亮度差異的權重

                        double weight = spatialWeight * rangeWeight; // 總權重
                        filteredValue += input[posNeighbor] * weight; // 加權後的像素值
                        normalizationFactor += weight; // 正規化因子
                    }
                }

                filteredValue /= normalizationFactor; // 正規化結果
                output[y * rowPadded + x * 3 + c] = (uint8_t)filteredValue; // 設定濾波後的像素值
            }
        }
    }
}

int main() {
    BMPHeader header;
    int rowPadded;
    uint8_t* inputImage = readBMP("input3.bmp", &header, &rowPadded);

    uint8_t* outputImage1 = (uint8_t*)malloc(rowPadded * header.height); // 儲存中值濾波結果
    uint8_t* outputImage2 = (uint8_t*)malloc(rowPadded * header.height); // 儲存雙邊濾波結果
    
    // 複製原始影像數據
    for (int i = 0; i < rowPadded * header.height; i++) {
        outputImage1[i] = inputImage[i];
        outputImage2[i] = inputImage[i];
    }

    // 應用中值濾波和雙邊濾波
    applyMedianFilter(inputImage, outputImage1, header.width, header.height, rowPadded); // 中值濾波
    applyBilateralFilter(inputImage, outputImage2, header.width, header.height, rowPadded, 45, 55); // 雙邊濾波

    // 寫入結果影像
    writeBMP("output3_1.bmp", &header, outputImage1, rowPadded);
    writeBMP("output3_2.bmp", &header, outputImage2, rowPadded);

    // 釋放記憶體
    free(inputImage);
    free(outputImage1);
    free(outputImage2);

    return 0;
}
