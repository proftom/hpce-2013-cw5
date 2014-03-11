HELLO DAVID!

Pair is:
Oskar Weigl - ow610
Thomas Morrison - tm1810

As the great David once said, "great parallisation is no substitute for a greater algorithm". Or close enough. With this in mind we embarked on a journey, no wait, odysee, in quest of a great algorithm. Ultimately, a minimum (maximum) sliding window approach was adopted to reduce the algorithm from O(N^2) complexity to O(N). We further coupled this in a pipeline with TTB to achieve further performance.

It should be pointed out, we also implemented (marked as deprecated in the funciton names), optimisations for the original approach, whereby the von-neuman neighbours are extended to a neighbourhood equal to the number of levels (looks like a daimond). Basically, in the original code you do n passes with both the fwd and rev methods. Using the afromentioned optimisation, you only need on epass in each direction. This immediately lowers the memory requirements from having to store the entire image to just 2*N*w+1 (slightly less than the total rows that pass through of a "diamond"). 

As for the work division. Initially, we read academic literature on different approaches, and to give inspiration. Credit goes to Oscar for the original thought of using a histogram. Sliding windows also developed from a white boarding session.  Thomas initially worked on taking the (which was always the fallback - risk management = bonus points). Oskar then moulded this for the pipeline, and built the pipeline. Thomas threaded up said pipeline, while Oscar then impleneted the sliding window method. We intended then to focus on bit packing to reduce memory hits, and then, a stretch goal of the histogram. The last 2 didn't quite happen.

Testing wise we didn't get super super keen and start doing lots of test scripts. We generated test images of different dimensions, size, bit width, depth and made sure our output against the original compiled code was the same. We made sure to test around the edges of the image where stuff normally hits the fan. We also performed some time test (a snippet below)

Anyway, back to the O(N) algorithm, an example bench mark to whet your appetite
512x512 @ 4 px, 16 window. Ours = 0.285s, original = 0.780s

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

== future work / potential optimisations ==
Another algorithm considered is using a histogram to keep track of the value frequencies within the SE. Sliding the kernel we would requrie 2N additive updates and 2N subtractive updates for the head/tail of the SE respectively.
The issue is that when the current extreme value has been dropped off the end of the SE, we need to find the next extreme value. This requires searching all the bins, so we get worst case O(k) time to complete, where k is the number of grayscale values that can exist.
We could maintain an auxiliry priority queue (a min/max heap) to quickly fetch the extreme value in O(log k).
The best use of this algorithm would be when using binary data, as the lookup would be trivial, and does not require any notable auxiliry data structure like the sliding window approach does.

Currently, the data is still stored as ints, even for binary. Storing packed would generate less memory pressure, which is most likely the bottleneck currently.