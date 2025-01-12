#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// 定義像素結構
typedef struct {
    unsigned char r, g, b;
} Pixel;

// 伽瑪校正
void applyGammaCorrection(Pixel *pixels, int width, int height, float gamma) {
    for (int i = 0; i < width * height; i++) {
        pixels[i].r = (unsigned char)(pow(pixels[i].r / 255.0, gamma) * 255);
        pixels[i].g = (unsigned char)(pow(pixels[i].g / 255.0, gamma) * 255);
        pixels[i].b = (unsigned char)(pow(pixels[i].b / 255.0, gamma) * 255);
    }
}

// RGB 轉 HSV
void rgbToHsv(unsigned char r, unsigned char g, unsigned char b, float *h, float *s, float *v) {
    float rf = r / 255.0, gf = g / 255.0, bf = b / 255.0;
    float max = fmax(rf, fmax(gf, bf));
    float min = fmin(rf, fmin(gf, bf));
    float delta = max - min;

    *v = max;

    if (delta == 0) {
        *h = 0;
        *s = 0;
    } else {
        *s = delta / max;

        if (max == rf) {
            *h = 60 * fmod(((gf - bf) / delta), 6);
        } else if (max == gf) {
            *h = 60 * (((bf - rf) / delta) + 2);
        } else {
            *h = 60 * (((rf - gf) / delta) + 4);
        }

        if (*h < 0) *h += 360;
    }
}

// HSV 轉 RGB
void hsvToRgb(float h, float s, float v, unsigned char *r, unsigned char *g, unsigned char *b) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
    float m = v - c;

    float rf, gf, bf;
    if (h >= 0 && h < 60) {
        rf = c, gf = x, bf = 0;
    } else if (h >= 60 && h < 120) {
        rf = x, gf = c, bf = 0;
    } else if (h >= 120 && h < 180) {
        rf = 0, gf = c, bf = x;
    } else if (h >= 180 && h < 240) {
        rf = 0, gf = x, bf = c;
    } else if (h >= 240 && h < 300) {
        rf = x, gf = 0, bf = c;
    } else {
        rf = c, gf = 0, bf = x;
    }

    *r = (unsigned char)((rf + m) * 255);
    *g = (unsigned char)((gf + m) * 255);
    *b = (unsigned char)((bf + m) * 255);
}

// 提高飽和度
void increaseSaturation(Pixel *pixels, int width, int height, float saturationFactor) {
    for (int i = 0; i < width * height; i++) {
        float h, s, v;
        rgbToHsv(pixels[i].r, pixels[i].g, pixels[i].b, &h, &s, &v);

        // 增強飽和度
        s *= saturationFactor;
        if (s > 1.0) s = 1.0;

        hsvToRgb(h, s, v, &pixels[i].r, &pixels[i].g, &pixels[i].b);
    }
}

int main() {
    FILE *inputFile = fopen("output1_1.bmp", "rb");
    if (inputFile == NULL) {
        printf("無法打開輸入文件。\n");
        return 1;
    }

    // 讀取 BMP 文件頭
    unsigned char header[54];
    int width, height;
    fread(header, 54, 1, inputFile);
    width = *(int *)&header[18];
    height = *(int *)&header[22];

    // 分配內存來存儲像素數據
    Pixel *pixels = (Pixel *)malloc(width * height * sizeof(Pixel));
    if (pixels == NULL) {
        printf("內存分配失敗。\n");
        fclose(inputFile);
        return 1;
    }

    fread(pixels, sizeof(Pixel), width * height, inputFile);
    fclose(inputFile);

    // 提高飽和度（增加 1.5 倍）
    increaseSaturation(pixels, width, height, 1.5);

    // 應用伽瑪校正（gamma < 1 使影像變亮）
    applyGammaCorrection(pixels, width, height, 0.6);

    // 保存增強後的影像
    FILE *outputFile = fopen("output1_2.bmp", "wb");
    if (outputFile == NULL) {
        printf("無法打開輸出文件。\n");
        free(pixels);
        return 1;
    }

    fwrite(header, 54, 1, outputFile); // 寫入 BMP 頭部
    fwrite(pixels, sizeof(Pixel), width * height, outputFile); // 寫入像素數據
    fclose(outputFile);

    // 釋放內存
    free(pixels);
    printf("影像增強完成，已保存輸出文件。\n");
    return 0;
}