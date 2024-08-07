.. _gdal_retile:

================================================================================
gdal_retile
================================================================================

.. only:: html

    Retiles a set of tiles and/or build tiled pyramid levels.

.. Index:: gdal_retile

Synopsis
--------

.. code-block::

    gdal_retile [--help] [--help-general]
                   [-v] [-co <NAME>=<VALUE>]... [-of <out_format>] [-ps <pixelWidth> <pixelHeight>]
                   [-overlap <val_in_pixel>]
                   [-ot  {Byte/Int8/Int16/UInt16/UInt32/Int32/Float32/Float64/
                           CInt16/CInt32/CFloat32/CFloat64}]'
                   [ -tileIndex <tileIndexName> [-tileIndexField <tileIndexFieldName>]]
                   [-csv <fileName> [-csvDelim <delimiter>]]
                   [-s_srs <srs_def>]  [-pyramidOnly]
                   [-r {near|bilinear|cubic|cubicspline|lanczos}]
                   -levels <numberoflevels>
                   [-useDirForEachRow] [-resume]
                   -targetDir <TileDirectory> <input_file> <input_file>...

Description
-----------

This utility will retile a set of input tile(s). All the input tile(s) must
be georeferenced in the same coordinate system and have a matching number of bands.
The geotransform matrix of input tiles must not contain rotation terms.

Optionally pyramid levels are generated. All pyramid levels are generated from the
input tiles (not from previous levels).

It is possible to generate shape file(s) for the tiled output.

If your number of input tiles exhausts the command line buffer, use the general
:ref:`--optfile <raster_common_options_optfile>` option

.. note::

    gdal_retile is a Python utility, and is only available if GDAL Python bindings are available.

.. program:: gdal_retile

.. include:: options/help_and_help_general.rst

.. option:: -targetDir <directory>

    The directory where the tile result is created. Pyramids are stored
    in  sub-directories  numbered  from  1. Created tile names have a numbering
    schema and contain the name of the source tiles(s)

.. include:: options/of.rst

.. include:: options/co.rst

.. include:: options/ot.rst

.. option:: -ps <pixelsize_x> <pixelsize_y>

    Pixel size to be used for the
    output file.  If not specified, 256 x 256 is the default

.. option:: -overlap <val_in_pixel>

    Overlap in pixels between consecutive tiles. If not specified, 0 is the default

    .. versionadded:: 2.2

.. option:: -levels <numberOfLevels>

    Number of pyramids levels to build.

.. option:: -v

    Generate verbose output of tile operations as they are done.

.. option:: -pyramidOnly

    No retiling, build only the pyramids

.. option:: -r <algorithm>

    Resampling algorithm, default is near

.. option:: -s_srs <srs_def>

    Source spatial reference to use. The coordinate systems  that  can  be
    passed  are  anything  supported by the OGRSpatialReference.SetFromUserInput()  call,
    which  includes  EPSG, PCS, and GCSes (i.e. EPSG:4296), PROJ.4 declarations (as above),
    or the name of a .prj file containing well known text.
    If  no  srs_def  is  given,  the srs_def  of the source tiles is used (if there is any).
    The srs_def will be propagated to created tiles (if possible) and  to  the  optional
    shape file(s)

.. option:: -tileIndex <tileIndexName>

    The name of shape file containing the result tile(s) index

.. option:: -tileIndexField <tileIndexFieldName>

    The name of the attribute containing the tile name

.. option:: -csv <csvFileName>

    The name of the csv file containing the tile(s) georeferencing information.
    The file contains 5 columns: tilename,minx,maxx,miny,maxy

.. option:: -csvDelim <column delimiter>

    The column delimiter used in the CSV file, default value is a semicolon ";"

.. option:: -useDirForEachRow

    Normally the tiles of the base image are stored as described in :option:`-targetDir`.
    For large images, some file systems have performance problems if the number of files
    in a directory is too big, causing gdal_retile not to finish in reasonable time.
    Using this parameter creates a different output structure. The tiles of the base image
    are stored in a sub-directory called 0, the pyramids in sub-directories numbered 1,2,....
    Within each of these directories another level of sub-directories is created, numbered from
    0...n, depending of how many tile rows are needed for each level. Finally, a directory contains
    only the tiles for one row for a specific level. For large images a performance improvement
    of a factor N could be achieved.

.. option:: -resume

    Resume mode. Generate only missing files.
