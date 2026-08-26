# AutoDraw-SMF

A fork-in-spirit of **Auto Draw** (Nexus 183642), converting its settings from PrismaUI/PMCM
to SKSE Menu Framework, plus a new bound-weapon condition on the existing draw/sheathe
automation.

**Licence: GPL-3.0**, confirmed from the shipped DLL's own version resource
(`LegalCopyright: GPL-3.0 License`). GPL-3.0 explicitly permits modifying and redistributing
this code, which is what this fork does - stated here plainly rather than left implicit. It
is copyleft: this fork must stay GPL-3.0 (no relicensing to something more permissive), must
keep making its own source available, and must mark what was actually changed rather than
presenting modified files as the unmodified original - AI assistance does not change any of
this; a modified version is still a derivative work bound by the same terms whether the
modification was made by hand or with help. The original author never published a public git
repository, but did distribute the full C++ source as part of the Nexus download and licensed
it under GPL-3.0.

`LICENSE` in this repository is GPL-3.0's own text, fetched verbatim and unaltered - the
license's own terms prohibit changing that text, so no summary of it lives there. The
copyright notice below uses the wording GPL-3.0 itself provides for this purpose (its "How to
Apply These Terms" section), filled in for this fork rather than left as the template's
placeholders:

    AutoDraw-SMF - Auto Draw ported to SKSE Menu Framework
    Copyright (C) the original Auto Draw author (Nexus mods/183642)
    Copyright (C) the contributors to this fork

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.

Since there is no public repository, this is `gh repo create`, not `git fork` - a new
repository seeded with the source as the author shipped it, credited below rather than
forked from a repo that doesn't exist.

**Original mod:** https://www.nexusmods.com/skyrimspecialedition/mods/183642
**This fork stays GPL-3.0**, matching the original's licence - see `LICENSE`.

See `PORT-NOTES.md` for the conversion plan.
