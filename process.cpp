#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstdint>
#include <task_group.h>

//Oskar Weigl and Thomas Morrison
//
//Used minimum sliding window approach to reduce the algorithm from O(N^2) complexity to O(N)
//As the great David once said, "great parallisation is no substitute for a greater algorithm". Or close enough
//
//An example bench mark:
//512x512 @ 4 px, 16 window. Ours = 0.285s, original = 0.780s
//
//Time allowing we would of also produced a further optimisation - i.e. bit packing & "histogram of minimums (maximums)"
//The former will save the hits on memory access, which should be a principle barrier for performance here
//The latter optimisation would of been using a "histogram" to maintain the frequency of the (current) min/max in the diamond. 
//I recommened waiting for the oral to hear about it. 
//This will be very fast for binary images. 
//
//We look forward to the oral interview



using namespace std;

#if !(defined(_WIN32) || defined(_WIN64))
//"I just love Windows. Microsoft only hires super smart people." - Oskar
#include <unistd.h>
void set_binary_io()
{}
#else
// http://stackoverflow.com/questions/341817/is-there-a-replacement-for-unistd-h-for-windows-visual-c
// http://stackoverflow.com/questions/13198627/using-file-descriptors-in-visual-studio-2010-and-windows
// Note: I could have just included <io.h> and msvc would whinge mightily, but carry on
	
#include <io.h>
#include <fcntl.h>

#define read _read
#define write _write
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

void set_binary_io()
{
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
}
#endif

#pragma region Davids Stuff

////////////////////////////////////////////
// Routines for bringing in binary images

/*! Reverse the orders of bits if necessary
	\note This is laborious and a bit pointless. I'm sure it could be removed, or at least moved...
*/
uint64_t shuffle64(unsigned bits, uint64_t x)
{
	if(bits==1){
		x=((x&0x0101010101010101ull)<<7)
			| ((x&0x0202020202020202ull)<<5)
			| ((x&0x0404040404040404ull)<<3)
			| ((x&0x0808080808080808ull)<<1)
			| ((x&0x1010101010101010ull)>>1)
			| ((x&0x2020202020202020ull)>>3)
			| ((x&0x4040404040404040ull)>>5)
			| ((x&0x8080808080808080ull)>>7);
	}else if(bits==2){
		x=((x&0x0303030303030303ull)<<6)
			| ((x&0x0c0c0c0c0c0c0c0cull)<<2)
			| ((x&0x3030303030303030ull)>>2)
			| ((x&0xc0c0c0c0c0c0c0c0ull)>>6);
	}else if(bits==4){
		x=((x&0x0f0f0f0f0f0f0f0full)<<4)
			| ((x&0xf0f0f0f0f0f0f0f0ull)>>4);
	}
	return x;
}

/*! Take data packed into incoming format, and exand to one integer per pixel */
void unpack_blob(unsigned numpix, unsigned bits, const uint64_t *pRaw, uint32_t *pUnpacked)
{
	uint64_t buffer=0;
	unsigned bufferedBits=0;
	
	const uint64_t MASK=0xFFFFFFFFFFFFFFFFULL>>(64-bits);
	
	for(unsigned i=0;i<numpix;i++){
		if(bufferedBits==0){
			buffer=shuffle64(bits, *pRaw++);
			bufferedBits=64;
		}
		
		pUnpacked[i]=uint32_t(buffer&MASK);
		buffer=buffer>>bits;
		bufferedBits-=bits;
	}
	
	assert(bufferedBits==0);
}

/*! Go back from one integer per pixel to packed format for output. */
void pack_blob(unsigned numpix, unsigned bits, const uint32_t *pUnpacked, uint64_t *pRaw)
{
	uint64_t buffer=0;
	unsigned bufferedBits=0;
	
	const uint64_t MASK=0xFFFFFFFFFFFFFFFFULL>>(64-bits);
	
	for(unsigned i=0;i<numpix;i++){
		buffer=buffer | (uint64_t(pUnpacked[i]&MASK)<< bufferedBits);
		bufferedBits+=bits;
		
		if(bufferedBits==64){
			*pRaw++ = shuffle64(bits, buffer);
			buffer=0;
			bufferedBits=0;
		}
	}
	
	assert(bufferedBits==0);
}

