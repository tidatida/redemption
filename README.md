Master branch: [![Build Status from master](https://travis-ci.org/wallix/redemption.svg?branch=master)](https://travis-ci.org/wallix/redemption)

Future branch: [![Build Status from future](https://travis-ci.org/wallix/redemption.svg?branch=future)](https://travis-ci.org/wallix/redemption)


A RDP (Remote Desktop Protocol) proxy.

(A RDP client in `projects/ClientQtGraphicAPI`)

Support of:

- RDP client to RDP server and
- RDP client to VNC server


Dependencies
============

To compile ReDemPtion you need the following packages:
- libboost-tools-dev (contains bjam: software build tool) (http://sourceforge.net/projects/boost/files/boost/)
- libboost-test-dev (unit-test dependency)
- libssl-dev
- libkrb5-dev
- libgssglue-dev (unnecessary since Ubuntu 17.10)
- libsnappy-dev
- libpng12-dev
- libffmpeg-dev (see below)
- g++ >= 7.2 or clang++ >= 5.0 or other C++17 compiler

<!--Optionally:
- python (python-dev)-->

Submodule ($ `git submodule update --init`):
- https://github.com/wallix/program_options
- https://github.com/wallix/ppocr


## FFmpeg:

### Ubuntu:
- libavcodec-dev
- libavformat-dev
- libavutil-dev
- libswscale-dev
- libx264-dev
- libbz2-dev
<!-- ok with 53 (?) and 54 version-->
<!-- - libavcodec-ffmpeg56 -->
<!-- - libavformat-ffmpeg56 -->
<!-- - libavutil-ffmpeg54 -->
<!-- - libswscale-ffmpeg3 -->

### Other distros:
- https://github.com/FFmpeg/FFmpeg/archive/n2.5.11.tar.gz

And set environment variable (optionally)
- `export FFMPEG_INC_PATH=/my/ffmpeg/include/path`
- `export FFMPEG_LIB_PATH=/my/ffmpeg/library/path` (/!\\ without `/` terminal)
- `export FFMPEG_LINK_MODE=shared` (static or shared, shared by default)

### Note:

Disable ffmpeg with `NO_FFMPEG=1`.


## Environment variable setting

List with `sed -E '/\[ setvar/!d;s/.*\[ setvar ([^ ]+).*/\1/' jam/defines.jam`

    export FFMPEG_INC_PATH=$HOME/ffmpeg/includes
    bjam ....

Or

    FFMPEG_INC_PATH=$HOME/ffmpeg/includes FFMPEG_LIB_PATH=... bjam ....

Or with `-s` to bjam

    bjam -s FFMPEG_INC_PATH=$HOME/ffmpeg/includes ...


Compilation
===========

Well, that's pretty easy once you installed the required dependencies.

Just run (as user):

$ `bjam` or `bjam toolset=gcc`, `bjam toolset=clang` or `bjam toolset=your-compiler` (see http://www.boost.org/build/doc/html/bbv2/overview/configuration.html)

Verbose tests:

$ `export REDEMPTION_LOG_PRINT=1`\
$ `bjam test`

or

$ `REDEMPTION_LOG_PRINT=1 bjam test`

/!\ `bjam tests` execute files directly in tests directory, but not recursively.

Compile executables without tests (as user):

$ `bjam exe libs`

and install (as administrator):

\# `bjam install`

Binaries are located in /usr/local/bin.

## Modes

$ `bjam [variant=]{release|debug|san}`

- `release`: default
- `debug`: debug mode (compile with `-g -D_GLIBCXX_DEBUG`)
- `san`: enable sanitizers: asan, lsan, usan


Run ReDemPtion
==============

To test it, executes:

$ `python tools/passthrough/passthrough.py`

\# `/usr/local/bin/rdpproxy -nf`
<!-- $ `./bin/${BJAM_TOOLSET_NAME}/${BJAM_MODE}/rdpproxy -nf` -->


Now, at that point you'll just have two servers waiting for connections
not much fun. You still have to run some RDP client to connect to proxy. Choose
whichever you like xfreerdp, rdesktop, remmina, tsclient on Linux or of course
mstsc.exe if you are on windows. All are supposed to work. If some problem
occurs just report it to us so that we can correct it.

Example with freerdp when the proxy runs on the same host as the client:

$ `xfreerdp 127.0.0.1`

A dialog box should open in which you can type a username and a password.
With default authhook at least internal services should work. Try login: bouncer
and password: bouncer, or login: card and password: card. To access your own
remote RDP hosts you'll of course have to configure them in authhook.py.
Hopefully at some time in the future these won't be hardcoded, but authhook.py
will access to some configuration file. If you want to provide such extensions
to current authhook.py, please contribute it, it will be much appreciated.

You can also bypass login dialog box and go directly to the RDP server by
providing a login and a password from command line.

$ `xfreerdp -u 'bouncer' -p 'bouncer' 127.0.0.1`


Generate target and lib/obj dependencies
========================================

When create a new test or when a target fail with link error:

`bjam targets.jam` for updated `targets.jam` file.

Or run `./tools/bjam/gen_targets.py > targets.jam`

Specific deps (libs, header, cpp, etc) in `./tools/bjam/gen_targets.py`.


Packaging
=========

    ./tools/packager.py --build-package

- `--force-target target`: target is a file in packaging/targets
- `--force-build`


Tag and Version
===============

    ./public/tools/packager.py --update-version 1.2.7 --git-tag --git-push-tag --git-push


FAQ
===

Q - Why do you use bjam for ReDemPtion instead of make, cmake, scons, etc ?
---------------------------------------------------------------------------

It is simple, more that could be thought at first sight, and bjam has the major
feature over make to keep source directories clean, all build related
informations for all architecture are kept together in bin directory.

The main drawback of bjam is the smaller user base.

But keeping in mind the complexity of make (or worse autotools + make), bjam is
a great help. We also used to have an alternative cmake build system, but it was
more complex than bjam and not maintained, so was removed.
