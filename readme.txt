HELLO DAVID!

Oskar Weigl and Thomas Morrison

As the great David once said, "great parallisation is no substitute for a greater algorithm". Or close enough. With this in mind we embarked on a journey, no wait, odysee, in quest of a great algorithm. Ultimately, a minimum (maximum) sliding window approach was adopted to reduce the algorithm from O(N^2) complexity to O(N). We further coupled this in a pipeline with TTB to achieve further performance.

It should be pointed out, we also implemented (marked as deprecated in the funciton names), optimisations for the original approach, whereby the von-neuman neighbours are extended to a neighbourhood equal to the number of levels (looks like a daimond). Basically, in the original code you do n passes with both the fwd and rev methods. Using the afromentioned optimisation, you only need on epass in each direction. This immediately lowers the memory requirements from having to store the entire image to just 2*N*w+1 (slightly less than the total rows that pass through of a "diamond"). 

As for the work division. Initially, we read academic literature on different approaches, and to give inspiration. Credit goes to Oscar for the original thought of using a histogram. Sliding windows also developed from a white boarding session.  Thomas initially worked on taking the (which was always the fallback - risk management = bonus points). Oskar then moulded this for the pipeline, and built the pipeline. Thomas threaded up said pipeline, while Oscar then impleneted the sliding window method. We intended then to focus on bit packing to reduce memory hits, and then, a stretch goal of the histogram. The last 2 didn't quite happen.

Testing wise we didn't get super super keen and start doing lots of test scripts. We generated test images of different dimensions, size, bit width, depth and made sure our output against the original compiled code was the same. We made sure to test around the edges of the image where stuff normally hits the fan. We also performed some time test (a snippet below)

Anyway, back to the O(N) algorithm, an example bench mark to whet your appetite
512x512 @ 4 px, 16 window. Ours = 0.285s, original = 0.780s

Time allowing we would of also produced a further optimisation - i.e. bit packing & "histogram of minimums (maximums)"
The former will save the hits on memory access, which should be a principle barrier for performance here
The latter optimisation would of been using a "histogram" to maintain the frequency of the (current) min/max in the diamond. 
I recommened waiting for the oral to hear about it. 
This will be very fast for binary images. 

We look forward to the oral interview

