/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Define some core portability services for cross-platform OGR code.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 * Copyright (c) 2007-2014, Even Rouault <even dot rouault at spatialys.com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#ifndef OGR_CORE_H_INCLUDED
#define OGR_CORE_H_INCLUDED

#include "cpl_port.h"
#if defined(GDAL_COMPILATION)
#define DO_NOT_DEFINE_GDAL_DATE_NAME
#endif
#include "gdal_version.h"

/**
 * \file
 *
 * Core portability services for cross-platform OGR code.
 */

#if defined(__cplusplus) && !defined(CPL_SUPRESS_CPLUSPLUS)

extern "C++"
{
#if !defined(DOXYGEN_SKIP)
#include <cmath>
#include <limits>
#endif

    class OGREnvelope3D;

    /**
     * Simple container for a bounding region (rectangle)
     */
    class CPL_DLL OGREnvelope
    {
      public:
        /** Default constructor. Defines an empty rectangle  */
        OGREnvelope()
            : MinX(std::numeric_limits<double>::infinity()),
              MaxX(-std::numeric_limits<double>::infinity()),
              MinY(std::numeric_limits<double>::infinity()),
              MaxY(-std::numeric_limits<double>::infinity())
        {
        }

        /** Copy constructor */
        OGREnvelope(const OGREnvelope &oOther)
            : MinX(oOther.MinX), MaxX(oOther.MaxX), MinY(oOther.MinY),
              MaxY(oOther.MaxY)
        {
        }

        /** Assignment operator */
        OGREnvelope &operator=(const OGREnvelope &) = default;

        /** Minimum X value */
        double MinX;

        /** Maximum X value */
        double MaxX;

        /** Minimum Y value */
        double MinY;

        /** Maximum Y value */
        double MaxY;

#ifdef HAVE_GCC_DIAGNOSTIC_PUSH
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
        /** Return whether the object has been initialized, that is, is non
         * empty */
        int IsInit() const
        {
            return MinX != std::numeric_limits<double>::infinity();
        }

#ifdef HAVE_GCC_DIAGNOSTIC_PUSH
#pragma GCC diagnostic pop
#endif

        /** Update the current object by computing its union with the other
         * rectangle */
        void Merge(OGREnvelope const &sOther)
        {
            MinX = MIN(MinX, sOther.MinX);
            MaxX = MAX(MaxX, sOther.MaxX);
            MinY = MIN(MinY, sOther.MinY);
            MaxY = MAX(MaxY, sOther.MaxY);
        }

        /** Update the current object by computing its union with the provided
         * point */
        void Merge(double dfX, double dfY)
        {
            MinX = MIN(MinX, dfX);
            MaxX = MAX(MaxX, dfX);
            MinY = MIN(MinY, dfY);
            MaxY = MAX(MaxY, dfY);
        }

        /** Update the current object by computing its intersection with the
         * other rectangle */
        void Intersect(OGREnvelope const &sOther)
        {
            if (Intersects(sOther))
            {
                if (IsInit())
                {
                    MinX = MAX(MinX, sOther.MinX);
                    MaxX = MIN(MaxX, sOther.MaxX);
                    MinY = MAX(MinY, sOther.MinY);
                    MaxY = MIN(MaxY, sOther.MaxY);
                }
                else
                {
                    MinX = sOther.MinX;
                    MaxX = sOther.MaxX;
                    MinY = sOther.MinY;
                    MaxY = sOther.MaxY;
                }
            }
            else
            {
                *this = OGREnvelope();
            }
        }

        /** Return whether the current object intersects with the other
         * rectangle */
        int Intersects(OGREnvelope const &other) const
        {
            return MinX <= other.MaxX && MaxX >= other.MinX &&
                   MinY <= other.MaxY && MaxY >= other.MinY;
        }

        /** Return whether the current object contains the other rectangle */
        int Contains(OGREnvelope const &other) const
        {
            return MinX <= other.MinX && MinY <= other.MinY &&
                   MaxX >= other.MaxX && MaxY >= other.MaxY;
        }

        /** Return whether the current rectangle is equal to the other rectangle
         */
        bool operator==(const OGREnvelope &other) const
        {
#ifdef HAVE_GCC_DIAGNOSTIC_PUSH
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
            return MinX == other.MinX && MinY == other.MinY &&
                   MaxX == other.MaxX && MaxY == other.MaxY;

#ifdef HAVE_GCC_DIAGNOSTIC_PUSH
#pragma GCC diagnostic pop
#endif
        }

        /** Return whether the current rectangle is not equal to the other
         * rectangle */
        bool operator!=(const OGREnvelope &other) const
        {
            return !(*this == other);
        }
    };

}  // extern "C++"

#else
typedef struct
{
    double MinX;
    double MaxX;
    double MinY;
    double MaxY;
} OGREnvelope;
#endif

#if defined(__cplusplus) && !defined(CPL_SUPRESS_CPLUSPLUS)

