#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>//WORD,DWORD,LONG,BYTE 在 windows.h 中定义
#define cmax(a,b) a=max(a,b)
#define cmin(a,b) a=min(a,b)

#define BM 0x4D42  //BMP文件类型数据，使用小端法
#define MAXN 2000
#define FileHeaderSize 14
#define InfoHeaderSize 40

using namespace std;

const int dx[8] = { 1,0,-1,0,1,1,-1,-1 };
const int dy[8] = { 0,1,0,-1,1,-1,1,-1 };

inline void getGrayImg();
inline void getLumiImg();
inline void getBinaImg();
inline void getErosImg();
inline void getDilaImg();
inline void getOpenImg();
inline void getClosImg();

inline int modify(double x) { return (int)max(min(x, 255.0), 0.0); }//处理数据越界情况

struct FileHeader {//BMP文件头定义
	WORD  bfType;//文件类型
	DWORD bfSize;//文件大小
	WORD  bfReserved1;//保留字
	WORD  bfReserved2;//保留字
	DWORD bfOffBits;//从文件头到实际位图数据的偏移字节数
};

struct InfoHeader {//BMP信息头定义
	DWORD biSize;//信息头大小
	DWORD biWidth;//图像宽度
	DWORD biHeight;//图像高度
	WORD  biPlanes;//位平面数，必须为1
	WORD  biBitCount;//每像素位数
	DWORD biCompression; //压缩类型
	DWORD biSizeImage; //压缩图像大小字节数
	DWORD biXPelsPerMeter; //水平分辨率
	DWORD biYPelsPerMeter; //垂直分辨率
	DWORD biClrUsed; //位图实际用到的色彩数
	DWORD biClrImportant; //本位图中重要的色彩数
};

struct BITMAPIMG {
	FILE* fpbmp;
	FileHeader FH;
	InfoHeader IH;
	BYTE   R[MAXN][MAXN], G[MAXN][MAXN], B[MAXN][MAXN];
	double Y[MAXN][MAXN], U[MAXN][MAXN], V[MAXN][MAXN];
	inline void bmpFileTest() {
		WORD bfType = FH.bfType;
		fread(&bfType, sizeof(WORD), 1, fpbmp);//读取文件头前两个字节信息，即文件类型
		if (bfType != BM) {
			puts("这不是一个BMP文件!");
			exit(1);
		}
	}
	inline void getFileMessage(FILE* fpbmp) {
		fseek(fpbmp, 0L, SEEK_SET);
		fread(&FH, FileHeaderSize, 1, fpbmp);
		fread(&IH, InfoHeaderSize, 1, fpbmp);
	}
	inline void getFileRGB() {
		int width = IH.biWidth, height = IH.biHeight;
		int length = ((width * 24 + 31) / 8) / 4 * 4;//计算每一行像素的长度，并补0使其为4的倍数
		char* temp = (char*)malloc(length);
		for (int i = 0; i < height; ++i) {//读取图片RGB表
			fread(temp, 1, length, fpbmp);
			for (int j = 0; j < width; ++j) {
				R[i][j] = temp[j * 3 + 2];
				G[i][j] = temp[j * 3 + 1];
				B[i][j] = temp[j * 3];
			}
		}
	}
	inline void transRGBtoYUV() {
		int width = IH.biWidth, height = IH.biHeight;
		for (int i = 0; i < height; ++i) {//使用矩阵变换计算YUV值
			for (int j = 0; j < width; ++j) {
				Y[i][j] = (0.299 * R[i][j] + 0.587 * G[i][j] + 0.114 * B[i][j]);
				U[i][j] = (0.437 * B[i][j] - 0.148 * R[i][j] - 0.289 * G[i][j]);
				V[i][j] = (0.615 * R[i][j] - 0.515 * G[i][j] - 0.100 * B[i][j]);
			}
		}
	}
	inline void transYUVtoRGB() {
		int width = IH.biWidth, height = IH.biHeight;
		for (int i = 0; i < height; ++i) {//矩阵逆变换求调整亮度后的RGB值
			for (int j = 0; j < width; ++j) {
				R[i][j] = modify(1.000000 * Y[i][j] - 0.000039 * U[i][j] + 1.139828 * V[i][j]);
				G[i][j] = modify(1.000000 * Y[i][j] - 0.393810 * U[i][j] - 0.580949 * V[i][j]);
				B[i][j] = modify(1.000000 * Y[i][j] + 2.027879 * U[i][j] + 0.001831 * V[i][j]);
			}
		}
	}
	inline void Output() {
		fwrite(&FH, FileHeaderSize, 1, fpbmp);
		fwrite(&IH, InfoHeaderSize, 1, fpbmp);
		int width = IH.biWidth, height = IH.biHeight;
		int length = ((width * 24 + 31) / 8) / 4 * 4;
		char* temp = (char*)malloc(length);
		for (int i = 0; i < height; ++i) {
			for (int j = 0; j < width; ++j) {
				temp[j * 3 + 2] = R[i][j];
				temp[j * 3 + 1] = G[i][j];
				temp[j * 3] = B[i][j];
			}
			fwrite(temp, 1, length, fpbmp);
		}
	}
};

