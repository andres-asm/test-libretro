# libretro test

A libretro core for testing libretro **frontends**. Instead of emulating a
system, it walks you through a series of test pages, next, next, next, and
reports what the frontend did.

It draws its own UI in software ([Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
with its `rawfb` renderer) into a normal libretro framebuffer, so it runs in any frontend that
can run a software-rendered core.

## Pages

1. **Environment**: what the frontend answered to the core's environment calls
   (log interface, pixel formats, directories, input bitmasks, language, ...),
   plus which core options version it reported, which options API the core
   used and the option values it got. Answers that break the libretro spec are
   shown in red.
2. **Video**: color bars and red/green/blue/grey ramps in the active pixel
   format. The framebuffer pitch is deliberately wider than the image, so a
   frontend that ignores `pitch` shows a slanted picture. Also shows the
   current size and frame rate, and how many `SET_GEOMETRY` /
   `SET_SYSTEM_AV_INFO` calls the size and timing options made.
3. **Input**: every joypad button and both analog sticks of controller 1, live,
   plus the pointer. Checks that bitmask reads match per-button reads.
4. **Audio**: 440 Hz tone, left / right / both, through either the batch or the
   single-sample callback. Starts with channels **off**: pick a channel with
   D-pad Left/Right.
5. **Save RAM**: a text field stored in the core's save RAM
   (`RETRO_MEMORY_SAVE_RAM`, 60 bytes: magic, text, checksum). Type something,
   quit, start the core again: the page shows whether the frontend restored it
   (green), left it empty (grey) or loaded the wrong bytes (red, corrupted).
   Typing needs the frontend's keyboard callback (`SET_KEYBOARD_CALLBACK`).
6. **Savestates**: a state holds the frame counter, the current page and every
   page's result. Loading a state must make the frame counter jump back and
   return to the page it was saved on, with those results. The page shows how
   many states were saved and loaded, and where.
7. **Summary**: results, then the core asks the frontend to shut down
   (`RETRO_ENVIRONMENT_SHUTDOWN`).

Every result is also written to the frontend's log.

## Controls

| Input | Action |
|-------|--------|
| Hold Start (1 s) | Pass this page, go to the next |
| Hold Select (1 s) | Fail this page, go to the next |
| L | Previous page |
| Mouse / pointer | Click Back, Fail, Pass (always at the top of the page) |
| D-pad Left/Right, Up | Audio page: channels, callback type |
| Keyboard | Save RAM page: type into the text field (the frontend must send key events; in Multitap's test frontend, turn on game focus first) |

## Core options

| Option | Values | Applied |
|--------|--------|---------|
| `libretro_test_pixel_format` | `XRGB8888` (default), `RGB565`, `0RGB1555` | When the core is loaded again |
| `libretro_test_width` | `640` (default), `720` | Right away, with `SET_GEOMETRY` (fits the reported maximum size) |
| `libretro_test_height` | `480` (default), `600`, `720` | Right away, with `SET_SYSTEM_AV_INFO`: the core reports only the current height as its maximum, so a taller image needs a new maximum |
| `libretro_test_fps` | `60` (default), `50`, `40`, `30` | Right away, with `SET_SYSTEM_AV_INFO`. Audio stays 48000 Hz, so each frame carries 48000 / fps samples (800 / 960 / 1200 / 1600) |

The image always has square pixels, so the reported aspect ratio is width /
height (4:3 at 640x480, 3:2 at 720x480, and so on): a frontend that stretches
by pixel count instead of the aspect ratio shows squeezed or stretched text.

Options are defined as core options v2 (category "Video") and sent in the
newest format the frontend reports with `GET_CORE_OPTIONS_VERSION`: v2, v1, or
`SET_VARIABLES` when the frontend doesn't answer (the default value is moved
first, as v0 requires). The Environment page shows which one was used.

## What it exercises

| libretro feature | Where |
|------------------|-------|
| `SET_PIXEL_FORMAT` (all three formats), `pitch` | Environment, Video |
| `GET_CORE_OPTIONS_VERSION`, `SET_CORE_OPTIONS_V2` / `SET_CORE_OPTIONS` / `SET_VARIABLES`, `GET_VARIABLE`, `GET_VARIABLE_UPDATE` | Environment, options |
| `SET_GEOMETRY`, `SET_SYSTEM_AV_INFO` | Video (size and frame rate options) |
| Joypad, bitmask, analog, pointer input | Input |
| `audio_sample` and `audio_sample_batch` | Audio |
| `SET_KEYBOARD_CALLBACK`, `RETRO_MEMORY_SAVE_RAM` | Save RAM |
| `retro_serialize` / `retro_unserialize` | Savestates |
| `SHUTDOWN` | Summary |

## Building

```sh
git clone --recursive https://github.com/andres-asm/test-libretro.git
cd test-libretro
cmake -S . -B build -G Ninja
cmake --build build
# -> build/test_libretro.so (.dll / .dylib)
```

Options: `TEST_LIBRETRO_WARNINGS_AS_ERRORS`, `TEST_LIBRETRO_SANITIZE` (ASan + UBSan).

When built as part of Multitap (as the `hosts/host-libretro/test-libretro`
submodule), those default to Multitap's own settings.

## Dependencies

Git submodules in `deps/`, never edited:

| Submodule | Pinned at | Used for | License |
|-----------|-----------|----------|---------|
| `deps/nuklear` | `bab1849` | `nuklear.h`, `demo/rawfb/nuklear_rawfb.h` | Public domain / MIT; rawfb: MIT |
| `deps/libretro-common` | `52781c4` | `include/libretro.h` only | MIT |

Note: upstream `nk_rawfb_shutdown()` doesn't free the font atlas;
`src/nuklear_impl.c` works around it.

## License

MIT, see [`LICENSE`](LICENSE).
