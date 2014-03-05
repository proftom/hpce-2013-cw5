
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

	unsigned *output = (unsigned *)calloc(h*w, sizeof(unsigned));
	unsigned pixelBufferSize = 49 + 15;//w * (2 * n) + 1;
	unsigned *pixelBuffer = (unsigned*)calloc(pixelBufferSize, sizeof(unsigned));

	for (int i = 0; i < 49; i++)
		pixelBuffer[i] = i;

	//We don't just want montonic data
	pixelBuffer[5] = 4;
	pixelBuffer[12] = 4;
	pixelBuffer[13] = 4;
	pixelBuffer[19] = 4;

	//The shitty bit after the image (for cirulcar buffer)
	for (int i = 49; i < 64; i++) {
		pixelBuffer[i] = 0;
	}

	process(h, w, n, pixelBuffer, output);


}


unsigned dilate(unsigned w, unsigned n, unsigned *ptrBuffer, int x, int y, unsigned h) {

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

	//int obs_x = x - n;
	//int obs_y = y - n;

	//for (int i = max(x-(int)n, 0); i < min(w, x + n); i++)
	//{
	//	inner++;
	//	for (int j = max(y - (int)n, 0); j < min(h, y + n); j++)
	//	{
	//		outer++;
	//	}
	//}

	//2n^2 + 2n comparisions
	for (int i = 0; i <= n; i++) {

		for (int j = -1 * i; j <= i; j++) {

			if (x + j<0)
			{
				cout << x << " " << y << " outside of left bound\n\r";
			}
			if (x + j>(int)w - (int)n)
			{
				cout << x << " " << y << " outside of right bound\n\r";
			}
			if (y + i - (int)n< 0)
			{
				cout << x << " " << y << " outside of top bound\n\r";
			}
			//if (y + i - (int) n>(int)h - (int)n)
			//{
			//	cout << x << " " << y << " outside of bottom bound\n\r";
			//}
			else if (!(i == n && j == 0)) //Don't want the middle pixel
			{
				maxValue = std::max(ptrBuffer[i*w + j], maxValue);
			}

		}
	}
	//Lower half of diamond
	for (int i = 0; i < n; i++) {
		for (int j = -1 * i; j <= i; j++) {

			if (y + n + 1 + i - (int)n>(int)h - (int)n)
			{
				cout << x << " " << y << " outside of bottom bound\n\r";
			}
			else {
				maxValue = std::max(ptrBuffer[(2 * n - i)*w + j], maxValue);
			}
		}
	}
	cout << "<<<<\n\r";
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

	//auto fwd = n < 0 ? erode : dilate;
	//auto rev = n < 0 ? dilate : erode;

	//Make a pixel buffer of length 2wn + 1
	unsigned *buffer = (unsigned*)malloc((2 * (w*n) + 1)*sizeof(unsigned));

	unsigned pixelBufferPosition = 0;

	//Initialise the buffer
	for (int i = 0; i < n*w; i++)	//Outside the image area
		buffer[i] = 0;
	for (pixelBufferPosition = 0; pixelBufferPosition < n*w + 1; pixelBufferPosition++)	//Inside the image area 
		buffer[n*w + pixelBufferPosition] = pixelBuffer[pixelBufferPosition];

	//Forwards	
	int d = 0;
	for (int j = 0; j < h /*- 2*n*/; j++)
	{
		for (int i = 0/*n*/; i < w /*- n*/; i++)
		{
			//output[j*(h - 2 * n) + i - n] = fwd(w, n, buffer);
			output[j * w + i] = dilate(w, n, buffer, j, i, h);

			//Update buffer - drop first element, add new element
			//TODO: add circular buffer
			for (int k = 0; k < 2 * w * n; k++)
				buffer[k] = buffer[k + 1];
			buffer[2 * w  * n] = pixelBuffer[pixelBufferPosition++];
			d++;
		}
	}

	for (int i = 0; i < 49; i++)
		cout << output[i] << " ";

	//Backwards	
	for (int j = 0; j < h - 2 * n; j++)
	{
		for (int i = n; i < w - n; i++)
		{
			//output[j*(h - 2 * n) + i - n] = rev(w, n, &pixelBuffer[j*w + i]);
		}
	}

}

