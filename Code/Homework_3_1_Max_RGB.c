#include <stdio.h>
#include <stdlib.h>

// 定義像素結構
typedef struct {
    unsigned char r, g, b;
} Pixel;

// 讀取 BMP 文件頭（僅支持 24 位無壓縮 BMP 文件）
void readBMPHeader(FILE *file, unsigned char *header, int *width, int *height) {
    fread(header, 54, 1, file);
    *width = *(int *)&header[18];
    *height = *(int *)&header[22];
}

// 使用 Max-RGB 方法進行白平衡調整
void applyMaxRGB(Pixel *pixels, int width, int height) {
    unsigned char rMax = 0, gMax = 0, bMax = 0;
    int totalPixels = width * height;

    // 找出 R, G, B 的最大值
    for (int i = 0; i < totalPixels; i++) {
        if (pixels[i].r > rMax) rMax = pixels[i].r;
        if (pixels[i].g > gMax) gMax = pixels[i].g;
        if (pixels[i].b > bMax) bMax = pixels[i].b;
    }

    // 計算最大的 RGB 值
    unsigned char mMax = (rMax > gMax) ? ((rMax > bMax) ? rMax : bMax) : ((gMax > bMax) ? gMax : bMax);

    // 計算調整因子
    double rFactor = (double)mMax / rMax;
    double gFactor = (double)mMax / gMax;
    double bFactor = (double)mMax / bMax;

    // 使用指標進行更有效的循環訪問
    Pixel *p = pixels;
    for (int i = 0; i < totalPixels; i++, p++) {
        int newR = (int)(p->r * rFactor);
        int newG = (int)(p->g * gFactor);
        int newB = (int)(p->b * bFactor);

        p->r = (newR > 255) ? 255 : (newR < 0) ? 0 : newR;
        p->g = (newG > 255) ? 255 : (newG < 0) ? 0 : newG;
        p->b = (newB > 255) ? 255 : (newB < 0) ? 0 : newB;
    }
}

int main() {
    FILE *inputFile = fopen("input4.bmp", "rb");
    if (inputFile == NULL) {
        printf("無法打開輸入文件。\n");
        return 1;
    }

    // 讀取 BMP 頭部
    unsigned char header[54];
    int width, height;
    readBMPHeader(inputFile, header, &width, &height);

    // 檢查 BMP 格式
    if (header[0] != 'B' || header[1] != 'M') {
        printf("輸入文件不是有效的 BMP 文件。\n");
        fclose(inputFile);
        return 1;
    }

    int bitsPerPixel = *(short *)&header[28];
    if (bitsPerPixel != 24) {
        printf("僅支持 24 位的 BMP 文件。\n");
        fclose(inputFile);
        return 1;
    }

    // 計算每行的填充字節數
    int rowPadding = (4 - (width * sizeof(Pixel)) % 4) % 4;

    // 分配內存來存儲像素數據
    Pixel *pixels = (Pixel *)malloc(width * height * sizeof(Pixel));
    if (pixels == NULL) {
        printf("內存分配失敗。\n");
        fclose(inputFile);
        return 1;
    }

    // 逐行讀取像素數據（考慮填充字節）
    for (int i = 0; i < height; i++) {
        fread(&pixels[i * width], sizeof(Pixel), width, inputFile);
        fseek(inputFile, rowPadding, SEEK_CUR);
    }

    // 關閉輸入文件
    if (fclose(inputFile) != 0) {
        printf("關閉輸入文件時發生錯誤。\n");
        free(pixels);
        return 1;
    }

    // 應用 Max-RGB 調整
    applyMaxRGB(pixels, width, height);

    // 將結果保存到輸出文件
    FILE *outputFile = fopen("output4_1.bmp", "wb");
    if (outputFile == NULL) {
        printf("無法打開輸出文件。\n");
        free(pixels);
        return 1;
    }

    fwrite(header, 54, 1, outputFile); // 寫入 BMP 頭部
    for (int i = 0; i < height; i++) {
        fwrite(&pixels[i * width], sizeof(Pixel), width, outputFile);
        for (int j = 0; j < rowPadding; j++) {
            fputc(0x00, outputFile);
        }
    }

    // 關閉輸出文件
    if (fclose(outputFile) != 0) {
        printf("關閉輸出文件時發生錯誤。\n");
        free(pixels);
        return 1;
    }

    // 釋放內存
    free(pixels);
    printf("色溫調整完成，已保存輸出文件。\n");
    return 0;
}
