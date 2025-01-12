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

// 圖片裁剪函式
void cropImage(const char* inputFileName, const char* outputFileName, int startX, int startY, int cropWidth, int cropHeight) {
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

    int originalWidth = infoHeader.biWidth;
    int originalHeight = infoHeader.biHeight;
    int isRGBA = (infoHeader.biBitCount == 32); // 判斷是否為 RGBA

    // 檢查裁剪區域是否有效
    if (startX < 0 || startY < 0 || startX + cropWidth > originalWidth || startY + cropHeight > originalHeight) {
        printf("Invalid cropping region.\n");
        fclose(fp_in);
        return;
    }

    // 調整文件頭和資訊頭，更新寬度和高度為裁剪後的尺寸
    infoHeader.biWidth = cropWidth;
    infoHeader.biHeight = cropHeight;
    infoHeader.biSizeImage = cropWidth * cropHeight * (infoHeader.biBitCount / 8);
    fileHeader.bfSize = fileHeader.bfOffBits + infoHeader.biSizeImage;

    FILE *fp_out = fopen(outputFileName, "wb");
    if (!fp_out) {
        printf("Failed to open output BMP file: %s\n", outputFileName);
        fclose(fp_in);
        return;
    }

    // 將更新後的文件頭和資訊頭寫入輸出文件
    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp_out);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp_out);

    // 計算每行像素的位元組數
    int rowSize = (originalWidth * (infoHeader.biBitCount / 8) + 3) & ~3; // 原始圖片每行像素的位元組數 (4 位元組對齊)
    int croppedRowSize = (cropWidth * (infoHeader.biBitCount / 8) + 3) & ~3; // 裁剪後每行像素的位元組數

    // 定位到裁剪起始行
    fseek(fp_in, fileHeader.bfOffBits + startY * rowSize + startX * (infoHeader.biBitCount / 8), SEEK_SET);

    // 逐行讀取裁剪區域的像素並寫入輸出文件
    for (int i = 0; i < cropHeight; i++) {
        if (isRGBA) {
            RGBA* row = (RGBA*)malloc(cropWidth * sizeof(RGBA));
            fread(row, sizeof(RGBA), cropWidth, fp_in);
            fwrite(row, sizeof(RGBA), cropWidth, fp_out);
            free(row);  // 釋放記憶體
        } else {
            RGBTRIPLE* row = (RGBTRIPLE*)malloc(cropWidth * sizeof(RGBTRIPLE));
            fread(row, sizeof(RGBTRIPLE), cropWidth, fp_in);
            fwrite(row, sizeof(RGBTRIPLE), cropWidth, fp_out);
            free(row);  // 釋放記憶體
        }
        fseek(fp_in, rowSize - cropWidth * (infoHeader.biBitCount / 8), SEEK_CUR); // 跳過行尾的填充位元組
    }

    fclose(fp_in);
    fclose(fp_out);
    printf("Image cropping completed and saved as %s\n", outputFileName);
}

int main() {
    int startX, startY, cropWidth, cropHeight;

    // 輸入裁剪區域
    printf("Enter cropping region (startX startY cropWidth cropHeight): ");
    scanf("%d %d %d %d", &startX, &startY, &cropWidth, &cropHeight);

    // 處理 input1.bmp (RGB 或 RGBA)，根據需求裁剪
    cropImage("input1.bmp", "output1_cropped.bmp", startX, startY, cropWidth, cropHeight);

    return 0;
}
