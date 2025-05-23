#!/usr/bin/env pytest
# -*- coding: utf-8 -*-
###############################################################################
# Project:  GDAL/OGR Test Suite
# Purpose:  'gdal raster mosaic' testing
# Author:   Even Rouault <even dot rouault @ spatialys.com>
#
###############################################################################
# Copyright (c) 2024, Even Rouault <even dot rouault at spatialys.com>
#
# SPDX-License-Identifier: MIT
###############################################################################

import pytest

from osgeo import gdal, osr


def get_mosaic_alg():
    return gdal.GetGlobalAlgorithmRegistry()["raster"]["mosaic"]


def test_gdalalg_raster_mosaic_from_dataset_handle():

    alg = get_mosaic_alg()
    alg["input"] = [
        gdal.Translate(
            "", "../gcore/data/byte.tif", options="-f MEM -srcwin 0 0 10 20"
        ),
        gdal.Translate(
            "", "../gcore/data/byte.tif", options="-f MEM -srcwin 10 0 10 20"
        ),
    ]
    alg["output"] = ""
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.RasterXSize == 20
    assert ds.RasterYSize == 20
    assert ds.GetGeoTransform() == pytest.approx(
        (440720.0, 60.0, 0.0, 3751320.0, 0.0, -60.0)
    )
    assert ds.GetRasterBand(1).Checksum() == 4672


def test_gdalalg_raster_mosaic_from_dataset_name():

    alg = get_mosaic_alg()
    alg["input"] = ["../gcore/data/byte.tif"]
    alg["output"] = ""
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.RasterXSize == 20
    assert ds.RasterYSize == 20
    assert ds.GetGeoTransform() == pytest.approx(
        (440720.0, 60.0, 0.0, 3751320.0, 0.0, -60.0)
    )
    assert ds.GetRasterBand(1).Checksum() == 4672


def test_gdalalg_raster_mosaic_overwrite(tmp_vsimem):

    out_filename = str(tmp_vsimem / "out.vrt")

    alg = get_mosaic_alg()
    assert alg.ParseRunAndFinalize(["../gcore/data/utmsmall.tif", out_filename])

    with gdal.Open(out_filename) as ds:
        assert ds.GetRasterBand(1).Checksum() == 50054

    alg = get_mosaic_alg()
    with pytest.raises(
        Exception, match="already exists. Specify the --overwrite option"
    ):
        alg.ParseRunAndFinalize(["../gcore/data/byte.tif", out_filename])

    alg = get_mosaic_alg()
    assert alg.ParseRunAndFinalize(
        ["--overwrite", "../gcore/data/byte.tif", out_filename]
    )

    with gdal.Open(out_filename) as ds:
        assert ds.GetRasterBand(1).Checksum() == 4672


def test_gdalalg_raster_mosaic_bbox():

    alg = get_mosaic_alg()
    assert alg.ParseCommandLineArguments(
        [
            "--bbox=440780.0,3750180.0,441860.0,3751260.0",
            "../gcore/data/byte.tif",
            "",
        ]
    )
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.RasterXSize == 18
    assert ds.RasterYSize == 18
    assert ds.GetGeoTransform() == pytest.approx(
        (440780.0, 60.0, 0.0, 3751260.0, 0.0, -60.0)
    )
    assert ds.GetRasterBand(1).Checksum() == 3695


def test_gdalalg_raster_mosaic_resolution_average():

    src1_ds = gdal.GetDriverByName("MEM").Create("", 1, 1)
    src1_ds.SetGeoTransform([2, 1, 0, 49, 0, -1])
    src2_ds = gdal.GetDriverByName("MEM").Create("", 2, 2)
    src2_ds.SetGeoTransform([3, 0.5, 0, 49, 0, -0.5])

    alg = get_mosaic_alg()
    alg["input"] = [src1_ds, src2_ds]
    alg["resolution"] = "average"
    alg["output"] = ""
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.RasterXSize == 3
    assert ds.RasterYSize == 1
    assert ds.GetGeoTransform() == pytest.approx((2.0, 0.75, 0.0, 49.0, 0.0, -0.75))


def test_gdalalg_raster_mosaic_resolution_highest():

    src1_ds = gdal.GetDriverByName("MEM").Create("", 1, 1)
    src1_ds.SetGeoTransform([2, 1, 0, 49, 0, -1])
    src2_ds = gdal.GetDriverByName("MEM").Create("", 2, 2)
    src2_ds.SetGeoTransform([3, 0.5, 0, 49, 0, -0.5])

    alg = get_mosaic_alg()
    alg["input"] = [src1_ds, src2_ds]
    alg["output"] = ""
    alg["resolution"] = "highest"
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.RasterXSize == 4
    assert ds.RasterYSize == 2
    assert ds.GetGeoTransform() == pytest.approx((2.0, 0.5, 0.0, 49.0, 0.0, -0.5))


