#include "flow_video.hpp"

//Mostly from https://github.com/opencv/opencv/blob/master/samples/gpu/optical_flow.cpp


//http://stackoverflow.com/questions/24221605/find-all-files-in-a-directory-and-its-subdirectory
int getdir(string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL)
    {
        cout << "Error opening " << dir << endl;
        return -1;
    }

    while ((dirp = readdir(dp)) != NULL)
    {
        string s = string(dirp->d_name);
        if(strcmp(s.c_str(), ".") && strcmp(s.c_str(), ".."))
            files.push_back(s);
    }
    closedir(dp);
    return 1;
}

inline bool isFlowCorrect(Point2f u)
{
    return !cvIsNaN(u.x) && !cvIsNaN(u.y) && fabs(u.x) < 1e9 && fabs(u.y) < 1e9;
}

static Vec3b computeColor(float fx, float fy)
{
    static bool first = true;

    // relative lengths of color transitions:
    // these are chosen based on perceptual similarity
    // (e.g. one can distinguish more shades between red and yellow
    //  than between yellow and green)
    const int RY = 15;
    const int YG = 6;
    const int GC = 4;
    const int CB = 11;
    const int BM = 13;
    const int MR = 6;
    const int NCOLS = RY + YG + GC + CB + BM + MR;
    static Vec3i colorWheel[NCOLS];

    if (first)
    {
        int k = 0;

        for (int i = 0; i < RY; ++i, ++k)
            colorWheel[k] = Vec3i(255, 255 * i / RY, 0);

        for (int i = 0; i < YG; ++i, ++k)
            colorWheel[k] = Vec3i(255 - 255 * i / YG, 255, 0);

        for (int i = 0; i < GC; ++i, ++k)
            colorWheel[k] = Vec3i(0, 255, 255 * i / GC);

        for (int i = 0; i < CB; ++i, ++k)
            colorWheel[k] = Vec3i(0, 255 - 255 * i / CB, 255);

        for (int i = 0; i < BM; ++i, ++k)
            colorWheel[k] = Vec3i(255 * i / BM, 0, 255);

        for (int i = 0; i < MR; ++i, ++k)
            colorWheel[k] = Vec3i(255, 0, 255 - 255 * i / MR);

        first = false;
    }

    const float rad = sqrt(fx * fx + fy * fy);
    const float a = atan2(-fy, -fx) / (float) CV_PI;

    const float fk = (a + 1.0f) / 2.0f * (NCOLS - 1);
    const int k0 = static_cast<int>(fk);
    const int k1 = (k0 + 1) % NCOLS;
    const float f = fk - k0;

    Vec3b pix;

    for (int b = 0; b < 3; b++)
    {
        const float col0 = colorWheel[k0][b] / 255.0f;
        const float col1 = colorWheel[k1][b] / 255.0f;

        float col = (1 - f) * col0 + f * col1;

        if (rad <= 1)
            col = 1 - rad * (1 - col); // increase saturation with radius
        else
            col *= .75; // out of range

        pix[2 - b] = static_cast<uchar>(255.0 * col);
    }

    return pix;
}

static void drawOpticalFlow(const Mat_<float>& flowx, const Mat_<float>& flowy, Mat& dst, float maxmotion = -1)
{
    dst.create(flowx.size(), CV_8UC3);
    dst.setTo(Scalar::all(0));

    // determine motion range:
    float maxrad = maxmotion;

    if (maxmotion <= 0)
    {
        maxrad = 1;
        for (int y = 0; y < flowx.rows; ++y)
        {
            for (int x = 0; x < flowx.cols; ++x)
            {
                Point2f u(flowx(y, x), flowy(y, x));

                if (!isFlowCorrect(u))
                    continue;

                maxrad = max(maxrad, sqrt(u.x * u.x + u.y * u.y));
            }
        }
    }

    for (int y = 0; y < flowx.rows; ++y)
    {
        for (int x = 0; x < flowx.cols; ++x)
        {
            Point2f u(flowx(y, x), flowy(y, x));

            if (isFlowCorrect(u))
                dst.at<Vec3b>(y, x) = computeColor(u.x / maxrad, u.y / maxrad);
        }
    }
}

static void showFlow(const char* name, const GpuMat& d_flow)
{
    GpuMat planes[2];
    cuda::split(d_flow, planes);

    Mat flowx(planes[0]);
    Mat flowy(planes[1]);

    Mat out;
    drawOpticalFlow(flowx, flowy, out, 10);

    //imwrite("deneme.jpg", out);
    imshow(name, out);
}

