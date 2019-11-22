#!/bin/sh
rm -f jni
rm -rf obj
ln -s src jni
cd jni
~/Library/Android/android-ndk-r9d/ndk-build
cd ..