def test_gdalalg_raster_mosaic_resolution_lowest():

    src1_ds = gdal.GetDriverByName("MEM").Create("", 1, 1)
    src1_ds.SetGeoTransform([2, 1, 0, 49, 0, -1])
    src2_ds = gdal.GetDriverByName("MEM").Create("", 2, 2)
    src2_ds.SetGeoTransform([3, 0.5, 0, 49, 0, -0.5])

    alg = get_mosaic_alg()
    alg["input"] = [src1_ds, src2_ds]
    alg["output"] = ""
    alg["resolution"] = "lowest"
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.RasterXSize == 2
    assert ds.RasterYSize == 1
    assert ds.GetGeoTransform() == pytest.approx((2.0, 1.0, 0.0, 49.0, 0.0, -1.0))


def test_gdalalg_raster_mosaic_resolution_common():

    # resolution = 3
    src1_ds = gdal.GetDriverByName("MEM").Create("", 5, 5)
    src1_ds.SetGeoTransform([2, 3, 0, 49, 0, -3])

    # resolution = 5
    src2_ds = gdal.GetDriverByName("MEM").Create("", 3, 3)
    src2_ds.SetGeoTransform([17, 5, 0, 49, 0, -5])

    alg = get_mosaic_alg()
    alg["input"] = [src1_ds, src2_ds]
    alg["output"] = ""
    alg["resolution"] = "common"
    assert alg.Run()
    ds = alg["output"].GetDataset()

    assert ds.RasterXSize == 30
    assert ds.RasterYSize == 15
    assert ds.GetGeoTransform() == pytest.approx((2.0, 1.0, 0.0, 49.0, 0.0, -1.0))


def test_gdalalg_raster_mosaic_resolution_custom():

    src1_ds = gdal.GetDriverByName("MEM").Create("", 1, 1)
    src1_ds.SetGeoTransform([2, 1, 0, 49, 0, -1])
    src2_ds = gdal.GetDriverByName("MEM").Create("", 2, 2)
    src2_ds.SetGeoTransform([3, 0.5, 0, 49, 0, -0.5])

    alg = get_mosaic_alg()
    alg["input"] = [src1_ds, src2_ds]
    alg["output"] = ""
    alg["resolution"] = "0.5,1"
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.RasterXSize == 4
    assert ds.RasterYSize == 1
    assert ds.GetGeoTransform() == pytest.approx((2.0, 0.5, 0.0, 49.0, 0.0, -1.0))


def test_gdalalg_raster_mosaic_target_aligned_pixels():

    src1_ds = gdal.GetDriverByName("MEM").Create("", 1, 1)
    src1_ds.SetGeoTransform([2, 1, 0, 49, 0, -1])
    src2_ds = gdal.GetDriverByName("MEM").Create("", 2, 2)
    src2_ds.SetGeoTransform([3, 0.5, 0, 49, 0, -0.5])

    alg = get_mosaic_alg()
    alg["input"] = [src1_ds, src2_ds]
    alg["output"] = ""
    alg["resolution"] = "0.3,0.6"
    alg["target-aligned-pixels"] = True
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.RasterXSize == 8
    assert ds.RasterYSize == 2
    assert ds.GetGeoTransform() == pytest.approx((1.8, 0.3, 0.0, 49.2, 0.0, -0.6))


def test_gdalalg_raster_mosaic_resolution_same_default():

    src1_ds = gdal.GetDriverByName("MEM").Create("", 1, 1)
    src1_ds.SetGeoTransform([2, 1, 0, 49, 0, -1])
    src2_ds = gdal.GetDriverByName("MEM").Create("", 2, 2)
    src2_ds.SetGeoTransform([3, 0.5, 0, 49, 0, -0.5])

    alg = get_mosaic_alg()
    alg["input"] = [src1_ds, src2_ds]
    alg["output"] = ""
    with pytest.raises(
        Exception,
        match="whereas previous sources have resolution",
    ):
        assert alg.Run()


def test_gdalalg_raster_mosaic_resolution_invalid():

    alg = get_mosaic_alg()
    with pytest.raises(
        Exception,
        match="resolution: two comma separated positive values should be provided, or ",
    ):
        alg["resolution"] = "invalid"

    with pytest.raises(
        Exception,
        match="resolution: two comma separated positive values should be provided, or ",
    ):
        alg["resolution"] = "0.5"

    with pytest.raises(
        Exception,
        match="resolution: two comma separated positive values should be provided, or ",
    ):
        alg["resolution"] = "-0.5,-0.5"


def test_gdalalg_raster_mosaic_srcnodata_dstnodata():

    src1_ds = gdal.GetDriverByName("MEM").Create("", 1, 1)
    src1_ds.SetGeoTransform([2, 1, 0, 49, 0, -1])
    src1_ds.GetRasterBand(1).Fill(1)

    alg = get_mosaic_alg()
    alg["input"] = [src1_ds]
    alg["output"] = ""
    alg["src-nodata"] = [1]
    alg["dst-nodata"] = [2]
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.GetRasterBand(1).Checksum() == 2
    assert ds.GetRasterBand(1).GetNoDataValue() == 2


