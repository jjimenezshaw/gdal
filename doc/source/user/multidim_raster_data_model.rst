.. _multidim_raster_data_model:

================================================================================
Multidimensional Raster Data Model
================================================================================

This document attempts to describe the GDAL multidimensional data model,
that has been added in GDAL 3.1. That is
the types of information that a GDAL multidimensional dataset can contain, and their semantics.

The multidimensional raster API is a generalization of the traditional
:ref:`raster_data_model`, to address 3D, 4D or
higher dimension datasets. Currently, it is
limited to basic read/write API, and is not that much plugged into other higher
level utilities.

It is strongly inspired from the netCDF and HDF5 API and data models.
See `HDF5 format and data model <https://portal.opengeospatial.org/files/81716>`_.

A :cpp:class:`GDALDataset` with multidimensional content contains a root
:cpp:class:`GDALGroup`.

Group
-----

A :cpp:class:`GDALGroup` (modelling a `HDF5 Group <https://portal.opengeospatial.org/files/81716#_hdf5_group>`_)
is a named container of GDALAttribute, GDALMDArray or
other GDALGroup. Hence GDALGroup can describe a hierarchy of objects.

Attribute
---------

A :cpp:class:`GDALAttribute` (modelling a `HDF5 Attribute <https://portal.opengeospatial.org/files/81716#_hdf5_attribute>`_)
has a name and a value, and is typically used to describe a metadata item.
The value can be (for the HDF5 format) in the general case a multidimensional array
of "any" type (in most cases, this will be a single value of string or numeric type)

Multidimensional array
----------------------

A :cpp:class:`GDALMDArray` (modelling a `HDF5 Dataset <https://portal.opengeospatial.org/files/81716#_hdf5_dataset>`_)
has a name, a multidimensional array, references a number of GDALDimension, and
has a list of GDALAttribute.

Most drivers use the row-major convention for dimensions: that is, when considering
that the array elements are stored consecutively in memory, the first dimension
is the slowest varying one (in a 2D image, the row), and the last dimension the
fastest varying one (in a 2D image, the column). That convention is the default
convention used for NumPy arrays, the MEM driver and the HDF5 and netCDF APIs.
The GDAL API is mostly agnostic
about that convention, except when passing a NULL array as the *stride* parameter
for the :cpp:func:`GDALAbstractMDArray::Read` and  :cpp:func:`GDALAbstractMDArray::Write` methods.
You can refer to `NumPy documentation about multidimensional array indexing order issues <https://docs.scipy.org/doc/numpy/reference/internals.html#multidimensional-array-indexing-order-issues>`_

a GDALMDArray has also optional properties:

    - Coordinate reference system: :cpp:class:`OGRSpatialReference`
    - No data value:
    - Unit
    - Offset, such that unscaled_value = offset + scale * raw_value
    - Scale, such that unscaled_value = offset + scale * raw_value

Number of operations can be applied on an array to get modified views of it:
:cpp:func:`GDALMDArray::Transpose()`, :cpp:func:`GDALMDArray::GetView()`, etc.

The :cpp:func:`GDALMDArray::Cache()` method can be used to cache the value of
a view array into a sidecar file.

Dimension
---------

A :cpp:class:`GDALDimension` describes a dimension / axis used to index multidimensional arrays.
It has the following properties:

  - a name
  - a size, that is the number of values that can be indexed along
    the dimension
  - a type, which is a string giving the nature of the dimension.
    Predefined values are: HORIZONTAL_X, HORIZONTAL_Y, VERTICAL, TEMPORAL, PARAMETRIC
    Other values might be used. Empty value means unknown.
  - a direction. Predefined values are:
    EAST, WEST, SOUTH, NORTH, UP, DOWN, FUTURE, PAST
    Other values might be used. Empty value means unknown.
  - a reference to a GDALMDArray variable, typically
    one-dimensional, describing the values taken by the dimension.
    For a georeferenced GDALMDArray and its X dimension, this will be typically
    the values of the easting/longitude for each grid point.

Data Type
---------

A :cpp:class:`GDALExtendedDataType` (modelling a
`HDF5 datatype <https://portal.opengeospatial.org/files/81716#_hdf5_datatype>`_)
describes the type taken by an individual value of
a GDALAttribute or GDALMDArray. Its class can be NUMERIC,
STRING or COMPOUND. For NUMERIC, the existing :cpp:enum:`GDALDataType` enumerated
values are supported. For COMPOUND, the data type is a list of members, each
member being described by a name, a offset in byte in the compound structure and
a GDALExtendedDataType.