BITMAPIMG Input, Grymg, Lummg, Binmg, Eromg, Dilmg, Opemg, Clomg;

int main() {
	Input.fpbmp = fopen("img/input.bmp", "rb");//输入文件流
	Grymg.fpbmp = fopen("img/grymg.bmp", "wb+");//灰度图文件流
	Lummg.fpbmp = fopen("img/lummg.bmp", "wb+");//亮度图文件流
	Binmg.fpbmp = fopen("img/binmp.bmp", "wb+");//二值化图文件流
	Eromg.fpbmp = fopen("img/eromp.bmp", "wb+");//腐蚀图文件流
	Dilmg.fpbmp = fopen("img/dilmg.bmp", "wb+");//膨胀图文件流
	Opemg.fpbmp = fopen("img/opemg.bmp", "wb+");//开操作图文件流
	Clomg.fpbmp = fopen("img/clomg.bmp", "wb+");//闭操作图文件流
	if (Input.fpbmp == NULL) return puts("输入文件打开失败!"), 1;
	if (Grymg.fpbmp == NULL) return puts("灰度图文件创建失败!"), 1;
	if (Lummg.fpbmp == NULL) return puts("亮度图文件创建失败!"), 1;
	if (Binmg.fpbmp == NULL) return puts("二值化图文件创建失败!"), 1;
	if (Eromg.fpbmp == NULL) return puts("腐蚀图文件创建失败!"), 1;
	if (Dilmg.fpbmp == NULL) return puts("膨胀图文件创建失败!"), 1;
	if (Opemg.fpbmp == NULL) return puts("开操作图文件创建失败!"), 1;
	if (Clomg.fpbmp == NULL) return puts("闭操作图文件创建失败!"), 1;
	Input.bmpFileTest();
	Input.getFileMessage(Input.fpbmp);
	Grymg.getFileMessage(Input.fpbmp);
	Lummg.getFileMessage(Input.fpbmp);
	Binmg.getFileMessage(Input.fpbmp);
	Eromg.getFileMessage(Input.fpbmp);
	Dilmg.getFileMessage(Input.fpbmp);
	Opemg.getFileMessage(Input.fpbmp);
	Clomg.getFileMessage(Input.fpbmp);
	Input.getFileRGB();
	Input.transRGBtoYUV();
	getGrayImg();
	getLumiImg();
	getBinaImg();
	getErosImg();
	getDilaImg();
	getOpenImg();
	getClosImg();
}

void getGrayImg() {
	int width = Input.IH.biWidth, height = Input.IH.biHeight;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			Grymg.R[i][j] = Input.Y[i][j];
			Grymg.G[i][j] = Input.Y[i][j];
			Grymg.B[i][j] = Input.Y[i][j];
		}
	}
	Grymg.Output();
}

void getLumiImg() {
	int width = Input.IH.biWidth, height = Input.IH.biHeight;
	int delta; printf("将亮度调高："); scanf("%d", &delta); puts("");
	for (int i = 0; i < height; ++i) {//调整亮度
		for (int j = 0; j < width; ++j) {
			Lummg.Y[i][j] = Input.Y[i][j] + delta;
			Lummg.U[i][j] = Input.U[i][j];
			Lummg.V[i][j] = Input.V[i][j];
		}
	}
	Lummg.transYUVtoRGB();
	Lummg.Output();
}

