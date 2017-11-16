# Flow toolbox
Optical flow extraction tool using OpenCV. The code is simple and mostly from documentation of OpenCV, but it makes it easy to use for pre-processing of videos with several options. It is easy to customize and change the algorithms used. Currently Brox algorithm is used for GPU and Farneback is used for CPU implementations.

## Help
Type `./flow_video -h` to see the help message below.


```shell
USAGE:
[-h] [-p <proc_type>] [-o <out_dir>] [-b <interval_beg>] [-e <interval_end>] [-v <visualize>] [-m <output_mm>] <input_name> [-n <nested>] [-f <img_format>]

INPUT:
<input_dir>		    : Path to folder containing the video file or image directory
OPTIONS:
-h					    : Display this help message
-p	<proc_type>	[farn]	  : Processor type [farn|brox|tvl1]
-o	<out_dir>	    [./]	: Output folder containing flow images and minmax.txt
-b	<interval_beg>	[1]	  : Frame index to start (one-based indexing)
-e	<interval_end>	[last]	  : Frame index to stop
-v	<visualize>	[0]	  : Boolean for visualization of the optical flow
-m	<output_mm>	[<out_dir>/<basename(input_name)>_minmax.txt]	: Name of the minmax file.
-n	<nested>	[0]       : 1 to extract videos by going through the directories in <input_dir> (videos are innested folders and not directly in <input_dir>).
-f	<img_format>	[%05d.jpg]: format of consecutive frames or name of video file


Notes:
*Only <imagename>_%0xd.jpg (x any digit) is supported for image sequence input.
```

## Example usage
Extract flow from the frame interval 5-10.

```shell
./flow_video -b 5 -e 10 -o samples/out -p tvl1 -f video.avi samples/videos
./flow_video -b 5 -e 10 -o samples/out -p tvl1 -f img1_%05d.jpg samples/images
```
