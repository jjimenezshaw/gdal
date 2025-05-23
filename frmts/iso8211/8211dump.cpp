/******************************************************************************
 *
 * Project:  SDTS Translator
 * Purpose:  Dump 8211 file in verbose form - just a junk program.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 * Copyright (c) 2013, Even Rouault <even dot rouault at spatialys.com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#include <stdio.h>
#include "iso8211.h"
#include "cpl_vsi.h"
#include "cpl_string.h"

int main(int nArgc, char **papszArgv)

{
    DDFModule oModule;
    const char *pszFilename = nullptr;
    bool bFSPTHack = false;
    bool bXML = false;
    bool bAllDetails = false;

    /* -------------------------------------------------------------------- */
    /*      Check arguments.                                                */
    /* -------------------------------------------------------------------- */
    for (int iArg = 1; iArg < nArgc; iArg++)
    {
        if (EQUAL(papszArgv[iArg], "-fspt_repeating"))
        {
            bFSPTHack = true;
        }
        else if (EQUAL(papszArgv[iArg], "-xml"))
        {
            bXML = true;
        }
        else if (EQUAL(papszArgv[iArg], "-xml_all_details"))
        {
            bXML = TRUE;
            bAllDetails = true;
        }
        else
        {
            pszFilename = papszArgv[iArg];
        }
    }

    if (pszFilename == nullptr)
    {
        printf("Usage: 8211dump [-xml|-xml_all_details] "
               "[-fspt_repeating] filename\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*      Open file.                                                      */
    /* -------------------------------------------------------------------- */
    if (!oModule.Open(pszFilename))
        exit(1);

    /* -------------------------------------------------------------------- */
    /*      Apply FSPT hack if required.                                    */
    /* -------------------------------------------------------------------- */
    if (bFSPTHack)
    {
        DDFFieldDefn *poFSPT = oModule.FindFieldDefn("FSPT");

        if (poFSPT == nullptr)
            fprintf(stderr,
                    "unable to find FSPT field to set repeating flag.\n");
        else
            poFSPT->SetRepeatingFlag(TRUE);
    }

    /* -------------------------------------------------------------------- */
    /*      Dump header, and all records.                                   */
    /* -------------------------------------------------------------------- */
    if (bXML)
    {
        printf("<DDFModule");
        if (bAllDetails)
        {
            printf(" _interchangeLevel=\"%c\"", oModule.GetInterchangeLevel());
            printf(" _leaderIden=\"%c\"", oModule.GetLeaderIden());
            printf(" _inlineCodeExtensionIndicator=\"%c\"",
                   oModule.GetCodeExtensionIndicator());
            printf(" _versionNumber=\"%c\"", oModule.GetVersionNumber());
            printf(" _appIndicator=\"%c\"", oModule.GetAppIndicator());
            printf(" _extendedCharSet=\"%s\"", oModule.GetExtendedCharSet());
            printf(" _fieldControlLength=\"%d\"",
                   oModule.GetFieldControlLength());
            printf(" _sizeFieldLength=\"%d\"", oModule.GetSizeFieldLength());
            printf(" _sizeFieldPos=\"%d\"", oModule.GetSizeFieldPos());
            printf(" _sizeFieldTag=\"%d\"", oModule.GetSizeFieldTag());
        }
        printf(">\n");

        int nFieldDefnCount = oModule.GetFieldCount();
        for (int i = 0; i < nFieldDefnCount; i++)
        {
            DDFFieldDefn *poFieldDefn = oModule.GetField(i);
            const char *pszDataStructCode = nullptr;
            switch (poFieldDefn->GetDataStructCode())
            {
                case dsc_elementary:
                    pszDataStructCode = "elementary";
                    break;

                case dsc_vector:
                    pszDataStructCode = "vector";
                    break;

                case dsc_array:
                    pszDataStructCode = "array";
                    break;

                case dsc_concatenated:
                    pszDataStructCode = "concatenated";
                    break;

                default:
                    pszDataStructCode = "(unknown)";
                    break;
            }

            const char *pszDataTypeCode = nullptr;
            switch (poFieldDefn->GetDataTypeCode())
            {
                case dtc_char_string:
                    pszDataTypeCode = "char_string";
                    break;

                case dtc_implicit_point:
                    pszDataTypeCode = "implicit_point";
                    break;

                case dtc_explicit_point:
                    pszDataTypeCode = "explicit_point";
                    break;

                case dtc_explicit_point_scaled:
                    pszDataTypeCode = "explicit_point_scaled";
                    break;

                case dtc_char_bit_string:
                    pszDataTypeCode = "char_bit_string";
                    break;

                case dtc_bit_string:
                    pszDataTypeCode = "bit_string";
                    break;

                case dtc_mixed_data_type:
                    pszDataTypeCode = "mixed_data_type";
                    break;

                default:
                    pszDataTypeCode = "(unknown)";
                    break;
            }

            printf("<DDFFieldDefn tag=\"%s\" fieldName=\"%s\""
                   " dataStructCode=\"%s\" dataTypeCode=\"%s\"",
                   poFieldDefn->GetName(), poFieldDefn->GetDescription(),
                   pszDataStructCode, pszDataTypeCode);
            int nSubfieldCount = poFieldDefn->GetSubfieldCount();
            if (bAllDetails || nSubfieldCount == 0)
            {
                printf(" arrayDescr=\"%s\"", poFieldDefn->GetArrayDescr());
                printf(" formatControls=\"%s\"",
                       poFieldDefn->GetFormatControls());
            }
            printf(">\n");
            for (int iSubField = 0; iSubField < nSubfieldCount; iSubField++)
            {
                const DDFSubfieldDefn *poSubFieldDefn =
                    poFieldDefn->GetSubfield(iSubField);
                printf("  <DDFSubfieldDefn name=\"%s\" format=\"%s\"/>\n",
                       poSubFieldDefn->GetName(), poSubFieldDefn->GetFormat());
            }
            printf("</DDFFieldDefn>\n");
        }

        // DDFRecord       *poRecord;
        for (DDFRecord *poRecord = oModule.ReadRecord(); poRecord != nullptr;
             poRecord = oModule.ReadRecord())
        {
            printf("<DDFRecord");
            if (bAllDetails)
            {
                if (poRecord->GetReuseHeader())
                    printf(" reuseHeader=\"1\"");
                printf(" dataSize=\"%d\"", poRecord->GetDataSize());
                printf(" _sizeFieldTag=\"%d\"", poRecord->GetSizeFieldTag());
                printf(" _sizeFieldPos=\"%d\"", poRecord->GetSizeFieldPos());
                printf(" _sizeFieldLength=\"%d\"",
                       poRecord->GetSizeFieldLength());
            }
            printf(">\n");
            int nFieldCount = poRecord->GetFieldCount();
            for (int iField = 0; iField < nFieldCount; iField++)
            {
                const DDFField *poField = poRecord->GetField(iField);
                const DDFFieldDefn *poDefn = poField->GetFieldDefn();
                const char *pszFieldName = poDefn->GetName();
                printf("  <DDFField name=\"%s\"", pszFieldName);
                if (poField->GetRepeatCount() > 1)
                    printf(" repeatCount=\"%d\"", poField->GetRepeatCount());
                int iOffset = 0, nLoopCount;
                int nRepeatCount = poField->GetRepeatCount();
                const char *pachData = poField->GetData();
                int nDataSize = poField->GetDataSize();
                if (nRepeatCount == 1 && poDefn->GetSubfieldCount() == 0)
                {
                    printf(" value=\"0x");
                    for (int i = 0; i < nDataSize - 1; i++)
                        printf("%02X", pachData[i]);
                    printf("\">\n");
                }
                else
                    printf(">\n");
                for (nLoopCount = 0; nLoopCount < nRepeatCount; nLoopCount++)
                {
                    for (int iSubField = 0;
                         iSubField < poDefn->GetSubfieldCount(); iSubField++)
                    {
                        int nBytesConsumed;
                        const DDFSubfieldDefn *poSubFieldDefn =
                            poDefn->GetSubfield(iSubField);
                        const char *pszSubFieldName = poSubFieldDefn->GetName();
                        printf("    <DDFSubfield name=\"%s\" ",
                               pszSubFieldName);
                        DDFDataType eType = poSubFieldDefn->GetType();
                        const char *pachSubdata = pachData + iOffset;
                        int nMaxBytes = nDataSize - iOffset;
                        if (eType == DDFFloat)
                            printf("type=\"float\">%f",
                                   poSubFieldDefn->ExtractFloatData(
                                       pachSubdata, nMaxBytes, nullptr));
                        else if (eType == DDFInt)
                            printf("type=\"integer\">%d",
                                   poSubFieldDefn->ExtractIntData(
                                       pachSubdata, nMaxBytes, nullptr));
                        else if (eType == DDFBinaryString)
                        {
                            int nBytes, i;
                            GByte *pabyBString =
                                (GByte *)poSubFieldDefn->ExtractStringData(
                                    pachSubdata, nMaxBytes, &nBytes);

                            printf("type=\"binary\">0x");
                            for (i = 0; i < nBytes; i++)
                                printf("%02X", pabyBString[i]);
                        }
                        else
                        {
                            GByte *pabyString =
                                (GByte *)poSubFieldDefn->ExtractStringData(
                                    pachSubdata, nMaxBytes, nullptr);
                            int bBinary = FALSE;
                            int i;
                            for (i = 0; pabyString[i] != '\0'; i++)
                            {
                                if (pabyString[i] < 32 || pabyString[i] > 127)
                                {
                                    bBinary = TRUE;
                                    break;
                                }
                            }
                            if (bBinary)
                            {
                                printf("type=\"binary\">0x");
                                for (i = 0; pabyString[i] != '\0'; i++)
                                    printf("%02X", pabyString[i]);
                            }
                            else
                            {
                                char *pszEscaped = CPLEscapeString(
                                    (const char *)pabyString, -1, CPLES_XML);
                                printf("type=\"string\">%s", pszEscaped);
                                CPLFree(pszEscaped);
                            }
                        }
                        printf("</DDFSubfield>\n");

                        poSubFieldDefn->GetDataLength(pachSubdata, nMaxBytes,
                                                      &nBytesConsumed);

                        iOffset += nBytesConsumed;
                    }
                }
                printf("  </DDFField>\n");
            }
            printf("</DDFRecord>\n");
        }
        printf("</DDFModule>\n");
    }
    else
    {
        oModule.Dump(stdout);
        long nStartLoc;

        nStartLoc = VSIFTellL(oModule.GetFP());
        for (DDFRecord *poRecord = oModule.ReadRecord(); poRecord != nullptr;
             poRecord = oModule.ReadRecord())
        {
            printf("File Offset: %ld\n", nStartLoc);
            poRecord->Dump(stdout);

            nStartLoc = VSIFTellL(oModule.GetFP());
        }
    }

    oModule.Close();
}
