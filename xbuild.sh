#!/bin/sh
#xmake f -p android --ndk=/data/zsx/android-ndk-r16b -k shared -c -D; xmake
RELEASE_PATH=android/lib/armeabi-v7a
BUILD_PATH=build/release/armv7-a
mkdir -p $RELEASE_PATH
rm -rf .xmake
rm -r build/release
xmake f -k shared -p android --ndk=/data/zsx/android-ndk-r16b/ -c --ndk_cxxstl=gnustl_shared; xmake
cp -a $BUILD_PATH/*.so $RELEASE_PATH/

rm -rf .xmake
rm -r build/release
xmake f -k static -p android --ndk=/data/zsx/android-ndk-r16b -c --ndk_cxxstl=gnustl_shared; xmake
cp $BUILD_PATH/*.a $RELEASE_PATH/