// returns false if encountered EOF within requested segment
bool read_blob(int fd, uint64_t cbBlob, uint64_t &done, void *pBlob)
{
	uint8_t *pBytes=(uint8_t*)pBlob;
	
	done=0;
	while(done<cbBlob){
		int todo=(int)min(uint64_t(1)<<30, cbBlob-done);		
		
		int got=read(fd, pBytes+done, todo);
		if(got==0)
			return false;	// end of file
		if(got<=0){
			throw invalid_argument("Read failure.");
		}
		done+=got;
	}
	
	return true;
}

void write_blob(int fd, uint64_t cbBlob, const void *pBlob)
{
	const uint8_t *pBytes=(const uint8_t*)pBlob;
	
	uint64_t done=0;
	while(done<cbBlob){
		int todo=(int)min(uint64_t(1)<<30, cbBlob-done);
		
		int got=write(fd, pBytes+done, todo);
		if(got<=0)
			throw invalid_argument("Write failure.");
		done+=got;
	}
}
#pragma endregion 

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//////////////////   CODE					 ///////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


//Algorithm inspired by http://leetcode.com/2011/01/sliding-window-maximum.html
// and http://people.cs.uct.ac.za/~ksmith/articles/sliding_window_minimum.html#sliding-window-minimum-algorithm
//Sliding window object
template <class T>
class minmaxSlidingWindow
{
private:
	// Using a circular buffer with the "leave one space empty" full vs empty destinction strategy
	vector<pair<int, T>> Q;
	int headidx;
	int tailidx;
	int buffsize; //we actually allocate buffsize+1
	bool computemax;

	int wrap(int idx){
		if (idx < 0)
			return buffsize;
		else
			return idx>buffsize ? 0 : idx;
	}

	//less_equal for min, greater_equal for max
	bool comp(T a, T b){
		return computemax ? (a >= b) : (a <= b);
	}
public:
	minmaxSlidingWindow(int win, bool computemaxin)
		: Q(win + 1), headidx(0), tailidx(0), buffsize(win), computemax(computemaxin)
	{}

	//windowhead is standard algo idx, windowtail is usually idx-windowsize except at edges

	T push(int windowhead, T val){
		while (headidx != tailidx && comp(val, Q[wrap(headidx - 1)].second))
			headidx = wrap(--headidx);

		Q[headidx] = make_pair(windowhead, val);
		headidx = wrap(++headidx);

		return Q[tailidx].second;
	}

	T pushpop(int windowhead, int windowtail, T val){
		while (headidx != tailidx && comp(val, Q[wrap(headidx - 1)].second))
			headidx = wrap(--headidx);
		while (headidx != tailidx && Q[tailidx].first <= windowtail)
			tailidx = wrap(++tailidx);

		Q[headidx] = make_pair(windowhead, val);
		headidx = wrap(++headidx);

		return Q[tailidx].second;
	}

	T pop(int windowtail){
		while (headidx != tailidx && Q[tailidx].first <= windowtail)
			tailidx = wrap(++tailidx);

		return Q[tailidx].second;
	}

	T current(){
		return Q[tailidx].second;
	}

	void reset(){
		headidx = 0;
		tailidx = 0;
	}

};


//Algorithm from http://stackoverflow.com/a/1322548/1128910
unsigned getnextpow2(unsigned n){
	n--;
	n |= n >> 1;   // Divide by 2^k for consecutive doublings of k up to 32,
	n |= n >> 2;   // and then or the results.
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;

	return n;
}


//Erode-dialate functions

