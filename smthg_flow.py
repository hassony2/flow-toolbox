import os
import subprocess

from tqdm import tqdm


def mkdir(path):
    if not os.path.exists(path):
        os.makedirs(path)


smthg_root = '/home/local2/yhasson/datasets/smthg-smthg/'
split_root = os.path.join(smthg_root, 'split-dataset')
flow_type = "brox"  # in [farn | tvl1 | brox]
dest_root = os.path.join(smthg_root, 'split-{}').format(flow_type)
for i in tqdm(range(1, 10), desc='clip-split'):
    split_path = os.path.join(split_root, str(i))
    split_folders = os.listdir(split_path)
    for video_name in tqdm(sorted(split_folders), desc='video'):
        video_path = os.path.join(split_root, str(i), video_name, "%05d.jpg")

        # Create destination folder
        dest_video_path = os.path.join(dest_root, str(i), video_name)
        mkdir(dest_video_path)

        # Launch flow script
        # success = subprocess.call(["./flow_video", video_path, "-o ",
        #                            dest_video_path, "-p ", flow_type])

        args = video_path + " -o " + dest_video_path + " -p " + flow_type + " -s 1"
        success = subprocess.call('./flow_video ' + args, shell=True)
        if success == 1:
            print("something went wrong for clip {}".format(video_name))
