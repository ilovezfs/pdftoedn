# Change Log

## 0.32.2 - 2016-08-12

### Added
* wrapped non-ruby support code in #else clause to allow gem build

### Changed
* fix incorrect output with closed path commands introduced with
  format changed on `v0.30.2`
* app now returns error codes greater than 0 to allow shell status
  check. Errors prior to document processing now go to stdout. Invalid
  PDF now returns error code 5 (will start introducing others and
  documenting them somewhere).
* `make check` now works from alternate build dirs. Added additional
  checks.

### Removed
* disabled logic to omit spans overlapped by other spans as it is
  incorrectly removing some valid cases. This should really be
  resolved by correctly tracking PDF operation order between text and
  graphics

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
