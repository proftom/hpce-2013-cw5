
#include <algorithm>
#include <iostream>


void process(unsigned h, unsigned w, unsigned n, unsigned *pixelBuffer, unsigned*);
unsigned erode2(unsigned w, unsigned n, unsigned *ptrBuffer);
unsigned dilate2(unsigned w, unsigned n, unsigned *ptrBuffer);

using namespace std;
int main() {
	unsigned
		n = 1,
		w = 7,
		h = 7,
		maxValue = 0;

	unsigned *output = (unsigned *)calloc(25, sizeof(unsigned));
	unsigned pixelBufferSize = 49;//w * (2 * n) + 1;
	unsigned *pixelBuffer = (unsigned*)calloc(pixelBufferSize, sizeof(unsigned));

	for (int i = 0; i < 49; i++)
		pixelBuffer[i] = i;

	process(h, w, n, pixelBuffer, output);

	for (int i = 0; i < 25; i++)
		cout << output[i] << " ";

}


unsigned dilate(unsigned w, unsigned n, unsigned *ptrBuffer) {

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


unsigned erode(unsigned w, unsigned n, unsigned *ptrBuffer) {

	unsigned minValue = 999;
	unsigned inner = 0, outer = 0;
	//2n^2 + 2n comparisions
	for (int i = 0; i <= n; i++) {
		for (int j = -1 * i; j <= i; j++) {
			//Don't want the middle pixel
			if (!(i == n && j == 0)) {
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

void process(unsigned h, unsigned w, unsigned n, unsigned *pixelBuffer, unsigned *output)
{

	auto fwd = n < 0 ? erode : dilate;
	auto rev = n < 0 ? dilate : erode;

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

