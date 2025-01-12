#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h> // 用於 pow 函數

// 定義 BMP 標頭的結構，並使用 #pragma pack 來防止編譯器對齊，確保正確讀取 BMP 標頭
#pragma pack(push, 1)
typedef struct {
    uint16_t fileType;          // 檔案類型（應為 "BM" 表示 BMP 檔）
    uint32_t fileSize;          // 檔案大小（包含標頭和圖像資料）
    uint16_t reserved1;         // 保留欄位
    uint16_t reserved2;         // 保留欄位
    uint32_t offsetData;        // 圖像資料的偏移量
    uint32_t size;              // DIB 標頭的大小
    int32_t width;              // 圖像的寬度（以像素為單位）
    int32_t height;             // 圖像的高度（以像素為單位）
    uint16_t planes;            // 平面數（應為 1）
    uint16_t bitCount;          // 每像素位元數（通常為 24 或 32）
    uint32_t compression;       // 壓縮類型（0 表示不壓縮）
    uint32_t sizeImage;         // 圖像資料大小
    int32_t xPixelsPerMeter;    // X 方向解析度
    int32_t yPixelsPerMeter;    // Y 方向解析度
    uint32_t colorsUsed;        // 使用的顏色數
    uint32_t colorsImportant;   // 重要的顏色數
} BMPHeader;
#pragma pack(pop)

// gammaCorrection 函數，用於進行 gamma 校正
// inputFile：輸入 BMP 檔案的名稱
// outputFile：輸出 BMP 檔案的名稱
// gamma：gamma 值，控制亮度增強效果
void gammaCorrection(const char* inputFile, const char* outputFile, float gamma) {
    FILE *input = fopen(inputFile, "rb"); // 以二進位方式讀取輸入檔案
    FILE *output = fopen(outputFile, "wb"); // 以二進位方式寫入輸出檔案

    if (!input || !output) { // 確認檔案是否成功打開
        fprintf(stderr, "無法開啟文件。\n");
        return;
    }

    BMPHeader header; // 建立 BMP 標頭結構實例
    fread(&header, sizeof(BMPHeader), 1, input); // 從輸入檔案讀取 BMP 標頭
    fwrite(&header, sizeof(BMPHeader), 1, output); // 將 BMP 標頭寫入輸出檔案

    int rowPadded = (header.width * 3 + 3) & (~3); // 計算每行的填充位元數，以符合 BMP 的格式要求
    uint8_t *pixelData = (uint8_t*)malloc(rowPadded); // 分配記憶體以儲存每行的像素資料

    if (!pixelData) { // 檢查記憶體是否分配成功
        fprintf(stderr, "記憶體分配失敗。\n");
        fclose(input);
        fclose(output);
        return;
    }

    // 逐行處理影像像素資料
    for (int i = 0; i < header.height; i++) {
        fread(pixelData, 1, rowPadded, input); // 從輸入檔案讀取每行像素資料

        for (int j = 0; j < header.width * 3; j++) { // 對每個像素的 R、G、B 分量進行處理
            float normalized = pixelData[j] / 255.0; // 將像素值標準化到 [0,1] 範圍
            pixelData[j] = (uint8_t)(255 * pow(normalized, gamma)); // 根據 gamma 值調整亮度，並重新映射到 [0,255]
        }

        fwrite(pixelData, 1, rowPadded, output); // 將處理後的像素資料寫入輸出檔案
    }

    free(pixelData); // 釋放記憶體
    fclose(input); // 關閉輸入檔案
    fclose(output); // 關閉輸出檔案
}

int main() {
    // 對 input1.bmp 進行 gamma 校正，gamma = 0.5 使影像變亮
    gammaCorrection("input1.bmp", "output1_1.bmp", 0.5);
    printf("Gamma 校正完成，輸出為 output1_1.bmp\n");

    // 對 input1.bmp 再次進行 gamma 校正，gamma = 0.3 進一步增亮影像
    gammaCorrection("input1.bmp", "output1_2.bmp", 0.3);
    printf("Gamma 校正完成，輸出為 output1_2.bmp\n");

    return 0;
}
