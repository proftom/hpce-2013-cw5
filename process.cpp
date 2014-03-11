#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstdint>
#include <task_group.h>

//Oskar Weigl and Professor Thomas Morrison




using namespace std;
/*
This is a program for performing morphological operations in gray-scale
images, and in particular very large images. The general idea of
morphological operations can be found here:
http://homepages.inf.ed.ac.uk/rbf/HIPR2/matmorph.htm

Changelog:
- 2014/03/03: Patches to make native windows work, plus some comments
              about platform portability.
- 2014/03/02: Typo in second paragraph(!): "a close is a dilate followed by an open" -> "a close is a dilate followed by an erode."
              Thanks to Oskar Weigl.
              Made clear that negative values of levels are open, positive are close (wasn't explicit before).
              Thanks to Thomas Parker.
- 2014/02/28: Clarified the constraints on the maximum value of the levels parameter (otherwise it is too complex).
              Thanks to Ryan Savitski
              Also put a concrete limit on how long a program can wait before reading a pixel (as otherwise I leave
              it open to legalistic definitions of the metric).

Functionality:

The program performs either open or close operations,
where an open is an erode followed by a dilate, and
a close is a dilate followed by an erode.

Our erode operation replaces each pixel with the minimum
of its Von Neumann neighbourhood, i.e. the left-right
up-down cross we have used before. Dilate does the same, but
takes the maximum. At each boundary, the neigbourhood
it truncated to exclude parts which extend beyond the
image. So for example, in the top-left corner the
neighbourhood consists of just {middle,down,right}.

Images are input and output as raw binary gray-scale
images, with no header. The processing parameters are
set on the command line as "width height [bits] [levels]":
    - Width: positive integer, up to and including 2^24
    - Height: positive integer, up to and including 2^24
    - Bits: a binary power <=32, default=8
    - Levels: number of times to erode before dilating (or vice-versa),
              0 <= abs(levels) <= min(width/4, height/4, 64) 
              default=1,
              negative values are open, positive values are close

A constraint is that mod(Width*bits,64)==0. There
is no constraint on image size, beyond that imposed
by the width, height, and bits parameters. To be
more specific: you may be asked to process images up
to 2^44 or so in pixel count, and your program won't
be running on a machine with 2^41 bytes of host
memory, let alone GPU memory.

Image format:

Images are represented as packed binary, in little-endian
scanline order. For images with less than 8 bits per pixel, the
left-most pixel is in the MSB. For example, the 64x3 1-bit
image with packed hex representation:

    00 00 00 00 ff ff ff ff  00 00 f0 f0 00 00 0f 0f  01 02 04 08 10 20 40 80

represents the image:

    0000000000000000000000000000000011111111111111111111111111111111
    0000000000000000111100001111000000000000000000000000111100001111
    0000000100000010000001000000100000010000001000000100000010000000

You can use imagemagick to convert to and from binary representation.
For example, to convert a 512x512 image input.png to 2-bit:

    convert input.png -depth 2 gray:input.raw

and to convert output.raw back again:

    convert -size 512x512 -depth 2 gray:output.raw output.png

They can also read/write on stdin/stdout for streaming. But, it is
also easy to generate images programmatically, particularly when
dealing with large images. You can even use /dev/zero and
friends if you just need something large to shove through,
or want to focus on performance.

Correctness:

The output images should be bit-accurate correct. Any conflict
between the operation of this program and the definition of the
image processing and image format should be seen as a bug in
this program. For example, this program cannot handle very
large images, which is a bug.

Performance and constraints:

The metric used to evaluate this work is maximum pixel
latency. Pixel latency is defined as the time between
a given pixel entering the program, and the transformed
pixel at the same co-ordinates leaving the program. The
goal is to minimise the maximimum latency over all
pixels. Latency measuring does not start until the first
pixel enters the pipeline, so performance measurement
is "on-hold" till that point. However, your program
must eventually read the first pixel. Practically speaking,
if your program doesn't read a pixel within ten minutes,
it will be considered to have hung.

The program should support the full spectrum of input
parameters correctly, including all image sizes and
all pixel depths. However, particular emphasis is
placed on very large 1-bit and 8-bit images. The
images you'll process can have any pixel distribution, but
they are often quite sparse, meaning a large proportion
of the pixels are zero. The zero to non-zero ratio may
rise to 1:1000000 for very large images, with a large
amount of spatial clustering of the non-zero pixels.
That may be useful in some cases... or not.

Execution and compilation:

Your program will be executed on a number of machines,
and it is your job to try to make it work efficiently
on whatever it finds. It should work correctly
on whatever hardware it is executed on, whether it has a K40
or a dual-core Celeron. Performance is subordinate to
correctness, so be careful when allocating buffers etc.
It is always better to fall back to software if
something goes wrong with OpenCL setup, rather than crashing
or having the program quit.

Both OpenCL 1.1 and TBB 4.2 will be available at compilation
time, and it is up to you what you want to use. Recall
the "on-hold" metric, and consider the possibilities...

Submission:

Your submission should consist of a tar.gz (not a rar or a zip),
and there should be a directory called "src", which contains
all the .cpp files that need to be compiled together to
create your executable. Any header files need to be in that
directory, or included with relative paths.

The compiler will make OpenCL 1.1 available for #include from
"CL/*" and "OpenCL/*", and TBB 4.2 from "tbb/*". No other
libraries should be assumed, beyond standard C++11 libraries.
Practically speaking, common posix libraries are acceptable (i.e.
things which are in cygwin+linux+macos+mingw), but attempt for
portability. Visual studio can be used for development, but be
careful to avoid windows-specific APIs (e.g. try compiling on
cygwin before submitting).

Your program will always be run with the "src" directory
as its working directory, so you can load any kernel
files from there, or from sub-directories of "src".

As well as your source code, you should also include a
"readme.txt" or "readme.pdf", which covers the following:
- What is the approach used to improve performance, in
  terms of algorithms, patterns, and optimisations.
- A description of any testing methodology or verification.
- A summary of how work was partitioned within the pair,
  including planning, design, and testing, as well as coding.
This document does not need to be long, it is not a project
report.

As well as the code and the document, feel free to include
any other interesting artefacts, such as testing code
or performance measurements. Just make sure to clean out
anything large, like test images, object files, executables.

Marking scheme:

33% : Performance, relative to other implementations
33% : Correctness
33% : Code review (style, appropriateness of approach, readability...)

Plagiarism:

Everyones submission will be different for this coursework,
there is little chance of seeing the same code by accident.
However, you can use whatever code you like from this file.
You may also use third-party code, but you need to attribute
it very clearly and carefully, and be able to justify why
it was used - submissions created from mostly third-party
code will be frowned upon, but only in terms of marks.

Any code transfer between pairs will be treated as plagiarism.
I would suggest you keep your code private, not least because
you are competing with the others.

*/


