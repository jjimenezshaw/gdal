/******************************************************************************
 *
 * Project:  NGSGEOID driver
 * Purpose:  GDALDataset driver for NGSGEOID dataset.
 * Author:   Even Rouault, <even dot rouault at spatialys.com>
 *
 ******************************************************************************
 * Copyright (c) 2011, Even Rouault <even dot rouault at spatialys.com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#include "cpl_string.h"
#include "cpl_vsi_virtual.h"
#include "gdal_frmts.h"
#include "gdal_pam.h"
#include "ogr_srs_api.h"

#define HEADER_SIZE (4 * 8 + 3 * 4)

/************************************************************************/
/* ==================================================================== */
/*                            NGSGEOIDDataset                           */
/* ==================================================================== */
/************************************************************************/

class NGSGEOIDRasterBand;

class NGSGEOIDDataset final : public GDALPamDataset
{
    friend class NGSGEOIDRasterBand;

    VSILFILE *fp{};
    GDALGeoTransform m_gt{};
    int bIsLittleEndian{};
    mutable OGRSpatialReference m_oSRS{};

    static int GetHeaderInfo(const GByte *pBuffer, GDALGeoTransform &gt,
                             int *pnRows, int *pnCols, int *pbIsLittleEndian);

    CPL_DISALLOW_COPY_ASSIGN(NGSGEOIDDataset)

  public:
    NGSGEOIDDataset();
    virtual ~NGSGEOIDDataset();

    virtual CPLErr GetGeoTransform(GDALGeoTransform &gt) const override;
    const OGRSpatialReference *GetSpatialRef() const override;

    static GDALDataset *Open(GDALOpenInfo *);
    static int Identify(GDALOpenInfo *);
};

/************************************************************************/
/* ==================================================================== */
/*                          NGSGEOIDRasterBand                          */
/* ==================================================================== */
/************************************************************************/

class NGSGEOIDRasterBand final : public GDALPamRasterBand
{
    friend class NGSGEOIDDataset;

  public:
    explicit NGSGEOIDRasterBand(NGSGEOIDDataset *);

    virtual CPLErr IReadBlock(int, int, void *) override;

    virtual const char *GetUnitType() override
    {
        return "m";
    }
};

/************************************************************************/
/*                        NGSGEOIDRasterBand()                          */
/************************************************************************/

NGSGEOIDRasterBand::NGSGEOIDRasterBand(NGSGEOIDDataset *poDSIn)

