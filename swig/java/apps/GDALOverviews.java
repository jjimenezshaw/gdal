/******************************************************************************
 *
 * Name:     GDALOverviews.java
 * Project:  GDAL Java Interface
 * Purpose:  A sample app to create GDAL raster overviews.
 * Author:   Even Rouault, <even dot rouault at spatialys.com>
 *
 * Port from GDALOverviews.cs by Tamas Szekeres
 *
 ******************************************************************************
 * Copyright (c) 2009, Even Rouault
 * Copyright (c) 2007, Tamas Szekeres
 *
 * SPDX-License-Identifier: MIT
 *****************************************************************************/

import org.gdal.gdalconst.gdalconst;
import org.gdal.gdal.gdal;
import org.gdal.gdal.Dataset;
import org.gdal.gdal.Band;
import org.gdal.gdal.TermProgressCallback;


/**

 * <p>Title: GDAL Java GDALOverviews example.</p>
 * <p>Description: A sample app to create GDAL raster overviews.</p>
 * @author Tamas Szekeres (szekerest@gmail.com)
 * @version 1.0
 */



/// <summary>
/// A Java based sample to create GDAL raster overviews.
/// </summary>

class GDALOverviews {

	public static void usage()

	{
		System.out.println("usage: gdaloverviews {GDAL dataset name} {resamplealg} {level1} {level2} ....");
		System.out.println("example: gdaloverviews sample.tif \"NEAREST\" 2 4");
		System.exit(-1);
	}

    public static void main(String[] args)
    {
        /* -------------------------------------------------------------------- */
        /*      Register driver(s).                                             */
        /* -------------------------------------------------------------------- */
        gdal.AllRegister();

        args = gdal.GeneralCmdLineProcessor(args);
        if (args.length <= 2) usage();

        try
        {
            /* -------------------------------------------------------------------- */
            /*      Open dataset.                                                   */
            /* -------------------------------------------------------------------- */
            Dataset ds = gdal.Open( args[0], gdalconst.GA_Update );

            if (ds == null)
            {
                System.out.println("Can't open " + args[0]);
                System.exit(-1);
            }

            System.out.println("Raster dataset parameters:");
            System.out.println("  Projection: " + ds.GetProjectionRef());
            System.out.println("  RasterCount: " + ds.getRasterCount());
            System.out.println("  RasterSize (" + ds.getRasterXSize() + "," + ds.getRasterYSize() + ")");

            int[] levels = new int[args.length -2];

            System.out.println(levels.length);

            for (int i = 2; i < args.length; i++)
            {
                levels[i-2] = Integer.parseInt(args[i]);
            }

            if (ds.BuildOverviews(args[1], levels, new TermProgressCallback()) != gdalconst.CE_None)
            {
                System.out.println("The BuildOverviews operation doesn't work");
                System.exit(-1);
            }

            /* -------------------------------------------------------------------- */
            /*      Displaying the raster parameters                                */
            /* -------------------------------------------------------------------- */
            for (int iBand = 1; iBand <= ds.getRasterCount(); iBand++)
            {
                Band band = ds.GetRasterBand(iBand);
                System.out.println("Band " + iBand + " :");
                System.out.println("   DataType: " + band.getDataType());
                System.out.println("   Size (" + band.getXSize() + "," + band.getYSize() + ")");
                System.out.println("   PaletteInterp: " + gdal.GetColorInterpretationName(band.GetRasterColorInterpretation()));

                for (int iOver = 0; iOver < band.GetOverviewCount(); iOver++)
                {
                    Band over = band.GetOverview(iOver);
                    System.out.println("      OverView " + iOver + " :");
                    System.out.println("         DataType: " + over.getDataType());
                    System.out.println("         Size (" + over.getXSize() + "," + over.getYSize() + ")");
                    System.out.println("         PaletteInterp: " + gdal.GetColorInterpretationName(over.GetRasterColorInterpretation()));
                }
            }

            /* explicit closing of dataset */
            ds.delete();

            System.out.println("Completed.");
            System.out.println("Use:  gdalread " + args[0] + " outfile.png [overview] to extract a particular overview!" );
        }
        catch (Exception e)
        {
            System.out.println("Application error: " + e.getMessage());
        }
    }

}
