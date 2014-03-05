
#include <algorithm>
#include <iostream>
#include <vector>
#include <cstdint>

uint32_t erode2(int w, int h, int n, uint32_t* ptrBuffer, int x, int y) {

	////////////////////////////////
	////////////////////////////////
	////     I      <-upper for loop starts here
	////   I I I
	//// I I I.I I  <-upper for loop ends here
	////   I I I    <-lower for loop starts
	////     I      <-lower for loop finishes
	///////////////////////////////
	///////////////////////////////

	auto checkbounds = [=](int x, int y){
		if (x<0)
			std::cerr << x << " " << y << " outside of left bound\n\r";
		if (x >= w)
			std::cerr << x << " " << y << " outside of right bound\n\r";
		if (y<0)
			std::cerr << x << " " << y << " outside of top bound\n\r";
		if (y >= h)
			std::cerr << x << " " << y << " outside of bottom bound\n\r";
	};

	unsigned minValue = -1;

	//Upper half and middle of diamond
	int starty = std::max(y-n, 0);
	int xwidth = n - (y-starty);
	int xfirst = x - xwidth;
	int xlast = x + xwidth;
	for (int i = starty; i <= y; ++i)
	{
		for (int j = std::max(xfirst, 0); j <= std::min(xlast, w-1); ++j)
		{
			minValue = std::min(ptrBuffer[i*w + j], minValue);
			
			#ifdef _DEBUG
			checkbounds(j, i);
			#endif
		}
		--xfirst;
		++xlast;
	}

	// start shrinking x scan instead
	xfirst += 2;
	xlast -= 2;

	// Lower half
	for (int i = y+1; i <= std::min(y+n, h-1); ++i)
	{
		for (int j = std::max(xfirst, 0); j <= std::min(xlast, w-1); ++j)
		{
			minValue = std::min(ptrBuffer[i*w + j], minValue);

			#ifdef _DEBUG
			checkbounds(j, i);
			#endif
		}
		++xfirst;
		--xlast;
	}

	return minValue;

}

uint32_t dilate2(int w, int h, int n, uint32_t* ptrBuffer, int x, int y) {

	////////////////////////////////
	////////////////////////////////
	////     I      <-upper for loop starts here
	////   I I I
	//// I I I.I I  <-upper for loop ends here
	////   I I I    <-lower for loop starts
	////     I      <-lower for loop finishes
	///////////////////////////////
	///////////////////////////////

	auto checkbounds = [=](int x, int y){
		if (x<0)
			std::cerr << x << " " << y << " outside of left bound\n\r";
		if (x >= w)
			std::cerr << x << " " << y << " outside of right bound\n\r";
		if (y<0)
			std::cerr << x << " " << y << " outside of top bound\n\r";
		if (y >= h)
			std::cerr << x << " " << y << " outside of bottom bound\n\r";
	};

	unsigned maxValue = 0;

	//Upper half and middle of diamond
	int starty = std::max(y-n, 0);
	int xwidth = n - (y-starty);
	int xfirst = x - xwidth;
	int xlast = x + xwidth;
	for (int i = starty; i <= y; ++i)
	{
		for (int j = std::max(xfirst, 0); j <= std::min(xlast, w-1); ++j)
		{
			maxValue = std::max(ptrBuffer[i*w + j], maxValue);
			
			#ifdef _DEBUG
			checkbounds(j, i);
			#endif
		}
		--xfirst;
		++xlast;
	}

	// start shrinking x scan instead
	xfirst += 2;
	xlast -= 2;

	// Lower half
	for (int i = y+1; i <= std::min(y+n, h-1); ++i)
	{
		for (int j = std::max(xfirst, 0); j <= std::min(xlast, w-1); ++j)
		{
			maxValue = std::max(ptrBuffer[i*w + j], maxValue);

			#ifdef _DEBUG
			checkbounds(j, i);
			#endif
		}
		++xfirst;
		--xlast;
	}

	return maxValue;

}

void erode2full(unsigned w, unsigned h, int n, const std::vector<uint32_t> &input, std::vector<uint32_t> &output){
	for (unsigned y = 0; y < h; ++y)
	{
		for (unsigned x = 0; x < w; ++x)
		{
			output[y*w + x] = erode2(w, h, n, (uint32_t*)input.data(), x, y);
		}
	}
}

void dilate2full(unsigned w, unsigned h, int n, const std::vector<uint32_t> &input, std::vector<uint32_t> &output){
	for (unsigned y = 0; y < h; ++y)
	{
		for (unsigned x = 0; x < w; ++x)
		{
			output[y*w + x] = dilate2(w, h, n, (uint32_t*)input.data(), x, y);
		}
	}
}

///////////////////////////////////////////////////////////////////
// Composite image processing

void process2(int levels, unsigned w, unsigned h, unsigned /*bits*/, std::vector<uint32_t> &pixels)
{
	std::vector<uint32_t> buffer(w*h);
	
	// Depending on whether levels is positive or negative,
	// we flip the order round.
	auto fwd=levels < 0 ? erode2full : dilate2full;
	auto rev=levels < 0 ? dilate2full : erode2full;
	
	fwd(w, h, std::abs(levels), pixels, buffer);
	std::swap(pixels, buffer);
	rev(w, h, std::abs(levels), pixels, buffer);
	std::swap(pixels, buffer);
}

