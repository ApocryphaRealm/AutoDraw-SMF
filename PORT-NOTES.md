# Auto Draw → SMF port notes

Working notes for this port. Background research (before this repo existed) is at
`current wip mods\AutoDraw-research-notes.md` - read that first, this picks up from it.

## Source layout, as vendored (first commit, untouched)

```
src/AnimEventSink.cpp/h    - animation event hook(s)
src/APIHandler.cpp/h       - almost certainly where PMCM's registration lives
src/AutoDraw.cpp/h         - the mod's core logic
src/CombatEvent.cpp/h      - combat state tracking
src/InputEvents.cpp/h      - input handling
src/LoadGame.cpp/h         - save/load hooks
src/PCH.h                  - precompiled header
src/Settings.cpp/h         - the actual setting values
src/XSEPlugin.cpp          - SKSE plugin entry point
```

No `CMakeLists.txt`, `vcpkg.json` or any build scaffold was included in the source download -
only `src/`. The scaffold has to be built from scratch, following this project's established
CommonLibSSE-NG + CMake + vcpkg pattern (see `SMF-CONVERSION-PLAYBOOK.md`).

## The two independent halves (from the research notes - do not conflate)

1. **Settings framework swap: PrismaUI/PMCM → SMF.** Start by reading `APIHandler.cpp/h` to
   confirm PMCM's registration lives there, and `Settings.cpp/h` for the actual values - these
   should carry over with the same settings and defaults, only what draws them changes.
2. **New feature: a bound-weapon condition**, gating the existing
   `RE::Actor::DrawWeaponMagicHands(bool)` call on `RE::TESObjectWEAP::IsBound()` rather than
   adding new cast/dismiss logic. the author's explicit choice - see the research notes for why.

## GPL-3.0 obligations, since this is copyleft (not MIT like the other three forks)

- **Stays GPL-3.0.** No relicensing to something more permissive - not even partially, not
  even for new files this fork adds.
- **Mark what changed.** GPL-3.0 requires modified files to be marked as changed, so problems
  in the modified version aren't mistakenly attributed to the original author. When a vendored
  file is actually edited (not just moved/renamed), add a short note at the top of that file
  saying so - not on files that stay untouched.
- **Keep source available.** Already satisfied by the public repo; stays true as long as it
  does.
- **AI assistance doesn't change any of this.** A modified version is still a derivative work
  bound by GPL-3.0's terms whether a human or an AI made the edit - see `README.md`.

## Status

- [x] Source vendored as the baseline commit, credited to the original author
- [ ] Build scaffold (CMakeLists.txt, vcpkg.json, cmake/, presets, build.bat/configure.bat)
- [ ] Confirm the vendored source actually builds before changing anything in it
- [ ] Read `APIHandler.cpp` and `Settings.cpp` in full to scope the PMCM → SMF swap precisely
- [ ] The SMF settings page itself
- [ ] The bound-weapon condition (independent of the above, can be done in either order)

Nothing beyond vendoring the source has been built yet.
