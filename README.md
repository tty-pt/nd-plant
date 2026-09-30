# axil-nd-plant

`nd-plant` for [axil-nd](../axil-nd), ported from SIC to libxylem.

Owns plants: the ten tree/crop species, their skeletons and drops, map
generation (`on_noise`, `on_empty_tile`, `on_spawn`), and the carrot/tomato
consumables that feed nd-drink. The `plant_tile_t` the map layer and nd-mob
read is defined in `<nd/plant.h>`.

## Install

```sh
make install
```

Installs:

```
lib/libnd-plant.so
include/nd/plant.h
```

There is deliberately no `lib/nd-plant.so` symlink (see `axil-nd-wts` for
why: `mods.load` names the installed filename, and the OpenBSD packing list
never lists a symlink).

Also packaged for deb, apk, rpm, brew and openbsd from a `v*` tag.

## Build from source

```sh
make
```

Needs [libxylem](https://github.com/tty-pt/libxylem) and the engine's game
API, `<nd/xy.h>`, plus `<nd/drink.h>` (consumables) and `<nd/core.h>` (the
icon decorator) — from checkouts beside this repo or from installed
packages:

```sh
git clone https://github.com/tty-pt/nd-plant && cd nd-plant
git clone https://github.com/tty-pt/axil-nd ../axil-nd
git clone https://github.com/tty-pt/nd-drink ../axil-nd-drink
git clone https://github.com/tty-pt/nd-core ../axil-nd-core
make
```

Both the checkout `-I` flags and the installed-package paths are on the
command line at once (see `Makefile`), and a missing `-I` is ignored, so the
same command works either way. CI names the deps explicitly
(`axil-nd,libxylem,nd-drink,nd-core,xxhash`).

XXH32 comes from `<xxhash.h>`, which is self-contained for `XXH32` — no
`-lxxhash` on the link line. The old module got it from `<nd/nd.h>`, which
is gone.

## What it does

* `xy_install()` registers the species skeletons and drops (chop action
  first, then the type, consumables, drops and species — the order the
  original FIXME comments insist on).
* `on_noise` hashes the tile (`XXH32`), grows the noise tile and shuffles
  it; `on_empty_tile` renders a side; `on_spawn` plants the tile's species;
  `on_examine` reports `plid` and size; `on_add` drops the plant.
* The old `on_icon` body is now a `core_icon_decorate` decorator registered
  in `xy_install` (the shop precedent): it replaces the glyph with the
  growth stage and adds the chop action.

## Testing

There is no `test.sh` here. Behaviour is asserted by the engine's own suite:

```sh
cd ../axil-nd
make && ./test.sh
```

## Notes from the port

* `SIC_DEF` → `XY_IMPL`, `mod_install` → `xy_install`, `object_add` takes 4
  args (the old 5-arg tail is dropped), `nd_owritef`-style calls become
  `snprintf` + `nd_rwrite` (a `va_list` cannot cross the XY boundary).
* `memset(pd.id, 0, 3)` cleared 3 bytes of a 12-byte array; it is now
  `sizeof(pd.id)`.
* The link line is libxylem alone. `NEEDED` is `libxylem.so` and `libc.so.6`.

## License

BSD 2-Clause, carried over from `tty-pt/nd-plant`. See `LICENSE`.