{
    poDS = poDSIn;
    nBand = 1;

    eDataType = GDT_Float32;

    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr NGSGEOIDRasterBand::IReadBlock(CPL_UNUSED int nBlockXOff, int nBlockYOff,
                                      void *pImage)

{
    NGSGEOIDDataset *poGDS = cpl::down_cast<NGSGEOIDDataset *>(poDS);

    /* First values in the file corresponds to the south-most line of the
     * imagery */
    VSIFSeekL(poGDS->fp,
              HEADER_SIZE +
                  static_cast<vsi_l_offset>(nRasterYSize - 1 - nBlockYOff) *
                      nRasterXSize * 4,
              SEEK_SET);

    if (static_cast<int>(VSIFReadL(pImage, 4, nRasterXSize, poGDS->fp)) !=
        nRasterXSize)
        return CE_Failure;

#ifdef CPL_MSB
    if (poGDS->bIsLittleEndian)
    {
        GDALSwapWords(pImage, 4, nRasterXSize, 4);
    }
#endif

#ifdef CPL_LSB
    if (!poGDS->bIsLittleEndian)
    {
        GDALSwapWords(pImage, 4, nRasterXSize, 4);
    }
#endif

    return CE_None;
}

/************************************************************************/
/*                          ~NGSGEOIDDataset()                          */
/************************************************************************/

NGSGEOIDDataset::NGSGEOIDDataset() : fp(nullptr), bIsLittleEndian(TRUE)
{
    m_oSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
}

/************************************************************************/
/*                           ~NGSGEOIDDataset()                         */
/************************************************************************/

NGSGEOIDDataset::~NGSGEOIDDataset()

{
    FlushCache(true);
    if (fp)
        VSIFCloseL(fp);
}

/************************************************************************/
/*                            GetHeaderInfo()                           */
/************************************************************************/

int NGSGEOIDDataset::GetHeaderInfo(const GByte *pBuffer, GDALGeoTransform &gt,
                                   int *pnRows, int *pnCols,
                                   int *pbIsLittleEndian)
{
    /* First check IKIND marker to determine if the file */
    /* is in little or big-endian order, and if it is a valid */
    /* NGSGEOID dataset */
    int nIKIND;
    memcpy(&nIKIND, pBuffer + HEADER_SIZE - 4, 4);
    CPL_LSBPTR32(&nIKIND);
    if (nIKIND == 1)
    {
        *pbIsLittleEndian = TRUE;
    }
    else
    {
        memcpy(&nIKIND, pBuffer + HEADER_SIZE - 4, 4);
        CPL_MSBPTR32(&nIKIND);
        if (nIKIND == 1)
        {
            *pbIsLittleEndian = FALSE;
        }
        else
        {
            return FALSE;
        }
    }

    double dfSLAT;
    memcpy(&dfSLAT, pBuffer, 8);
    if (*pbIsLittleEndian)
    {
        CPL_LSBPTR64(&dfSLAT);
    }
    else
    {
        CPL_MSBPTR64(&dfSLAT);
    }
    pBuffer += 8;

    double dfWLON;
    memcpy(&dfWLON, pBuffer, 8);
    if (*pbIsLittleEndian)
    {
        CPL_LSBPTR64(&dfWLON);
    }
    else
    {
        CPL_MSBPTR64(&dfWLON);
    }
    pBuffer += 8;

    double dfDLAT;
    memcpy(&dfDLAT, pBuffer, 8);
    if (*pbIsLittleEndian)
    {
        CPL_LSBPTR64(&dfDLAT);
    }
    else
    {
        CPL_MSBPTR64(&dfDLAT);
    }
    pBuffer += 8;

    double dfDLON;
    memcpy(&dfDLON, pBuffer, 8);
    if (*pbIsLittleEndian)
    {
        CPL_LSBPTR64(&dfDLON);
    }
    else
    {
        CPL_MSBPTR64(&dfDLON);
    }
    pBuffer += 8;

    int nNLAT;
    memcpy(&nNLAT, pBuffer, 4);
    if (*pbIsLittleEndian)
    {
        CPL_LSBPTR32(&nNLAT);
    }
    else
    {
        CPL_MSBPTR32(&nNLAT);
    }
    pBuffer += 4;

    int nNLON;
    memcpy(&nNLON, pBuffer, 4);
    if (*pbIsLittleEndian)
    {
        CPL_LSBPTR32(&nNLON);
    }
    else
    {
        CPL_MSBPTR32(&nNLON);
    }
    /*pBuffer += 4;*/

    /*CPLDebug("NGSGEOID", "SLAT=%f, WLON=%f, DLAT=%f, DLON=%f, NLAT=%d,
       NLON=%d, IKIND=%d", dfSLAT, dfWLON, dfDLAT, dfDLON, nNLAT, nNLON,
       nIKIND);*/

    if (nNLAT <= 0 || nNLON <= 0 || dfDLAT <= 1e-15 || dfDLON <= 1e-15)
        return FALSE;

    /* Grids go over +180 in longitude */
    // Test written that way to be robust to NaN values
    if (!(dfSLAT >= -90.0 && dfSLAT + nNLAT * dfDLAT <= 90.0 &&
          dfWLON >= -180.0 && dfWLON + nNLON * dfDLON <= 360.0))
        return FALSE;

    gt[0] = dfWLON - dfDLON / 2;
    gt[1] = dfDLON;
    gt[2] = 0.0;
    gt[3] = dfSLAT + nNLAT * dfDLAT - dfDLAT / 2;
    gt[4] = 0.0;
    gt[5] = -dfDLAT;

    *pnRows = nNLAT;
    *pnCols = nNLON;

    return TRUE;
}

/************************************************************************/
/*                             Identify()                               */
/************************************************************************/

int NGSGEOIDDataset::Identify(GDALOpenInfo *poOpenInfo)
{
    if (poOpenInfo->nHeaderBytes < HEADER_SIZE)
        return FALSE;

    GDALGeoTransform gt;
    int nRows, nCols;
    int bIsLittleEndian;
    if (!GetHeaderInfo(poOpenInfo->pabyHeader, gt, &nRows, &nCols,
                       &bIsLittleEndian))
        return FALSE;

    return TRUE;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *NGSGEOIDDataset::Open(GDALOpenInfo *poOpenInfo)

{
    if (!Identify(poOpenInfo) || poOpenInfo->fpL == nullptr)
        return nullptr;

    if (poOpenInfo->eAccess == GA_Update)
    {
        ReportUpdateNotSupportedByDriver("NGSGEOID");
        return nullptr;
    }

    /* -------------------------------------------------------------------- */
    /*      Create a corresponding GDALDataset.                             */
    /* -------------------------------------------------------------------- */
    NGSGEOIDDataset *poDS = new NGSGEOIDDataset();
    poDS->fp = poOpenInfo->fpL;
    poOpenInfo->fpL = nullptr;

    int nRows = 0, nCols = 0;
    GetHeaderInfo(poOpenInfo->pabyHeader, poDS->m_gt, &nRows, &nCols,
                  &poDS->bIsLittleEndian);
    poDS->nRasterXSize = nCols;
    poDS->nRasterYSize = nRows;

    /* -------------------------------------------------------------------- */
    /*      Create band information objects.                                */
    /* -------------------------------------------------------------------- */
    poDS->nBands = 1;
    poDS->SetBand(1, new NGSGEOIDRasterBand(poDS));

    /* -------------------------------------------------------------------- */
    /*      Initialize any PAM information.                                 */
    /* -------------------------------------------------------------------- */
    poDS->SetDescription(poOpenInfo->pszFilename);
    poDS->TryLoadXML();

    /* -------------------------------------------------------------------- */
    /*      Support overviews.                                              */
    /* -------------------------------------------------------------------- */
    poDS->oOvManager.Initialize(poDS, poOpenInfo->pszFilename);
    return poDS;
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr NGSGEOIDDataset::GetGeoTransform(GDALGeoTransform &gt) const

{
    gt = m_gt;

    return CE_None;
}

/************************************************************************/
/*                         GetSpatialRef()                              */
/************************************************************************/

const OGRSpatialReference *NGSGEOIDDataset::GetSpatialRef() const
{
    if (!m_oSRS.IsEmpty())
    {
        return &m_oSRS;
    }

    const CPLString osFilename =
        CPLString(CPLGetBasenameSafe(GetDescription())).tolower();

    // See https://www.ngs.noaa.gov/GEOID/GEOID12B/faq_2012B.shtml

    // GEOID2012 files ?
    if (STARTS_WITH(osFilename, "g2012") && osFilename.size() >= 7)
    {
        if (osFilename[6] == 'h' /* Hawai */ ||
            osFilename[6] == 's' /* Samoa */)
        {
            // NAD83 (PA11)
            m_oSRS.importFromEPSG(6322);
        }
        else if (osFilename[6] == 'g' /* Guam */)
        {
            // NAD83 (MA11)
            m_oSRS.importFromEPSG(6325);
        }
        else
        {
            // NAD83 (2011)
            m_oSRS.importFromEPSG(6318);
        }

        return &m_oSRS;
    }

    // USGG2012 files ? We should return IGS08, but there is only a
    // geocentric CRS in EPSG, so manually forge a geographic one from it
    if (STARTS_WITH(osFilename, "s2012"))
    {
        m_oSRS.importFromWkt(
            "GEOGCS[\"IGS08\",\n"
            "    DATUM[\"IGS08\",\n"
            "        SPHEROID[\"GRS 1980\",6378137,298.257222101,\n"
            "            AUTHORITY[\"EPSG\",\"7019\"]],\n"
            "        AUTHORITY[\"EPSG\",\"1141\"]],\n"
            "    PRIMEM[\"Greenwich\",0,\n"
            "        AUTHORITY[\"EPSG\",\"8901\"]],\n"
            "    UNIT[\"degree\",0.0174532925199433,\n"
            "        AUTHORITY[\"EPSG\",\"9122\"]]]");
        return &m_oSRS;
    }

    m_oSRS.importFromWkt(SRS_WKT_WGS84_LAT_LONG);
    return &m_oSRS;
}

/************************************************************************/
/*                       GDALRegister_NGSGEOID()                        */
/************************************************************************/

void GDALRegister_NGSGEOID()

{
    if (GDALGetDriverByName("NGSGEOID") != nullptr)
        return;

    GDALDriver *poDriver = new GDALDriver();

    poDriver->SetDescription("NGSGEOID");
    poDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME, "NOAA NGS Geoid Height Grids");
    poDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC,
                              "drivers/raster/ngsgeoid.html");
    poDriver->SetMetadataItem(GDAL_DMD_EXTENSION, "bin");

    poDriver->SetMetadataItem(GDAL_DCAP_VIRTUALIO, "YES");

    poDriver->pfnOpen = NGSGEOIDDataset::Open;
    poDriver->pfnIdentify = NGSGEOIDDataset::Identify;

    GetGDALDriverManager()->RegisterDriver(poDriver);
}
