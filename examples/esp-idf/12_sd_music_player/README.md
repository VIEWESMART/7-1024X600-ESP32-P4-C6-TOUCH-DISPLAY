# SD card music player

This folder plays **MP3** and **WAV** files from the SD card root. The normal screen is the player page. Swipe up from the bottom to open the track list. It does not play video.

Flashing replaces whichever of the other two programs is on the board:

| Folder | What it does |
| --- | --- |
| `sd_music_player` | This example. Plays MP3 and WAV |
| `10_7inch_lvgl_demo_v9` | Split-screen video |
| `sd_video_fullscreen` | Full-screen video |

Display and SD drivers are in this folder at `components/esp32_p4_s070_wba`. This example stands alone and does not need the video projects next to it.

## What you need

- The 10.7-inch board: **ESP32-P4**, PSRAM, **16 MB** flash
- Panel: **JD9165, 1024×600, RGB565**
- Speaker: **NS4168**. MP3 is decoded on the chip into PCM. The board has no hardware MP3 decoder
- A **FAT32** SD card. Do not use exFAT
- **ESP-IDF v5.5.4**. Do not use v5.4 or v6.0
- A USB serial port. Development used `COM3`

## Chip revision in this project

The default image is for **ESP32-P4 v3.1 or newer**, CPU **400 MHz**. The board used here is **v3.2**. A v3.x image and a v1.x image are not interchangeable.

The monitor should show:

```text
Chip rev:                 v3.2
Minimum supported chip rev: v3.1
```

### Running on a v1.3 chip

ESP-IDF v5.5.4 has no item named “v1.3”. Enable support for chips older than v3.0 and set the minimum to **Rev v1.0**. The image then allows **v1.0 through v1.99**, so **v1.3 boots**. A v3.2 board will not boot that image.

1. Open **this folder** `sd_music_player`. Press `F1` → `ESP-IDF: SDK Configuration Editor (menuconfig)`, or run `idf.py menuconfig`.
2. Go to `Component config → Hardware Settings → Chip revision`.
3. Enable `Select ESP32-P4 revisions <3.0 (No >=3.x Support)`.
4. Set the minimum revision to **Rev v1.0**.
5. Use **360 MHz**. Do not enable the experimental 400 MHz option.
6. Save, then:

```bat
idf.py fullclean
idf.py build
idf.py -p COM3 flash monitor
```

To return to v3.2, turn `<3.0` off, set the minimum to **Rev v3.1**, set the CPU to **400 MHz**, then `fullclean` and flash.

## Build and flash

The first build needs a network connection. Open this folder.

```bat
cd <full path of this folder>
idf.py set-target esp32p4
idf.py build
idf.py -p COM3 flash monitor
```

Leave the monitor with `Ctrl+]`.

## What to put on the SD card

- Format the card as **FAT32**. Put songs in the **root**, not in a subfolder.
- Long file names are enabled here, so `mysong.mp3` is fine. Avoid very long names and odd symbols.
- At most **32** tracks are listed.
- Two kinds of files work:
  - **MP3**, extension `.mp3`. A common 128 kbps, 44.1 kHz stereo file is fine. VBR also decodes. The total time is estimated from the bitrate at the start of the file, and seeking is an approximate file position.
  - **WAV**. It must be PCM, 16-bit, mono or stereo, about 8 kHz to 96 kHz. ADPCM, float WAV, and 24-bit WAV are skipped.
- AAC, FLAC, and WMA are ignored. Files in subfolders are ignored.
- `video.avi` is not used. Video files on the card are not played by this program.

Insert the card and power on. The first track starts by itself.

## What a successful run looks like

The screen lights up on a white player page:

- A circle in the center shows the first letter of the title, with short bars around it.
- The title is under the circle. The next line is **SD card**. The line under that looks like **MP3  44 kHz stereo** or **WAV  48 kHz stereo**.
- Below that: elapsed time, a gradient progress bar, and total time.
- At the bottom of the player: previous, play/pause, and next.
- The bottom edge of the white page says **ALL TRACKS** with a short dash.

When songs are present, the first one starts, the speaker plays, the progress bar moves, and the bars around the circle bounce. Volume is about 40% of the file level.

**Swipe up** from the player page. The white page slides away and a dark track list appears. Each row shows the title, **MP3** or **WAV**, and the duration. Tap a row to play it. The list stays open. **Swipe down** on the white strip at the top to hide the list again.

If the root has no usable song, the list says **No WAV or MP3 in the SD root.** and the title says **No music**. The program is not stuck.

The serial log prints how many audio files were found and the name now playing. If the sample rate changes, it also prints one I2S rate line.

## Notes

- This firmware replaces the video program. Flash a video folder again when you want video back.
- MP3 duration is only an estimate for VBR files. Dragging the bar seeks by file position, not by an exact frame.
- A WAV file changes the speaker clock, so 44.1 kHz and 48 kHz both play at their own speed.
- The end of a track starts the next one. After the last track, playback returns to the first.
- Changing tracks or seeking clears the speaker buffer first. The start of the new audio can hesitate for a fraction of a second.

## If something looks wrong

| What you see | What to do |
| --- | --- |
| Build cannot find the board component | Check that `components/esp32_p4_s070_wba` is inside this folder |
| Chip revision check failed | For v1.3, set minimum Rev v1.0. For v3.2, keep Rev v3.1. Then `fullclean` |
| White screen, no song, title No music | The root has no `.mp3` or usable `.wav`, or the card is not FAT32 |
| Some WAV files are skipped | They are not 16-bit PCM. Convert them to `pcm_s16le` |
| MP3 is listed but silent | Confirm the onboard NS4168, and that the file is not empty or damaged |
| Duration does not match a phone | Usually VBR. Playback still works. The bar follows file position |
| Swipe does not show the list | Swipe up from the **ALL TRACKS** edge. Tapping play does not open the list |
| Port is busy | Press `Ctrl+]` in the monitor, then flash |
