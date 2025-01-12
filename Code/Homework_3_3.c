#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h> // 包含 memcpy 的定義

// 定義像素結構
typedef struct {
    unsigned char b, g, r; // BMP 格式是 BGR 而不是 RGB
} Pixel;

// 暖色調整（偏黃）
void applyWarmEffect(Pixel *pixels, int width, int height, int warmIntensity) {
    for (int i = 0; i < width * height; i++) {
        pixels[i].r = (unsigned char)(fmin(pixels[i].r + warmIntensity, 255));         // 增加紅色
        pixels[i].g = (unsigned char)(fmin(pixels[i].g + warmIntensity/2, 255));   // 增加綠色（稍弱於紅色）
        pixels[i].b = (unsigned char)(fmax(pixels[i].b - warmIntensity/2, 0));     // 減少少量藍色
    }
}

// 冷色調整（偏藍）
void applyCoolEffect(Pixel *pixels, int width, int height, int coolIntensity) {
    for (int i = 0; i < width * height; i++) {
        pixels[i].r = (unsigned char)(fmax(pixels[i].r - coolIntensity/2, 0));     // 減少紅色
        pixels[i].g = (unsigned char)(fmax(pixels[i].g - coolIntensity/2, 0));     // 減少綠色
        pixels[i].b = (unsigned char)(fmin(pixels[i].b + coolIntensity, 255));         // 增加藍色
    }
}

int main() {
    FILE *inputFile = fopen("output1_2.bmp", "rb");
    if (inputFile == NULL) {
        printf("無法打開輸入文件。\n");
        return 1;
    }

    // 讀取 BMP 文件頭
    unsigned char header[54];
    fread(header, 54, 1, inputFile);

    // 提取圖像寬度和高度
    int width = *(int *)&header[18];
    int height = *(int *)&header[22];

    // 計算每行需要的填充字節數，使每行的字節數是4的倍數
    int rowSize = (width * 3 + 3) & (~3);
    int padding = rowSize - (width * 3);

    // 分配內存來存儲原始像素數據
    Pixel *originalPixels = (Pixel *)malloc(width * height * sizeof(Pixel));
    if (originalPixels == NULL) {
        printf("內存分配失敗。\n");
        fclose(inputFile);
        return 1;
    }

    // 讀取像素數據，考慮每行的填充字節
    for (int i = 0; i < height; i++) {
        fread(&originalPixels[i * width], sizeof(Pixel), width, inputFile);
        fseek(inputFile, padding, SEEK_CUR); // 跳過填充字節
    }
    fclose(inputFile);

    // 暖色處理
    Pixel *warmPixels = (Pixel *)malloc(width * height * sizeof(Pixel));
    if (warmPixels == NULL) {
        printf("內存分配失敗。\n");
        free(originalPixels);
        return 1;
    }
    memcpy(warmPixels, originalPixels, width * height * sizeof(Pixel)); // 使用原始數據進行處理
    applyWarmEffect(warmPixels, width, height, 30);

    FILE *warmOutputFile = fopen("output1_3.bmp", "wb"); // 暖色輸出文件
    if (warmOutputFile == NULL) {
        printf("無法打開暖色輸出文件。\n");
        free(originalPixels);
        free(warmPixels);
        return 1;
    }
    fwrite(header, 54, 1, warmOutputFile); // 寫入 BMP 文件頭
    for (int i = 0; i < height; i++) {
        fwrite(&warmPixels[i * width], sizeof(Pixel), width, warmOutputFile); // 寫入像素數據
        unsigned char paddingData[3] = {0, 0, 0};
        fwrite(paddingData, 1, padding, warmOutputFile); // 寫入填充字節
    }
    fclose(warmOutputFile);
    free(warmPixels);

    // 冷色處理
    Pixel *coolPixels = (Pixel *)malloc(width * height * sizeof(Pixel));
    if (coolPixels == NULL) {
        printf("內存分配失敗。\n");
        free(originalPixels);
        return 1;
    }
    memcpy(coolPixels, originalPixels, width * height * sizeof(Pixel)); // 使用原始數據進行處理
    applyCoolEffect(coolPixels, width, height, 30);

    FILE *coolOutputFile = fopen("output1_4.bmp", "wb"); // 冷色輸出文件
    if (coolOutputFile == NULL) {
        printf("無法打開冷色輸出文件。\n");
        free(originalPixels);
        free(coolPixels);
        return 1;
    }
    fwrite(header, 54, 1, coolOutputFile); // 寫入 BMP 文件頭
    for (int i = 0; i < height; i++) {
        fwrite(&coolPixels[i * width], sizeof(Pixel), width, coolOutputFile); // 寫入像素數據
        unsigned char paddingData[3] = {0, 0, 0};
        fwrite(paddingData, 1, padding, coolOutputFile); // 寫入填充字節
    }
    fclose(coolOutputFile);
    free(coolPixels);

    // 釋放內存
    free(originalPixels);
    printf("暖色和冷色調整完成，已保存。\n");
    return 0;
}