static void showFlow(const char* name, const Mat& d_flow)
{
    vector<Mat> planes;
    split(d_flow, planes);
    Mat flowx(planes[0]);
    Mat flowy(planes[1]);

    Mat out;
    drawOpticalFlow(flowx, flowy, out, 10);

    //imwrite("deneme.jpg", out);
    imshow(name, out);
}

/* Compute the magnitude of flow given x and y components */
static void computeFlowMagnitude(const Mat_<float>& flowx, const Mat_<float>& flowy, Mat& dst)
{
    dst.create(flowx.size(), CV_32FC1);
    for (int y = 0; y < flowx.rows; ++y)
    {
        for (int x = 0; x < flowx.cols; ++x)
        {
            Point2f u(flowx(y, x), flowy(y, x));

            if (!isFlowCorrect(u))
                continue;

            dst.at<float>(y, x) = sqrt(u.x * u.x + u.y * u.y);
        }
    }
}

/* Write raw optical flow values into txt file
   Example usage:
   writeFlowRaw<float>(name+"_x_raw.txt", flowx);
   writeFlowRaw<int>(name+"_x_raw_n.txt", flowx_n);
   */
    template <typename T>
static void writeFlowRaw(string name, const Mat& flow)
{
    ofstream file;
    file.open(name.c_str());
    for(int y=0; y<flow.rows; ++y)
    {
        for(int x=0; x<flow.cols; ++x)
        {
            file << flow.at<T>(y, x) << " ";
        }
        file << endl;
    }
    file.close();
}

//min_x max_x min_y max_y
static void writeMM(string name, vector<double> mm)
{
    ofstream file;
    file.open(name.c_str());
    for(int i=0; i<mm.size(); i++)
    {
        file << mm[i] << " ";
    }
    file.close();
}

//min_x max_x min_y max_y (one line per frame)
static void writeMM(string name, vector<vector<double> > mm)
{
    ofstream file;
    file.open(name.c_str());
    for(int i=0; i<mm.size(); i++)
    {
        for(int j=0; j<mm[i].size(); j++)
        {
            file << mm[i][j] << " ";
        }
        file << endl;
    }
    file.close();
}

static vector<double> getMM(const Mat& flow)
{
    double min, max;
    cv::minMaxLoc(flow, &min, &max);
    vector<double> mm;
    mm.push_back(min);
    mm.push_back(max);
    return mm;
}

/* Write a 3-channel jpg image (flow_x, flow_y, flow_magnitude) in 0-255 range */
static void writeFlowMergedJpg(string name, const GpuMat& d_flow)
{
    GpuMat planes[2];
    cuda::split(d_flow, planes);

    Mat flowx(planes[0]);
    Mat flowy(planes[1]);

    Mat flowmag;
    computeFlowMagnitude(flowx, flowy, flowmag);

    Mat flowx_n, flowy_n, flowmag_n;
    cv::normalize(flowx, flowx_n, 0, 255, NORM_MINMAX, CV_8UC1);
    cv::normalize(flowy, flowy_n, 0, 255, NORM_MINMAX, CV_8UC1);
    cv::normalize(flowmag, flowmag_n, 0, 255, NORM_MINMAX, CV_8UC1);

    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(95);

    Mat flow;
    vector<Mat> array_to_merge;
    array_to_merge.push_back(flowx_n);
    array_to_merge.push_back(flowy_n);
    array_to_merge.push_back(flowmag_n);
    cv::merge(array_to_merge, flow);

    imwrite(name+".jpg", flow, compression_params);
}

/* Write two 1-channel jpg images (flow_x and flow_y) in 0-255 range (input flow is gpumat)*/
static vector<double> writeFlowJpg(string name, const GpuMat& d_flow)
{
    // Split flow into x and y components in CPU
    GpuMat planes[2];
    cuda::split(d_flow, planes);
    Mat flowx(planes[0]);
    Mat flowy(planes[1]);

    // Normalize optical flows in range [0, 255]
    Mat flowx_n, flowy_n;
    cv::normalize(flowx, flowx_n, 0, 255, NORM_MINMAX, CV_8UC1);
    cv::normalize(flowy, flowy_n, 0, 255, NORM_MINMAX, CV_8UC1);

    // Save optical flows (x, y) as jpg images
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(95);

    imwrite(name + "x.jpg", flowx_n, compression_params);
    imwrite(name + "y.jpg", flowy_n, compression_params);

    // Return normalization elements
    vector<double> mm_frame;
    vector<double> temp = getMM(flowx);
    mm_frame.insert(mm_frame.end(), temp.begin(), temp.end());
    temp = getMM(flowy);
    mm_frame.insert(mm_frame.end(), temp.begin(), temp.end());

    return mm_frame;
}

