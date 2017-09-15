import os

from matplotlib import pyplot as plt
from matplotlib import animation
import numpy as np
from tqdm import tqdm
from PIL import Image


def update_image(flow_idx, axes, flow_path_template):
    flow_path = flow_path_template.format(int(flow_idx) + 1)
    img = np.array(Image.open(flow_path))
    axes.imshow(img, cmap="gray")
    return axes


smthg_root = '/home/local2/yhasson/datasets/smthg-smthg'
flow_type = 'tvl1'  # [farn | brox | tvl1]
split_folder = 'split-{}/1'.format(flow_type)
split_path = os.path.join(smthg_root, split_folder)
videos = os.listdir(split_path)
gif_folder = os.path.join(smthg_root, 'flow_gifs', flow_type)
for video in tqdm(sorted(videos)):
    video_path = os.path.join(split_path, video)
    # Infer number of frames from images
    video_frame_nb = int((len(os.listdir(video_path)) - 1)/2)
    fig, axes = plt.subplots(1, 1)

    flow_path_template_x = os.path.join(video_path, '{:05d}x.jpg')
    flow_path_template_y = os.path.join(video_path, '{:05d}y.jpg')
    # Create animations
    animx = animation.FuncAnimation(fig, update_image, video_frame_nb,
                                    fargs=(axes,
                                           flow_path_template_x),
                                    interval=200, repeat=False)
    animy = animation.FuncAnimation(fig, update_image, video_frame_nb,
                                    fargs=(axes,
                                           flow_path_template_y),
                                    interval=200, repeat=False)
    # Save animations
    anim_name_x = '{}x.gif'.format(video)
    anim_path_x = os.path.join(gif_folder, anim_name_x)
    animx.save(anim_path_x, dpi=80, writer='imagemagick')
    anim_name_y = '{}y.gif'.format(video)
    anim_path_y = os.path.join(gif_folder, anim_name_y)
    animy.save(anim_path_y, dpi=80, writer='imagemagick')
    print('saved to {}'.format(anim_path_y))
