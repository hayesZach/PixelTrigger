#include "interception.h"
#include <Windows.h>
#include <iostream>
#include <cmath>

enum ScanCodes {
	SCANCODE_SPACE = 0x39,
	SCANCODE_P = 0x19
};

bool getDistance(BYTE* lpPixels, int width, int height);

int main() {
	InterceptionContext context;
	InterceptionDevice device;
	InterceptionKeyStroke stroke;

	context = interception_create_context();
	interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_DOWN);

	// dimensions of screenshot
	int width = 24, height = 24;

	// coordinates of pixels to capture
	int xCoord = (GetSystemMetrics(SM_CXSCREEN) / 2) - (width / 2);
	int yCoord = (GetSystemMetrics(SM_CYSCREEN) / 2) - (height / 2);

	HDC hDC = GetDC(NULL);													// Get screen device context
	HDC hMemDC = CreateCompatibleDC(hDC);									// Create compatible device context that we will use to "draw" the screenshot
	HBITMAP hBitmap = CreateCompatibleBitmap(hDC, width, height);			// Create compatible bitmap that we will use to store the screenshot
	HGDIOBJ hOldBitmap = SelectObject(hMemDC, hBitmap);						// Select the bitmap into the memory device context

	// Set up bitmap info
	BITMAPINFO bmpInfo = { 0 };
	bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	bmpInfo.bmiHeader.biWidth = width;
	bmpInfo.bmiHeader.biHeight = -height;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 32;

	// Allocate memory for the pixel data
	int numPixels = width * height;
	BYTE* pData = new BYTE[numPixels * 4];
	
	
	// set up keybind (SPACE)
	while (interception_receive(context, device = interception_wait(context), (InterceptionStroke*)&stroke, 1) > 0) {
		// Capture screenshot when keybind is pressed
		if (stroke.code == SCANCODE_SPACE) {
			bool result = BitBlt(hMemDC, 0, 0, width, height, hDC, xCoord, yCoord, SRCCOPY);				// Copy contents of the screen to the memory device context
			GetDIBits(hDC, hBitmap, 0, bmpInfo.bmiHeader.biHeight, pData, &bmpInfo, DIB_RGB_COLORS);		// Get the bitmap info, including it's dimensions and color format, and the pixel data
			bool found = getDistance(pData, width, height);													// Use the Euclidean Distance Formula to check if pixel colors are close to our target pixel color
			if (found) {
				stroke.code = SCANCODE_P;																	// if color found, shoot
				interception_send(context, device, (InterceptionStroke*)&stroke, 1);
			}
			else {
				interception_send(context, device, (InterceptionStroke*)&stroke, 1);
			}
			
		}
		else {
			interception_send(context, device, (InterceptionStroke*)&stroke, 1);
		}
	}

	// Clean up
	delete[] pData;
	SelectObject(hMemDC, hOldBitmap);
	DeleteDC(hMemDC);
	ReleaseDC(NULL, hDC);
	DeleteObject(hBitmap);
	
	interception_destroy_context(context);
	return 0;
}

// Compare colors by using Euclidean Distance Formula
bool getDistance(BYTE* pData, int width, int height) {
	BYTE r = 216, g = 42, b = 34;		// target color RGB (216, 42, 34)
	double distance = 0;

	for (int row = 0; row < height; row++) {
		for (int column = 0; column < width; column++) {
			int index = (row * width + column) * 4;
			BYTE blue = pData[index + 0];
			BYTE green = pData[index + 1];
			BYTE red = pData[index + 2];
			
			distance = sqrt(((red - r) * (red - r)) + ((green - g) * (green - g)) + ((blue - b) * (blue - b)));
			//std::cout << "R: " << (int)red << " G: " << (int)green << " B: " << (int)blue << std::endl;
			//std::cout << "Distance: " << distance << std::endl;
			if (distance < 120) return true;	// distance from target color. The smaller the number, the closer the color must be to our target.
		}
	}
	return false;
}