def test_gdalalg_raster_mosaic_hidenodata():

    src1_ds = gdal.GetDriverByName("MEM").Create("", 1, 1)
    src1_ds.SetGeoTransform([2, 1, 0, 49, 0, -1])
    src1_ds.GetRasterBand(1).Fill(1)

    alg = get_mosaic_alg()
    alg["input"] = [src1_ds]
    alg["output"] = ""
    alg["src-nodata"] = [1]
    alg["dst-nodata"] = [2]
    alg["hide-nodata"] = True
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.GetRasterBand(1).Checksum() == 2
    assert ds.GetRasterBand(1).GetNoDataValue() is None


def test_gdalalg_raster_mosaic_addalpha():

    alg = get_mosaic_alg()
    alg["input"] = ["../gcore/data/rgbsmall.tif"]
    alg["output"] = ""
    alg["band"] = [3, 2]
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.RasterCount == 2
    assert ds.GetRasterBand(1).Checksum() == 21349
    assert ds.GetRasterBand(2).Checksum() == 21053


def test_gdalalg_raster_mosaic_glob():

    alg = get_mosaic_alg()
    alg["input"] = ["../gcore/data/rgbsm?ll.tif"]
    alg["output"] = ""
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.RasterCount == 3


def test_gdalalg_raster_mosaic_at_filename(tmp_vsimem):

    input_file_list = str(tmp_vsimem / "tmp.txt")
    gdal.FileFromMemBuffer(input_file_list, "../gcore/data/byte.tif")

    alg = get_mosaic_alg()
    alg["input"] = [f"@{input_file_list}"]
    alg["output"] = ""
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.GetRasterBand(1).Checksum() == 4672


def test_gdalalg_raster_mosaic_at_filename_error():

    alg = get_mosaic_alg()
    alg["input"] = ["@i_do_not_exist"]
    alg["output"] = ""
    with pytest.raises(Exception, match="mosaic: Cannot open i_do_not_exist"):
        alg.Run()


def test_gdalalg_raster_mosaic_co():

    alg = get_mosaic_alg()
    alg["input"] = ["../gcore/data/byte.tif"]
    alg["output"] = ""
    alg["creation-option"] = ["BLOCKXSIZE=10", "BLOCKYSIZE=15"]
    assert alg.Run()
    ds = alg["output"].GetDataset()
    assert ds.GetRasterBand(1).GetBlockSize() == [10, 15]


def test_gdalalg_raster_mosaic_tif_output_implicit(tmp_vsimem):

    out_filename = str(tmp_vsimem / "out.tif")

    alg = get_mosaic_alg()
    assert alg.ParseRunAndFinalize(["../gcore/data/utmsmall.tif", out_filename])

    with gdal.Open(out_filename) as ds:
        assert ds.GetRasterBand(1).Checksum() == 50054


def test_gdalalg_raster_mosaic_tif_output_explicit(tmp_vsimem):

    out_filename = str(tmp_vsimem / "out.xxx")

    alg = get_mosaic_alg()
    assert alg.ParseRunAndFinalize(
        ["--of=GTiff", "../gcore/data/utmsmall.tif", out_filename]
    )

    with gdal.Open(out_filename) as ds:
        assert ds.GetRasterBand(1).Checksum() == 50054


def test_gdalalg_raster_mosaic_tif_creation_options(tmp_vsimem):

    out_filename = str(tmp_vsimem / "out.xxx")

    alg = get_mosaic_alg()
    assert alg.ParseRunAndFinalize(
        ["--of=GTiff", "--co=TILED=YES", "../gcore/data/utmsmall.tif", out_filename]
    )

    with gdal.Open(out_filename) as ds:
        assert ds.GetRasterBand(1).Checksum() == 50054
        assert ds.GetRasterBand(1).GetBlockSize() == [256, 256]


def test_gdalalg_raster_mosaic_inconsistent_characteristics():

    src1_ds = gdal.GetDriverByName("MEM").Create("", 1, 1)
    src1_ds.SetGeoTransform([2, 1, 0, 49, 0, -1])
    src2_ds = gdal.GetDriverByName("MEM").Create("", 2, 2)
    src2_ds.SetGeoTransform([3, 0.5, 0, 49, 0, -0.5])
    srs = osr.SpatialReference()
    srs.SetAxisMappingStrategy(osr.OAMS_TRADITIONAL_GIS_ORDER)
    srs.SetFromUserInput("WGS84")
    src2_ds.SetSpatialRef(srs)

    alg = get_mosaic_alg()
    alg["input"] = [src1_ds, src2_ds]
    alg["output"] = ""
    with pytest.raises(
        Exception, match="gdal raster mosaic does not support heterogeneous projection"
    ):
        assert alg.Run()
