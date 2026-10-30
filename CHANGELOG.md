## [1.1.0]

- **Declarative DSV Table (`data/plants.dsv`)**:
  - Exported 10 botanical species into declarative `data/plants.dsv` table.
  - Refactored `src/libnd-plant.c` to load flora definitions dynamically via `nd_table_load()`.
- **Two-Header Architecture (`<nd/plant-types.h>`)**:
  - Extracted public types header `<nd/plant-types.h>` defining `plant_tile_t`.
- **Documentation**:
  - Standardized README with man-src, art, and modder guide.

## [1.0.2]

- **The sibling `-I` lines are gone.** `CFLAGS += -I$(shell cd .. && pwd)/…`
  pointed at the axil-nd/nd sibling checkouts and only existed for a dev
  build: in CI those directories do not exist and every header comes from the
  installed packages named in `.github/workflows/ci.yml`. The build now
  resolves `<nd/…>` the way a packager sees it.
- **macOS: link with `-undefined dynamic_lookup`.** macOS `ld` rejects
  undefined symbols in a shared library, but `WARN` needs `qsyslog` — an
  engine-provided function pointer resolved at `dlopen` time (Linux allows
  this by default). `-undefined dynamic_lookup` is the Darwin equivalent, set
  as `LDFLAGS-libnd-plant-Darwin` so no other platform is affected.

## [1.0.0]

- **nd-plant is now an installable library rather than a build artifact of
  the engine.** It builds and installs exactly two files,
  `lib/libnd-plant.so` and `include/nd/plant.h`, following the same layout as
  `axil-tty` and `axil-auth`, and the same layout `nd-core` was converted to
  first. Previously `make` produced a `plant.so` named by the engine's
  `mods.load` and installed nothing. There is no `lib/nd-plant.so` symlink:
  `mods.load` names this module `libnd-plant`, the installed filename, and
  `module_load_path()` only appends `.so`.

- **The link line is libxylem alone.** `LDLIBS := -lxylem`; the engine is not
  linked. `NEEDED` is `libxylem.so` and `libc.so.6`. XXH32 comes from
  `<xxhash.h>`, which is self-contained for `XXH32` — no `-lxxhash`
  needed (the old module got it from `<nd/nd.h>`, which is gone).

- **The map tile is declared in `<nd/plant.h>`.** `plant_tile_t` lives there
  because the engine's map layer and nd-mob (spawn density) read it. No
  `XY_DECL`s — nd-plant implements engine hooks only.

- **Fixed a one-byte stack overflow in `plants_shuffle`.** The bubble pass
  ran `i <= 3` over the 3-element `apln`, writing `apln[3]`; the bound is now
  `i < 3`. Latent since the initial commit — nothing implemented `on_noise`
  while the module was unloaded, so the function had never run — and exposed
  by this port the first time vanilla's `on_new_player` teleported a player
  through `noise_full`. Also `memset(pd.id, 0, 3)` → `sizeof(pd.id)`.

- **`object_add` takes 4 args, not 5** (the old creation-flags tail is
  dropped), and `nd_owritef`-style calls are `snprintf` + `nd_rwrite` — a
  `va_list` cannot cross the XY boundary. The old `on_icon` body is now a
  `core_icon_decorate` decorator.

- **Dropped the `nd-mod.mk` dependency.** `nd-mod.mk` has now been deleted.