/* Write two 1-channel jpg images (flow_x and flow_y) in 0-255 range (input flow is cpu mat)*/
static vector<double> writeFlowJpg(string name, const Mat& d_flow)
{
    vector<Mat> planes;
    split(d_flow, planes);
    Mat flowx(planes[0]);
    Mat flowy(planes[1]);
    // Normalize optical flows in range [0, 255]
    Mat flowx_n, flowy_n;
    cv::normalize(flowx, flowx_n, 0, 255, NORM_MINMAX, CV_8UC1); //TO-DO
    cv::normalize(flowy, flowy_n, 0, 255, NORM_MINMAX, CV_8UC1); //TO-DO

    // Save optical flows (x, y) as jpg images
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(95);

    imwrite(name + "x.jpg", flowx_n, compression_params);
    imwrite(name + "y.jpg", flowy_n, compression_params);

    // Return normalization elements
    vector<double> mm_frame;
    vector<double> temp = getMM(flowx); //TO-DO
    mm_frame.insert(mm_frame.end(), temp.begin(), temp.end());
    temp = getMM(flowy); //TO-DO
    mm_frame.insert(mm_frame.end(), temp.begin(), temp.end());

    return mm_frame;
}

int extractGPUFlows(const string input_dir, const string img_format, const string out_dir, const string proc_type,
        string output_mm, const int interval_beg, int interval_end,
        const bool visualize, const bool silence)
{
    // Declare useful variables
    Mat frame0, frame1;
    char name[200];
    vector<vector<double> > mm;

    // VIDEO INPUT
    if (!silence)
    {
        cout << "Extracting flow from [" << input_dir << "] using GPU." << endl;
        cout << "Initialization (this may take awhile)..." << endl;
    }
    GpuMat temp = GpuMat(3, 3, CV_32FC1);
    // Declare gpu mats
    GpuMat g_frame0, g_frame1;
    GpuMat gf_frame0, gf_frame1;
    GpuMat g_flow;
    // Create optical flow object
    Ptr<cuda::BroxOpticalFlow> brox = cuda::BroxOpticalFlow::create(0.197f, 50.0f, 0.8f, 10, 77, 10);
    Ptr<cuda::OpticalFlowDual_TVL1> tvl1 = cuda::OpticalFlowDual_TVL1::create();

    // Open video file
    string input_name = input_dir + img_format;
    VideoCapture cap(input_name);
    double cap_fourcc = cap.get(CV_CAP_PROP_FOURCC);
    if(cap.isOpened())
    {
        int noFrames = cap.get(CV_CAP_PROP_FRAME_COUNT); //get the frame count
        if(interval_end == -1)
        {
            interval_end = noFrames;
        }
        if(cap_fourcc == 0) // image sequence
        {
            output_mm = out_dir + "minmax.txt";
        }
        if(!silence)
        {
            cout << "Total number of frames: " << noFrames << endl;
            cout << "Extracting interval [" << interval_beg  << "-" << interval_end << "]" << endl;
        }
        // cap.set(CV_CAP_PROP_POS_FRAMES, interval_beg-1); // causes problem for image sequence!

        // Read first frame
        if(noFrames>0)
        {
            bool bSuccess = cap.read(frame0);
            if(!bSuccess)   { cout << "Cannot read frame!" << endl;   }
            else            { cvtColor(frame0, frame0, CV_BGR2GRAY);  }
        }
        // For each frame in video (starting from the 2nd)
        for(int k=1; k<interval_end-interval_beg+1; k++)
        {
            // Remove extension
            size_t ext_idx = img_format.find_last_of(".");
            string img_prefix = img_format.substr(0, ext_idx);
            sprintf(name, (string("%s") + img_prefix).c_str(), out_dir.c_str(), k+interval_beg-1);

            bool bSuccess = cap.read(frame1);
            //imshow("Frame", frame1);
            //waitKey();
            if(!bSuccess)
            {
                cout << "Cannot read frame " << name << "!" << endl;
                return 1;
            }
            else
            {
                if(!silence)
                {
                    cout << "Outputting " << name << endl;
                }
                cvtColor(frame1, frame1, CV_BGR2GRAY);

                // Upload images to GPU
                g_frame0 = GpuMat(frame0); // Has an image in format CV_32FC1
                g_frame1 = GpuMat(frame1); // Has an image in format CV_32FC1

                // Convert to float
                g_frame0.convertTo(gf_frame0, CV_32F, 1.0/255.0);
                g_frame1.convertTo(gf_frame1, CV_32F, 1.0/255.0);

                // Prepare receiving variable
                g_flow = GpuMat(frame0.size(), CV_32FC2);

                // Perform Brox optical flow
                if(proc_type.compare("tvl1") == 0)
                {
                    tvl1->calc(gf_frame0, gf_frame1, g_flow);
                }
                else
                {
                    brox->calc(gf_frame0, gf_frame1, g_flow);
                }
                vector<double> mm_frame = writeFlowJpg(name, g_flow);
                if(visualize)
                {
                    showFlow("Flow", g_flow);
                    waitKey(30);
                }

                mm.push_back(mm_frame);
                frame1.copyTo(frame0);
            }
        }
        if (!silence)
        {
            cout << "Outputting " << output_mm << endl;
        }
        writeMM(output_mm, mm);
        cap.release();
    }
    else
    {
        cout << "Video " << input_name << " cannot be opened." << endl;
        return 1;
    }
}

