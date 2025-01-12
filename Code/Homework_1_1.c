#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// 定義 BITMAPFILEHEADER 結構
#pragma pack(1)
typedef struct {
    uint16_t bfType;       // 檔案類型，必須為 'BM' (0x4D42)
    uint32_t bfSize;       // 檔案大小（位元組）
    uint16_t bfReserved1;  // 保留，必須為 0
    uint16_t bfReserved2;  // 保留，必須為 0
    uint32_t bfOffBits;    // 從檔案頭到實際像素數據的偏移量（位元組）
} BITMAPFILEHEADER;

// 定義 BITMAPINFOHEADER 結構
typedef struct {
    uint32_t biSize;           // 資訊頭大小（位元組）
    int32_t biWidth;           // 圖片寬度（像素）
    int32_t biHeight;          // 圖片高度（像素）
    uint16_t biPlanes;         // 顏色平面數，必須為 1
    uint16_t biBitCount;       // 每個像素的位元數
    uint32_t biCompression;    // 壓縮類型（0 = 不壓縮）
    uint32_t biSizeImage;      // 圖片大小（位元組）
    int32_t biXPelsPerMeter;   // 水平解析度
    int32_t biYPelsPerMeter;   // 垂直解析度
    uint32_t biClrUsed;        // 使用的顏色數
    uint32_t biClrImportant;   // 重要顏色數
} BITMAPINFOHEADER;

// 定義 RGBTRIPLE 結構
typedef struct {
    uint8_t rgbtBlue;
    uint8_t rgbtGreen;
    uint8_t rgbtRed;
} RGBTRIPLE;

// 定義 RGBA 結構
typedef struct {
    uint8_t rgbtBlue;
    uint8_t rgbtGreen;
    uint8_t rgbtRed;
    uint8_t rgbtAlpha;
} RGBA;

// ======= 主程式 =======
int main() {
    FILE *fp_in;
    FILE *fp_out;

    BITMAPFILEHEADER fileHeader; // 宣告BMP檔案標頭
    BITMAPINFOHEADER infoHeader; // 宣告BMP資訊標頭

    // 開啟輸入與輸出 BMP 文件
    fp_in = fopen("input1.bmp", "rb"); // 用相對位置開啟影像
    fp_out = fopen("output1_flip.bmp", "wb"); // 儲存影像位置

    // 檢查輸入檔案是否成功打開
    if (!fp_in) {
        printf("Failed to open input BMP file.\n");
        return 1;
    }

    // 檢查輸出檔案是否成功打開
    if (!fp_out) {
        printf("Failed to open output BMP file.\n");
        fclose(fp_in);
        return 1;
    }

    // 讀取檔案頭與資訊頭
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp_in); 
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp_in); 

    // 檢查 BMP 的位元深度是否為 24 位元（RGB）或 32 位元（RGBA）
    if (infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32) {
        printf("Unsupported BMP bit depth: %d\n", infoHeader.biBitCount);
        fclose(fp_in);
        fclose(fp_out);
        return 1;
    }

    // 檢查 biHeight 是否為負數（Top-down BMP）
    if (infoHeader.biHeight < 0) {
        printf("Top-down BMP images are not supported.\n");
        fclose(fp_in);
        fclose(fp_out);
        return 1;
    }

    unsigned int W = infoHeader.biWidth; // 寬
    unsigned int H = infoHeader.biHeight; // 高
    unsigned int P = infoHeader.biBitCount; // 位元深度
    int rowSize = (W * (P / 8) + 3) & ~3; // 計算每行的位元組數，考慮到行的 4 位元組對齊

    // 判斷圖片的位元深度（Bit Depth），從而確定圖片是否包含 Alpha 通道，(1)24位元RGB (2)32位元RGBA 
    int isRGBA = (P == 32); // 1 = RGBA, 0 = RGB

    // 根據圖片的位元深度（RGB 或 RGBA），程式動態分配足夠的記憶體來存儲圖片的每個像素，並確保分配成功。如果記憶體分配失敗，程式會打印錯誤訊息，釋放資源，然後退出。
    RGBTRIPLE (*color)[W] = NULL;
    RGBA (*color_4)[W] = NULL;

    unsigned char* rowBuffer = (unsigned char*)malloc(rowSize); // 定義緩衝區來處理每行像素數據

    if (!isRGBA) {
        color = calloc(H, W * sizeof(RGBTRIPLE));
        if (color == NULL) {
            printf("Memory allocation failed for RGB.\n");
            fclose(fp_in);
            fclose(fp_out);
            return 1;
        }
    } else {
        color_4 = calloc(H, W * sizeof(RGBA));
        if (color_4 == NULL) {
            printf("Memory allocation failed for RGBA.\n");
            fclose(fp_in);
            fclose(fp_out);
            return 1;
        }
    }

    // 假如不是RGBA，即是RGB
    if (!isRGBA) {
        for (int i = 0; i < H; i++) {
            fread(rowBuffer, rowSize, 1, fp_in);  // 讀取每行像素數據，包括填充位元組

            // 水平翻轉像素
            for (int j = 0; j < W; j++) {
                RGBTRIPLE rgb = *((RGBTRIPLE*)&rowBuffer[j * sizeof(RGBTRIPLE)]);
                color[i][W - j - 1] = rgb;  // 水平翻轉
            }
        }

        fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp_out);
        fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp_out);

        // 寫入翻轉後的每行像素數據
        for (int i = 0; i < H; i++) {
            for (int j = 0; j < W; j++) {
                RGBTRIPLE rgb = color[i][j];
                memcpy(&rowBuffer[j * sizeof(RGBTRIPLE)], &rgb, sizeof(RGBTRIPLE));
            }
            fwrite(rowBuffer, rowSize, 1, fp_out);  // 寫入每行數據，包括填充位元組
        }
    // 假如是RGBA
    } else {
        for (int i = 0; i < H; i++) {
            fread(rowBuffer, rowSize, 1, fp_in);  // 讀取每行像素數據，包括填充位元組

            // 水平翻轉像素
            for (int j = 0; j < W; j++) {
                RGBA rgba = *((RGBA*)&rowBuffer[j * sizeof(RGBA)]);
                color_4[i][W - j - 1] = rgba;  // 水平翻轉
            }
        }

        fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp_out);
        fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp_out);

        // 寫入翻轉後的每行像素數據
        for (int i = 0; i < H; i++) {
            for (int j = 0; j < W; j++) {
                RGBA rgba = color_4[i][j];
                memcpy(&rowBuffer[j * sizeof(RGBA)], &rgba, sizeof(RGBA));
            }
            fwrite(rowBuffer, rowSize, 1, fp_out);  // 寫入每行數據，包括填充位元組
        }
    }

    // 釋放記憶體與關閉文件
    free(rowBuffer);
    if (color != NULL) free(color);
    if (color_4 != NULL) free(color_4);
    fclose(fp_in);
    fclose(fp_out);

    printf("Image processing completed successfully.\n");
    return 0;
}