extern "C++"
{

    /**
     * Simple container for a bounding region in 3D.
     */
    class CPL_DLL OGREnvelope3D : public OGREnvelope
    {
      public:
        /** Default constructor. Defines an empty rectangle  */
        OGREnvelope3D()
            : OGREnvelope(), MinZ(std::numeric_limits<double>::infinity()),
              MaxZ(-std::numeric_limits<double>::infinity())
        {
        }

        /** Copy constructor */
        OGREnvelope3D(const OGREnvelope3D &oOther)
            : OGREnvelope(oOther), MinZ(oOther.MinZ), MaxZ(oOther.MaxZ)
        {
        }

        /** Assignment operator */
        OGREnvelope3D &operator=(const OGREnvelope3D &) = default;

        /** Returns TRUE if MinZ and MaxZ are both valid numbers. */
        bool Is3D() const
        {
            return std::isfinite(MinZ) && std::isfinite(MaxZ);
        }

        /** Minimum Z value */
        double MinZ;

        /** Maximum Z value */
        double MaxZ;

#ifdef HAVE_GCC_DIAGNOSTIC_PUSH
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
        /** Return whether the object has been initialized, that is, is non
         * empty */
        int IsInit() const
        {
            return MinX != std::numeric_limits<double>::infinity();
        }
#ifdef HAVE_GCC_DIAGNOSTIC_PUSH
#pragma GCC diagnostic pop
#endif

        /** Update the current object by computing its union with the other
         * rectangle */
        void Merge(OGREnvelope3D const &sOther)
        {
            MinX = MIN(MinX, sOther.MinX);
            MaxX = MAX(MaxX, sOther.MaxX);
            MinY = MIN(MinY, sOther.MinY);
            MaxY = MAX(MaxY, sOther.MaxY);
            MinZ = MIN(MinZ, sOther.MinZ);
            MaxZ = MAX(MaxZ, sOther.MaxZ);
        }

        /** Update the current object by computing its union with the other
         * rectangle */
        void Merge(OGREnvelope const &sOther)
        {
            MinX = MIN(MinX, sOther.MinX);
            MaxX = MAX(MaxX, sOther.MaxX);
            MinY = MIN(MinY, sOther.MinY);
            MaxY = MAX(MaxY, sOther.MaxY);
        }

        /** Update the current object by computing its union with the provided
         * point */
        void Merge(double dfX, double dfY, double dfZ)
        {
            MinX = MIN(MinX, dfX);
            MaxX = MAX(MaxX, dfX);
            MinY = MIN(MinY, dfY);
            MaxY = MAX(MaxY, dfY);
            MinZ = MIN(MinZ, dfZ);
            MaxZ = MAX(MaxZ, dfZ);
        }

        /** Update the current object by computing its intersection with the
         * other rectangle */
        void Intersect(OGREnvelope3D const &sOther)
        {
            if (Intersects(sOther))
            {
                if (IsInit())
                {
                    MinX = MAX(MinX, sOther.MinX);
                    MaxX = MIN(MaxX, sOther.MaxX);
                    MinY = MAX(MinY, sOther.MinY);
                    MaxY = MIN(MaxY, sOther.MaxY);
                    MinZ = MAX(MinZ, sOther.MinZ);
                    MaxZ = MIN(MaxZ, sOther.MaxZ);
                }
                else
                {
                    MinX = sOther.MinX;
                    MaxX = sOther.MaxX;
                    MinY = sOther.MinY;
                    MaxY = sOther.MaxY;
                    MinZ = sOther.MinZ;
                    MaxZ = sOther.MaxZ;
                }
            }
            else
            {
                *this = OGREnvelope3D();
            }
        }

        /** Return whether the current object intersects with the other
         * rectangle */
        int Intersects(OGREnvelope3D const &other) const
        {
            return MinX <= other.MaxX && MaxX >= other.MinX &&
                   MinY <= other.MaxY && MaxY >= other.MinY &&
                   MinZ <= other.MaxZ && MaxZ >= other.MinZ;
        }

        /** Return whether the current object contains the other rectangle */
        int Contains(OGREnvelope3D const &other) const
        {
            return MinX <= other.MinX && MinY <= other.MinY &&
                   MaxX >= other.MaxX && MaxY >= other.MaxY &&
                   MinZ <= other.MinZ && MaxZ >= other.MaxZ;
        }
    };

}  // extern "C++"

#else
typedef struct
{
    double MinX;
    double MaxX;
    double MinY;
    double MaxY;
    double MinZ;
    double MaxZ;
} OGREnvelope3D;
#endif

CPL_C_START

/*! @cond Doxygen_Suppress */
void CPL_DLL *OGRMalloc(size_t) CPL_WARN_DEPRECATED("Use CPLMalloc instead.");
void CPL_DLL *OGRCalloc(size_t, size_t)
    CPL_WARN_DEPRECATED("Use CPLCalloc instead.");
void CPL_DLL *OGRRealloc(void *, size_t)
    CPL_WARN_DEPRECATED("Use CPLRealloc instead.");
char CPL_DLL *OGRStrdup(const char *)
    CPL_WARN_DEPRECATED("Use CPLStrdup instead.");
void CPL_DLL OGRFree(void *) CPL_WARN_DEPRECATED("Use CPLFree instead.");
/*! @endcond */

#ifdef STRICT_OGRERR_TYPE
/** Type for a OGR error */
typedef enum
{
    OGRERR_NONE,                      /**< Success */
    OGRERR_NOT_ENOUGH_DATA,           /**< Not enough data to deserialize */
    OGRERR_NOT_ENOUGH_MEMORY,         /**< Not enough memory */
    OGRERR_UNSUPPORTED_GEOMETRY_TYPE, /**< Unsupported geometry type */
    OGRERR_UNSUPPORTED_OPERATION,     /**< Unsupported operation */
    OGRERR_CORRUPT_DATA,              /**< Corrupt data */
    OGRERR_FAILURE,                   /**< Failure */
    OGRERR_UNSUPPORTED_SRS,           /**< Unsupported SRS */
    OGRERR_INVALID_HANDLE,            /**< Invalid handle */
    OGRERR_NON_EXISTING_FEATURE /**< Non existing feature. Added in GDAL 2.0 */
} OGRErr;
#else
/** Type for a OGR error */
typedef int OGRErr;

#define OGRERR_NONE 0              /**< Success */
#define OGRERR_NOT_ENOUGH_DATA 1   /**< Not enough data to deserialize */
#define OGRERR_NOT_ENOUGH_MEMORY 2 /**< Not enough memory */
#define OGRERR_UNSUPPORTED_GEOMETRY_TYPE 3 /**< Unsupported geometry type */
#define OGRERR_UNSUPPORTED_OPERATION 4     /**< Unsupported operation */
#define OGRERR_CORRUPT_DATA 5              /**< Corrupt data */
#define OGRERR_FAILURE 6                   /**< Failure */
#define OGRERR_UNSUPPORTED_SRS 7           /**< Unsupported SRS */
#define OGRERR_INVALID_HANDLE 8            /**< Invalid handle */
#define OGRERR_NON_EXISTING_FEATURE                                            \
    9 /**< Non existing feature. Added in GDAL 2.0 */

#endif

/** Type for a OGR boolean */
typedef int OGRBoolean;

/* -------------------------------------------------------------------- */
/*      ogr_geometry.h related definitions.                             */
/* -------------------------------------------------------------------- */

#if defined(HAVE_GCC_DIAGNOSTIC_PUSH) && __STDC_VERSION__ < 202311L
/* wkbPoint25D and friends cause warnings with -Wpedantic prior to C23. */
/* Cf https://github.com/OSGeo/gdal/issues/2322 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

/**
 * List of well known binary geometry types.  These are used within the BLOBs
 * but are also returned from OGRGeometry::getGeometryType() to identify the
 * type of a geometry object.
 */
