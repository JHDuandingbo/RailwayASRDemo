#!/bin/bash
#convert pcm to wav
#16bit 16K
target_dir=$1
for pcm_file in $(ls $target_dir/*.pcm)
do
#pcm_file=$1
echo ffmpeg -f s16le -acodec pcm_s16le -ar 16000  -ac 1 -i $pcm_file "$target_dir/$(basename $pcm_file pcm)wav"
ffmpeg -f s16le -acodec pcm_s16le -ar 16000  -ac 1  -i $pcm_file  -y "$target_dir/$(basename $pcm_file pcm)wav"
done
