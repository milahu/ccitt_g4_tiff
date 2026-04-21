# ccitt_g4_tiff

minimal decoder for Group4-compressed TIFF images for Chromium



## status

early draft



## todo

- test multi-strip images
- test too-large images
- streaming decode (for large TIFFs)



## why

the TIFF image format is useful  
to store [binary images](https://en.wikipedia.org/wiki/Binary_image) compressed with  
[CCITT Group 4 compression](https://en.wikipedia.org/wiki/Group_4_compression)

this binary image compression method  
is already supported in the PDF format  
which is supported by practically all web browsers

it is useful for images of text  
for example scanned book pages  
with single-bit depth (bitonal images)  
and high resolution (usually 600 dpi)

the only usable alternative is the AVIF image format  
but compressing AVIF images  
takes about 100 times more CPU time  
than compressing CCITT-G4 images  
to get similar file size and similar visual quality

binary image compression methods  
are useful for [EPUB-FXL documents](https://github.com/internetarchive/archive-hocr-tools/pull/23)  
(FXL = fixed layout)  
which are similar to PDF documents of scanned book pages  
but implemented in HTML



## complexity

the TIFF image format is terribly complex  
which is probably why it is not-yet supported by chromium  
(high complexity = large attack surface)  
so i would drastically limit the feature set to

- CCITT Group 4 compression
- one strip
- one image
- 1-bit depth only
- size limits (4 GB, max width, max height)

... which allows us to write a minimal TIFF parser  
and integrate an existing CCITT-G4 decoder  
to reduce the attack surface



## browser support

some web browsers already support TIFF images  
probably with different feature sets

- Amaya
- Avast Secure Browser
- Falkon
- Internet Explorer
- Ladybird
- Links
- OmniWeb
- Safari
- Shiira
- WorldWideWeb

... based on
https://en.wikipedia.org/wiki/Comparison_of_web_browsers#Image_format_support



## related

https://github.com/Fyrd/caniuse/issues/4281  
Tiff image format

https://stackoverflow.com/questions/2176991  
How would I display a TIFF images in all web browsers?

https://en.wikipedia.org/wiki/TIFF

> TIFF is a complex format, defining many tags  
> of which typically only a few are used in each file.  
> This led to implementations  
> supporting many varying subsets of the format,  
> a situation that gave rise to the joke that TIFF stands for  
> Thousands of Incompatible File Formats.  
> This problem was addressed in revision 6.0  
> of the TIFF specification (June 1992)  
> by introducing a distinction between Baseline TIFF  
> (which all implementations were required to support)  
> and TIFF Extensions (which are optional).  

https://en.wikipedia.org/wiki/Group_4_compression

> Group 4 compression is available in many proprietary image file formats  
> as well as standardized formats such as TIFF, CALS,  
> CIT (Intergraph Raster Type 24) and the PDF document format.  

https://github.com/photopea/UTIF.js

TIFF decoder written in javascript,
also supports CCITT Group 4 compression



## TIFF parsers

https://gitlab.com/libtiff/libtiff  
many features  
complex = risky

https://github.com/jkriege2/TinyTIFF  
no compression

https://github.com/syoyo/tinydng  
DNG and TIFF format  
no CCITT-G4 compression



## CCITT-G4 decoders

https://pdfium.googlesource.com/pdfium/+/refs/heads/main/core/fxcodec/fax/faxmodule.cpp

https://gitlab.com/libtiff/libtiff/-/blob/master/libtiff/tif_fax3.c



## alternative container formats

except PDF and TIFF, there are ...



### CALS

https://en.wikipedia.org/wiki/CALS_Raster_file_format

> Continuous Acquisition and Life-cycle Support (CALS)

> a standard storing raster (bit-mapped) image data,
> either uncompressed or compressed using CCITT Group 4 compression.

https://www.fileformat.info/format/cals/egff.htm

https://gist.github.com/el-hult/90f7495907f8edaa1783db23151e4e36  
Code for converting a CALS raster file to TIFF. Using vanilla Python.

https://github.com/OSGeo/gdal/blob/master/frmts/cals/calsdataset.cpp



### CIT (Intergraph Raster Type 24)

no: closed-source

https://www.leadtools.com/help/sdk/v21/dh/to/file-formats-intergraph.html

> Intergraph is a monochrome bitmap format developed by Intergraph. LEADTOOLS supports the RLE and CIT versions of this file format.

> The default extensions used by this format are: ITG and CIT.

> For this file format, LEAD supports CCITT G4 and RLE compression.

https://github.com/OSGeo/gdal-extra-drivers/blob/master/not_integrated_in_build/ingr/intergraphraster.rst

> File Extension
>
> .g4	CCITT G4 1-bit data

https://www.cadstudio.cz/en/citin.asp

> CIT is a widely used format in the MicroStation community. CIT is a special case of Intergraph Raster File Format (INGR) - a 1-bit version with CCITT G4 compression).

chatGPT

> - It was developed by Intergraph (now part of Hexagon Geospatial) for use in their GIS and CAD/raster systems (e.g., MGE, GeoMedia, and related tooling).
> - “Type 24” refers to a specific CIT raster encoding/compression variant used within that ecosystem.
> - The format was designed for internal interoperability within Intergraph software, not as an open standard.

> The format is partially reverse-engineered



## out of scope



### JBIG2 compression

[JBIG2 compression](https://en.wikipedia.org/wiki/JBIG2)
is out of scope,
because JBIG2-compressed images dont need a container format like TIFF,
because JBIG2-compressed images can be standalone image files

```sh
# compress
jbig2 src.png >dst.jbig2

# decompress
jbig2dec -o dst.png src.jbig2
```

CCITT-Group4-compressed images are special,
because the CCITT-Group4 format cannot store image width and height,
so we need a container format like TIFF (or PDF)



#### JBIG2 decoders

https://github.com/ArtifexSoftware/jbig2dec

https://cgit.freedesktop.org/poppler/poppler/tree/poppler/JBIG2Stream.cc