typedef enum
{
    wkbUnknown = 0, /**< unknown type, non-standard */

    wkbPoint = 1,      /**< 0-dimensional geometric object, standard WKB */
    wkbLineString = 2, /**< 1-dimensional geometric object with linear
                        *   interpolation between Points, standard WKB */
    wkbPolygon = 3,    /**< planar 2-dimensional geometric object defined
                        *   by 1 exterior boundary and 0 or more interior
                        *   boundaries, standard WKB */
    wkbMultiPoint = 4, /**< GeometryCollection of Points, standard WKB */
    wkbMultiLineString =
        5,               /**< GeometryCollection of LineStrings, standard WKB */
    wkbMultiPolygon = 6, /**< GeometryCollection of Polygons, standard WKB */
    wkbGeometryCollection = 7, /**< geometric object that is a collection of 1
                                    or more geometric objects, standard WKB */

    wkbCircularString = 8, /**< one or more circular arc segments connected end
                            * to end, ISO SQL/MM Part 3. GDAL &gt;= 2.0 */
    wkbCompoundCurve = 9, /**< sequence of contiguous curves, ISO SQL/MM Part 3.
                             GDAL &gt;= 2.0 */
    wkbCurvePolygon = 10, /**< planar surface, defined by 1 exterior boundary
                           *   and zero or more interior boundaries, that are
                           * curves. ISO SQL/MM Part 3. GDAL &gt;= 2.0 */
    wkbMultiCurve = 11,   /**< GeometryCollection of Curves, ISO SQL/MM Part 3.
                             GDAL &gt;= 2.0 */
    wkbMultiSurface = 12, /**< GeometryCollection of Surfaces, ISO SQL/MM
                             Part 3. GDAL &gt;= 2.0 */
    wkbCurve =
        13, /**< Curve (abstract type). ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbSurface =
        14, /**< Surface (abstract type). ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbPolyhedralSurface =
        15,      /**< a contiguous collection of polygons, which share common
                  * boundary segments,      ISO SQL/MM Part 3. GDAL &gt;= 2.3 */
    wkbTIN = 16, /**< a PolyhedralSurface consisting only of Triangle patches
                  *    ISO SQL/MM Part 3. GDAL &gt;= 2.3 */
    wkbTriangle = 17, /**< a Triangle. ISO SQL/MM Part 3. GDAL &gt;= 2.3 */

    wkbNone = 100,       /**< non-standard, for pure attribute records */
    wkbLinearRing = 101, /**< non-standard, just for createGeometry() */

    wkbCircularStringZ = 1008, /**< wkbCircularString with Z component. ISO
                                  SQL/MM Part 3. GDAL &gt;= 2.0 */
    wkbCompoundCurveZ = 1009, /**< wkbCompoundCurve with Z component. ISO SQL/MM
                                 Part 3. GDAL &gt;= 2.0 */
    wkbCurvePolygonZ = 1010,  /**< wkbCurvePolygon with Z component. ISO SQL/MM
                                 Part 3. GDAL &gt;= 2.0 */
    wkbMultiCurveZ = 1011,    /**< wkbMultiCurve with Z component. ISO SQL/MM
                                 Part 3. GDAL &gt;= 2.0 */
    wkbMultiSurfaceZ = 1012,  /**< wkbMultiSurface with Z component. ISO SQL/MM
                                 Part 3. GDAL &gt;= 2.0 */
    wkbCurveZ = 1013,   /**< wkbCurve with Z component. ISO SQL/MM Part 3. GDAL
                           &gt;= 2.1 */
    wkbSurfaceZ = 1014, /**< wkbSurface with Z component. ISO SQL/MM Part 3.
                           GDAL &gt;= 2.1 */
    wkbPolyhedralSurfaceZ = 1015, /**< ISO SQL/MM Part 3. GDAL &gt;= 2.3 */
    wkbTINZ = 1016,               /**< ISO SQL/MM Part 3. GDAL &gt;= 2.3 */
    wkbTriangleZ = 1017,          /**< ISO SQL/MM Part 3. GDAL &gt;= 2.3 */

    wkbPointM = 2001,              /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbLineStringM = 2002,         /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbPolygonM = 2003,            /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbMultiPointM = 2004,         /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbMultiLineStringM = 2005,    /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbMultiPolygonM = 2006,       /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbGeometryCollectionM = 2007, /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbCircularStringM = 2008,     /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbCompoundCurveM = 2009,      /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbCurvePolygonM = 2010,       /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbMultiCurveM = 2011,         /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbMultiSurfaceM = 2012,       /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbCurveM = 2013,              /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbSurfaceM = 2014,            /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbPolyhedralSurfaceM = 2015,  /**< ISO SQL/MM Part 3. GDAL &gt;= 2.3 */
    wkbTINM = 2016,                /**< ISO SQL/MM Part 3. GDAL &gt;= 2.3 */
    wkbTriangleM = 2017,           /**< ISO SQL/MM Part 3. GDAL &gt;= 2.3 */

    wkbPointZM = 3001,              /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbLineStringZM = 3002,         /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbPolygonZM = 3003,            /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbMultiPointZM = 3004,         /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbMultiLineStringZM = 3005,    /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbMultiPolygonZM = 3006,       /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbGeometryCollectionZM = 3007, /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbCircularStringZM = 3008,     /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbCompoundCurveZM = 3009,      /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbCurvePolygonZM = 3010,       /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbMultiCurveZM = 3011,         /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbMultiSurfaceZM = 3012,       /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbCurveZM = 3013,              /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbSurfaceZM = 3014,            /**< ISO SQL/MM Part 3. GDAL &gt;= 2.1 */
    wkbPolyhedralSurfaceZM = 3015,  /**< ISO SQL/MM Part 3. GDAL &gt;= 2.3 */
    wkbTINZM = 3016,                /**< ISO SQL/MM Part 3. GDAL &gt;= 2.3 */
    wkbTriangleZM = 3017,           /**< ISO SQL/MM Part 3. GDAL &gt;= 2.3 */

#if defined(DOXYGEN_SKIP)
    // Sphinx doesn't like 0x8000000x constants
    wkbPoint25D = -2147483647,             /**< 2.5D extension as per 99-402 */
    wkbLineString25D = -2147483646,        /**< 2.5D extension as per 99-402 */
    wkbPolygon25D = -2147483645,           /**< 2.5D extension as per 99-402 */
    wkbMultiPoint25D = -2147483644,        /**< 2.5D extension as per 99-402 */
    wkbMultiLineString25D = -2147483643,   /**< 2.5D extension as per 99-402 */
    wkbMultiPolygon25D = -2147483642,      /**< 2.5D extension as per 99-402 */
    wkbGeometryCollection25D = -2147483641 /**< 2.5D extension as per 99-402 */
#else
    wkbPoint25D = 0x80000001,             /**< 2.5D extension as per 99-402 */
    wkbLineString25D = 0x80000002,        /**< 2.5D extension as per 99-402 */
    wkbPolygon25D = 0x80000003,           /**< 2.5D extension as per 99-402 */
    wkbMultiPoint25D = 0x80000004,        /**< 2.5D extension as per 99-402 */
    wkbMultiLineString25D = 0x80000005,   /**< 2.5D extension as per 99-402 */
    wkbMultiPolygon25D = 0x80000006,      /**< 2.5D extension as per 99-402 */
    wkbGeometryCollection25D = 0x80000007 /**< 2.5D extension as per 99-402 */
#endif
} OGRwkbGeometryType;

