#!/bin/csh

if ( "$1" == "SUB" ) then
  shift
  qsub -cwd -sync y -S /bin/csh -l arch=lx-amd64 ./jenkins-build.sh $*
  set code = $status
  if ( -e build-$1/build-env ) cat build-$1/build-env
  if ( -e build-$1/stdio ) cat build-$1/stdio
  exit ${code}
endif

# Pull in MIPS setup to get the module command
source /mips/tools/mips/etc/cshrc

# We need a couple of modules from /home/pburton
module use /home/pburton/modulefiles

# Load modules we need for the build
module load binutils/2.27
module load python/2.7.6
module load swig/3.0.12
module load srecord/1.64

# Handle arguments
set cfg = $1
set cross = $2

# Create build directory
set dir = "build-${cfg}"
mkdir ${dir}

# HACK: Some grid machines fail builds of pylibfdt looking for a 'gcc44'
# executable which I can find no mention of in their environment or in the
# source. Work around it by placing a symlink to gcc in the $PATH.
set bin = "${dir}/build-bin"
mkdir ${bin}
ln -sv `which gcc` ${bin}/cc
ln -sv `which gcc` ${bin}/gcc44
setenv PATH "${bin}:${PATH}"

# Point the build system at our cross compiler
setenv CROSS_COMPILE ${cross}

# Record the environment for later inspection
env >${dir}/build-env

# Configure U-Boot
make O=${dir} ${cfg} >&${dir}/stdio

# Build!
make O=${dir} V=1 -j -l 2 >>&${dir}/stdio
