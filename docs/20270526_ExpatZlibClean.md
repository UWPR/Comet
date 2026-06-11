# Visual Studio "Clean Solution" Support for MSToolkit Third-Party Libraries

**Date:** 2026-05-27
**Commits:** `3d4c8e2f` (initial zlib), `6115424b` (initial expat),
             latest commit (install-dir headers)
**Files changed:**
- `MSToolkit/VisualStudio/extern/zlibstat.vcxproj`
- `MSToolkit/VisualStudio/extern/libexpat.vcxproj`

---

## Problem

After building Comet under Linux with `make`, switching to Visual Studio and
attempting to compile produced errors such as:

```
C:\Work\Comet-master\MSToolkit\include\zconf.h(475,14):
error C1083: Cannot open include file: 'unistd.h': No such file or directory
```

The Linux `make clean` (which removes the affected directories and installed
headers entirely) restored the ability to compile under VS, but was an extra
manual step that had to be remembered.

---

## Root Cause

The MSToolkit Linux build (`MSToolkit/Makefile`) handles zlib and expat in two
stages.

### Stage 1 — Unpack and configure

1. Unzips `extern/zlib1211.zip` into `extern/zlib-1.2.11/`
   and `extern/expat-2.2.9.zip` into `extern/expat-2.2.9/`.
2. Runs `./configure` inside each directory.  For zlib, `configure` rewrites
   `extern/zlib-1.2.11/zconf.h`, changing:

```c
/* original (Windows-safe) */
#ifdef HAVE_UNISTD_H    /* may be set to #if 1 by ./configure */
#  define Z_HAVE_UNISTD_H
#endif
```

to:

```c
/* Linux-configured */
#if 1    /* was set to #if 1 by ./configure */
#  define Z_HAVE_UNISTD_H
#endif
```

The unconditional `#define Z_HAVE_UNISTD_H` triggers `#include <unistd.h>` later
in `zconf.h`, which does not exist on Windows.

### Stage 2 — Install headers

3. Runs `make install` with `--includedir MSToolkit/include`, copying the
   configure-modified headers into `MSToolkit/include/`:

| Installed file | Problem |
|---|---|
| `MSToolkit/include/zconf.h` | contains `#if 1` — always enables `Z_HAVE_UNISTD_H` |
| `MSToolkit/include/zlib.h` | shadows the tracked extern copy |
| `MSToolkit/include/expat.h` | shadows the tracked extern copy |
| `MSToolkit/include/expat_config.h` | Linux-specific content |
| `MSToolkit/include/expat_external.h` | shadows the tracked extern copy |

None of these five files are tracked by git; they are generated artifacts.

### Why VS fails

`Comet.vcxproj` and `CometSearch.vcxproj` list `MSToolkit\include` **before**
`MSToolkit\include\extern` in their include paths.  MSVC therefore finds the
Linux-installed `MSToolkit/include/zconf.h` before the Windows-safe tracked copy
at `MSToolkit/include/extern/zconf.h`.

The VS project files already had `UnpackZlib` / `UnpackExpat` targets that
auto-extract the zips before building, but only when the directory does not yet
exist:

```xml
<Target Name="UnpackZlib" BeforeTargets="PrepareForBuild"
        Condition="!Exists('...\\extern\\zlib-1.2.11')">
```

After a Linux `make`, the `extern/` directories exist, so VS skips
re-extraction and compiles against the Linux-modified files.

---

## Fix

Two MSBuild tasks were added to each VS project file, both hooked into
**Clean Solution** via `AfterTargets="Clean"`:

1. **`RemoveDir`** — removes the unpacked source directory so that `UnpackXxx`
   re-extracts a clean copy on the next Build.
2. **`Delete`** — removes the five headers that Linux `make install` placed in
   `MSToolkit/include/`.  After deletion, MSVC falls through to the Windows-safe
   copies in `MSToolkit/include/extern/`.  `Delete` is a no-op when a file does
   not exist, so Clean Solution is safe to run at any time.

**`zlibstat.vcxproj`:**

```xml
<Target Name="CleanZlib" AfterTargets="Clean">
  <RemoveDir Directories="$(MSBuildProjectDirectory)\..\..\extern\zlib-1.2.11" />
  <Delete Files="$(MSBuildProjectDirectory)\..\..\include\zconf.h;$(MSBuildProjectDirectory)\..\..\include\zlib.h" />
</Target>
```

**`libexpat.vcxproj`:**

```xml
<Target Name="CleanExpat" AfterTargets="Clean">
  <RemoveDir Directories="$(MSBuildProjectDirectory)\..\..\extern\expat-2.2.9" />
  <Delete Files="$(MSBuildProjectDirectory)\..\..\include\expat.h;$(MSBuildProjectDirectory)\..\..\include\expat_config.h;$(MSBuildProjectDirectory)\..\..\include\expat_external.h" />
</Target>
```

---

## Resulting Workflow

| Step | Action |
|------|--------|
| Build on Linux | `make` |
| Switch to VS | **Clean Solution** — removes `extern/zlib-1.2.11/`, `extern/expat-2.2.9/`, and the five installed headers from `MSToolkit/include/` |
| | **Build Solution** — re-extracts clean zips, compiles OK |

---

## Applying This Fix After an MSToolkit Update

If MSToolkit is updated to a newer version of zlib or expat, the same issue will
recur.  Steps to re-apply the fix:

1. **Identify** the new version directory names produced by the Linux build
   (e.g. `extern/zlib-1.3.1/`, `extern/expat-2.6.0/`).

2. **Update `RemoveDir`** in the `CleanZlib` / `CleanExpat` targets to use the
   new version strings.

3. **Update `Delete`** if the set of headers installed to `MSToolkit/include/`
   changes.  After a Linux `make`, the untracked files in `MSToolkit/include/`
   (shown by `git status`) are the ones to remove.

4. **Update `UnpackXxx`** targets — check that their `Condition` attributes and
   `Unzip` source paths also reference the new version strings.

5. **Test the round-trip:**
   - Run `make` on Linux.
   - In VS, run **Clean Solution** — confirm the source directories and installed
     headers are removed from `MSToolkit/include/`.
   - Run **Build Solution** — confirm extraction and compilation succeed.

If MSToolkit adds a third library with the same unzip-then-configure-then-install
pattern, apply the same `AfterTargets="Clean"` block containing both `RemoveDir`
and `Delete` tasks to its `.vcxproj`.