#if defined(HAVE_GCC_DIAGNOSTIC_PUSH) && __STDC_VERSION__ < 202311L
#pragma GCC diagnostic pop
#endif

/* clang-format off */
/**
 * Output variants of WKB we support.
 *
 * 99-402 was a short-lived extension to SFSQL 1.1 that used a high-bit flag
 * to indicate the presence of Z coordinates in a WKB geometry.
 *
 * SQL/MM Part 3 and SFSQL 1.2 use offsets of 1000 (Z), 2000 (M) and 3000 (ZM)
 * to indicate the present of higher dimensional coordinates in a WKB geometry.
 * Reference: <a href="https://portal.opengeospatial.org/files/?artifact_id=320243">
 * 09-009_Committee_Draft_ISOIEC_CD_13249-3_SQLMM_Spatial.pdf</a>,
 * ISO/IEC JTC 1/SC 32 N 1820, ISO/IEC CD 13249-3:201x(E), Date: 2009-01-16.
 * The codes are also found in §8.2.3 of <a href="http://portal.opengeospatial.org/files/?artifact_id=25355"> OGC
 * 06-103r4 "OpenGIS® Implementation Standard for Geographic information -
 * Simple feature access - Part 1: Common architecture", v1.2.1</a>
 */
/* clang-format on */

typedef enum
{
    wkbVariantOldOgc, /**< Old-style 99-402 extended dimension (Z) WKB types */
    wkbVariantIso, /**< SFSQL 1.2 and ISO SQL/MM Part 3 extended dimension (Z&M)
                      WKB types */
    wkbVariantPostGIS1 /**< PostGIS 1.X has different codes for CurvePolygon,
                          MultiCurve and MultiSurface */
} OGRwkbVariant;

#ifndef GDAL_COMPILATION
/** @deprecated in GDAL 2.0. Use wkbHasZ() or wkbSetZ() instead */
#define wkb25DBit 0x80000000
#endif

#ifndef __cplusplus
/** Return the 2D geometry type corresponding to the specified geometry type */
#define wkbFlatten(x) OGR_GT_Flatten((OGRwkbGeometryType)(x))
#else
                                          /** Return the 2D geometry type corresponding to the specified geometry type */
#define wkbFlatten(x) OGR_GT_Flatten(static_cast<OGRwkbGeometryType>(x))
#endif

/** Return if the geometry type is a 3D geometry type
 * @since GDAL 2.0
 */
#define wkbHasZ(x) (OGR_GT_HasZ(x) != 0)

/** Return the 3D geometry type corresponding to the specified geometry type.
 * @since GDAL 2.0
 */
#define wkbSetZ(x) OGR_GT_SetZ(x)

/** Return if the geometry type is a measured geometry type
 * @since GDAL 2.1
 */
#define wkbHasM(x) (OGR_GT_HasM(x) != 0)

/** Return the measured geometry type corresponding to the specified geometry
 * type.
 * @since GDAL 2.1
 */
#define wkbSetM(x) OGR_GT_SetM(x)

#ifndef DOXYGEN_SKIP
#define ogrZMarker 0x21125711
#endif

const char CPL_DLL *OGRGeometryTypeToName(OGRwkbGeometryType eType);
OGRwkbGeometryType CPL_DLL OGRMergeGeometryTypes(OGRwkbGeometryType eMain,
                                                 OGRwkbGeometryType eExtra);
OGRwkbGeometryType CPL_DLL OGRMergeGeometryTypesEx(OGRwkbGeometryType eMain,
                                                   OGRwkbGeometryType eExtra,
                                                   int bAllowPromotingToCurves);
OGRwkbGeometryType CPL_DLL OGR_GT_Flatten(OGRwkbGeometryType eType);
OGRwkbGeometryType CPL_DLL OGR_GT_SetZ(OGRwkbGeometryType eType);
OGRwkbGeometryType CPL_DLL OGR_GT_SetM(OGRwkbGeometryType eType);
OGRwkbGeometryType CPL_DLL OGR_GT_SetModifier(OGRwkbGeometryType eType,
                                              int bSetZ, int bSetM);
int CPL_DLL OGR_GT_HasZ(OGRwkbGeometryType eType);
int CPL_DLL OGR_GT_HasM(OGRwkbGeometryType eType);
int CPL_DLL OGR_GT_IsSubClassOf(OGRwkbGeometryType eType,
                                OGRwkbGeometryType eSuperType);
int CPL_DLL OGR_GT_IsCurve(OGRwkbGeometryType);
int CPL_DLL OGR_GT_IsSurface(OGRwkbGeometryType);
int CPL_DLL OGR_GT_IsNonLinear(OGRwkbGeometryType);
OGRwkbGeometryType CPL_DLL OGR_GT_GetCollection(OGRwkbGeometryType eType);
OGRwkbGeometryType CPL_DLL OGR_GT_GetSingle(OGRwkbGeometryType eType);
OGRwkbGeometryType CPL_DLL OGR_GT_GetCurve(OGRwkbGeometryType eType);
OGRwkbGeometryType CPL_DLL OGR_GT_GetLinear(OGRwkbGeometryType eType);

/** Enumeration to describe byte order */
typedef enum
{
    wkbXDR = 0, /**< MSB/Sun/Motorola: Most Significant Byte First   */
    wkbNDR = 1  /**< LSB/Intel/Vax: Least Significant Byte First      */
} OGRwkbByteOrder;

#ifndef DOXYGEN_SKIP

#ifndef NO_HACK_FOR_IBM_DB2_V72
#define HACK_FOR_IBM_DB2_V72
#endif

#ifdef HACK_FOR_IBM_DB2_V72
#define DB2_V72_FIX_BYTE_ORDER(x) ((((x)&0x31) == (x)) ? ((x)&0x1) : (x))
#define DB2_V72_UNFIX_BYTE_ORDER(x)                                            \
    CPL_STATIC_CAST(unsigned char, OGRGeometry::bGenerate_DB2_V72_BYTE_ORDER   \
                                       ? ((x) | 0x30)                          \
                                       : (x))
