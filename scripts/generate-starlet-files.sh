#!/bin/bash

# check path
if [ -z "$1" ]
then
   echo "installation path is empty";
   exit 1
fi

./GeneratePicolibcCrossFile.sh --target-arch=$DEVKITARM/bin/arm-none-eabi --cpu-family=arm --endianness=big --cpu=arm926ej-s --system=starlet \
    --cflag=-mbig-endian --cflag=-mcpu=arm926ej-s --cflag=-nostdlib --cflag=-fno-builtin --cflag=-Wall --cflag=-Wextra --cflag=-Os \
    --cflag=-fomit-frame-pointer --cflag=-ffunction-sections --cflag=-pipe --cflag=-g --cflag=-Wconversion --cflag=-D__STARLET__ \
    > cross-arm-wii-eabi.txt

installPath=$(realpath $1)

echo -e "#!/bin/sh\n#\nexec \"\$(dirname \"\$0\")\"/do-configure arm-wii-eabi \\
\t-Dio-c99-formats=true -Dio-long-long=true -Dio-pos-args=true -Dtests=false -Dmultilib=false -Dpicocrt=false -Dpicocrt-lib=false -Dsemihost=false \\
\t-Dnewlib-retargetable-locking=true -Dnewlib-multithread=true -Dthread-local-storage=false -Dpicolib=true -Dspecsdir=none -Dtinystdio=true \\
\t-Dposix-console=false -Dposix-io=false -Datomic-ungetc=false -Dnewlib-global-atexit=false -Dnewlib-iconv-encodings=none -Dlite-exit=false \\
\t-Dlibdir=lib -Dincludedir=include -Dprefix=$installPath \"\$@\"" \
> do-arm-wii-configure