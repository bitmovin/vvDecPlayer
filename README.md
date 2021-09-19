# vvDecPlayer

This is a POC player for VVC that can playback segmented VVC streams from a local or remote source. The player is built in parts like this:

 - The downloader is responsible for downloading segments. Download from http/https sources is supported as well as having the files in a local folder. The downloader has a fixed number of segments it can hold in memory so that it can download in advance.
 - A parser thread parses the VVC annex B bitstream to determine the resolution / nr of frames in each segment. It also parses the nr of bytes per frame for visualization.
 - The vvDecLib decoder is opened in a thread and waits for segments to be available to decoder. If a segment is available it decodes all frames (YUV) into memory.
 - A conversion thread takes care of the conversion of all frames from YUV to RGB.
 - Finally there is a timer running in the view widget which tries to update the view 'FPS' times per second with a new converted frame from the buffer.

## How to build

The only requirement for building this player is Qt. For running it, you will also require the [Fraunhofer Versatile Video Decoder (VVdeC)
](https://github.com/fraunhoferhhi/vvdec) library. 

We build using qmake so it depends a bit on what os you build

### Linux / macOS

```
mkdir build
cd build
qmake ..
make
```

### Windows

```
mkdir build
cd build
qmake ..
nmake
```