#else
#define DB2_V72_FIX_BYTE_ORDER(x) (x)
#define DB2_V72_UNFIX_BYTE_ORDER(x) (x)
#endif

#endif /* #ifndef DOXYGEN_SKIP */

/** Alter field name.
 * Used by OGR_L_AlterFieldDefn().
 */
#define ALTER_NAME_FLAG 0x1

/** Alter field type.
 * Used by OGR_L_AlterFieldDefn().
 */
#define ALTER_TYPE_FLAG 0x2

/** Alter field width and precision.
 * Used by OGR_L_AlterFieldDefn().
 */
#define ALTER_WIDTH_PRECISION_FLAG 0x4

/** Alter field NOT NULL constraint.
 * Used by OGR_L_AlterFieldDefn().
 * @since GDAL 2.0
 */
#define ALTER_NULLABLE_FLAG 0x8

/** Alter field DEFAULT value.
 * Used by OGR_L_AlterFieldDefn().
 * @since GDAL 2.0
 */
#define ALTER_DEFAULT_FLAG 0x10

/** Alter field UNIQUE constraint.
 * Used by OGR_L_AlterFieldDefn().
 * @since GDAL 3.2
 */
#define ALTER_UNIQUE_FLAG 0x20

/** Alter field domain name.
 * Used by OGR_L_AlterFieldDefn().
 * @since GDAL 3.3
 */
#define ALTER_DOMAIN_FLAG 0x40

/** Alter field alternative name.
 * Used by OGR_L_AlterFieldDefn().
 * @since GDAL 3.7
 */
#define ALTER_ALTERNATIVE_NAME_FLAG 0x80

/** Alter field comment.
 * Used by OGR_L_AlterFieldDefn().
 * @since GDAL 3.7
 */
#define ALTER_COMMENT_FLAG 0x100

/** Alter all parameters of field definition.
 * Used by OGR_L_AlterFieldDefn().
 */
#define ALTER_ALL_FLAG                                                         \
    (ALTER_NAME_FLAG | ALTER_TYPE_FLAG | ALTER_WIDTH_PRECISION_FLAG |          \
     ALTER_NULLABLE_FLAG | ALTER_DEFAULT_FLAG | ALTER_UNIQUE_FLAG |            \
     ALTER_DOMAIN_FLAG | ALTER_ALTERNATIVE_NAME_FLAG | ALTER_COMMENT_FLAG)

/** Alter geometry field name.
 * Used by OGR_L_AlterGeomFieldDefn().
 * @since GDAL 3.6
 */
#define ALTER_GEOM_FIELD_DEFN_NAME_FLAG 0x1000

/** Alter geometry field type.
 * Used by OGR_L_AlterGeomFieldDefn().
 * @since GDAL 3.6
 */
#define ALTER_GEOM_FIELD_DEFN_TYPE_FLAG 0x2000

/** Alter geometry field nullable state.
 * Used by OGR_L_AlterGeomFieldDefn().
 * @since GDAL 3.6
 */
#define ALTER_GEOM_FIELD_DEFN_NULLABLE_FLAG 0x4000

/** Alter geometry field spatial reference system (except its coordinate epoch)
 * Used by OGR_L_AlterGeomFieldDefn().
 * @since GDAL 3.6
 */
#define ALTER_GEOM_FIELD_DEFN_SRS_FLAG 0x8000

/** Alter geometry field coordinate epoch
 * Used by OGR_L_AlterGeomFieldDefn().
 * @since GDAL 3.6
 */
#define ALTER_GEOM_FIELD_DEFN_SRS_COORD_EPOCH_FLAG 0x10000

/** Alter all parameters of field definition.
 * Used by OGR_L_AlterGeomFieldDefn().
 * @since GDAL 3.6
 */
#define ALTER_GEOM_FIELD_DEFN_ALL_FLAG                                         \
    (ALTER_GEOM_FIELD_DEFN_NAME_FLAG | ALTER_GEOM_FIELD_DEFN_TYPE_FLAG |       \
     ALTER_GEOM_FIELD_DEFN_NULLABLE_FLAG | ALTER_GEOM_FIELD_DEFN_SRS_FLAG |    \
     ALTER_GEOM_FIELD_DEFN_SRS_COORD_EPOCH_FLAG)

/** Validate that fields respect not-null constraints.
 * Used by OGR_F_Validate().
 * @since GDAL 2.0
 */
#define OGR_F_VAL_NULL 0x00000001

/** Validate that geometries respect geometry column type.
 * Used by OGR_F_Validate().
 * @since GDAL 2.0
 */
#define OGR_F_VAL_GEOM_TYPE 0x00000002

/** Validate that (string) fields respect field width.
 * Used by OGR_F_Validate().
 * @since GDAL 2.0
 */
#define OGR_F_VAL_WIDTH 0x00000004

/** Allow fields that are null when there's an associated default value.
 * This can be used for drivers where the low-level layers will automatically
 * set the field value to the associated default value. This flag only makes
 * sense if OGR_F_VAL_NULL is set too. Used by OGR_F_Validate().
 * @since GDAL 2.0
 */
#define OGR_F_VAL_ALLOW_NULL_WHEN_DEFAULT 0x00000008

/** Allow geometry fields to have a different coordinate dimension that their
 * geometry column type.
 * This flag only makes sense if OGR_F_VAL_GEOM_TYPE is set too.
 * Used by OGR_F_Validate().
 * @since GDAL 2.1
 */
#define OGR_F_VAL_ALLOW_DIFFERENT_GEOM_DIM 0x00000010

/** Enable all validation tests (except OGR_F_VAL_ALLOW_DIFFERENT_GEOM_DIM)
 * Used by OGR_F_Validate().
 * @since GDAL 2.0
 */
#define OGR_F_VAL_ALL (0x7FFFFFFF & ~OGR_F_VAL_ALLOW_DIFFERENT_GEOM_DIM)

/************************************************************************/
/*                  ogr_feature.h related definitions.                  */
/************************************************************************/

/**
 * List of feature field types.  This list is likely to be extended in the
 * future ... avoid coding applications based on the assumption that all
 * field types can be known.
 */

