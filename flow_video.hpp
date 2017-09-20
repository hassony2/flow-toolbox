#pragma once

#include <iostream>
#include <fstream>
#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/cudaoptflow.hpp"
#include "opencv2/cudaarithm.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/cudacodec.hpp"
#include <getopt.h>
#include <stdio.h>
#include "opencv2/video/tracking.hpp"
#include <dirent.h>
#include "flow_video.hpp"

using namespace cv;
using namespace cv::cuda;
using namespace std;




int getdir(string dir, vector<string> &files);

int extractGPUFlows(const string input_dir, const string img_format,
		    const string out_dir, const string proc_type,
                    string output_mm, const int interval_beg, int interval_end,
                    const bool visualize, const bool silence);

int extractCPUFlows(const string input_dir, const string out_dir,
	            const string img_format, const string proc_type,
                    string output_mm, const int interval_beg, int interval_end,
                    const bool visualize, const bool silence);
