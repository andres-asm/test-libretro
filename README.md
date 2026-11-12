# libretro test

A libretro core for testing libretro **frontends**. Instead of emulating a
system, it walks you through a series of test pages, next, next, next, and
reports what the frontend did.

It draws its own UI in software ([Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
with its `rawfb` renderer) into a normal libretro framebuffer, so it runs in any frontend that
can run a software-rendered core.

## Pages

1. **Environment**: what the frontend answered to the core's environment calls
   (log interface, pixel formats, directories, input bitmasks, ...). Answers
   that break the libretro spec are shown in red.
2. **Video**: color bars and red/green/blue/grey ramps in the active pixel
   format. The framebuffer pitch is deliberately wider than the image, so a
   frontend that ignores `pitch` shows a slanted picture.
3. **Input**: every joypad button and both analog sticks of controller 1, live,
   plus the pointer. Checks that bitmask reads match per-button reads.
4. **Audio**: 440 Hz tone, left / right / both, through either the batch or the
   single-sample callback.
5. **Savestates**: a frame counter that must jump back when a state is loaded.
6. **Summary**: results, then the core asks the frontend to shut down.

Every result is also written to the frontend's log.

## Controls

| Input | Action |
|-------|--------|
| Hold Start (1 s) | Pass this page, go to the next |
| Hold Select (1 s) | Fail this page, go to the next |
| L | Previous page |
| Mouse / pointer | Click Pass, Fail, Back |
| D-pad Left/Right, Up | Audio page: channels, callback type |

## Core options

| Option | Values |
|--------|--------|
| `libretro_test_pixel_format` | `XRGB8888` (default), `RGB565`, `0RGB1555`. Restart the core to apply. |
| `libretro_test_width` | `640` (default, 640x480, 4:3), `720` (720x480, 3:2). Applied while running with `SET_GEOMETRY`. |

Options are defined as core options v2 (category "Video") and sent in the
newest format the frontend reports with `GET_CORE_OPTIONS_VERSION`: v2, v1, or
`SET_VARIABLES` when the frontend doesn't answer. The Environment page shows
which one was used.

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