typedef enum
{
    /** Single signed 32bit integer */ OFTInteger = 0,
    /** List of signed 32bit integers */ OFTIntegerList = 1,
    /** Double Precision floating point */ OFTReal = 2,
    /** List of doubles */ OFTRealList = 3,
    /** String of ASCII chars */ OFTString = 4,
    /** Array of strings */ OFTStringList = 5,
    /** deprecated */ OFTWideString = 6,
    /** deprecated */ OFTWideStringList = 7,
    /** Raw Binary data */ OFTBinary = 8,
    /** Date */ OFTDate = 9,
    /** Time */ OFTTime = 10,
    /** Date and Time */ OFTDateTime = 11,
    /** Single signed 64bit integer */ OFTInteger64 = 12,
    /** List of signed 64bit integers */ OFTInteger64List = 13,
    OFTMaxType = 13
} OGRFieldType;

/**
 * List of field subtypes. A subtype represents a hint, a restriction of the
 * main type, that is not strictly necessary to consult.
 * This list is likely to be extended in the
 * future ... avoid coding applications based on the assumption that all
 * field types can be known.
 * Most subtypes only make sense for a restricted set of main types.
 * @since GDAL 2.0
 */
typedef enum
{
    /** No subtype. This is the default value */ OFSTNone = 0,
    /** Boolean integer. Only valid for OFTInteger and OFTIntegerList.*/
    OFSTBoolean = 1,
    /** Signed 16-bit integer. Only valid for OFTInteger and OFTIntegerList. */
    OFSTInt16 = 2,
    /** Single precision (32 bit) floating point. Only valid for OFTReal and
       OFTRealList. */
    OFSTFloat32 = 3,
    /** JSON content. Only valid for OFTString.
     * @since GDAL 2.4
     */
    OFSTJSON = 4,
    /** UUID string representation. Only valid for OFTString.
     * @since GDAL 3.3
     */
    OFSTUUID = 5,
    OFSTMaxSubType = 5
} OGRFieldSubType;

/**
 * Display justification for field values.
 */

typedef enum
{
    OJUndefined = 0,
    OJLeft = 1,
    OJRight = 2
} OGRJustification;

/** Special value for a unset FID */
#define OGRNullFID -1

/* Special value for an unknown field type. This should only be used
 * while reading a file. At the end of file any unknown types should
 * be set to OFTString.
 */
/*! @cond Doxygen_Suppress */
#define OGRUnknownType static_cast<OGRFieldType>(-1)
/*! @endcond */

/** Special value set in OGRField.Set.nMarker1, nMarker2 and nMarker3 for
 *  a unset field.
 *  Direct use of this value is strongly discouraged.
 *  Use OGR_RawField_SetUnset() or OGR_RawField_IsUnset() instead.
 */
#define OGRUnsetMarker -21121

/** Special value set in OGRField.Set.nMarker1, nMarker2 and nMarker3 for
 *  a null field.
 *  Direct use of this value is strongly discouraged.
 *  Use OGR_RawField_SetNull() or OGR_RawField_IsNull() instead.
 *  @since GDAL 2.2
 */
#define OGRNullMarker -21122

/** Time zone flag indicating unknown timezone. For the
 *  OGRFieldDefn::GetTZFlag() property, this may also indicate a mix of
 *  unknown, localtime or known time zones in the same field.
 */
#define OGR_TZFLAG_UNKNOWN 0

/** Time zone flag indicating local time */
#define OGR_TZFLAG_LOCALTIME 1

/** Time zone flag only returned by OGRFieldDefn::GetTZFlag() to indicate
 * that all values in the field have a known time zone (ie different from
 * OGR_TZFLAG_UNKNOWN and OGR_TZFLAG_LOCALTIME), but it may be different among
 * features. */
#define OGR_TZFLAG_MIXED_TZ 2

/** Time zone flag indicating UTC.
 * Used to derived other time zone flags with the following logic:
 * - values above 100 indicate a 15 minute increment per unit.
 * - values under 100 indicate a 15 minute decrement per unit.
 * For example: a value of 101 indicates UTC+00:15, a value of 102 UTC+00:30,
 * a value of 99 UTC-00:15 ,etc. */
#define OGR_TZFLAG_UTC 100

/************************************************************************/
/*                               OGRField                               */
/************************************************************************/

/**
 * OGRFeature field attribute value union.
 */

typedef union
{
    /*! @cond Doxygen_Suppress */
    int Integer;
    GIntBig Integer64;
    double Real;
    char *String;

    struct
    {
        int nCount;
        int *paList;
    } IntegerList;

    struct
    {
        int nCount;
        GIntBig *paList;
    } Integer64List;

    struct
    {
        int nCount;
        double *paList;
    } RealList;

    struct
    {
        int nCount;
        char **paList;
    } StringList;

    struct
    {
        int nCount;
        GByte *paData;
    } Binary;

    struct
    {
        int nMarker1;
        int nMarker2;
        int nMarker3;
    } Set;

    struct
    {
        GInt16 Year;
        GByte Month;
        GByte Day;
        GByte Hour;
        GByte Minute;
        GByte TZFlag;   /* 0=unknown, 1=localtime(ambiguous),
                           100=GMT, 104=GMT+1, 80=GMT-5, etc */
        GByte Reserved; /* must be set to 0 */
        float Second; /* with millisecond accuracy. at the end of the structure,
                         so as to keep it 12 bytes on 32 bit */
    } Date;

    /*! @endcond */
} OGRField;

/** Return the number of milliseconds from a datetime with decimal seconds */
int CPL_DLL OGR_GET_MS(float fSec);

/** Option for OGRParseDate() to ask for lax checks on the input format */
#define OGRPARSEDATE_OPTION_LAX 1

int CPL_DLL OGRParseDate(const char *pszInput, OGRField *psOutput,
                         int nOptions);

/* -------------------------------------------------------------------- */
/*      Constants from ogrsf_frmts.h for capabilities.                  */
/* -------------------------------------------------------------------- */
#define OLCRandomRead "RandomRead" /**< Layer capability for random read */
#define OLCSequentialWrite                                                     \
    "SequentialWrite" /**< Layer capability for sequential write */
#define OLCRandomWrite "RandomWrite" /**< Layer capability for random write */
#define OLCFastSpatialFilter                                                   \
    "FastSpatialFilter" /**< Layer capability for fast spatial filter */
#define OLCFastFeatureCount                                                    \
    "FastFeatureCount" /**< Layer capability for fast feature count retrieval  \
                        */
#define OLCFastGetExtent                                                       \
    "FastGetExtent" /**< Layer capability for fast extent retrieval */
#define OLCFastGetExtent3D                                                     \
    "FastGetExtent3D" /**< Layer capability for fast 3D extent retrieval */
#define OLCCreateField                                                         \
    "CreateField" /**< Layer capability for field creation                     \
                   */
