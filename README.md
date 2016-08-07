# pdftoedn
A [poppler](https://poppler.freedesktop.org)-based PDF processing tool
to extract document data and save it in
[EDN](https://github.com/edn-format/edn) format. It supports:

* Extracting the PDF outline tree.
* Font and glyph remapping via user-defined font map configurations
  (in JSON format) to allow glyph substitutions for Type 1 or TT fonts
  with invalid/incorrect unicode tables and even embedded CID fonts
  with missing tables.
* Path data extraction.
* Transformed image output, written directly to disk in PNG format.

## Usage

Process a pdf document and write its output to `output_file.edn`:

```sh
pdftoedn -o output_file.edn input_file.pdf
```

## Installation

### Dependencies

* c++-11 compiler
* autotools
* poppler (>=0.45.0) compiled with `--enable-xpdf-headers`
* boost (>=1.60)
* leptonica (>=1.73)
* freetype (>=2.6.0)
* libpng (>=1.2)
* rapidjson (>=1.0)

### Compiling from source

**Note**: `pdftoedn` requires a C++-11 compiler and, while it compiles
under Ubuntu 12 (g++-4.8) and Ubuntu 14 (g++-4.9), it aborts on
certain files due to a virtual method error. Unfortunately, I've been
too busy to look into what's causing this and try to implement a
work-around but I was able to get it compiled under Ubuntu with
`clang`.

Download and extract archive, then change into the source
directory. If your compiler is C++-11 capable, then:

```sh
autoreconf -i
./configure
make && sudo make install
```

If your compiler is not C++-11 capable, here's an example specifying
`clang` to use to build:

```sh
autoreconf -i
CXX="clang++" ./configure
make && sudo make install
```