.. note::

   The HDF5 modelisation allows for more complex datatypes.

.. note::

    HDF5 does not have native data types for complex values whereas
    GDALDataType does. So a driver may decide to expose a GDT\_Cxxxx datatype
    from a HDF5 Compound data type representing a complex value.

Objects lifetime
----------------

Driver implementations of the multidimensional API are such that instances of
GDALGroup, GDALMDArray, GDALAttribute and GDALDimension can be used independently
of the objects from which they have been obtained from. So:

- the GDALGroup instance returned by :cpp:func:`GDALDataset::GetRootGroup()` can
  be used after the dataset has been closed.

- Sub-groups obtained from :cpp:func:`GDALGroup::OpenGroup()`
  or arrays obtained from :cpp:func:`GDALGroup::OpenMDArray()` can be used after
  the owning group has been released.

- Similarly for attributes or dimensions obtained from a group or an array.


So the following is perfectly legal:

.. code-block:: c++

    #include "gdal_priv.h"

    std::shared_ptr<GDALMDArray> GetArray()
    {
        auto poDataset = std::unique_ptr<GDALDataset>(
            GDALDataset::Open( "in.nc", GDAL_OF_MULTIDIM_RASTER ));
        if( !poDataset )
        {
            return nullptr:
        }
        auto poRootGroup = poDataset->GetRootGroup();
        if( !poRootGroup )
        {
            return nullptr:
        }
        auto poVar = poRootGroup->OpenMDArray("temperature");
        if( !poVar )
        {
            return nullptr:
        }
        return poVar;
    }

    int main()
    {
        GDALAllRegister();
        auto poVar = GetArray();
        if( !poVar )
        {
            exit(1);
        }

        // Do something with poVar

        return 0;
    }


A potential point of attention is that, when creating / editing a dataset, all
those objects keep alive the underlying file descriptors, so changes are only
guaranteed to be serialized when all objects related to a dataset have been released.

.. tabs::

   .. code-tab:: c++

      int main()
      {
          GDALAllRegister();

          auto poDataset = std::unique_ptr<GDALDataset>(
              GetGDALDriverManager()->GetDriverByName("netCDF")->
                  CreateMultiDimensional( "new.nc", nullptr, nullptr ));
          auto poRootGroup = poDataset->GetRootGroup();

          // Could be closed a bit later too
          poDataset.reset();

          auto poDim = poRootGroup->CreateDimension(
              "my_dim", std::string(), std::string(), 10);
          auto poArray = poRootGroup->CreateMDArray(
              "my_var", {poDim}, GDALExtendedDataType::Create(GDT_Byte), nullptr);

          // Can be closed in any order
          poArray.reset();
          poDim.reset();
          poRootGroup.reset();

          // new.nc is fully valid just now

          return 0;
      }

   .. code-tab:: python

      from osgeo import gdal

      with gdal.GetDriverByName("netCDF").CreateMultiDimensional("new.nc") as ds:
          rg = ds.GetRootGroup()
          dim = rg.CreateDimension("my_dim", "", "", 10)
          array = rg.CreateMDArray("my_var", [dim], gdal.ExtendedDataType.Create(gdal.GDT_Byte))

          del array
          del dim
          del rg

      # new.nc is fully valid just now

Differences with the GDAL 2D raster data model
----------------------------------------------

- The concept of GDALRasterBand is no longer used for multidimensional.
  This can be modelled as either different GDALMDArray, or using a compound
  data type.
- There is not yet a standardized concept multi-resolution arrays/overviews in
  the multidimensional API

Bridges between GDAL 2D classic raster data model and multidimensional data model
---------------------------------------------------------------------------------

The :cpp:func:`GDALRasterBand::AsMDArray` and :cpp:func:`GDALMDArray::AsClassicDataset`
can be used to respectively convert a raster band to a MD array or a 2D dataset
to a MD array.

Applications
---------------------------------------------------------------------------------

The following applications can be used to inspect and manipulate multidimensional
datasets:

- :ref:`gdalmdiminfo`
- :ref:`gdalmdimtranslate`