#if !(defined(_WIN32) || defined(_WIN64))
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

void process2(int levels, unsigned w, unsigned h, unsigned /*bits*/, vector<uint32_t> &pixels); //exists in diamondTest.cpp

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

///////////////////////////////////////////////////////////////////
// Basic image processing primitives

uint32_t vmin(uint32_t a, uint32_t b)
{ return min(a,b); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c)
{ return min(a,min(b,c)); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{ return min(min(a,d),min(b,c)); }

uint32_t vmin(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
{ return min(e, min(min(a,d),min(b,c))); }


void erode(unsigned w, unsigned h, const vector<uint32_t> &input, vector<uint32_t> &output)
{
	auto in=[&](int x, int y) -> uint32_t { return input[y*w+x]; };
	auto out=[&](int x, int y) -> uint32_t & {return output[y*w+x]; };
	
	for(unsigned x=0;x<w;x++){
		if(x==0){
			out(0,0)=vmin(in(0,0), in(0,1), in(1,0));
			for(unsigned y=1;y<h-1;y++){
				out(0,y)=vmin(in(0,y), in(0,y-1), in(1,y), in(0,y+1));
			}
			out(0,h-1)=vmin(in(0,h-1), in(0,h-2), in(1,h-1));
		}else if(x<w-1){
			out(x,0)=vmin(in(x,0), in(x-1,0), in(x,1), in(x+1,0));
			for(unsigned y=1;y<h-1;y++){
				out(x,y)=vmin(in(x,y), in(x-1,y), in(x,y-1), in(x,y+1), in(x+1,y));
			}
			out(x,h-1)=vmin(in(x,h-1), in(x-1,h-1), in(x,h-2), in(x+1,h-1));
		}else{
			out(w-1,0)=vmin(in(w-1,0), in(w-1,1), in(w-2,0));
			for(unsigned y=1;y<h-1;y++){
				out(w-1,y)=vmin(in(w-1,y), in(w-1,y-1), in(w-2,y), in(w-1,y+1));
			}
			out(w-1,h-1)=vmin(in(w-1,h-1), in(w-1,h-2), in(w-2,h-1));
		}
	}
}

uint32_t vmax(uint32_t a, uint32_t b)
{ return max(a,b); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c)
{ return max(a,max(b,c)); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{ return max(max(a,d),max(b,c)); }

uint32_t vmax(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
{ return max(e, max(max(a,d),max(b,c))); }

void dilate(unsigned w, unsigned h, const vector<uint32_t> &input, vector<uint32_t> &output)
{
	auto in=[&](int x, int y) -> uint32_t { return input[y*w+x]; };
	auto out=[&](int x, int y) -> uint32_t & {return output[y*w+x]; };
	
	for(unsigned x=0;x<w;x++){
		if(x==0){
			out(0,0)=vmax(in(0,0), in(0,1), in(1,0));
			for(unsigned y=1;y<h-1;y++){
				out(0,y)=vmax(in(0,y), in(0,y-1), in(1,y), in(0,y+1));
			}
			out(0,h-1)=vmax(in(0,h-1), in(0,h-2), in(1,h-1));
		}else if(x<w-1){
			out(x,0)=vmax(in(x,0), in(x-1,0), in(x,1), in(x+1,0));
			for(unsigned y=1;y<h-1;y++){
				out(x,y)=vmax(in(x,y), in(x-1,y), in(x,y-1), in(x,y+1), in(x+1,y));
			}
			out(x,h-1)=vmax(in(x,h-1), in(x-1,h-1), in(x,h-2), in(x+1,h-1));
		}else{
			out(w-1,0)=vmax(in(w-1,0), in(w-1,1), in(w-2,0));
			for(unsigned y=1;y<h-1;y++){
				out(w-1,y)=vmax(in(w-1,y), in(w-1,y-1), in(w-2,y), in(w-1,y+1));
			}
			out(w-1,h-1)=vmax(in(w-1,h-1), in(w-1,h-2), in(w-2,h-1));
		}
	}
}

///////////////////////////////////////////////////////////////////
// Composite image processing

void process(int levels, unsigned w, unsigned h, unsigned /*bits*/, vector<uint32_t> &pixels)
{
	vector<uint32_t> buffer(w*h);
	
	// Depending on whether levels is positive or negative,
	// we flip the order round.
	auto fwd=levels < 0 ? erode : dilate;
	auto rev=levels < 0 ? dilate : erode;
	
	for(int i=0;i<abs(levels);i++){
		fwd(w, h, pixels, buffer);
		swap(pixels, buffer);
	}
	for(int i=0;i<abs(levels);i++){
		rev(w,h,pixels, buffer);
		swap(pixels, buffer);
	}
}

// You may want to play with this to check you understand what is going on
void invert(unsigned w, unsigned h, unsigned bits, vector<uint32_t> &pixels)
{
	uint32_t mask=0xFFFFFFFFul>>bits;
	
	for(unsigned i=0;i<w*h;i++){
		pixels[i]=mask-pixels[i];
	}
}

uint32_t erode2(int w, int h, int n, uint32_t* ptrBuffer, int mask, int x, int y) {

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

uint32_t dilate2(int w, int h, int n, uint32_t* ptrBuffer, int mask, int x, int y) {

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

//Algorithm inspired by http://leetcode.com/2011/01/sliding-window-maximum.html
// and http://people.cs.uct.ac.za/~ksmith/articles/sliding_window_minimum.html#sliding-window-minimum-algorithm

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
	: Q(win+1), headidx(0), tailidx(0), buffsize(win), computemax(computemaxin)
	{}

	//windowhead is standard algo idx, windowtail is usually idx-windowsize except at edges

	T push(int windowhead, T val){
		while(headidx != tailidx && comp(val, Q[wrap(headidx-1)].second))
			headidx = wrap(--headidx);
		
		Q[headidx] = make_pair(windowhead, val);
		headidx = wrap(++headidx);

		return Q[tailidx].second;
	}

	T pushpop(int windowhead, int windowtail, T val){
		while(headidx != tailidx && comp(val, Q[wrap(headidx-1)].second))
			headidx = wrap(--headidx);
		while(headidx != tailidx && Q[tailidx].first <= windowtail)
			tailidx = wrap(++tailidx);

		Q[headidx] = make_pair(windowhead, val);
		headidx = wrap(++headidx);

		return Q[tailidx].second;
	}

	T pop(int windowtail){
		while(headidx != tailidx && Q[tailidx].first <= windowtail)
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

uint32_t dilate3(int w, int h, int n, uint32_t* ptrBuffer, int mask, int x, int y, vector<minmaxSlidingWindow<uint32_t>> &maxslideWindows){
	int starty = max(y-n, 0);
	int xwidth = n - (y-starty);
	int xfirst = x - xwidth;
	int xlast = x + xwidth;
	int offset = y-n;
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

		} else if (xlast >= (int)w) {
			slideval = maxslideWindows[i - offset].pop(xfirst-1);
		} else {
			slideval = maxslideWindows[i - offset].pushpop(xlast, xfirst-1, ptrBuffer[(i*w + xlast) & mask]);
		}

#ifdef _DEBUG
		uint32_t dbgval = 0;
		for (int j = max(xfirst, 0); j <= min(xlast, w-1); ++j)
		{
			dbgval = max(ptrBuffer[(i*w + j) & mask], dbgval);
		}
		assert(dbgval == slideval);
#endif

		maxVal = max(maxVal, slideval);
	};


	//Upper half and middle of diamond
	for (int i = starty; i <= y ; ++i)
	{
		computewindow(i);

		--xfirst;
		++xlast;
	}

	// start shrinking x scan instead
	xfirst += 2;
	xlast -= 2;

	// Lower half
	for (int i = y+1; i <= min(y+n, (int)h-1); ++i)
	{
		computewindow(i);

		++xfirst;
		--xlast;
	}

	return maxVal;
}

uint32_t erode3(int w, int h, int n, uint32_t* ptrBuffer, int mask, int x, int y, vector<minmaxSlidingWindow<uint32_t>> &minslideWindows){
	int starty = max(y-n, 0);
	int xwidth = n - (y-starty);
	int xfirst = x - xwidth;
	int xlast = x + xwidth;
	int offset = y-n;
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

		} else if (xlast >= (int)w) {
			slideval = minslideWindows[i - offset].pop(xfirst-1);
		} else {
			slideval = minslideWindows[i - offset].pushpop(xlast, xfirst-1, ptrBuffer[(i*w + xlast) & mask]);
		}

#ifdef _DEBUG
		uint32_t dbgval = -1;
		for (int j = max(xfirst, 0); j <= min(xlast, w-1); ++j)
		{
			dbgval = min(ptrBuffer[(i*w + j) & mask], dbgval);
		}
		assert(dbgval == slideval);
#endif

		minVal = min(minVal, slideval);
	};


	//Upper half and middle of diamond
	for (int i = starty; i <= y ; ++i)
	{
		computewindow(i);

		--xfirst;
		++xlast;
	}

	// start shrinking x scan instead
	xfirst += 2;
	xlast -= 2;

	// Lower half
	for (int i = y+1; i <= min(y+n, (int)h-1); ++i)
	{
		computewindow(i);

		++xfirst;
		--xlast;
	}

	return minVal;
}

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

		int N = abs(levels);
		int chunksizeBytes = 1<<12;
		//int chunksizeBytes = 8;
		int chunksizePix = chunksizeBytes*8/bits;
		int maxdata = w*(2*N + 1) + 2*chunksizePix;
		int Cbuffsize = getnextpow2(maxdata);

		vector<uint32_t> inpBuff(Cbuffsize);
		int inpHeadidx = 0;
		int wrapmask = Cbuffsize - 1;

		vector<uint32_t> midBuff(Cbuffsize);
		int midwrapmask = Cbuffsize - 1;
		int midHeadidx = 0;


		vector<uint32_t> outBuff(2*chunksizePix);
		int outHeadidx = 0;
		int outTailidx = 0;
		int wrapmaskout = 2*chunksizePix - 1;

		vector<uint64_t> rawchunk(chunksizeBytes/8);

		// Depending on whether levels is positive or negative,
		// we flip the order round.
		auto fwd=levels < 0 ? erode2 : dilate2;
		auto rev=levels < 0 ? dilate2 : erode2;
	
		int lastreadpix = -1;
		int reqpixFwd = w*N; //last pixel required by current computation
		int lastfwdpix = -1;
		int reqpixRev = w*N;
		int lastrevpix = -1;
		int lastwritepix = -1;
		
		bool EndOfFile = 0;

		bool bExit = false;

		int y1 = 0;
		int x1 = 0;
		int y2 = 0;
		int x2 = 0;
		
		vector<minmaxSlidingWindow<uint32_t>> minslideWindows;
		minslideWindows.reserve(2*N + 1);
		for (int i = 0; i < 2*N + 1; ++i)
			minslideWindows.push_back(minmaxSlidingWindow<uint32_t>(2*N+1, false));

		vector<minmaxSlidingWindow<uint32_t>> maxslideWindows;
		maxslideWindows.reserve(2*N + 1);
		for (int i = 0; i < 2*N + 1; ++i)
			maxslideWindows.push_back(minmaxSlidingWindow<uint32_t>(2*N+1, true));

		auto fwdwindows = levels < 0 ? minslideWindows : maxslideWindows;
		auto revwindows = levels < 0 ? maxslideWindows : minslideWindows;

		auto fwd3=levels < 0 ? erode3 : dilate3;
		auto rev3=levels < 0 ? dilate3 : erode3;
		bool bTest = true;
		bool bTest2 = true;

		int runTimesRead = 0;
		int runTimesForwrd = 0;
		int runTimesRev = 0;
		int runTimesWrite = 0;

		 

		tbb::task_group group;
		

		auto readData = [&]() {
			while (1) {
				//If the required foward pixel is greater than the last pixel read (
				while (reqpixFwd + chunksizePix >= lastreadpix /* lastreadpix - lastfwdpix + (int)w*N <=  Cbuffsize - chunksizePix*/)
				{
					runTimesRead++;
					fprintf(stderr, "reading\n");
					uint64_t bytesread;
					EndOfFile = !read_blob(STDIN_FILENO, chunksizeBytes, bytesread, (uint64_t*)&rawchunk[0]);
					//int numpixread = EndOfFile ? bytesread*8/bits : chunksizePix;
					int numpixread = chunksizePix; //we pretend we kept reading, to make the check easier for next stages.
					unpack_blob(numpixread, bits, &rawchunk[0], (uint32_t*)&inpBuff[inpHeadidx]);
					inpHeadidx = (inpHeadidx + numpixread) & wrapmask;
					lastreadpix += numpixread;

				}
				if (bExit)
					break;
			}
		};
		
		auto forward = [&]() {
			while (1) {
				//The last pixel read needs to be atleast the required pixel for the fwd dependecy
				//The required pixel for the reverse stage must be atleast the last forward pixel produced
				while (lastreadpix >= reqpixFwd && lastfwdpix <= reqpixRev + chunksizePix /*lastfwdpix < reqpixRev*/)
				{

					runTimesForwrd++;
					//fprintf(stderr, "fwd\n");
					uint32_t fwdVal = fwd3(w, h, N, inpBuff.data(), wrapmask, x1, y1, fwdwindows);
#ifdef _DEBUG
					uint32_t refval = fwd(w, h, N, inpBuff.data(), wrapmask, x1, y1);
					assert(refval == fwdVal);
#endif
					assert(lastreadpix - Cbuffsize <= lastfwdpix - (int)(w*N));

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

		auto reverse = [&]() {
			while (1) {

			// Can run
			//Must be sufficiet pixels generated from the forward pass for rev to proceed 
			//Do not generate too many pixels such that we write over non-written parts of the output buffer
				while (lastfwdpix >= reqpixRev &&  lastrevpix < lastwritepix + chunksizePix)
				{
					uint32_t revVal = rev3(w, h, N, midBuff.data(), midwrapmask, x2, y2, revwindows);
#ifdef _DEBUG
					uint32_t refval = rev(w, h, N, midBuff.data(), midwrapmask, x2, y2);
					assert(refval == revVal);
#endif

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

		//group.run(readData);
		group.run(forward);
		group.run(reverse);

		

		// While there are more images to process
		while(1){

			// The great pipeline.
			// When the input stream ends, the early stages will operate on invalid data.
			// This is fine as it does not affect the output
			while (1){
				while (reqpixFwd + chunksizePix >= lastreadpix /* lastreadpix - lastfwdpix + (int)w*N <=  Cbuffsize - chunksizePix*/)
				{
					runTimesRead++;
					fprintf(stderr, "reading\n");
					uint64_t bytesread;
					EndOfFile = !read_blob(STDIN_FILENO, chunksizeBytes, bytesread, (uint64_t*)&rawchunk[0]);
					//int numpixread = EndOfFile ? bytesread*8/bits : chunksizePix;
					int numpixread = chunksizePix; //we pretend we kept reading, to make the check easier for next stages.
					unpack_blob(numpixread, bits, &rawchunk[0], (uint32_t*)&inpBuff[inpHeadidx]);
					inpHeadidx = (inpHeadidx + numpixread) & wrapmask;
					lastreadpix += numpixread;
				}

				// Should run

				//fprintf(stderr, "lastreadpix: %#010x\t inpHeadidx: %#010x\n", lastreadpix, inpHeadidx);
				


				//fprintf(stderr, "lastfwdpix: %#010x\t midHeadidx: %#010x\t reqpixFwd: %#010x\n", lastfwdpix, inpHeadidx, reqpixFwd);



				//fprintf(stderr, "lastrevpix: %#010x\t outHeadidx: %#010x\t reqpixRev: %#010x\n", lastrevpix, outHeadidx, reqpixRev);


				//if the last generted pixel is more recent than the last written PLUS the chunkSizePix (which is what shall be written out), then write out.
				while (lastrevpix >= lastwritepix + chunksizePix  )
				{

					fprintf(stderr, "writing\n");
					//assert(outHeadidx == 0);
					runTimesWrite++;
					// if a regular chunksize write would over-write to output
					if (EndOfFile && lastwritepix + chunksizePix >= (int)w*(int)h )
					{
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

				//fprintf(stderr, "\n");
				
			}

			if(EndOfFile)
				break;

			//We may have processed some some of next image's data, so compensate and continue
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