#define OLCDeleteField                                                         \
    "DeleteField" /**< Layer capability for field deletion                     \
                   */
#define OLCReorderFields                                                       \
    "ReorderFields" /**< Layer capability for field reordering */
#define OLCAlterFieldDefn                                                      \
    "AlterFieldDefn" /**< Layer capability for field alteration */
#define OLCAlterGeomFieldDefn                                                  \
    "AlterGeomFieldDefn" /**< Layer capability for geometry field alteration   \
                          */
#define OLCTransactions                                                        \
    "Transactions" /**< Layer capability for transactions                      \
                    */
#define OLCDeleteFeature                                                       \
    "DeleteFeature" /**< Layer capability for feature deletion */
#define OLCUpsertFeature                                                       \
    "UpsertFeature" /**< Layer capability for feature upsert */
#define OLCUpdateFeature                                                       \
    "UpdateFeature" /**< Layer capability for specialized \
                                              UpdateFeature() implementation */
#define OLCFastSetNextByIndex                                                  \
    "FastSetNextByIndex" /**< Layer capability for setting next feature index  \
                          */
#define OLCStringsAsUTF8                                                       \
    "StringsAsUTF8" /**< Layer capability for strings returned with UTF-8      \
                       encoding */
#define OLCIgnoreFields                                                        \
    "IgnoreFields" /**< Layer capability for field ignoring */
#define OLCCreateGeomField                                                     \
    "CreateGeomField" /**< Layer capability for geometry field creation */
#define OLCCurveGeometries                                                     \
    "CurveGeometries" /**< Layer capability for curve geometries support */
#define OLCMeasuredGeometries                                                  \
    "MeasuredGeometries" /**< Layer capability for measured geometries support \
                          */
#define OLCZGeometries                                                         \
    "ZGeometries" /**< Layer capability for geometry with Z dimension support. \
                     Since GDAL 3.6. */
#define OLCRename                                                              \
    "Rename" /**< Layer capability for a layer that supports Rename() */
#define OLCFastGetArrowStream                                                  \
    "FastGetArrowStream" /**< Layer capability for fast GetArrowStream()       \
                            implementation */
#define OLCFastWriteArrowBatch                                                 \
    "FastWriteArrowBatch" /**< Layer capability for fast WriteArrowBatch()     \
                            implementation */

#define ODsCCreateLayer                                                        \
    "CreateLayer" /**< Dataset capability for layer creation */
#define ODsCDeleteLayer                                                        \
    "DeleteLayer" /**< Dataset capability for layer deletion */
/* Reserved:                   "RenameLayer" */
#define ODsCCreateGeomFieldAfterCreateLayer                                    \
    "CreateGeomFieldAfterCreateLayer" /**< Dataset capability for geometry     \
                                         field creation support */
#define ODsCCurveGeometries                                                    \
    "CurveGeometries" /**< Dataset capability for curve geometries support */
#define ODsCTransactions                                                       \
    "Transactions" /**< Dataset capability for dataset transcations */
#define ODsCEmulatedTransactions                                               \
    "EmulatedTransactions" /**< Dataset capability for emulated dataset        \
                              transactions */
#define ODsCMeasuredGeometries                                                 \
    "MeasuredGeometries" /**< Dataset capability for measured geometries       \
                            support */
#define ODsCZGeometries                                                        \
    "ZGeometries" /**< Dataset capability for geometry with Z dimension        \
                     support. Since GDAL 3.6. */
#define ODsCRandomLayerRead                                                    \
    "RandomLayerRead" /**< Dataset capability for GetNextFeature() returning   \
                         features from random layers */
/* Note the unfortunate trailing space at the end of the string */
#define ODsCRandomLayerWrite                                                   \
    "RandomLayerWrite " /**< Dataset capability for supporting CreateFeature   \
                           on layer in random order */
#define ODsCAddFieldDomain                                                     \
    "AddFieldDomain" /**< Dataset capability for supporting AddFieldDomain()   \
                        (at least partially) */
#define ODsCDeleteFieldDomain                                                  \
    "DeleteFieldDomain" /**< Dataset capability for supporting                 \
                           DeleteFieldDomain()*/
#define ODsCUpdateFieldDomain                                                  \
    "UpdateFieldDomain" /**< Dataset capability for supporting                 \
                           UpdateFieldDomain()*/

#define ODrCCreateDataSource                                                   \
    "CreateDataSource" /**< Driver capability for datasource creation */
#define ODrCDeleteDataSource                                                   \
    "DeleteDataSource" /**< Driver capability for datasource deletion */

/* -------------------------------------------------------------------- */
/*      Layer metadata items.                                           */
/* -------------------------------------------------------------------- */
/** Capability set to YES as metadata on a layer that has features with
  * 64 bit identifiers.
  @since GDAL 2.0
  */
#define OLMD_FID64 "OLMD_FID64"

/************************************************************************/
/*                  ogr_featurestyle.h related definitions.             */
/************************************************************************/

/**
 * OGRStyleTool derived class types (returned by GetType()).
 */

typedef enum ogr_style_tool_class_id
{
    OGRSTCNone = 0,   /**< None */
    OGRSTCPen = 1,    /**< Pen */
    OGRSTCBrush = 2,  /**< Brush */
    OGRSTCSymbol = 3, /**< Symbol */
    OGRSTCLabel = 4,  /**< Label */
    OGRSTCVector = 5  /**< Vector */
} OGRSTClassId;

/**
 * List of units supported by OGRStyleTools.
 */
typedef enum ogr_style_tool_units_id
{
    OGRSTUGround = 0, /**< Ground unit */
    OGRSTUPixel = 1,  /**< Pixel */
    OGRSTUPoints = 2, /**< Points */
    OGRSTUMM = 3,     /**< Millimeter */
    OGRSTUCM = 4,     /**< Centimeter */
    OGRSTUInches = 5  /**< Inch */
} OGRSTUnitId;

/**
 * List of parameters for use with OGRStylePen.
 */
typedef enum ogr_style_tool_param_pen_id
{
    OGRSTPenColor = 0,     /**< Color */
    OGRSTPenWidth = 1,     /**< Width */
    OGRSTPenPattern = 2,   /**< Pattern */
    OGRSTPenId = 3,        /**< Id */
    OGRSTPenPerOffset = 4, /**< Perpendicular offset */
    OGRSTPenCap = 5,       /**< Cap */
    OGRSTPenJoin = 6,      /**< Join */
    OGRSTPenPriority = 7,  /**< Priority */
#ifndef DOXYGEN_SKIP
    OGRSTPenLast = 8
#endif
} OGRSTPenParam;

