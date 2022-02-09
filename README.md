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

## Keyboard shortcuts

There are a bunch of keyboard shortcuts to make your life easier. Most of them can also be accessed through the menu.

 - `Up/Down`: Switch to the next higher / lower rendition
 - `Space`: Pause/Resume playback
 - `Right`: Go to the next frame when paused
 - `Ctrl+F`: Toggle full screen
 - `Ctrl+G`: Jump to segment number
 - `Ctrl+S`: Toggle scaling of video to the window size
 - `Ctrl+D`: Show thread debug ingo
 - `Ctrl+P`: Show graph debug info

## JSON Manifest Files

The player will need to know what to play from what source. For this we use a small JSON based manifest file that describes where all the VVC segments can be downloaded. Here is an example. There are also some hard coded manifests which can be directly opened from "```File -> Bitmovin Streams```".

📝 Try it out! Save this to a JSON file and open it.

```
{
  "Name": "Sprite Fright",
  "NrSegments": 629,
  "PlotMaxBitrate": 400000,
  "Renditions": [
    {
      "Name": "430p",
      "Resolution": "1026x430",
      "Fps": 24,
      "Url": "https://d2g8oy21og5mdp.cloudfront.net/vvcBlogPostDemo/SpriteFright/video/430-600000/segment-%i.vvc"
    },
    {
      "Name": "536p",
      "Resolution": "1280x536",
      "Fps": 24,
      "Url": "https://d2g8oy21og5mdp.cloudfront.net/vvcBlogPostDemo/SpriteFright/video/536-900000/segment-%i.vvc"
    },
    {
      "Name": "640p",
      "Resolution": "1582x640",
      "Fps": 24,
      "Url": "https://d2g8oy21og5mdp.cloudfront.net/vvcBlogPostDemo/SpriteFright/video/640-1200000/segment-%i.vvc"
    },
    {
      "Name": "858p",
      "Resolution": "2048x858",
      "Fps": 24,
      "Url": "https://d2g8oy21og5mdp.cloudfront.net/vvcBlogPostDemo/SpriteFright/video/858-2000000/segment-%i.vvc"
    }
  ]
}
```

Some notes: 
 - `NrSegments`: The segment index will iterate from 0 to `NrSegments - 1`
 - `PlotMaxBitrate`: This value is just used to scale the bitrate plot which you can activate in the player. It has no immediate influence on playback.
 - `Url`: For each rendition a URL must be provided where the file can be downloaded from. This can be a link (starting with `http` or `https`) or it can be a path on the local filesystem. It must contain a `%i` indicator which will be replaced by the segment index.
