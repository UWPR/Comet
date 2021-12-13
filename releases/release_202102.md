### Comet releases 2021.02

Documentation for parameters for release 2021.02 [can be found here](/Comet/parameters/parameters_202102/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2021.02 rev. 0 (2021.02.0), release date 2021/12/09
- This is the first Comet release since the project was migrate to GitHub. There's no significant new
  functionality so there's no real need for 2021.01 users to update to this release.
- Huge thanks to D. Schweppe and W. Fondrie for the migration and setup.
- Matched fragment neutral loss peaks are now reported for real-time search interface.
- Adds supports for macOS, thanks to W. Fondrie.
- This releases addresses the segfault in linux Crux Comet search introduced in the 2021.01.0 release.
  Thanks to C. Grant for debugging the issue.
- This release modifies Crux Comet output file naming which were initially implemented in the
  2018.01 releases. Unfortunately those changes did not make it back to trunk code so the
  file naming fixes reverted as of the 2019.01.0 release.
- There are no parameters changes so this version will work with comet.params files annotated as being
  for versions 2021.01 and 2020.01.
- The docker container lives in the [GitHub Container Registry (GHCR)](https://github.com/UWPR/Comet/pkgs/container/comet).
  Docker command: "docker pull ghcr.io/uwpr/comet:latest"