int extractCPUFlows(const string input_dir, const string img_format,
        const string out_dir, const string proc_type,
        string output_mm, const int interval_beg, int interval_end,
        const bool visualize, const bool silence)
{
    // Declare useful variables
    Mat frame0, frame1;
    char name[200];
    vector<vector<double> > mm;

    string input_name = input_dir + img_format;
    if(!silence)
    {
        cout << "Extracting flow from [" << input_name << "] using CPU." << endl;
    }

    VideoCapture cap(input_name);
    if(cap.isOpened())
    {
        int noFrames = cap.get(CV_CAP_PROP_FRAME_COUNT);
        if(interval_end == -1)
        {
            interval_end = noFrames;
        }
        if(!silence)
        {
            cout << "Total number of frames: " << noFrames << endl;
            cout << "Extracting interval [" << interval_beg  << "-" << interval_end << "]" << endl;
        }
        // cap.set(CV_CAP_PROP_POS_FRAMES, interval_beg-1);
        // Read first frame
        if(noFrames>0)
        {
            bool bSuccess = cap.read(frame0);
            if(!bSuccess)   { cout << "Cannot read frame!" << endl;   }
            else            { cvtColor(frame0, frame0, CV_BGR2GRAY);        }
        }
        // For each frame in video (starting from the 2nd)
        for(int k=1; k<interval_end-interval_beg+1; k++)
        {
            size_t ext_idx = img_format.find_last_of(".");
            string img_prefix = img_format.substr(0, ext_idx);
            sprintf(name, (string("%s") + img_prefix).c_str(), out_dir.c_str(), k+interval_beg-1);
            bool bSuccess = cap.read(frame1);
            if(!bSuccess)   {
                cout << "Cannot read frame " << name << "!" << endl;
                return 1;
            }

            else
            {
                if(!silence)
                {
                    cout << "Outputting " << name << endl;
                }
                cvtColor(frame1, frame1, CV_BGR2GRAY);

                // Convert to float
                frame0.convertTo(frame0, CV_32F, 1.0/255.0);
                frame1.convertTo(frame1, CV_32F, 1.0/255.0);

                // Prepare receiving variable
                Mat flow = Mat(frame0.size(), CV_32FC2);

                calcOpticalFlowFarneback(frame0, frame1, flow, 0.5, 3, 3, 3, 5, 1.1, 0);
                vector<double> mm_frame = writeFlowJpg(name, flow);

                if(visualize)
                {
                    showFlow("Flow", flow);
                    waitKey(30);
                }

                mm.push_back(mm_frame);
                frame1.copyTo(frame0);

            }
        }
        if(!silence)
        {
            cout << "Outputting " << output_mm << endl;
        }
        writeMM(output_mm, mm);
        cap.release();
    }
    else
    {
        cout << "Video " << input_name << " cannot be opened." << endl;
        return 1;
    }
}

