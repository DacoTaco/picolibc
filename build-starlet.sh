#!/bin/bash

#generate build configuration
installationPath=$(realpath $1)
curpath=$(realpath $(pwd))
cd scripts
printf "generating configurations..."
./generate-starlet-files.sh $installationPath
ret=$?
cd $curpath

if [ "$ret" != 0 ]
then
     printf "failed to generate configurations!\n"
     exit $ret
fi

printf "done\n"
echo configuring build ...
rm -rf ./build-wii
mkdir build-wii
cd build-wii
../scripts/do-arm-wii-configure
ret=$?
if [ "$ret" != 0 ]
then
     printf "failed to configure build!\n"
     cd $curpath
     exit $ret
fi

echo building picolibc...
ninja
ninja install