//These 2 methods use the sliding window objects to keep a current min/max of the diamond
uint32_t dilate(int w, int h, int n, uint32_t* ptrBuffer, int mask, int x, int y, vector<minmaxSlidingWindow<uint32_t>> &maxslideWindows){

	int starty = max(y - n, 0);
	int xwidth = n - (y - starty);
	int xfirst = x - xwidth;
	int xlast = x + xwidth;
	int offset = y - n;
	uint32_t maxVal = 0;

	auto computewindow = [&](int i){
		uint32_t slideval;
		if (x == 0)
		{
			maxslideWindows[i - offset].reset();
			for (int j = 0; j <= xlast; ++j)
			{
				maxslideWindows[i - offset].push(j, ptrBuffer[(i*w + j) & mask]);
			}
			slideval = maxslideWindows[i - offset].current();

		}
		else if (xlast >= (int)w) {
			slideval = maxslideWindows[i - offset].pop(xfirst - 1);
		}
		else {
			slideval = maxslideWindows[i - offset].pushpop(xlast, xfirst - 1, ptrBuffer[(i*w + xlast) & mask]);
		}

#ifdef _DEBUG
		uint32_t dbgval = 0;
		for (int j = max(xfirst, 0); j <= min(xlast, w - 1); ++j)
		{
			dbgval = max(ptrBuffer[(i*w + j) & mask], dbgval);
		}
		assert(dbgval == slideval);
#endif

		maxVal = max(maxVal, slideval);
	};


	//Upper half and middle of diamond
	for (int i = starty; i <= y; ++i)
	{
		computewindow(i);

		--xfirst;
		++xlast;
	}

	// start shrinking x scan instead
	xfirst += 2;
	xlast -= 2;

	// Lower half
	for (int i = y + 1; i <= min(y + n, (int)h - 1); ++i)
	{
		computewindow(i);

		++xfirst;
		--xlast;
	}

	return maxVal;
}

uint32_t erode(int w, int h, int n, uint32_t* ptrBuffer, int mask, int x, int y, vector<minmaxSlidingWindow<uint32_t>> &minslideWindows){
	int starty = max(y - n, 0);
	int xwidth = n - (y - starty);
	int xfirst = x - xwidth;
	int xlast = x + xwidth;
	int offset = y - n;
	uint32_t minVal = -1;

	auto computewindow = [&](int i){
		uint32_t slideval;
		if (x == 0)
		{
			minslideWindows[i - offset].reset();
			for (int j = 0; j <= xlast; ++j)
			{
				minslideWindows[i - offset].push(j, ptrBuffer[(i*w + j) & mask]);
			}
			slideval = minslideWindows[i - offset].current();

		}
		else if (xlast >= (int)w) {
			slideval = minslideWindows[i - offset].pop(xfirst - 1);
		}
		else {
			slideval = minslideWindows[i - offset].pushpop(xlast, xfirst - 1, ptrBuffer[(i*w + xlast) & mask]);
		}

#ifdef _DEBUG
		uint32_t dbgval = -1;
		for (int j = max(xfirst, 0); j <= min(xlast, w - 1); ++j)
		{
			dbgval = min(ptrBuffer[(i*w + j) & mask], dbgval);
		}
		assert(dbgval == slideval);
#endif

		minVal = min(minVal, slideval);
	};


	//Upper half and middle of diamond
	for (int i = starty; i <= y; ++i)
	{
		computewindow(i);

		--xfirst;
		++xlast;
	}

	// start shrinking x scan instead
	xfirst += 2;
	xlast -= 2;

	// Lower half
	for (int i = y + 1; i <= min(y + n, (int)h - 1); ++i)
	{
		computewindow(i);

		++xfirst;
		--xlast;
	}

	return minVal;
}

