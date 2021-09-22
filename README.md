# vvDecPlayer

This is a POC player for VVC that can playback segmented VVC streams from a local or remote source. The player is built in parts like this:

 - The downloader is responsible for downloading segments. Download from http/https sources is supported as well as having the files in a local folder. The downloader has a fixed number of segments it can hold in memory so that it can download in advance.
 - A parser thread parses the VVC annex B bitstream to determine the resolution / nr of frames in each segment. It also parses the nr of bytes per frame for visualization.
 - The vvDecLib decoder is opened in a thread and waits for segments to be available to decoder. If a segment is available it decodes all frames (YUV) into memory.
 - A conversion thread takes care of the conversion of all frames from YUV to RGB.
 - Finally there is a timer running in the view widget which tries to update the view 'FPS' times per second with a new converted frame from the buffer.

## How to build

The only requirement for building this player is Qt. For running it, you will also require the [Fraunhofer Versatile Video Decoder (VVdeC)
](https://github.com/fraunhoferhhi/vvdec) library. You can get precompiled libraries [here](https://github.com/ChristianFeldmann/vvdec/releases).

We build using qmake so it depends a bit on what os you build on. A good reference is the [Github Actions YML](.github/workflows/Build.yml) file where we build on Ubuntu/Windows/MacOs.

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

## JSON Manifest Files

The player will need to know what to play from what source. For this we use a small JSON based manifest file that describes where all the VVC segments can be downloaded. Here is an example. 

📝 Try it out! Save this to a JSON file and open it.

```
{
  "Name": "Sintel Bitmovin S3",
  "NrSegments": 53,
  "PlotMaxBitrate": 400000,
  "Renditions": [
    {
      "Name": "360p",
      "Resolution": "640x360",
      "Fps": 24,
      "Url": "https://bitmovin-api-eu-west1-ci-input.s3.amazonaws.com/feldmann/VVCDemo/SintelTrailer1080p/360-200000/segment-%i.vvc"
    },
    {
      "Name": "576p",
      "Resolution": "1024x576",
      "Fps": 24,
      "Url": "https://bitmovin-api-eu-west1-ci-input.s3.amazonaws.com/feldmann/VVCDemo/SintelTrailer1080p/576-400000/segment-%i.vvc"
    },
    {
      "Name": "720p",
      "Resolution": "1280x720",
      "Fps": 24,
	  "Url": "https://bitmovin-api-eu-west1-ci-input.s3.amazonaws.com/feldmann/VVCDemo/SintelTrailer1080p/720-750000/segment-%i.vvc"
    },
    {
      "Name": "1080p",
      "Resolution": "1920x1080",
      "Fps": 24,
      
	  "Url": "https://bitmovin-api-eu-west1-ci-input.s3.amazonaws.com/feldmann/VVCDemo/SintelTrailer1080p/1080-1500000/segment-%i.vvc"
    }
  ]
}
```

Some notes: 
 - `NrSegments`: The segment index will iterate from 0 to `NrSegments - 1`
 - `PlotMaxBitrate`: This value is just used to scale the bitrate plot which you can activate in the player. It has no immediate influence on playback.
 - `Url`: For each rendition a URL must be provided where the file can be downloaded from. This can be a link (starting with `http` or `https`) or it can be a path on the local filesystem. It must contain a `%i` indicator which will be replaced by the segment index.
