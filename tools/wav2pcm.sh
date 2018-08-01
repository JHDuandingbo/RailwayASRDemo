#!/bin/bash

#16bit 16K
#convert wav to pcm
target_dir=$1

for wav_file in $(ls $target_dir/*.wav)
do
#wav_file=$1
echo  ffmpeg -i $wav_file -f s16le -acodec pcm_s16le -ar 16000  -ac 1  "$(basename $wav_file wav)pcm"
ffmpeg -i $wav_file -f s16le -acodec pcm_s16le -ar 16000  -ac 1  "$(basename $wav_file wav)pcm"
done