//Deprecated functions that first optimised by increasing size of diamond rather than doing it in lots of little passes. 
//This is more memory efficient than the original implementation as we only need to keep 2*w*N+1 things in the buffer 
//N is "height" (from center) of "diamon" - see diagram below
//We keep the code below for purposes of showcasing and benchmarking
uint32_t erode_deprecated(int w, int h, int n, uint32_t* ptrBuffer, int mask, int x, int y) {

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
			cerr << x << " " << y << " outside of left bound\n\r";
		if (x >= w)
			cerr << x << " " << y << " outside of right bound\n\r";
		if (y<0)
			cerr << x << " " << y << " outside of top bound\n\r";
		if (y >= h)
			cerr << x << " " << y << " outside of bottom bound\n\r";
	};

	unsigned minValue = -1;

	//Upper half and middle of diamond
	int starty = max(y-n, 0);
	int xwidth = n - (y-starty);
	int xfirst = x - xwidth;
	int xlast = x + xwidth;
	for (int i = starty; i <= y; ++i)
	{
		for (int j = max(xfirst, 0); j <= min(xlast, w-1); ++j)
		{
			minValue = min(ptrBuffer[(i*w + j) & mask], minValue);
			
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
	for (int i = y+1; i <= min(y+n, h-1); ++i)
	{
		for (int j = max(xfirst, 0); j <= min(xlast, w-1); ++j)
		{
			minValue = min(ptrBuffer[(i*w + j) & mask], minValue);

			#ifdef _DEBUG
			checkbounds(j, i);
			#endif
		}
		++xfirst;
		--xlast;
	}

	return minValue;

}

uint32_t dilate_deprecated(int w, int h, int n, uint32_t* ptrBuffer, int mask, int x, int y) {

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
			cerr << x << " " << y << " outside of left bound\n\r";
		if (x >= w)
			cerr << x << " " << y << " outside of right bound\n\r";
		if (y<0)
			cerr << x << " " << y << " outside of top bound\n\r";
		if (y >= h)
			cerr << x << " " << y << " outside of bottom bound\n\r";
	};

	unsigned maxValue = 0;

	//Upper half and middle of diamond
	int starty = max(y-n, 0);
	int xwidth = n - (y-starty);
	int xfirst = x - xwidth;
	int xlast = x + xwidth;
	for (int i = starty; i <= y; ++i)
	{
		for (int j = max(xfirst, 0); j <= min(xlast, w-1); ++j)
		{
			maxValue = max(ptrBuffer[(i*w + j) & mask], maxValue);
			
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
	for (int i = y+1; i <= min(y+n, h-1); ++i)
	{
		for (int j = max(xfirst, 0); j <= min(xlast, w-1); ++j)
		{
			maxValue = max(ptrBuffer[(i*w + j) & mask], maxValue);

			#ifdef _DEBUG
			checkbounds(j, i);
			#endif
		}
		++xfirst;
		--xlast;
	}

	return maxValue;

}


int main(int argc, char *argv[])
{

	try{
		if(argc<3){
			fprintf(stderr, "Usage: process width height [bits] [levels]\n");
			fprintf(stderr, "   bits=8 by default\n");
			fprintf(stderr, "   levels=1 by default\n");
			exit(1);
		}
		
		unsigned w=atoi(argv[1]);
		unsigned h=atoi(argv[2]);
		
		unsigned bits=8;
		if(argc>3){
			bits=atoi(argv[3]);
		}
		
		if(bits>32)
			throw invalid_argument("Bits must be <= 32.");
		
		unsigned tmp=bits;
		while(tmp!=1){
			tmp>>=1;
			if(tmp==0)
				throw invalid_argument("Bits must be a binary power.");
		}
		
		if( ((w*bits)%64) != 0){
			throw invalid_argument(" width*bits must be divisible by 64.");
		}
		
		int levels=1;
		if(argc>4){
			levels=atoi(argv[4]);
		}
		
		fprintf(stderr, "Processing %d x %d image with %d bits per pixel.\n", w, h, bits);
		
		set_binary_io();



		//TODO: use bitset to represent the binary versions. It supports hardware counting of bits on SSE4.2

		int 
			N = abs(levels),
			chunksizeBytes = 1<<12,	//"Work unit" - how much we move in or out of the buffer
			chunksizePix = chunksizeBytes*8/bits,
			maxdata = w*(2*N + 1) + 2*chunksizePix,
			Cbuffsize = getnextpow2(maxdata);

		vector<uint32_t> inpBuff(Cbuffsize);
		int 
			inpHeadidx = 0,
			wrapmask = Cbuffsize - 1;

		vector<uint32_t> midBuff(Cbuffsize);
		int 
			midwrapmask = Cbuffsize - 1,
			midHeadidx = 0;
		
		//Output buffer between reverse output and writing out. 
		//As we write out in chunks, only needs to be 2*chunkSize (2x to prevent starvation)
		vector<uint32_t> outBuff(2*chunksizePix);
		int 
			outHeadidx = 0,
			outTailidx = 0,
			wrapmaskout = 2*chunksizePix - 1;

		vector<uint64_t> rawchunk(chunksizeBytes/8);
		
		//Sliding windows 
		vector<minmaxSlidingWindow<uint32_t>> minslideWindows;
		minslideWindows.reserve(2 * N + 1);
		for (int i = 0; i < 2 * N + 1; ++i)
			minslideWindows.push_back(minmaxSlidingWindow<uint32_t>(2 * N + 1, false));

		vector<minmaxSlidingWindow<uint32_t>> maxslideWindows;
		maxslideWindows.reserve(2 * N + 1);
		for (int i = 0; i < 2 * N + 1; ++i)
			maxslideWindows.push_back(minmaxSlidingWindow<uint32_t>(2 * N + 1, true));
		
		// Depending on whether levels is positive or negative,
		// we flip the order round.
		//auto fwd=levels < 0 ? erode2 : dilate2;
		//auto rev=levels < 0 ? dilate2 : erode2;

		auto fwdwindows = levels < 0 ? minslideWindows : maxslideWindows;
		auto revwindows = levels < 0 ? maxslideWindows : minslideWindows;

		auto fwd3 = levels < 0 ? erode : dilate;
		auto rev3 = levels < 0 ? dilate : erode;
	
		//Pipeline/inter-thread communication variables
		//Note that only ever 1 thread writes to a variable, all other threads can only read
		int 
			lastreadpix = -1,
			reqpixFwd = w*N, //last pixel dependency required by fwd computation
			lastfwdpix = -1,
			reqpixRev = w*N, //last pixel dependency required by rev computation
			lastrevpix = -1,
			lastwritepix = -1;
		
		bool 
			EndOfFile = 0,	
			bExit = false;

		int 
			y1 = 0,
			x1 = 0,
			y2 = 0,
			x2 = 0;		

		tbb::task_group group;
		
		//Use spin locking. Avoids OS overhead of context switching, and will be blocked for relatively "small" periods of time, so no mutexing today.
		//3 seperate threads 
		//			- 1 for forward pass
		//			- 1 for reverse pass
		//			- 1 for writing and reading 

		//Forward pass 
		auto forward = [&]() {
			while (1) {
				//The last pixel read needs to be atleast the required pixel for the fwd dependecy
				//The required pixel for the reverse stage must be atleast the last forward pixel produced
				//Chunk size for reading ahead, so we don't starve the pipeline
				while (lastreadpix >= reqpixFwd && lastfwdpix <= reqpixRev + chunksizePix /*lastfwdpix < reqpixRev*/)
				{
					
					uint32_t fwdVal = fwd3(w, h, N, inpBuff.data(), wrapmask, x1, y1, fwdwindows);
#ifdef _DEBUG
					//One Naboo crusier got past the blockade
					assert(lastreadpix - Cbuffsize <= lastfwdpix - (int)(w*N));
#endif				

					midBuff[midHeadidx & midwrapmask] = fwdVal;
					
					++midHeadidx;
					if (++x1 >= (int)w) {
						x1 = 0;
						if (++y1 >= (int)h){ //y overflow means we are on next image.
							y1 = 0;
						}
					}
						++lastfwdpix;
						++reqpixFwd;
					
				}
				if (bExit)
					break;
			}
		};

		//Reverse pass 
		auto reverse = [&]() {
			while (1) {

			//Must be sufficiet pixels generated from the forward pass for rev to proceed 
			//Do not generate too many pixels such that we write over non-written parts of the output buffer
				while (lastfwdpix >= reqpixRev &&  lastrevpix < lastwritepix + chunksizePix)
				{
					uint32_t revVal = rev3(w, h, N, midBuff.data(), midwrapmask, x2, y2, revwindows);

					outBuff[outHeadidx] = revVal;
					outHeadidx = (outHeadidx + 1) & wrapmaskout;
					if (++x2 >= (int)w) {
						x2 = 0;
						if (++y2 >= (int)h){ //y overflow means we are on next image.
							y2 = 0;
						}
					}
					++lastrevpix;
					++reqpixRev;
				}
			if (bExit)
				break;
		}
		};

		//Start the reactor 
		group.run(forward);
		group.run(reverse);
		

		// While there are more images to process
		while(1){

			// When the input stream ends, the early stages will operate on invalid data.
			// This is fine as it does not affect the output
			while (1){
				
				//While the forwad pass requires new pixels greater than a chunk (our granulised unit of work), read another unit of work in
				//The additional chunksizePix exists so the buffer reads one ahead, preventing the pipeline from starving
				while (reqpixFwd + chunksizePix >= lastreadpix)
				{

					uint64_t bytesread;
					EndOfFile = !read_blob(STDIN_FILENO, chunksizeBytes, bytesread, (uint64_t*)&rawchunk[0]);
					//int numpixread = EndOfFile ? bytesread*8/bits : chunksizePix;
					int numpixread = chunksizePix; //we pretend we kept reading, to make the check easier for next stages.
					unpack_blob(numpixread, bits, &rawchunk[0], (uint32_t*)&inpBuff[inpHeadidx]);
					inpHeadidx = (inpHeadidx + numpixread) & wrapmask;
					lastreadpix += numpixread;
				}



				//if the last generted pixel is more recent than the last written PLUS the chunkSizePix 
				//(which is what shall be written out), then write out to file
				while (lastrevpix >= lastwritepix + chunksizePix )
				{
					
					// if a regular chunksize write would over-write to output
					if (EndOfFile && lastwritepix + chunksizePix >= (int)w*(int)h )
					{
						//Called either when the stream ends or EOF reached
						int pixleft = (w*h - 1) - lastwritepix;
						pack_blob(pixleft, bits, &outBuff[outTailidx], &rawchunk[0]);
						outTailidx = (outTailidx + pixleft) & wrapmaskout;
						write_blob(STDOUT_FILENO, pixleft*8/bits, &rawchunk[0]);
						bExit = true;
						break;
					} else {
						pack_blob(chunksizePix, bits, &outBuff[outTailidx], &rawchunk[0]);
						outTailidx = (outTailidx + chunksizePix) & wrapmaskout;
						write_blob(STDOUT_FILENO, chunksizeBytes, &rawchunk[0]);
						lastwritepix += chunksizePix;
					}
				}

				if (bExit)
					break;
				
			}

			if(EndOfFile)
				break;

			//We may have processed some of next image's data, so compensate and continue
			lastreadpix -= w*h;
			reqpixFwd -= w*h;
			lastfwdpix -= w*h;
			lastrevpix -= w*h;
		}
		
		return 0;
	}catch(exception &e){
		cerr<<"Caught exception : "<<e.what()<<"\n";
		return 1;
	}
}

