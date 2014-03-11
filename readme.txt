HELLO DAVID!

Pair is:
Oskar Weigl - ow610
Thomas Morrison - tm1810

As the great David once said, "great parallisation is no substitute for a greater algorithm". Or close enough. With this in mind we embarked on a journey, no wait, odysee, in quest of a great algorithm. Ultimately, a minimum (maximum) sliding window approach was adopted to reduce the algorithm from O(N^2) complexity to O(N). We further coupled this in a pipeline with TTB to achieve further performance.

It should be pointed out, we also implemented (marked as deprecated in the funciton names), optimisations for the original approach, whereby the von-neuman neighbours are extended to a neighbourhood equal to the number of levels (looks like a daimond). Basically, in the original code you do n passes with both the fwd and rev methods. Using the afromentioned optimisation, you only need on epass in each direction. This immediately lowers the memory requirements from having to store the entire image to just 2*N*w+1 (slightly less than the total rows that pass through of a "diamond"). 

As for the work division. Initially, we read academic literature on different approaches, and to give inspiration. Credit goes to Oscar for the original thought using a histogram. Sliding windows also developed from a white boarding session.  Thomas initially worked on taking the (which was always the fallback - risk management = bonus points). Oskar then moulded this for the pipeline, and built the pipeline. Thomas threaded it up said pipeline, while Oscar then impleneted the sliding window method). We intended then to focus on bit packing to reduce memory hits, and then, a stretch goal of the histogram.

Testing wise. We use david's. We didn't get super super keen and start doing lots of test scripts. We generated test images. Computed output 

Anyway, back to the O(N) algorithm, an example bench mark to whet your appetite
512x512 @ 4 px, 16 window. Ours = 0.285s, original = 0.780s

Time allowing we would of also produced a further optimisation - i.e. bit packing & "histogram of minimums (maximums)"
The former will save the hits on memory access, which should be a principle barrier for performance here
The latter optimisation would of been using a "histogram" to maintain the frequency of the (current) min/max in the diamond. 
I recommened waiting for the oral to hear about it. 
This will be very fast for binary images. 

We look forward to the oral interview

The provided reference code will store the entire frame in memory and then process it in two passes. This is not feasable for large data sets, as input dataset can scale up to 32 bits per pixel data with images of up to 2^22 in each dimension, which is simply too large to store in memory.
The solution is to realise that the min/max operation is associative and transitive, so we can combine the N passes of the aplication of a "size one" cross kernel to a size N diamond, as shown in the lectures, so we will omit detailed explanation.
This allows us to complete the open/close operation as a succession of one each of erode and dilate operations with a structuring element that is a diamond of size N.
This is done by simply computing a min/max reduction across all pixels in the structuring element.

As we only do two passes, it is fairly easy to identifiy the pixels that these operations depend on: namely all pixels that lie inside the structuring element i.e. the diamond.
This means that the first and last dependency is w*N pixels behind and ahead in the pixel stream respectively (where w is the width of the image).
Therefore we can use a circular buffer to store the data inbetween:
* the read pass and the forward pass (w*N pixels required)
* the forward pass and the reverse pass (w*N pixels rquired)
* the reverse pass and writing to the output. (A sufficiently large chunk to reduce writing overhead)

The reduction across the size N diamond (a diamond of lengths across it of size 2*N + 1) will take O(N^2) time, and have O(N^2) memory accesses across O(N) rows. This means that on any reasonably sized kernel size, the prefetch buffer on the CPU will not have any chance to preload the acesses. This means that such an explicit reduction will be slow.

The key thing to note is that the structuring element (SE) slides from left to right for each row of the image. So we can use a sliding window algorithm for each row inside the structuring element.
We implemented a sliding window min/max algororithm with ammortised O(1) complexity, which was inspired from here (http://leetcode.com/2011/01/sliding-window-maximum.html).
Thus as there are O(N) rows to reduce across, we get O(N) time.

The min/max sliding window algorithm maintains a queue of values and index pairs that could possibly be the extreme value if the current extreme value drops out of the tail of the sliding window. Values are poped off the tail of the queue if they drop out of the window spatially.
Candidate extreme values are pushed into the head of the queue if they could be an extreme value. They recursivley replace any less extreme value at the head of the queue, as the newest value, if more extreme, will drive the value of the window for longer than the previous head of the queue (which entered earlier).

As the queue is implemented as a circular buffer, there is very good temporal locality for accessing this auxiliry data structure, so combined with the O(N) complexity of reducing the contents of the SE, we should see an improvement in execution time per pixel.