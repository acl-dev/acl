#!/bin/sh
rm -f jni
rm -rf obj
ln -s src jni
cd jni

export app_abi="armeabi armeabi-v7a x86"
export app_platform=android-14
export ndk_toolchain_version=4.6
export app_stl=gnustl_static

ndk-build
#~/Library/Android/android-ndk-r9d/ndk-build

cd ..