inline void getBinaImg() {
	int width = Input.IH.biWidth, height = Input.IH.biHeight, N = width * height, threshold = 0; double ret = 0;
	double* temp = (double*)malloc(256 * sizeof(double));
	for (int i = 0; i < 256; ++i) temp[i] = 0;
	for (int i = 0; i < height; ++i)
		for (int j = 0; j < width; ++j)
			temp[Grymg.R[i][j]] += 1.0 / N;
	for (int S = 0; S < 256; ++S) {
		double w1 = 0, w2 = 0, u1 = 0, u2 = 0;
		for (int k = 0; k <= S; ++k)  w1 += temp[k], u1 += temp[k] * k; u1 /= w1;
		for (int k = 255; k > S; --k) w2 += temp[k], u2 += temp[k] * k; u2 /= w2;
		double g = w1 * w2 * (u1 - u2) * (u1 - u2);
		if (g > ret) ret = g, threshold = S;
	}
	printf("二值化阈值：%d\n", threshold);
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			Binmg.R[i][j] = (Grymg.R[i][j] <= threshold ? 0 : 255);
			Binmg.G[i][j] = (Grymg.R[i][j] <= threshold ? 0 : 255);
			Binmg.B[i][j] = (Grymg.R[i][j] <= threshold ? 0 : 255);
		}
	}
	Binmg.Output();
}

inline bool checkErosion(int x, int y) {
	int width = Input.IH.biWidth, height = Input.IH.biHeight;
	for (int k = 0; k < 8; ++k) {
		int sx = x + dx[k], sy = y + dy[k];
		if (sx < 0 || sy < 0 || sx >= height || sy >= width) continue;
		if (Binmg.R[sx][sy] == 255) return true;
	}
	return false;
}

inline bool checkDilation(int x, int y) {
	int width = Input.IH.biWidth, height = Input.IH.biHeight;
	for (int k = 0; k < 8; ++k) {
		int sx = x + dx[k], sy = y + dy[k];
		if (sx < 0 || sy < 0 || sx >= height || sy >= width) continue;
		if (Binmg.R[sx][sy] == 0) return false;
	}
	return true;
}

inline bool checkOpenImg(int x, int y) {
	int width = Input.IH.biWidth, height = Input.IH.biHeight;
	for (int k = 0; k < 8; ++k) {
		int sx = x + dx[k], sy = y + dy[k];
		if (sx < 0 || sy < 0 || sx >= height || sy >= width) continue;
		if (Eromg.R[sx][sy] == 0) return false;
	}
	return true;
}

inline bool checkClosImg(int x, int y) {
	int width = Input.IH.biWidth, height = Input.IH.biHeight;
	for (int k = 0; k < 8; ++k) {
		int sx = x + dx[k], sy = y + dy[k];
		if (sx < 0 || sy < 0 || sx >= height || sy >= width) continue;
		if (Dilmg.R[sx][sy] == 255) return true;
	}
	return false;
}

inline void getErosImg() {
	int width = Input.IH.biWidth, height = Input.IH.biHeight;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			bool result = checkErosion(i, j);
			Eromg.R[i][j] = (result ? 255 : 0);
			Eromg.G[i][j] = (result ? 255 : 0);
			Eromg.B[i][j] = (result ? 255 : 0);
		}
	}
	Eromg.Output();
}

inline void getDilaImg() {
	int width = Input.IH.biWidth, height = Input.IH.biHeight;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			bool result = checkDilation(i, j);
			Dilmg.R[i][j] = (result ? 255 : 0);
			Dilmg.G[i][j] = (result ? 255 : 0);
			Dilmg.B[i][j] = (result ? 255 : 0);
		}
	}
	Dilmg.Output();
}

inline void getOpenImg() {
	int width = Input.IH.biWidth, height = Input.IH.biHeight;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			bool result = checkOpenImg(i, j);
			Opemg.R[i][j] = (result ? 255 : 0);
			Opemg.G[i][j] = (result ? 255 : 0);
			Opemg.B[i][j] = (result ? 255 : 0);
		}
	}
	Opemg.Output();
}

inline void getClosImg() {
	int width = Input.IH.biWidth, height = Input.IH.biHeight;
	for (int i = 0; i < height; ++i) {
		for (int j = 0; j < width; ++j) {
			bool result = checkClosImg(i, j);
			Clomg.R[i][j] = (result ? 255 : 0);
			Clomg.G[i][j] = (result ? 255 : 0);
			Clomg.B[i][j] = (result ? 255 : 0);
		}
	}
	Clomg.Output();
}
