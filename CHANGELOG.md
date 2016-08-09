# Change Log

## 0.32.1 - 2016-08-09

### Added
* autotools-based test suite (run with `make check`)
* function to output loaded font maps

### Changed
* fix for -1 page number argument. Ignored but should not be accepted

## 0.32.0 - 2016-08-08

Initial public release. Completed conversion from ruby gem - some
clode still in place to allow parallel builds for the time being.

### Added
* using boost program options for arg parsing
* autotools for setting up build
* man page and WIKI documentation

### Changed
* scoping of poppler and freetype headers
* internalized default font map to avoid requiring installed file
* changed config dir to `~/.pdftoedn/`
* output file format version for minor tweaks due to app name change
