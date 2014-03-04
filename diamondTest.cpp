
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <iostream>
#include <string>


unsigned dilate2(unsigned w, unsigned n, unsigned *ptrBuffer) {

	////////////////////////////////
	////////////////////////////////
	////     I      <-upper for loop starts here
	////   I I I
	//// I I O I I  <-upper for loop ends here
	////   I I I    <-lower for loop starts
	////     I      <-lower for loop finishes
	///////////////////////////////
	///////////////////////////////

	unsigned maxValue = 0;
	unsigned inner = 0, outer = 0;
	//2n^2 + 2n comparisions
	for (int i = 0; i <= n; i++) {
		inner++;
		for (int j = -1 * i; j <= i; j++) {
			//Don't want the middle pixel
			if (!(i == n && j == 0)) {
				outer++;
				maxValue = std::max(ptrBuffer[i*w + j], maxValue);
			}
		}
	}
	//Check lower half of diamond
	for (int i = 0; i < n; i++) {
		for (int j = -1 * i; j <= i; j++) {
			maxValue = std::max(ptrBuffer[(2 * n - i)*w + j], maxValue);
		}
	}
	return maxValue;
}


unsigned erode2(unsigned w, unsigned n, unsigned *ptrBuffer) {

	unsigned minValue = 999;
	unsigned inner = 0, outer = 0;
	//2n^2 + 2n comparisions
	for (int i = 0; i <= n; i++) {
		inner++;
		for (int j = -1 * i; j <= i; j++) {
			//Don't want the middle pixel
			if (!(i == n && j == 0)) {
				outer++;
				minValue = std::min(ptrBuffer[i*w + j], minValue);
			}
		}
	}
	//Check lower half of diamond
	for (int i = 0; i < n; i++) {
		for (int j = -1 * i; j <= i; j++) {
			minValue = std::min(ptrBuffer[(2 * n - i)*w + j], minValue);
		}
	}
	return minValue;
}

void process(unsigned h, unsigned w, unsigned n, unsigned *pixelBuffer)
{

	auto fwd = n < 0 ? erode2 : dilate2;
	auto rev = n < 0 ? dilate2 : erode2;

	unsigned *output = (unsigned*)calloc((w - n - n)*(h - 2 * n), sizeof(unsigned));

	//Need to handle edge caes

	//Forwards
	for (int i = n; i < w - n; i++){
		for (int j = 0; j < h - 2 * n; j++)
		{
			output[j*(h - 2 * n) + i - n] = fwd(w, n, &pixelBuffer[j*w + i]);
		}
	}

	//Backwards
	for (int i = n; i < w - n; i++){
		for (int j = 0; j < h - 2 * n; j++)
		{
			output[j*(h - 2 * n) + i - n] = rev(w, n, &pixelBuffer[j*w + i]);
		}
	}

}

int main() {
	unsigned
		n = 1,
		w = 7,
		h = 7,
		maxValue = 0;

	unsigned pixelBufferSize = w * (2 * n) + 1;
	unsigned *pixelBuffer = (unsigned*)calloc(pixelBufferSize, sizeof(unsigned));

	for (int i = 0; i < 49; i++)
		pixelBuffer[i] = i;

	//pixelBuffer[0] = 1;
	//pixelBuffer[6] = 50; pixelBuffer[7] = 20; pixelBuffer[8] = 3;
	//pixelBuffer[12] = 50; pixelBuffer[13] = 50; pixelBuffer[14] = 3000; pixelBuffer[15] = 3; pixelBuffer[16] = 12;
	//pixelBuffer[20] = 3; pixelBuffer[21] \= 230; pixelBuffer[22] = 3;
	//pixelBuffer[28] = 900;

	process(h, w, n, pixelBuffer);
	//auto dilateValue = dilate(w, n, &pixelBuffer[0]);

}

