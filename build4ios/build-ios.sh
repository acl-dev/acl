#!/bin/bash

set -e

CURRENT_DIR=${PWD}

cd $CURRENT_DIR

if  [ ! -d "$CURRENT_DIR/dist" ]; then
    mkdir -p $CURRENT_DIR/dist
    mkdir -p $CURRENT_DIR/dist/include
    mkdir -p $CURRENT_DIR/dist/lib
else
    rm -rf $CURRENT_DIR/dist
    mkdir -p $CURRENT_DIR/dist
    mkdir -p $CURRENT_DIR/dist/include
    mkdir -p $CURRENT_DIR/dist/lib
fi

TARGET_FILE_PATH=$CURRENT_DIR/dist

XCODEPROJ_FILE_PATH=$CURRENT_DIR/xcodeproj-path

while read ProjectSub
do
    echo "${ProjectSub} build ..."

    #TargetPath lib_${ProjectSub}/${ProjectSub}

    echo "Target xcodeproj path is lib_${ProjectSub}"

    cd $CURRENT_DIR/../lib_${ProjectSub}

    if [ -d "build" ]; then
        rm -rf build
    fi

    echo "***************************************************"
    echo "***************iPhoneOS.platform*******************"

    #iPhoneOS.platform

    DEVPATH="";
    # Set the main iPhone developer directory, if not set
    if test "x${DEVPATH}" = "x"; then
    DEVPATH=/Applications/XCode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer
    if test ! -d $DEVPATH; then
        DEVPATH=/Developer/Platforms/iPhoneOS.platform/Developer
    fi
    echo "$F: DEVPATH is not specified, using ${DEVPATH}"
    fi

    # Make sure $DEVPATH directory exist
    if test ! -d $DEVPATH; then
        echo "$F error: directory $DEVPATH does not exist. Please install iPhone development kit"
        exit 1
    fi

    # Choose SDK version to use
    if test "" = ""; then
    #if test "$IPHONESDK" = ""; then
    # If IPHONESDK is not set, use the latest one
    for f in `ls $DEVPATH/SDKs/`;
        do echo $f | sed 's/\(.sdk\)//';
    done | sort | tail -1 > tmpsdkname
        IPHONESDK=`cat tmpsdkname`.sdk
    rm -f tmpsdkname
    SDKPATH=${DEVPATH}/SDKs/${IPHONESDK}
    echo "$F: IPHONESDK is not specified, choosing ${IPHONESDK}"
    elif test -d ${IPHONESDK}; then
    # .. else if IPHONESDK is set and it points to a valid path, just use it
        SDKPATH=${IPHONESDK}
    else
    # .. else assume the SDK name is used.
        SDKPATH=${DEVPATH}/SDKs/${IPHONESDK}
    fi

    echo ${IPHONESDK%.sdk} | awk '{print tolower($0);}' > tempFile
        IPHONESDK=`cat tempFile`
    rm tempFile

    xcodebuild -sdk $IPHONESDK -configuration Release
    #cp build/Release-iphoneos/*.a $TARGET_FILE_PATH/lib
    #rm -rf build

    echo "***************************************************"
    echo "************iPhoneSimulator.platform***************"

    #iPhoneSimulator.platform

    DEVPATH="";
    # Set the main iPhone developer directory, if not set
    if test "x${DEVPATH}" = "x"; then
    DEVPATH=/Applications/XCode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer
    if test ! -d $DEVPATH; then
        DEVPATH=/Developer/Platforms/iPhoneSimulator.platform/Developer
    fi
    echo "$F: DEVPATH is not specified, using ${DEVPATH}"
    fi

    # Make sure $DEVPATH directory exist
    if test ! -d $DEVPATH; then
        echo "$F error: directory $DEVPATH does not exist. Please install iPhone development kit"
        exit 1
    fi

    # Choose SDK version to use
    if test "" = ""; then
    #if test "$IPHONESDK" = ""; then
    # If IPHONESDK is not set, use the latest one
    for f in `ls $DEVPATH/SDKs/`;
        do echo $f | sed 's/\(.sdk\)//';
    done | sort | tail -1 > tmpsdkname
        IPHONESDK=`cat tmpsdkname`.sdk
    rm -f tmpsdkname
    SDKPATH=${DEVPATH}/SDKs/${IPHONESDK}
    echo "$F: IPHONESDK is not specified, choosing ${IPHONESDK}"
    elif test -d ${IPHONESDK}; then
    # .. else if IPHONESDK is set and it points to a valid path, just use it
        SDKPATH=${IPHONESDK}
    else
    # .. else assume the SDK name is used.
        SDKPATH=${DEVPATH}/SDKs/${IPHONESDK}
    fi

    echo ${IPHONESDK%.sdk} | awk '{print tolower($0);}' > tempFile
        IPHONESDK=`cat tempFile`
    rm tempFile

    xcodebuild -sdk $IPHONESDK -configuration Release
    #cp build/Release-iphonesimulator/*.a $TARGET_FILE_PATH/lib
    #rm -rf build

    lipo -create \
	build/Release-iphoneos/lib${ProjectSub}.a \
	build/Release-iphonesimulator/lib${ProjectSub}.a \
	-output $TARGET_FILE_PATH/lib/lib${ProjectSub}.a

    echo "***************************************************"
    #echo "${ProjectSub} build success"

    #remove all build file
    rm -rf build

#    if [ -d "$CURRENT_DIR/../lib_${ProjectSub}" ]; then
#
#        cd $CURRENT_DIR/../lib_${ProjectSub}
#        if [ -d "include" ]; then
#            mkdir -p $TARGET_FILE_PATH/include
#            cp -r include/* $TARGET_FILE_PATH/include
#        fi

#    elif test "$ProjectSub" = "acl"; then
     if test "$ProjectSub" = "acl"; then

        #subPath acl path is lib_acl
        if [ -d "$CURRENT_DIR/../lib_acl" ]; then
            cd $CURRENT_DIR/../lib_acl
            if [ -d "include" ]; then
                mkdir -p $TARGET_FILE_PATH/include/acl
                cp -r include/* $TARGET_FILE_PATH/include/acl
            fi
        fi

    elif test "$ProjectSub" = "acl_cpp"; then

        #subPath acl_cpp path is lib_acl_cpp
        if [ -d "$CURRENT_DIR/../lib_acl_cpp" ]; then
            cd $CURRENT_DIR/../lib_acl_cpp
            if [ -d "include" ]; then
                mkdir -p $TARGET_FILE_PATH/include
                cp -r include/* $TARGET_FILE_PATH/include
            fi
        fi

    elif test "$ProjectSub" = "protocol"; then

        #subPath protocol path is lib_protocol
        if [ -d "$CURRENT_DIR/../lib_protocol" ]; then
            cd $CURRENT_DIR/../lib_protocol
            if [ -d "include" ]; then
                mkdir -p $TARGET_FILE_PATH/include/protocol
                cp -r include/* $TARGET_FILE_PATH/include/protocol
            fi
        fi
    fi

done < $XCODEPROJ_FILE_PATH #read xcodeproj

