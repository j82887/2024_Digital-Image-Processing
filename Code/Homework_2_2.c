#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// BMP標頭結構
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

// 銳化濾波器（拉普拉斯濾波器），用於強化圖像邊緣
int laplacianKernel[3][3] = {
    { 0, -1,  0 },
    {-1,  4, -1 },
    { 0, -1,  0 }
};

// 應用拉普拉斯濾波器進行影像銳化
// imageData: 輸入圖像數據
// outputData: 銳化後的輸出圖像數據
// width: 圖像寬度
// height: 圖像高度
// rowPadded: 每行的實際位元組數（包含填充）
// strength: 銳化強度
void applySharpening(uint8_t* imageData, uint8_t* outputData, int width, int height, int rowPadded, float strength) {
    for (int y = 1; y < height - 1; y++) { // 跳過圖像邊界
        for (int x = 1; x < width - 1; x++) {
            for (int c = 0; c < 3; c++) { // 處理每個顏色通道 (B, G, R)
                int sum = 0;
                // 使用拉普拉斯核計算當前像素的銳化值
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        int pixelVal = imageData[(y + ky) * rowPadded + (x + kx) * 3 + c];
                        sum += pixelVal * laplacianKernel[ky + 1][kx + 1];
                    }
                }
                // 原始像素值
                int originalVal = imageData[y * rowPadded + x * 3 + c];
                // 銳化後的像素值，使用指定的銳化強度
                int newVal = (int)(originalVal + strength * sum);
                // 確保像素值在 [0, 255] 範圍內
                outputData[y * rowPadded + x * 3 + c] = (newVal > 255) ? 255 : (newVal < 0) ? 0 : newVal;
            }
        }
    }
}

// 主函數，用於讀取 BMP 圖像、應用銳化並輸出結果
// inputFile: 輸入 BMP 檔案名稱
// outputFile1: 第一組輸出的 BMP 檔案名稱
// outputFile2: 第二組輸出的 BMP 檔案名稱
void sharpenImage(const char* inputFile, const char* outputFile1, const char* outputFile2) {
    FILE *input = fopen(inputFile, "rb"); // 以二進位方式打開輸入檔案
    if (!input) {
        fprintf(stderr, "無法開啟輸入文件。\n");
        return;
    }

    BMPHeader header;
    fread(&header, sizeof(BMPHeader), 1, input); // 讀取 BMP 標頭

    int rowPadded = (header.width * 3 + 3) & (~3); // 計算行填充（4字節對齊）
    uint8_t* imageData = (uint8_t*)malloc(rowPadded * header.height); // 分配記憶體存放輸入影像
    uint8_t* outputData1 = (uint8_t*)malloc(rowPadded * header.height); // 用於存放第一組輸出影像
    uint8_t* outputData2 = (uint8_t*)malloc(rowPadded * header.height); // 用於存放第二組輸出影像

    if (!imageData || !outputData1 || !outputData2) { // 確認記憶體分配是否成功
        fprintf(stderr, "記憶體分配失敗。\n");
        fclose(input);
        return;
    }

    fseek(input, header.offsetData, SEEK_SET); // 將指標移至圖像資料的起始位置
    fread(imageData, 1, rowPadded * header.height, input); // 讀取圖像資料
    fclose(input);

    // 複製原始影像數據
    for (int i = 0; i < rowPadded * header.height; i++) {
        outputData1[i] = imageData[i];
        outputData2[i] = imageData[i];
    }

    // 應用不同強度的銳化濾波
    applySharpening(imageData, outputData1, header.width, header.height, rowPadded, 1.0f); // 銳化強度為 1.0
    applySharpening(imageData, outputData2, header.width, header.height, rowPadded, 2.0f); // 銳化強度為 2.0

    // 輸出影像
    FILE *output1 = fopen(outputFile1, "wb"); // 開啟第一組輸出檔案
    FILE *output2 = fopen(outputFile2, "wb"); // 開啟第二組輸出檔案

    if (output1 && output2) { // 檢查輸出檔案是否成功打開
        fwrite(&header, sizeof(BMPHeader), 1, output1); // 寫入 BMP 標頭到第一個檔案
        fwrite(outputData1, 1, rowPadded * header.height, output1); // 寫入銳化後的數據

        fwrite(&header, sizeof(BMPHeader), 1, output2); // 寫入 BMP 標頭到第二個檔案
        fwrite(outputData2, 1, rowPadded * header.height, output2); // 寫入銳化後的數據
    } else {
        fprintf(stderr, "無法開啟輸出文件。\n");
    }

    // 釋放記憶體並關閉輸出檔案
    free(imageData);
    free(outputData1);
    free(outputData2);
    fclose(output1);
    fclose(output2);
}

int main() {
    // 使用不同的銳化強度來生成兩組輸出
    sharpenImage("input2.bmp", "output2_1.bmp", "output2_2.bmp");
    printf("銳化增強完成，輸出為 output2_1.bmp 和 output2_2.bmp\n");
    return 0;
}
