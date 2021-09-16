#!/bin/bash
#
# get a list of png file with *"96px.png"
#
find | grep 96px.png > 96px-list.txt
#
# read file list line by line an convert it to
# 32x32px, 48x48px and 64x64px
#
while read LINE
do
	echo $LINE
	NAME=`echo $LINE | cut -d"9" -f1`
	DEST64=`echo $NAME`64px.png
	DEST48=`echo $NAME`48px.png
	DEST32=`echo $NAME`32px.png
	convert $LINE -resize 64x64 $DEST64
	convert $LINE -resize 48x48 $DEST48
	convert $LINE -resize 32x32 $DEST32
done < 96px-list.txt
#
# get a file list with "px.png"
#
find | grep "px.png" > png-list.txt
#
# read file line by line an convert files
# to c files with online image converter
# from LVGL.io
#
while read LINE
do
	CNAME=`echo $LINE | cut -d"." -f2`
	FILE=${CNAME##*/}
	echo "$LINE -> $CNAME.c ($FILE)"
	curl POST -F img_file=@$LINE -F name=$FILE -F cf=true_color_alpha -F format=c_array -F submit=Convert https://lvgl.io/tools/img_conv_core.php > .$CNAME.c
done < png-list.txt
#
# get a list of png file with *"96px.png"
#
find | grep px.c > pxc-list.txt

while read LINE
do
	FILE=$(basename $LINE)
	echo "$LINE -> $FILE"
	cp $LINE ../../src/gui/images/
done < pxc-list.txt

rm *.txt
