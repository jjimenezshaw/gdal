#!/usr/bin/env pytest
# -*- coding: utf-8 -*-
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test InterpolateAtPoint functionality
# Author:   Javier Jimenez Shaw <j1@jimenezshaw.com>
#
###############################################################################
# Copyright (c) 2024, Javier Jimenez Shaw <j1@jimenezshaw.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
###############################################################################

import pytest

from osgeo import gdal

###############################################################################
# Test some cases of InterpolateAtPoint


def test_interpolateatpoint_1():

    ds = gdal.Open("data/byte.tif")

    # TODO add explanation for 10,12
    got_bilinear = ds.GetRasterBand(1).InterpolateAtPoint(10, 12, gdal.GRIORA_Bilinear)
    assert got_bilinear == pytest.approx(139.75, 1e-6)
    got_cubic = ds.GetRasterBand(1).InterpolateAtPoint(10, 12, gdal.GRIORA_Cubic)
    assert got_cubic == pytest.approx(138.02, 1e-2)
