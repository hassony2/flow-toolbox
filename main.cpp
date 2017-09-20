#include "flow_video.hpp"

int main(int argc, const char* argv[])
{
    // Parse parameters and options
    string input_dir;              // name of the video file or directory of the image sequence
    string proc_type    = "farn";    // "farn", "brox" or "tvl1"
    string out_dir       = "./";     // directory for the output files
    int interval_beg      = 1;           // 1 for the beginning
    int interval_end    = -1;       // End (-1 for uninitialized, default set below)
    bool nested         = 0;        // boolean looping through nested dirs
    string img_format       = "%05d.jpg";    // "farn", "brox" or "tvl1"
    bool visualize      = 0;        // boolean for flow visualization
    bool silence        = 0;         // Remove logs
    string output_mm    = "";       // name of the minmax.txt file (default set below)

    const char* usage = "[-h] [-p <proc_type>] [-o <out_dir>] [-b <interval_beg>] [-e <interval_end>] [-v <visualize>] [-m <output_mm>] <input_dir>";
    string help  = "\n\n\nUSAGE:\n\t"+ string(usage) +"\n\n"
        "INPUT:\n"
        "\t<input_dir>\t \t: Path to video file or image directory \n"
        "OPTIONS:\n"
        "-h \t \t \t \t: Display this help message\n"
        "-p \t<proc_type>  \t[farn] \t: Processor type (farn, brox or tvl1)\n"
        "-o \t<out_dir> \t[./] \t: Output folder containing flow images and minmax.txt\n"
        "-b \t<interval_beg> \t[1] \t: Frame index to start (one-based indexing)\n"
        "-e \t<interval_end> \t[last] \t: Frame index to stop\n"
        "-v \t<visualize> \t[0] \t: Boolean for visualization of the optical flow\n"
        "-s \t<silence> \t[0] \t: Boolean for removing logs\n"
        "-m \t<output_mm> \t[<out_dir>/<basename(input_dir)>_minmax.txt] \t: Name of the minmax file.\n"
        "-n \t<nested> \t [0] \t whether to go through first level nested dirs \n"
        "-f \t<img_format> \t [%05d.jpg|video.avi] \t image format of input\n"
        "\n"
        "Notes:\n*GPU method: Brox, CPU method: Farneback.\n"
        "*Only <imagename>_%0xd.jpg (x any digit) is supported for image sequence input.\n\n\n";
// brox cpu
//fourcc check for image sequence detection, but not sure
    int option_char;
    while ((option_char = getopt(argc, (char **)argv, "hp:o:b:e:m:n:f:s:v:?")) != EOF)
    {
        switch (option_char)
        {  
            case 'p': proc_type      = optarg;       break;
            case 'o': out_dir        = optarg;       break;
            case 'b': interval_beg   = atoi(optarg); break;
            case 'e': interval_end   = atoi(optarg); break;
            case 'v': visualize      = atoi(optarg); break;
            case 's': silence        = atoi(optarg); break;
            case 'm': output_mm      = optarg;       break;
            case 'n': nested         = atoi(optarg); break;
	    case 'f': img_format         = optarg; break;
            case 'h': cout << help; return 0;        break;
            case '?': fprintf(stderr, "Unknown option.\nUSAGE: %s %s\n", argv[0], usage); return -1; break;
        }
    }
    // Retrieve the (non-option) argument
    if ( (argc <= 1) || (argv[argc-1] == NULL) || (argv[argc-1][0] == '-') )
    {
        fprintf(stderr, "No input name provided.\nUSAGE: %s %s\n", argv[0], usage);
        return -1;
    }
    else
    {
        input_dir = argv[argc-1];
    }

    if(out_dir.compare("") != 0) 
    {
        if(out_dir[out_dir.length()-1]!= '/') { out_dir = out_dir + "/"; } //and if last char not /
        char cmd[200];
        sprintf(cmd, "mkdir -p %s", out_dir.c_str());
        system(cmd);
    }
    if(output_mm.compare("") == 0)
    {
        output_mm = out_dir + "minmax.txt";
    }
    // Ensure that folder name ends with '/'
    if(input_dir[input_dir.length()-1]!= '/')
    {
	    input_dir = input_dir + "/"; 
    } //and if last char not /
    if(nested == 1)
    {
        vector<string> files;
        getdir(input_dir, files);
	cout << input_dir << endl;
        cout << "Extracting " <<  files.size() << " videos" << endl;
        // Go through inner folders
        for(int i=0; i < files.size(); i++)
        {
            cout << "Extracting file " << i<< ": " << files[i] << endl;
            string video = files[i];
            string video_output = out_dir + video + '/';
            char cmd[300];
            sprintf(cmd, "mkdir -p %s", video_output.c_str());
            system(cmd);
            if((proc_type.compare("tvl1") == 0) || (proc_type.compare("brox") == 0))
            {
                int extract_success = extractGPUFlows(input_dir, img_format,
						      video_output,
                                                  proc_type, output_mm,
                                                  interval_beg, interval_end,
                                                  visualize, silence);
                if(extract_success == 1)
                {
                    cout << "Failed reading video file" << video << endl;
                    return 1;
                }
            }
            else if(proc_type.compare("farn") == 0)
            {
                int extract_success = extractCPUFlows(input_dir, img_format,
			                        	video_output,
                                                  proc_type, output_mm,
                                                  interval_beg, interval_end,
                                                  visualize, silence);
                if(extract_success == 1)
                {
                    cout << "Failed reading video file " << video << endl;
                    return 1;
                }
            }
            else
            {
                cout << "proc_type " << proc_type << " not recognized (should be in [farn | brox | tvl1]" << endl;
                return  1;
            }
        }
    }
    else
    {
        if((proc_type.compare("tvl1") == 0) || (proc_type.compare("brox") == 0))
        {
            int extract_success = extractGPUFlows(input_dir, img_format, out_dir,
                                              proc_type, output_mm,
                                              interval_beg, interval_end,
                                              visualize, silence);
            if(extract_success == 1)
            {
                cout << "Failed extracting flow from " << input_dir << endl;
                return 1;
            }
        }
        else if(proc_type.compare("farn") == 0)
        {
            int extract_success = extractCPUFlows(input_dir, img_format,
			                          out_dir,
                                                  proc_type, output_mm,
                                                  interval_beg, interval_end,
                                                  visualize, silence);
            if(extract_success == 1)
            {
                cout << "Failed extracting flow from " << input_dir << endl;
                return 1;
            }
        }
        else
        {
            cout << "proc_type " << proc_type << " not recognized (should be in [farn | brox | tvl1]" << endl;
            return  1;
        }
    }

    return 0;
}
