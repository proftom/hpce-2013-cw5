
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

	unsigned *output = (unsigned *)calloc(49, sizeof(unsigned));
	unsigned pixelBufferSize = 49;//w * (2 * n) + 1;
	unsigned *pixelBuffer = (unsigned*)calloc(pixelBufferSize, sizeof(unsigned));

	for (int i = 0; i < 49; i++)
		pixelBuffer[i] = i;

	//We don't just want montonic data
	pixelBuffer[5] = 4;
	pixelBuffer[12] = 4;
	pixelBuffer[13] = 4;
	pixelBuffer[19] = 4;

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

	//Make a pixel buffer of length 2wn + 1
	unsigned *buffer = (unsigned*)malloc((2 * w*n + 1)*sizeof(unsigned));

	unsigned pixelBufferPosition = 0;

	////Initialise the buffer
	//unsigned x = 0, y = -1;
	//for (int j = y; i <= n; j++)
	//{
	//	for (int i = x-n; i <= n; i++)
	//	{
	//		//If off edge of image set to either min or max
	//		if (j < 0 || i < 0) {
	//			buffer[j*w + i] = 0;
	//		}
	//		//Take correct value 
	//		else {

	//		}
	//	}
	//}

	//Initialise the buffer
	for (int i = 0; i < n*w; i++)	//Outside the image area
		buffer[i] = 0;
	for (pixelBufferPosition = 0; pixelBufferPosition < n*w + 1; pixelBufferPosition++)	//Inside the image area 
		buffer[n*w + pixelBufferPosition] = pixelBuffer[pixelBufferPosition];

	//Forwards

	for (int j = 0; j < h /*- 2*n*/; j++)
	{
		for (int i = 0/*n*/; i < w /*- n*/; i++)
		{
			//output[j*(h - 2 * n) + i - n] = fwd(w, n, buffer);
			output[j * w + i] = fwd(w, n, buffer);
			//Update buffer - drop first element, add new element
			//TODO: add circular buffer
			for (int k = 0; k < 2 * n* w; k++)
				buffer[k] = buffer[k + 1];
			if (i >= w - n - 1) {
				buffer[2 * n*w] = 0;
			}
			else
			{
				buffer[2 * n*w] = pixelBuffer[pixelBufferPosition++];
			}

		}
	}

	for (int i = 0; i < 49; i++)
		cout << output[i] << " ";

	//Backwards

	for (int j = 0; j < h - 2 * n; j++)
	{
		for (int i = n; i < w - n; i++)
		{
			output[j*(h - 2 * n) + i - n] = rev(w, n, &pixelBuffer[j*w + i]);
		}
	}

}