/**
 * List of parameters for use with OGRStyleBrush.
 */
typedef enum ogr_style_tool_param_brush_id
{
    OGRSTBrushFColor = 0,   /**< Foreground color */
    OGRSTBrushBColor = 1,   /**< Background color */
    OGRSTBrushId = 2,       /**< Id */
    OGRSTBrushAngle = 3,    /**< Angle */
    OGRSTBrushSize = 4,     /**< Size */
    OGRSTBrushDx = 5,       /**< Dx */
    OGRSTBrushDy = 6,       /**< Dy */
    OGRSTBrushPriority = 7, /**< Priority */
#ifndef DOXYGEN_SKIP
    OGRSTBrushLast = 8
#endif

} OGRSTBrushParam;

/**
 * List of parameters for use with OGRStyleSymbol.
 */
typedef enum ogr_style_tool_param_symbol_id
{
    OGRSTSymbolId = 0,        /**< Id */
    OGRSTSymbolAngle = 1,     /**< Angle */
    OGRSTSymbolColor = 2,     /**< Color */
    OGRSTSymbolSize = 3,      /**< Size */
    OGRSTSymbolDx = 4,        /**< Dx */
    OGRSTSymbolDy = 5,        /**< Dy */
    OGRSTSymbolStep = 6,      /**< Step */
    OGRSTSymbolPerp = 7,      /**< Perpendicular */
    OGRSTSymbolOffset = 8,    /**< Offset */
    OGRSTSymbolPriority = 9,  /**< Priority */
    OGRSTSymbolFontName = 10, /**< Font name */
    OGRSTSymbolOColor = 11,   /**< Outline color */
#ifndef DOXYGEN_SKIP
    OGRSTSymbolLast = 12
#endif
} OGRSTSymbolParam;

/**
 * List of parameters for use with OGRStyleLabel.
 */
typedef enum ogr_style_tool_param_label_id
{
    OGRSTLabelFontName = 0,   /**< Font name */
    OGRSTLabelSize = 1,       /**< Size */
    OGRSTLabelTextString = 2, /**< Text string */
    OGRSTLabelAngle = 3,      /**< Angle */
    OGRSTLabelFColor = 4,     /**< Foreground color */
    OGRSTLabelBColor = 5,     /**< Background color */
    OGRSTLabelPlacement = 6,  /**< Placement */
    OGRSTLabelAnchor = 7,     /**< Anchor */
    OGRSTLabelDx = 8,         /**< Dx */
    OGRSTLabelDy = 9,         /**< Dy */
    OGRSTLabelPerp = 10,      /**< Perpendicular */
    OGRSTLabelBold = 11,      /**< Bold */
    OGRSTLabelItalic = 12,    /**< Italic */
    OGRSTLabelUnderline = 13, /**< Underline */
    OGRSTLabelPriority = 14,  /**< Priority */
    OGRSTLabelStrikeout = 15, /**< Strike out */
    OGRSTLabelStretch = 16,   /**< Stretch */
    OGRSTLabelAdjHor = 17,    /**< OBSOLETE; do not use */
    OGRSTLabelAdjVert = 18,   /**< OBSOLETE; do not use */
    OGRSTLabelHColor = 19,    /**< Highlight color */
    OGRSTLabelOColor = 20,    /**< Outline color */
#ifndef DOXYGEN_SKIP
    OGRSTLabelLast = 21
#endif
} OGRSTLabelParam;

/* -------------------------------------------------------------------- */
/*                          Field domains                               */
/* -------------------------------------------------------------------- */

/** Associates a code and a value
 *
 * @since GDAL 3.3
 */
typedef struct
{
    /** Code. Content should be of the type of the OGRFieldDomain */
    char *pszCode;

    /** Value. Might be NULL */
    char *pszValue;
} OGRCodedValue;

/** Type of field domain.
 *
 * @since GDAL 3.3
 */
typedef enum
{
    /** Coded */
    OFDT_CODED,
    /** Range (min/max) */
    OFDT_RANGE,
    /** Glob (used by GeoPackage) */
    OFDT_GLOB
} OGRFieldDomainType;

/** Split policy for field domains.
 *
 * When a feature is split in two, defines how the value of attributes
 * following the domain are computed.
 *
 * @since GDAL 3.3
 */
typedef enum
{
    /** Default value */
    OFDSP_DEFAULT_VALUE,
    /** Duplicate */
    OFDSP_DUPLICATE,
    /** New values are computed by the ratio of their area/length compared to
       the area/length of the original feature */
    OFDSP_GEOMETRY_RATIO
} OGRFieldDomainSplitPolicy;

/** Merge policy for field domains.
 *
 * When a feature is built by merging two features, defines how the value of
 * attributes following the domain are computed.
 *
 * @since GDAL 3.3
 */
typedef enum
{
    /** Default value */
    OFDMP_DEFAULT_VALUE,
    /** Sum */
    OFDMP_SUM,
    /** New values are computed as the weighted average of the source values. */
    OFDMP_GEOMETRY_WEIGHTED
} OGRFieldDomainMergePolicy;

/* ------------------------------------------------------------------- */
/*                        Version checking                             */
/* -------------------------------------------------------------------- */

#ifndef DOXYGEN_SKIP

/* Note to developers : please keep this section in sync with gdal.h */

#ifndef GDAL_VERSION_INFO_DEFINED
#define GDAL_VERSION_INFO_DEFINED
const char CPL_DLL *CPL_STDCALL GDALVersionInfo(const char *);
#endif

#ifndef GDAL_CHECK_VERSION

/** Return TRUE if GDAL library version at runtime matches
   nVersionMajor.nVersionMinor.

    The purpose of this method is to ensure that calling code will run with the
   GDAL version it is compiled for. It is primarily indented for external
   plugins.

    @param nVersionMajor Major version to be tested against
    @param nVersionMinor Minor version to be tested against
    @param pszCallingComponentName If not NULL, in case of version mismatch, the
   method will issue a failure mentioning the name of the calling component.
  */
int CPL_DLL CPL_STDCALL GDALCheckVersion(int nVersionMajor, int nVersionMinor,
                                         const char *pszCallingComponentName);

/** Helper macro for GDALCheckVersion */
#define GDAL_CHECK_VERSION(pszCallingComponentName)                            \
    GDALCheckVersion(GDAL_VERSION_MAJOR, GDAL_VERSION_MINOR,                   \
                     pszCallingComponentName)

#endif

#endif /* #ifndef DOXYGEN_SKIP */

CPL_C_END

#endif /* ndef OGR_CORE_H_INCLUDED */
