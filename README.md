Mod Library
===========

Mod Library is a database for managing and searching your favourite music
modules. Thanks to libopenmpt, it supports a wealth of different module formats.

Alpha Stage!
------------

This software is currently in a very early development stage. Many things are
still expected to change. Since there has been no "official" release yet, you
should not expect that the database schema remains stable until that release.

Later versions of the software will naturally support schema upgrades (the code
for this does already exist), but until the first release, the schema version
will stay at version 1. In the worst case, you will have to delete the database
file and recreate your module database.  

Dependencies
------------

Mod Library is written in C++ using Visual Studio 2015. It should also work on
various other compilers on operating systems other than Windows, but this is
currently untested.
Mod Library has the following external dependencies:

 -  Qt 5.6 or newer (https://www.qt.io/download/)
 
 -  libopenmpt (https://lib.openmpt.org/)
 
    The Visual Studio solution assumes this to be placed in the folder
    lib/libopenmpt/

 -  PortAudio (http://portaudio.com/)
 
    The Visual Studio solution assumes this to be placed in the folder
    lib/libopempt/include/portaudio/ as the libopenmpt Windows package already
    comes with its own PortAudio package.

 -  KissFFT (https://sourceforge.net/projects/kissfft)
 
    The Visual Studio solution assumes this to be placed in the folder
    lib/kiss_fft/

 -  Chromaprint (https://acoustid.org/chromaprint)
 
    The Visual Studio solution assumes this to be placed in the folder
    lib/chromaprint/

Contact
-------

Mod Library was created by Johannes Schultz.
You can contact me through my websites:
 -  https://sagagames.de/
 -  https://sagamusix.de/