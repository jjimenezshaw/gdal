/******************************************************************************
 *
 * Project:  GDAL/OGR Geography Network support (Geographic Network Model)
 * Purpose:  GNM network class.
 * Authors:  Mikhail Gusev (gusevmihs at gmail dot com)
 *           Dmitry Baryshnikov, polimax@mail.ru
 *
 ******************************************************************************
 * Copyright (c) 2014, Mikhail Gusev
 * Copyright (c) 2014-2015, NextGIS <info@nextgis.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "gnm_api.h"
#include "gnm_priv.h"
#include "ogrsf_frmts.h"

#include <set>

//! @cond Doxygen_Suppress
GNMGenericNetwork::GNMGenericNetwork() : m_asRules()
{
}

GNMGenericNetwork::~GNMGenericNetwork()
{
    for (size_t i = 0; i < m_apoLayers.size(); i++)
        delete m_apoLayers[i];
}

int GNMGenericNetwork::GetLayerCount()
{
    return static_cast<int>(m_apoLayers.size());
}

OGRLayer *GNMGenericNetwork::GetLayer(int nIndex)
{
    if (nIndex < 0 || nIndex >= static_cast<int>(m_apoLayers.size()))
        return nullptr;
    return m_apoLayers[nIndex];
}

OGRErr GNMGenericNetwork::DeleteLayer(int nIndex)
{
    if (nIndex < 0 || nIndex >= static_cast<int>(m_apoLayers.size()))
        return OGRERR_FAILURE;

    const char *pszLayerName = m_apoLayers[nIndex]->GetName();
    OGRFeature *poFeature;

    std::set<GNMGFID> anGFIDs;
    std::set<GNMGFID>::iterator it;
    // remove layer GFID's from Features layer

    m_poFeaturesLayer->ResetReading();
    while ((poFeature = m_poFeaturesLayer->GetNextFeature()) != nullptr)
    {
        const char *pFeatureClass =
            poFeature->GetFieldAsString(GNM_SYSFIELD_LAYERNAME);

        if (EQUAL(pFeatureClass, pszLayerName))
        {
            anGFIDs.insert(poFeature->GetFieldAsGNMGFID(GNM_SYSFIELD_GFID));
            CPL_IGNORE_RET_VAL(
                m_poFeaturesLayer->DeleteFeature(poFeature->GetFID()));
        }
        OGRFeature::DestroyFeature(poFeature);
    }

    // remove GFID's from graph layer

    m_poGraphLayer->ResetReading();
    while ((poFeature = m_poGraphLayer->GetNextFeature()) != nullptr)
    {
        GNMGFID nGFID = poFeature->GetFieldAsGNMGFID(GNM_SYSFIELD_SOURCE);
        it = anGFIDs.find(nGFID);
        if (it != anGFIDs.end())
        {
            CPL_IGNORE_RET_VAL(
                m_poGraphLayer->DeleteFeature(poFeature->GetFID()));
            OGRFeature::DestroyFeature(poFeature);
            continue;
        }

        nGFID = poFeature->GetFieldAsGNMGFID(GNM_SYSFIELD_TARGET);
        it = anGFIDs.find(nGFID);
        if (it != anGFIDs.end())
        {
            CPL_IGNORE_RET_VAL(
                m_poGraphLayer->DeleteFeature(poFeature->GetFID()));
            OGRFeature::DestroyFeature(poFeature);
            continue;
        }

        nGFID = poFeature->GetFieldAsGNMGFID(GNM_SYSFIELD_CONNECTOR);
        it = anGFIDs.find(nGFID);
        if (it != anGFIDs.end())
        {
            CPL_IGNORE_RET_VAL(
                m_poGraphLayer->DeleteFeature(poFeature->GetFID()));
            OGRFeature::DestroyFeature(poFeature);
            continue;
        }

        OGRFeature::DestroyFeature(poFeature);
    }

    // remove connected rules
    for (size_t i = m_asRules.size(); i > 0; --i)
    {
        if (EQUAL(m_asRules[i - 1].GetSourceLayerName(), pszLayerName))
        {
            m_asRules.erase(m_asRules.begin() + i - 1);
            m_bIsRulesChanged = true;
        }
        else if (EQUAL(m_asRules[i - 1].GetTargetLayerName(), pszLayerName))
        {
            m_asRules.erase(m_asRules.begin() + i - 1);
            m_bIsRulesChanged = true;
        }
        else if (EQUAL(m_asRules[i - 1].GetConnectorLayerName(), pszLayerName))
        {
            m_asRules.erase(m_asRules.begin() + i - 1);
            m_bIsRulesChanged = true;
        }
    }

    delete m_apoLayers[nIndex];
    // remove from array
    m_apoLayers.erase(m_apoLayers.begin() + nIndex);
    return OGRERR_NONE;
}

CPLErr GNMGenericNetwork::Delete()
{
    CPLErr eResult = DeleteNetworkLayers();
    if (eResult != CE_None)
        return eResult;
    eResult = DeleteMetadataLayer();
    if (eResult != CE_None)
        return eResult;
    eResult = DeleteGraphLayer();
    if (eResult != CE_None)
        return eResult;

    return DeleteFeaturesLayer();
}

int GNMGenericNetwork::GetVersion() const
{
    return m_nVersion;
}

GIntBig GNMGenericNetwork::GetNewGlobalFID()
{
    return m_nGID++;
}

CPLString GNMGenericNetwork::GetAlgorithmName(GNMDirection eAlgorithm,
                                              bool bShortName)
{
    switch (eAlgorithm)
    {
        case GATDijkstraShortestPath:
            if (bShortName)
                return CPLString("Dijkstra");
            else
                return CPLString("Dijkstra shortest path");
        case GATKShortestPath:
            if (bShortName)
                return CPLString("Yens");
            else
                return CPLString("Yens shortest paths");
        case GATConnectedComponents:
            if (bShortName)
                return CPLString("Connected");
            else
                return CPLString("Connected components");
    }

    return CPLString("Invalid");
}

CPLErr GNMGenericNetwork::AddFeatureGlobalFID(GNMGFID nFID,
                                              const char *pszLayerName)
{
    OGRFeature *poFeature =
        OGRFeature::CreateFeature(m_poFeaturesLayer->GetLayerDefn());
    poFeature->SetField(GNM_SYSFIELD_GFID, nFID);
    poFeature->SetField(GNM_SYSFIELD_LAYERNAME, pszLayerName);

    if (m_poFeaturesLayer->CreateFeature(poFeature) != OGRERR_NONE)
    {
        OGRFeature::DestroyFeature(poFeature);
        CPLError(CE_Failure, CPLE_AppDefined, "Failed to create feature.");
        return CE_Failure;
    }

    OGRFeature::DestroyFeature(poFeature);

    return CE_None;
}

CPLErr GNMGenericNetwork::ConnectFeatures(GNMGFID nSrcGFID, GNMGFID nTgtGFID,
                                          GNMGFID nConGFID, double dfCost,
                                          double dfInvCost, GNMDirection eDir)
{
    if (!m_bIsGraphLoaded && LoadGraph() != CE_None)
    {
        return CE_Failure;
    }

    OGRFeature *poFeature = FindConnection(nSrcGFID, nTgtGFID, nConGFID);
    if (poFeature != nullptr)
    {
        OGRFeature::DestroyFeature(poFeature);
        CPLError(CE_Failure, CPLE_AppDefined, "The connection already created");
        return CE_Failure;
    }

    if (m_asRules.empty())
    {
        CPLError(CE_Failure, CPLE_AppDefined, "The connection forbidden");
        return CE_Failure;
    }
    else
    {
        CPLString soSrcLayerName = m_moFeatureFIDMap[nSrcGFID];
        CPLString soTgtLayerName = m_moFeatureFIDMap[nTgtGFID];
        CPLString soConnLayerName = m_moFeatureFIDMap[nConGFID];
        for (size_t i = 0; i < m_asRules.size(); ++i)
        {
            if (!m_asRules[i].CanConnect(soSrcLayerName, soTgtLayerName,
                                         soConnLayerName))
            {
                CPLError(CE_Failure, CPLE_AppDefined,
                         "The connection forbidden");
                return CE_Failure;
            }
        }
    }

    // we support both vertices and edge to be virtual
    if (nConGFID == -1)
        nConGFID = GetNewVirtualFID();
    if (nSrcGFID == -1)
        nSrcGFID = GetNewVirtualFID();
    if (nTgtGFID == -1)
        nTgtGFID = GetNewVirtualFID();

    poFeature = OGRFeature::CreateFeature(m_poGraphLayer->GetLayerDefn());
    poFeature->SetField(GNM_SYSFIELD_SOURCE, nSrcGFID);
    poFeature->SetField(GNM_SYSFIELD_TARGET, nTgtGFID);
    poFeature->SetField(GNM_SYSFIELD_CONNECTOR, nConGFID);
    poFeature->SetField(GNM_SYSFIELD_COST, dfCost);
    poFeature->SetField(GNM_SYSFIELD_INVCOST, dfInvCost);
    poFeature->SetField(GNM_SYSFIELD_DIRECTION, eDir);
    poFeature->SetField(GNM_SYSFIELD_BLOCKED, GNM_BLOCK_NONE);

    if (m_poGraphLayer->CreateFeature(poFeature) != OGRERR_NONE)
    {
        OGRFeature::DestroyFeature(poFeature);
        CPLError(CE_Failure, CPLE_AppDefined, "Failed to create feature.");
        return CE_Failure;
    }

    OGRFeature::DestroyFeature(poFeature);

    // update graph

    m_oGraph.AddEdge(nConGFID, nSrcGFID, nTgtGFID, eDir == GNM_EDGE_DIR_BOTH,
                     dfCost, dfInvCost);

    return CE_None;
}

CPLErr GNMGenericNetwork::DisconnectFeatures(GNMGFID nSrcGFID, GNMGFID nTgtGFID,
                                             GNMGFID nConGFID)
{
    if (!m_bIsGraphLoaded && LoadGraph() != CE_None)
    {
        return CE_Failure;
    }

    OGRFeature *poFeature = FindConnection(nSrcGFID, nTgtGFID, nConGFID);
    if (poFeature == nullptr)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "The connection not exist");
        return CE_Failure;
    }

    if (m_poGraphLayer->DeleteFeature(poFeature->GetFID()) != OGRERR_NONE)
    {
        OGRFeature::DestroyFeature(poFeature);
        return CE_Failure;
    }

    OGRFeature::DestroyFeature(poFeature);

    // update graph

    m_oGraph.DeleteEdge(nConGFID);

    return CE_None;
}

CPLErr GNMGenericNetwork::DisconnectFeaturesWithId(GNMGFID nFID)
{
    if (!m_bIsGraphLoaded && LoadGraph() != CE_None)
    {
        return CE_Failure;
    }

    CPLString soFilter;
    soFilter.Printf("%s = " GNMGFIDFormat " or %s = " GNMGFIDFormat
                    " or %s = " GNMGFIDFormat,
                    GNM_SYSFIELD_SOURCE, nFID, GNM_SYSFIELD_TARGET, nFID,
                    GNM_SYSFIELD_CONNECTOR, nFID);

    CPLDebug("GNM", "Set attribute filter: %s", soFilter.c_str());

    m_poGraphLayer->SetAttributeFilter(soFilter);
    m_poGraphLayer->ResetReading();
    OGRFeature *poFeature;
    while ((poFeature = m_poGraphLayer->GetNextFeature()) != nullptr)
    {
        if (m_poGraphLayer->DeleteFeature(poFeature->GetFID()) != CE_None)
        {
            OGRFeature::DestroyFeature(poFeature);
            CPLError(CE_Failure, CPLE_AppDefined,
                     "Failed to remove feature connection.");
            return CE_Failure;
        }
        OGRFeature::DestroyFeature(poFeature);
    }

    m_poGraphLayer->SetAttributeFilter(nullptr);

    m_oGraph.DeleteEdge(nFID);
    m_oGraph.DeleteVertex(nFID);

    return CE_None;
}

CPLErr GNMGenericNetwork::ReconnectFeatures(GNMGFID nSrcGFID, GNMGFID nTgtGFID,
                                            GNMGFID nConGFID, double dfCost,
                                            double dfInvCost, GNMDirection eDir)
{
    if (!m_bIsGraphLoaded && LoadGraph() != CE_None)
    {
        return CE_Failure;
    }

    OGRFeature *poFeature = FindConnection(nSrcGFID, nTgtGFID, nConGFID);
    if (poFeature == nullptr)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "The connection not exist");
        return CE_Failure;
    }

    poFeature->SetField(GNM_SYSFIELD_COST, dfCost);
    poFeature->SetField(GNM_SYSFIELD_INVCOST, dfInvCost);
    poFeature->SetField(GNM_SYSFIELD_DIRECTION, eDir);

    if (m_poGraphLayer->SetFeature(poFeature) != OGRERR_NONE)
    {
        OGRFeature::DestroyFeature(poFeature);
        CPLError(CE_Failure, CPLE_AppDefined, "Failed to update feature.");
        return CE_Failure;
    }

    OGRFeature::DestroyFeature(poFeature);

    // update graph

    m_oGraph.ChangeEdge(nConGFID, dfCost, dfInvCost);

    return CE_None;
}

CPLErr GNMGenericNetwork::DisconnectAll()
{
    if (!m_bIsGraphLoaded && LoadGraph() != CE_None)
    {
        return CE_Failure;
    }
    // delete everything from m_pGraphLayer

    OGRFeature *poFeature;
    m_poGraphLayer->ResetReading();
    while ((poFeature = m_poGraphLayer->GetNextFeature()) != nullptr)
    {
        CPL_IGNORE_RET_VAL(m_poGraphLayer->DeleteFeature(poFeature->GetFID()));
        OGRFeature::DestroyFeature(poFeature);
    }

    m_oGraph.Clear();

    return CE_None;
}

OGRFeature *GNMGenericNetwork::GetFeatureByGlobalFID(GNMGFID nFID)
{
    CPLString soLayerName = m_moFeatureFIDMap[nFID];
    for (size_t i = 0; i < m_apoLayers.size(); ++i)
    {
        if (EQUAL(soLayerName, m_apoLayers[i]->GetName()))
            return m_apoLayers[i]->GetFeature(nFID);
    }
    return nullptr;
}

CPLErr GNMGenericNetwork::CreateRule(const char *pszRuleStr)
{
    CPLDebug("GNM", "Try to create rule '%s'", pszRuleStr);
    GNMRule oRule(pszRuleStr);
    if (!oRule.IsValid())
    {
        return CE_Failure;
    }

    if (!oRule.IsAcceptAny())
    {
        bool bSrcExist = false;
        bool bTgtExist = false;
        bool bConnExist = false;
        // check layers exist
        for (size_t i = 0; i < m_apoLayers.size(); ++i)
        {
            if (EQUAL(oRule.GetSourceLayerName(), m_apoLayers[i]->GetName()))
            {
                bSrcExist = true;
            }
            else if (EQUAL(oRule.GetTargetLayerName(),
                           m_apoLayers[i]->GetName()))
            {
                bTgtExist = true;
            }
            else if (EQUAL(oRule.GetConnectorLayerName(),
                           m_apoLayers[i]->GetName()))
            {
                bConnExist = true;
            }
        }

        if (!bSrcExist || !bTgtExist)
        {
            CPLError(CE_Failure, CPLE_IllegalArg,
                     "Layers '%s' or '%s' not exist",
                     oRule.GetSourceLayerName().c_str(),
                     oRule.GetTargetLayerName().c_str());
            return CE_Failure;
        }

        if (!bConnExist && !oRule.GetConnectorLayerName().empty())
        {
            CPLError(CE_Failure, CPLE_IllegalArg,
                     "Connector layer '%s' not exist",
                     oRule.GetConnectorLayerName().c_str());
            return CE_Failure;
        }
    }

    m_asRules.push_back(std::move(oRule));
    m_bIsRulesChanged = true;

    return CE_None;
}

CPLErr GNMGenericNetwork::DeleteAllRules()
{
    CPLString soFilter;
    soFilter.Printf("%s LIKE '%s%%'", GNM_SYSFIELD_PARAMNAME, GNM_MD_RULE);
    m_poMetadataLayer->SetAttributeFilter(soFilter);

    m_poMetadataLayer->ResetReading();
    OGRFeature *poFeature;
    std::vector<GIntBig> aFIDs;
    while ((poFeature = m_poMetadataLayer->GetNextFeature()) != nullptr)
    {
        aFIDs.push_back(poFeature->GetFID());
        OGRFeature::DestroyFeature(poFeature);
    }

    m_poMetadataLayer->SetAttributeFilter(nullptr);
    for (size_t i = 0; i < aFIDs.size(); ++i)
    {
        CPL_IGNORE_RET_VAL(m_poMetadataLayer->DeleteFeature(aFIDs[i]));
    }

    return CE_None;
}

CPLErr GNMGenericNetwork::DeleteRule(const char *pszRuleStr)
{
    for (size_t i = 0; i < m_asRules.size(); ++i)
    {
        if (EQUAL(pszRuleStr, m_asRules[i]))
        {
            m_asRules.erase(m_asRules.begin() + i);
            m_bIsRulesChanged = true;
            return CE_None;
        }
    }

    return CE_Failure;
}

char **GNMGenericNetwork::GetRules() const
{
    char **papszRules = nullptr;
    for (size_t i = 0; i < m_asRules.size(); ++i)
    {
        papszRules = CSLAddString(papszRules, m_asRules[i]);
    }
    return papszRules;
}

CPLErr GNMGenericNetwork::ConnectPointsByLines(char **papszLayerList,
                                               double dfTolerance,
                                               double dfCost, double dfInvCost,
                                               GNMDirection eDir)
{
    if (CSLCount(papszLayerList) < 2)
    {
        CPLError(CE_Failure, CPLE_IllegalArg,
                 "Minimum 2 layers needed to connect");
        return CE_Failure;
    }

    std::vector<OGRLayer *> paLineLayers;
    std::vector<OGRLayer *> paPointLayers;
    int eType;
    int iLayer;
    OGRLayer *poLayer;

    for (iLayer = 0; papszLayerList[iLayer] != nullptr; ++iLayer)
    {
        poLayer = GetLayerByName(papszLayerList[iLayer]);
        if (nullptr == poLayer)
            continue;

        eType = wkbFlatten(poLayer->GetGeomType());
        if (eType == wkbLineString || eType == wkbMultiLineString)
        {
            paLineLayers.push_back(poLayer);
        }
        else if (eType == wkbPoint)
        {
            paPointLayers.push_back(poLayer);
        }
    }

    if (paLineLayers.empty() || paPointLayers.empty())
    {
        CPLError(CE_Failure, CPLE_IllegalArg,
                 "Need at least one line (or "
                 "multiline) layer and one point layer to connect");
        return CE_Failure;
    }

    // now walk through all lines and find nearest points for line start and end
    for (size_t i = 0; i < paLineLayers.size(); ++i)
    {
        poLayer = paLineLayers[i];

        poLayer->ResetReading();
        for (auto &&poFeature : poLayer)
        {
            const OGRGeometry *poGeom = poFeature->GetGeometryRef();
            if (nullptr != poGeom)
            {
                eType = wkbFlatten(poGeom->getGeometryType());
                if (eType == wkbLineString)
                {
                    const auto poLineString = poGeom->toLineString();
                    ConnectPointsByLine(poFeature->GetFID(), poLineString,
                                        paPointLayers, dfTolerance, dfCost,
                                        dfInvCost, eDir);
                }
                else if (eType == wkbMultiLineString)
                {
                    const auto poMultiLineString = poGeom->toMultiLineString();
                    ConnectPointsByMultiline(
                        poFeature->GetFID(), poMultiLineString, paPointLayers,
                        dfTolerance, dfCost, dfInvCost, eDir);
                }
            }
        }
    }

    return CE_None;
}

CPLErr GNMGenericNetwork::ChangeBlockState(GNMGFID nFID, bool bIsBlock)
{
    if (!m_bIsGraphLoaded && LoadGraph() != CE_None)
    {
        return CE_Failure;
    }

    // change block state in layer
    OGRLayer *poLayer = GetLayerByName(m_moFeatureFIDMap[nFID]);
    if (nullptr == poLayer)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Failed to get layer '%s'.",
                 m_moFeatureFIDMap[nFID].c_str());
        return CE_Failure;
    }

    OGRFeature *poFeature = poLayer->GetFeature(nFID);
    if (nullptr == poFeature)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Failed to get feature '" GNMGFIDFormat "'.", nFID);
        return CE_Failure;
    }

    if (bIsBlock)
    {
        poFeature->SetField(GNM_SYSFIELD_BLOCKED, GNM_BLOCK_ALL);
    }
    else
    {
        poFeature->SetField(GNM_SYSFIELD_BLOCKED, GNM_BLOCK_NONE);
    }

    if (poLayer->SetFeature(poFeature) != OGRERR_NONE)
    {
        OGRFeature::DestroyFeature(poFeature);
        CPLError(CE_Failure, CPLE_AppDefined, "Failed to update feature.");
        return CE_Failure;
    }

    OGRFeature::DestroyFeature(poFeature);

    GNMGFID nSrcFID, nTgtFID, nConFID;

    // change block state in graph layer
    m_poGraphLayer->ResetReading();
    while ((poFeature = m_poGraphLayer->GetNextFeature()) != nullptr)
    {
        nSrcFID = poFeature->GetFieldAsGNMGFID(GNM_SYSFIELD_SOURCE);
        nTgtFID = poFeature->GetFieldAsGNMGFID(GNM_SYSFIELD_TARGET);
        nConFID = poFeature->GetFieldAsGNMGFID(GNM_SYSFIELD_CONNECTOR);
        int nBlockState = poFeature->GetFieldAsInteger(GNM_SYSFIELD_BLOCKED);

        if (bIsBlock)
        {
            if (nSrcFID == nFID)
                nBlockState |= GNM_BLOCK_SRC;
            else if (nTgtFID == nFID)
                nBlockState |= GNM_BLOCK_TGT;
            else if (nConFID == nFID)
                nBlockState |= GNM_BLOCK_CONN;

            poFeature->SetField(GNM_SYSFIELD_BLOCKED, nBlockState);
        }
        else
        {
            if (nSrcFID == nFID)
                nBlockState &= ~GNM_BLOCK_SRC;
            else if (nTgtFID == nFID)
                nBlockState &= ~GNM_BLOCK_TGT;
            else if (nConFID == nFID)
                nBlockState &= ~GNM_BLOCK_CONN;

            poFeature->SetField(GNM_SYSFIELD_BLOCKED, nBlockState);
        }

        if (m_poGraphLayer->SetFeature(poFeature) != OGRERR_NONE)
        {
            OGRFeature::DestroyFeature(poFeature);
            CPLError(CE_Failure, CPLE_AppDefined, "Failed to update feature.");
            return CE_Failure;
        }

        OGRFeature::DestroyFeature(poFeature);
    }

    // change block state in graph
    m_oGraph.ChangeBlockState(nFID, bIsBlock);

    return CE_None;
}

CPLErr GNMGenericNetwork::ChangeAllBlockState(bool bIsBlock)
{
    if (!m_bIsGraphLoaded && LoadGraph() != CE_None)
    {
        return CE_Failure;
    }

    OGRFeature *poFeature;
    m_poGraphLayer->ResetReading();
    while ((poFeature = m_poGraphLayer->GetNextFeature()) != nullptr)
    {
        if (bIsBlock)
        {
            poFeature->SetField(GNM_SYSFIELD_BLOCKED, GNM_BLOCK_ALL);
        }
        else
        {
            poFeature->SetField(GNM_SYSFIELD_BLOCKED, GNM_BLOCK_NONE);
        }

        if (m_poGraphLayer->SetFeature(poFeature) != OGRERR_NONE)
        {
            OGRFeature::DestroyFeature(poFeature);
            CPLError(CE_Failure, CPLE_AppDefined, "Failed to update feature.");
            return CE_Failure;
        }

        OGRFeature::DestroyFeature(poFeature);
    }

    // change all network layers

    for (size_t i = 0; i < m_apoLayers.size(); ++i)
    {
        OGRLayer *poLayer = m_apoLayers[i];
        if (nullptr == poLayer)
            continue;
        while ((poFeature = poLayer->GetNextFeature()) != nullptr)
        {
            if (bIsBlock)
            {
                poFeature->SetField(GNM_SYSFIELD_BLOCKED, GNM_BLOCK_ALL);
            }
            else
            {
                poFeature->SetField(GNM_SYSFIELD_BLOCKED, GNM_BLOCK_NONE);
            }

            if (poLayer->SetFeature(poFeature) != OGRERR_NONE)
            {
                OGRFeature::DestroyFeature(poFeature);
                CPLError(CE_Failure, CPLE_AppDefined,
                         "Failed to update feature.");
                return CE_Failure;
            }

            OGRFeature::DestroyFeature(poFeature);
        }
    }

    m_oGraph.ChangeAllBlockState(bIsBlock);

    return CE_None;
}

OGRLayer *GNMGenericNetwork::GetPath(GNMGFID nStartFID, GNMGFID nEndFID,
                                     GNMGraphAlgorithmType eAlgorithm,
                                     char **papszOptions)
{

    if (!m_bIsGraphLoaded && LoadGraph() != CE_None)
    {
        return nullptr;
    }

    GDALDriver *poMEMDrv =
        OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName("Memory");
    if (poMEMDrv == nullptr)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Cannot load 'Memory' driver");
        return nullptr;
    }

    GDALDataset *poMEMDS =
        poMEMDrv->Create("dummy_name", 0, 0, 0, GDT_Unknown, nullptr);
    OGRSpatialReference oDstSpaRef(GetProjectionRef());
    OGRLayer *poMEMLayer =
        poMEMDS->CreateLayer(GetAlgorithmName(eAlgorithm, true), &oDstSpaRef,
                             wkbGeometryCollection, nullptr);

    OGRGNMWrappedResultLayer *poResLayer =
        new OGRGNMWrappedResultLayer(poMEMDS, poMEMLayer);

    const bool bReturnEdges =
        CPLFetchBool(papszOptions, GNM_MD_FETCHEDGES, true);
    const bool bReturnVertices =
        CPLFetchBool(papszOptions, GNM_MD_FETCHVERTEX, true);

    switch (eAlgorithm)
    {
        case GATDijkstraShortestPath:
        {
            GNMPATH path = m_oGraph.DijkstraShortestPath(nStartFID, nEndFID);

            // fill features in result layer
            FillResultLayer(poResLayer, path, 1, bReturnVertices, bReturnEdges);
        }
        break;
        case GATKShortestPath:
        {
            int nK =
                atoi(CSLFetchNameValueDef(papszOptions, GNM_MD_NUM_PATHS, "1"));

            CPLDebug("GNM", "Search %d path(s)", nK);

            std::vector<GNMPATH> paths =
                m_oGraph.KShortestPaths(nStartFID, nEndFID, nK);

            // fill features in result layer
            for (size_t i = 0; i < paths.size(); ++i)
            {
                FillResultLayer(poResLayer, paths[i], static_cast<int>(i + 1),
                                bReturnVertices, bReturnEdges);
            }
        }
        break;
        case GATConnectedComponents:
        {
            GNMVECTOR anEmitters;
            if (nullptr != papszOptions)
            {
                char **papszEmitter =
                    CSLFetchNameValueMultiple(papszOptions, GNM_MD_EMITTER);
                for (int i = 0; papszEmitter[i] != nullptr; ++i)
                {
                    GNMGFID nEmitter = atol(papszEmitter[i]);
                    anEmitters.push_back(nEmitter);
                }
                CSLDestroy(papszEmitter);
            }

            if (nStartFID != -1)
            {
                anEmitters.push_back(nStartFID);
            }

            if (nStartFID != -1)
            {
                anEmitters.push_back(nEndFID);
            }

            GNMPATH path = m_oGraph.ConnectedComponents(anEmitters);

            // fill features in result layer
            FillResultLayer(poResLayer, path, 1, bReturnVertices, bReturnEdges);
        }
        break;
    }

    return poResLayer;
}

void GNMGenericNetwork::ConnectPointsByMultiline(
    GIntBig nFID, const OGRMultiLineString *poMultiLineString,
    const std::vector<OGRLayer *> &paPointLayers, double dfTolerance,
    double dfCost, double dfInvCost, GNMDirection eDir)
{
    VALIDATE_POINTER0(poMultiLineString,
                      "GNMGenericNetwork::ConnectPointsByMultiline");
    for (auto &&poLineString : poMultiLineString)
    {
        ConnectPointsByLine(nFID, poLineString, paPointLayers, dfTolerance,
                            dfCost, dfInvCost, eDir);
    }
}

void GNMGenericNetwork::ConnectPointsByLine(
    GIntBig nFID, const OGRLineString *poLineString,
    const std::vector<OGRLayer *> &paPointLayers, double dfTolerance,
    double dfCost, double dfInvCost, GNMDirection eDir)
{
    VALIDATE_POINTER0(poLineString, "GNMGenericNetwork::ConnectPointsByLine");
    OGRPoint oStartPoint, oEndPoint;
    poLineString->StartPoint(&oStartPoint);
    poLineString->EndPoint(&oEndPoint);
    double dfHalfTolerance = dfTolerance / 2;

    GNMGFID nSrcFID =
        FindNearestPoint(&oStartPoint, paPointLayers, dfHalfTolerance);
    GNMGFID nTgtFID =
        FindNearestPoint(&oEndPoint, paPointLayers, dfHalfTolerance);

    if (nSrcFID == -1 || nTgtFID == -1)
        return;

    // connect nSrcFID with nTgtFID via nFID
    ConnectFeatures(nSrcFID, nTgtFID, static_cast<GNMGFID>(nFID), dfCost,
                    dfInvCost, eDir);
}

GNMGFID GNMGenericNetwork::FindNearestPoint(
    const OGRPoint *poPoint, const std::vector<OGRLayer *> &paPointLayers,
    double dfTolerance)
{
    VALIDATE_POINTER1(poPoint, "GNMGenericNetwork::FindNearestPoint", -1);
    double dfMinX = poPoint->getX() - dfTolerance;
    double dfMinY = poPoint->getY() - dfTolerance;
    double dfMaxX = poPoint->getX() + dfTolerance;
    double dfMaxY = poPoint->getY() + dfTolerance;

    OGRFeature *poFeature;

    for (size_t i = 0; i < paPointLayers.size(); ++i)
    {
        OGRLayer *poLayer = paPointLayers[i];

        poLayer->SetSpatialFilterRect(dfMinX, dfMinY, dfMaxX, dfMaxY);
        poLayer->ResetReading();
        while ((poFeature = poLayer->GetNextFeature()) != nullptr)
        {
            GNMGFID nRetFID = poFeature->GetFieldAsGNMGFID(GNM_SYSFIELD_GFID);
            OGRFeature::DestroyFeature(poFeature);
            return nRetFID;
        }
    }

    return -1;
}

OGRFeature *GNMGenericNetwork::FindConnection(GNMGFID nSrcFID, GNMGFID nTgtFID,
                                              GNMGFID nConFID)
{

    CPLString soFilter;
    soFilter.Printf("%s = " GNMGFIDFormat " and %s = " GNMGFIDFormat
                    " and %s = " GNMGFIDFormat,
                    GNM_SYSFIELD_SOURCE, nSrcFID, GNM_SYSFIELD_TARGET, nTgtFID,
                    GNM_SYSFIELD_CONNECTOR, nConFID);

    CPLDebug("GNM", "Set attribute filter: %s", soFilter.c_str());

    m_poGraphLayer->SetAttributeFilter(soFilter);
    m_poGraphLayer->ResetReading();
    OGRFeature *f = m_poGraphLayer->GetNextFeature();
    m_poGraphLayer->SetAttributeFilter(nullptr);
    return f;
}

bool GNMGenericNetwork::SaveRules()
{
    if (!m_bIsRulesChanged)
        return true;

    if (DeleteAllRules() != CE_None)
        return false;

    bool bOK = true;
    for (int i = 0; i < static_cast<int>(m_asRules.size()); ++i)
    {
        auto poFeature =
            std::make_unique<OGRFeature>(m_poMetadataLayer->GetLayerDefn());
        poFeature->SetField(GNM_SYSFIELD_PARAMNAME,
                            CPLSPrintf("%s%d", GNM_MD_RULE, i + 1));
        poFeature->SetField(GNM_SYSFIELD_PARAMVALUE, m_asRules[i]);
        if (m_poMetadataLayer->CreateFeature(poFeature.get()) != OGRERR_NONE)
        {
            CPLError(CE_Failure, CPLE_AppDefined, "Write rule '%s' failed",
                     m_asRules[i].c_str());
            bOK = false;
            // TODO: do we need interrupt here?
            // return CE_Failure;
        }
    }
    return bOK;
}

GNMGFID GNMGenericNetwork::GetNewVirtualFID()
{
    return --m_nVirtualConnectionGID;
}

void GNMGenericNetwork::FillResultLayer(OGRGNMWrappedResultLayer *poResLayer,
                                        const GNMPATH &path, int nNoOfPath,
                                        bool bReturnVertices, bool bReturnEdges)
{
    for (size_t i = 0; i < path.size(); ++i)
    {
        if (bReturnVertices)
        {
            GNMGFID nGFID = path[i].first;

            // TODO: create feature for virtual vertex
            // if(nGFID < -1) {...}

            CPLString soLayerName = m_moFeatureFIDMap[nGFID];
            OGRFeature *poFeature = GetFeatureByGlobalFID(nGFID);
            if (nullptr != poFeature)
            {
                poResLayer->InsertFeature(poFeature, soLayerName, nNoOfPath,
                                          false);

                OGRFeature::DestroyFeature(poFeature);
            }
        }

        if (bReturnEdges)
        {
            GNMGFID nGFID = path[i].second;

            // TODO: create feature for virtual edge
            // if(nGFID < -1) {...}

            CPLString soLayerName = m_moFeatureFIDMap[nGFID];
            OGRFeature *poFeature = GetFeatureByGlobalFID(nGFID);
            if (nullptr != poFeature)
            {
                poResLayer->InsertFeature(poFeature, soLayerName, nNoOfPath,
                                          true);
                OGRFeature::DestroyFeature(poFeature);
            }
        }
    }
}

CPLErr GNMGenericNetwork::CheckLayerDriver(const char *pszDefaultDriverName,
                                           char **papszOptions)
{
    if (nullptr == m_poLayerDriver)
    {
        const char *pszDriverName = CSLFetchNameValueDef(
            papszOptions, GNM_MD_FORMAT, pszDefaultDriverName);

        if (!CheckStorageDriverSupport(pszDriverName))
        {
            CPLError(CE_Failure, CPLE_IllegalArg,
                     "%s driver not supported as network storage",
                     pszDriverName);
            return CE_Failure;
        }

        m_poLayerDriver =
            GetGDALDriverManager()->GetDriverByName(pszDriverName);
        if (nullptr == m_poLayerDriver)
        {
            CPLError(CE_Failure, CPLE_IllegalArg, "%s driver not available",
                     pszDriverName);
            return CE_Failure;
        }
    }
    return CE_None;
}

CPLErr GNMGenericNetwork::CreateMetadataLayer(GDALDataset *const pDS,
                                              int nVersion, size_t nFieldSize)
{
    OGRLayer *pMetadataLayer =
        pDS->CreateLayer(GNM_SYSLAYER_META, nullptr, wkbNone, nullptr);
    if (nullptr == pMetadataLayer)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Creation of '%s' layer failed",
                 GNM_SYSLAYER_META);
        return CE_Failure;
    }

    OGRFieldDefn oFieldKey(GNM_SYSFIELD_PARAMNAME, OFTString);
    oFieldKey.SetWidth(static_cast<int>(nFieldSize));
    OGRFieldDefn oFieldValue(GNM_SYSFIELD_PARAMVALUE, OFTString);
    oFieldValue.SetWidth(static_cast<int>(nFieldSize));

    if (pMetadataLayer->CreateField(&oFieldKey) != OGRERR_NONE ||
        pMetadataLayer->CreateField(&oFieldValue) != OGRERR_NONE)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Creation of layer '%s' fields failed", GNM_SYSLAYER_META);
        return CE_Failure;
    }

    OGRFeature *poFeature;

    // write name
    poFeature = OGRFeature::CreateFeature(pMetadataLayer->GetLayerDefn());
    poFeature->SetField(GNM_SYSFIELD_PARAMNAME, GNM_MD_NAME);
    poFeature->SetField(GNM_SYSFIELD_PARAMVALUE, m_soName);
    if (pMetadataLayer->CreateFeature(poFeature) != OGRERR_NONE)
    {
        OGRFeature::DestroyFeature(poFeature);
        CPLError(CE_Failure, CPLE_AppDefined, "Write GNM name failed");
        return CE_Failure;
    }
    OGRFeature::DestroyFeature(poFeature);

    // write version
    poFeature = OGRFeature::CreateFeature(pMetadataLayer->GetLayerDefn());
    poFeature->SetField(GNM_SYSFIELD_PARAMNAME, GNM_MD_VERSION);
    poFeature->SetField(GNM_SYSFIELD_PARAMVALUE, CPLSPrintf("%d", nVersion));
    if (pMetadataLayer->CreateFeature(poFeature) != OGRERR_NONE)
    {
        OGRFeature::DestroyFeature(poFeature);
        CPLError(CE_Failure, CPLE_AppDefined, "Write GNM version failed");
        return CE_Failure;
    }
    OGRFeature::DestroyFeature(poFeature);

    // write description
    if (!sDescription.empty())
    {
        poFeature = OGRFeature::CreateFeature(pMetadataLayer->GetLayerDefn());
        poFeature->SetField(GNM_SYSFIELD_PARAMNAME, GNM_MD_DESCR);
        poFeature->SetField(GNM_SYSFIELD_PARAMVALUE, sDescription);
        if (pMetadataLayer->CreateFeature(poFeature) != OGRERR_NONE)
        {
            OGRFeature::DestroyFeature(poFeature);
            CPLError(CE_Failure, CPLE_AppDefined,
                     "Write GNM description failed");
            return CE_Failure;
        }
        OGRFeature::DestroyFeature(poFeature);
    }

    // write srs if < 254 or create file
    if (!m_oSRS.IsEmpty())
    {
        char *pszWKT = nullptr;
        m_oSRS.exportToWkt(&pszWKT);
        const std::string soSRS = pszWKT ? pszWKT : "";
        CPLFree(pszWKT);
        if (soSRS.size() >= nFieldSize)
        {
            // cppcheck-suppress knownConditionTrueFalse
            if (StoreNetworkSrs() != CE_None)
                return CE_Failure;
        }
        else
        {
            poFeature =
                OGRFeature::CreateFeature(pMetadataLayer->GetLayerDefn());
            poFeature->SetField(GNM_SYSFIELD_PARAMNAME, GNM_MD_SRS);
            poFeature->SetField(GNM_SYSFIELD_PARAMVALUE, soSRS.c_str());
            if (pMetadataLayer->CreateFeature(poFeature) != OGRERR_NONE)
            {
                OGRFeature::DestroyFeature(poFeature);
                CPLError(CE_Failure, CPLE_AppDefined, "Write GNM SRS failed");
                return CE_Failure;
            }
            OGRFeature::DestroyFeature(poFeature);
        }
    }

    m_poMetadataLayer = pMetadataLayer;

    m_nVersion = nVersion;

    // create default rule
    return CreateRule("ALLOW CONNECTS ANY");
}

CPLErr GNMGenericNetwork::StoreNetworkSrs()
{
    return CE_Failure;
}

CPLErr GNMGenericNetwork::LoadNetworkSrs()
{
    return CE_Failure;
}

CPLErr GNMGenericNetwork::CreateGraphLayer(GDALDataset *const pDS)
{
    m_poGraphLayer =
        pDS->CreateLayer(GNM_SYSLAYER_GRAPH, nullptr, wkbNone, nullptr);
    if (nullptr == m_poGraphLayer)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Creation of '%s' layer failed",
                 GNM_SYSLAYER_GRAPH);
        return CE_Failure;
    }

    OGRFieldDefn oFieldSrc(GNM_SYSFIELD_SOURCE, GNMGFIDInt);
    OGRFieldDefn oFieldDst(GNM_SYSFIELD_TARGET, GNMGFIDInt);
    OGRFieldDefn oFieldConnector(GNM_SYSFIELD_CONNECTOR, GNMGFIDInt);
    OGRFieldDefn oFieldCost(GNM_SYSFIELD_COST, OFTReal);
    OGRFieldDefn oFieldInvCost(GNM_SYSFIELD_INVCOST, OFTReal);
    OGRFieldDefn oFieldDir(GNM_SYSFIELD_DIRECTION, OFTInteger);
    OGRFieldDefn oFieldBlock(GNM_SYSFIELD_BLOCKED, OFTInteger);

    if (m_poGraphLayer->CreateField(&oFieldSrc) != OGRERR_NONE ||
        m_poGraphLayer->CreateField(&oFieldDst) != OGRERR_NONE ||
        m_poGraphLayer->CreateField(&oFieldConnector) != OGRERR_NONE ||
        m_poGraphLayer->CreateField(&oFieldCost) != OGRERR_NONE ||
        m_poGraphLayer->CreateField(&oFieldInvCost) != OGRERR_NONE ||
        m_poGraphLayer->CreateField(&oFieldDir) != OGRERR_NONE ||
        m_poGraphLayer->CreateField(&oFieldBlock) != OGRERR_NONE)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Creation of layer '%s' fields failed", GNM_SYSLAYER_GRAPH);
        return CE_Failure;
    }

    return CE_None;
}

CPLErr GNMGenericNetwork::CreateFeaturesLayer(GDALDataset *const pDS)
{
    m_poFeaturesLayer =
        pDS->CreateLayer(GNM_SYSLAYER_FEATURES, nullptr, wkbNone, nullptr);
    if (nullptr == m_poFeaturesLayer)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Creation of '%s' layer failed",
                 GNM_SYSLAYER_FEATURES);
        return CE_Failure;
    }

    OGRFieldDefn oFieldGID(GNM_SYSFIELD_GFID, GNMGFIDInt);
    OGRFieldDefn oFieldLayerName(GNM_SYSFIELD_LAYERNAME, OFTString);
    oFieldLayerName.SetWidth(254);

    if (m_poFeaturesLayer->CreateField(&oFieldGID) != OGRERR_NONE ||
        m_poFeaturesLayer->CreateField(&oFieldLayerName) != OGRERR_NONE)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Creation of layer '%s' fields failed", GNM_SYSLAYER_FEATURES);
        return CE_Failure;
    }

    return CE_None;
}

CPLErr GNMGenericNetwork::LoadMetadataLayer(GDALDataset *const pDS)
{
    // read version, description, SRS, classes, rules
    m_poMetadataLayer = pDS->GetLayerByName(GNM_SYSLAYER_META);
    if (nullptr == m_poMetadataLayer)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Loading of '%s' layer failed",
                 GNM_SYSLAYER_META);
        return CE_Failure;
    }

    std::map<int, GNMRule> moRules;
    int nRulePrefixLen = static_cast<int>(CPLStrnlen(GNM_MD_RULE, 255));
    OGRFeature *poFeature;
    m_poMetadataLayer->ResetReading();
    while ((poFeature = m_poMetadataLayer->GetNextFeature()) != nullptr)
    {
        const char *pKey = poFeature->GetFieldAsString(GNM_SYSFIELD_PARAMNAME);
        const char *pValue =
            poFeature->GetFieldAsString(GNM_SYSFIELD_PARAMVALUE);

        CPLDebug("GNM", "Load metadata. Key: %s, value %s", pKey, pValue);

        if (EQUAL(pKey, GNM_MD_NAME))
        {
            m_soName = pValue;
        }
        else if (EQUAL(pKey, GNM_MD_DESCR))
        {
            sDescription = pValue;
        }
        else if (EQUAL(pKey, GNM_MD_SRS))
        {
            m_oSRS.importFromWkt(pValue);
        }
        else if (EQUAL(pKey, GNM_MD_VERSION))
        {
            m_nVersion = atoi(pValue);
        }
        else if (EQUALN(pKey, GNM_MD_RULE, nRulePrefixLen))
        {
            moRules[atoi(pKey + nRulePrefixLen)] = GNMRule(pValue);
        }

        OGRFeature::DestroyFeature(poFeature);
    }

    for (std::map<int, GNMRule>::iterator it = moRules.begin();
         it != moRules.end(); ++it)
    {
        if (it->second.IsValid())
            m_asRules.push_back(it->second);
    }

    if (!m_oSRS.IsEmpty())
    {
        // cppcheck-suppress knownConditionTrueFalse
        if (LoadNetworkSrs() != CE_None)
            return CE_Failure;
    }

    return CE_None;
}

CPLErr GNMGenericNetwork::LoadGraphLayer(GDALDataset *const pDS)
{
    m_poGraphLayer = pDS->GetLayerByName(GNM_SYSLAYER_GRAPH);
    if (nullptr == m_poGraphLayer)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Loading of '%s' layer failed",
                 GNM_SYSLAYER_GRAPH);
        return CE_Failure;
    }

    return CE_None;
}

CPLErr GNMGenericNetwork::LoadGraph()
{
    if (m_bIsGraphLoaded)
        return CE_None;

    if (nullptr == m_poGraphLayer)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Loading of graph data failed");
        return CE_Failure;
    }

    OGRFeature *poFeature;
    m_poGraphLayer->ResetReading();
    GNMGFID nSrcFID, nTgtFID, nConFID;
    double dfCost, dfInvCost;
    while ((poFeature = m_poGraphLayer->GetNextFeature()) != nullptr)
    {
        nSrcFID = poFeature->GetFieldAsGNMGFID(GNM_SYSFIELD_SOURCE);
        nTgtFID = poFeature->GetFieldAsGNMGFID(GNM_SYSFIELD_TARGET);
        nConFID = poFeature->GetFieldAsGNMGFID(GNM_SYSFIELD_CONNECTOR);
        dfCost = poFeature->GetFieldAsDouble(GNM_SYSFIELD_COST);
        dfInvCost = poFeature->GetFieldAsDouble(GNM_SYSFIELD_INVCOST);
        GNMDirection eDir =
            poFeature->GetFieldAsInteger(GNM_SYSFIELD_DIRECTION);

        int nBlockState = poFeature->GetFieldAsInteger(GNM_SYSFIELD_BLOCKED);

        bool bIsBlock = GNM_BLOCK_NONE != nBlockState;

        m_oGraph.AddEdge(nConFID, nSrcFID, nTgtFID, eDir == GNM_EDGE_DIR_BOTH,
                         dfCost, dfInvCost);

        if (bIsBlock)
        {
            if (nBlockState & GNM_BLOCK_SRC)
                m_oGraph.ChangeBlockState(nSrcFID, bIsBlock);
            if (nBlockState & GNM_BLOCK_TGT)
                m_oGraph.ChangeBlockState(nTgtFID, bIsBlock);
            if (nBlockState & GNM_BLOCK_CONN)
                m_oGraph.ChangeBlockState(nConFID, bIsBlock);
        }

        if (nConFID < m_nVirtualConnectionGID)
            m_nVirtualConnectionGID = nConFID;

        OGRFeature::DestroyFeature(poFeature);
    }

    m_bIsGraphLoaded = true;
    return CE_None;
}

CPLErr GNMGenericNetwork::LoadFeaturesLayer(GDALDataset *const pDS)
{
    m_poFeaturesLayer = pDS->GetLayerByName(GNM_SYSLAYER_FEATURES);
    if (nullptr == m_poFeaturesLayer)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Loading of '%s' layer failed",
                 GNM_SYSLAYER_FEATURES);
        return CE_Failure;
    }

    OGRFeature *poFeature;
    m_poFeaturesLayer->ResetReading();
    while ((poFeature = m_poFeaturesLayer->GetNextFeature()) != nullptr)
    {
        GNMGFID nFID = poFeature->GetFieldAsGNMGFID(GNM_SYSFIELD_GFID);
        const char *pFeatureClass =
            poFeature->GetFieldAsString(GNM_SYSFIELD_LAYERNAME);

        if (nFID >= m_nGID)
            m_nGID = nFID + 1;

        m_moFeatureFIDMap[nFID] = pFeatureClass;

        // Load network layer. No error handling as we want to load whole
        // network
        LoadNetworkLayer(pFeatureClass);

        OGRFeature::DestroyFeature(poFeature);
    }
    return CE_None;
}

int GNMGenericNetwork::TestCapability(const char *pszCap)

{
    if (EQUAL(pszCap, ODsCCreateLayer))
        return TRUE;
    else if (EQUAL(pszCap, ODsCDeleteLayer))
        return TRUE;
    else
        return FALSE;
}

OGRLayer *GNMGenericNetwork::CopyLayer(OGRLayer *poSrcLayer,
                                       const char *pszNewName,
                                       char **papszOptions)
{
    CPLStringList aosOptions(CSLDuplicate(papszOptions));
    aosOptions.SetNameValue("DST_SRSWKT", GetProjectionRef());
    return GDALDataset::CopyLayer(poSrcLayer, pszNewName, aosOptions.List());
}

int GNMGenericNetwork::CloseDependentDatasets()
{
    size_t nCount = m_apoLayers.size();
    for (size_t i = 0; i < nCount; ++i)
    {
        delete m_apoLayers[i];
    }
    m_apoLayers.clear();

    GNMNetwork::CloseDependentDatasets();

    return nCount > 0 ? TRUE : FALSE;
}

CPLErr GNMGenericNetwork::FlushCache(bool bAtClosing)
{
    CPLErr eErr = CE_None;
    if (!SaveRules())
        eErr = CE_Failure;

    if (GNMNetwork::FlushCache(bAtClosing) != CE_None)
        eErr = CE_Failure;
    return eErr;
}

//--- C API --------------------------------------------------------------------

CPLErr CPL_STDCALL GNMConnectFeatures(GNMGenericNetworkH hNet, GNMGFID nSrcFID,
                                      GNMGFID nTgtFID, GNMGFID nConFID,
                                      double dfCost, double dfInvCost,
                                      GNMDirection eDir)
{
    VALIDATE_POINTER1(hNet, "GNMConnectFeatures", CE_Failure);

    return GNMGenericNetwork::FromHandle(hNet)->ConnectFeatures(
        nSrcFID, nTgtFID, nConFID, dfCost, dfInvCost, eDir);
}

CPLErr CPL_STDCALL GNMDisconnectFeatures(GNMGenericNetworkH hNet,
                                         GNMGFID nSrcFID, GNMGFID nTgtFID,
                                         GNMGFID nConFID)
{
    VALIDATE_POINTER1(hNet, "GNMDisconnectFeatures", CE_Failure);

    return GNMGenericNetwork::FromHandle(hNet)->DisconnectFeatures(
        nSrcFID, nTgtFID, nConFID);
}

CPLErr CPL_STDCALL GNMDisconnectFeaturesWithId(GNMGenericNetworkH hNet,
                                               GNMGFID nFID)
{
    VALIDATE_POINTER1(hNet, "GNMDisconnectFeaturesWithId", CE_Failure);

    return GNMGenericNetwork::FromHandle(hNet)->DisconnectFeaturesWithId(nFID);
}

CPLErr CPL_STDCALL GNMReconnectFeatures(GNMGenericNetworkH hNet,
                                        GNMGFID nSrcFID, GNMGFID nTgtFID,
                                        GNMGFID nConFID, double dfCost,
                                        double dfInvCost, GNMDirection eDir)
{
    VALIDATE_POINTER1(hNet, "GNMReconnectFeatures", CE_Failure);

    return GNMGenericNetwork::FromHandle(hNet)->ReconnectFeatures(
        nSrcFID, nTgtFID, nConFID, dfCost, dfInvCost, eDir);
}

CPLErr CPL_STDCALL GNMCreateRule(GNMGenericNetworkH hNet,
                                 const char *pszRuleStr)
{
    VALIDATE_POINTER1(hNet, "GNMCreateRule", CE_Failure);

    return GNMGenericNetwork::FromHandle(hNet)->CreateRule(pszRuleStr);
}

CPLErr CPL_STDCALL GNMDeleteAllRules(GNMGenericNetworkH hNet)
{
    VALIDATE_POINTER1(hNet, "GNMDeleteAllRules", CE_Failure);

    return GNMGenericNetwork::FromHandle(hNet)->DeleteAllRules();
}

CPLErr CPL_STDCALL GNMDeleteRule(GNMGenericNetworkH hNet,
                                 const char *pszRuleStr)
{
    VALIDATE_POINTER1(hNet, "GNMDeleteRule", CE_Failure);

    return GNMGenericNetwork::FromHandle(hNet)->DeleteRule(pszRuleStr);
}

char **CPL_STDCALL GNMGetRules(GNMGenericNetworkH hNet)
{
    VALIDATE_POINTER1(hNet, "GNMDeleteRule", nullptr);

    return GNMGenericNetwork::FromHandle(hNet)->GetRules();
}

CPLErr CPL_STDCALL GNMConnectPointsByLines(GNMGenericNetworkH hNet,
                                           char **papszLayerList,
                                           double dfTolerance, double dfCost,
                                           double dfInvCost, GNMDirection eDir)
{
    VALIDATE_POINTER1(hNet, "GNMConnectPointsByLines", CE_Failure);

    return GNMGenericNetwork::FromHandle(hNet)->ConnectPointsByLines(
        papszLayerList, dfTolerance, dfCost, dfInvCost, eDir);
}

CPLErr CPL_STDCALL GNMChangeBlockState(GNMGenericNetworkH hNet, GNMGFID nFID,
                                       bool bIsBlock)
{
    VALIDATE_POINTER1(hNet, "GNMChangeBlockState", CE_Failure);

    return GNMGenericNetwork::FromHandle(hNet)->ChangeBlockState(nFID,
                                                                 bIsBlock);
}

CPLErr CPL_STDCALL GNMChangeAllBlockState(GNMGenericNetworkH hNet, int bIsBlock)
{
    VALIDATE_POINTER1(hNet, "GNMChangeAllBlockState", CE_Failure);

    return GNMGenericNetwork::FromHandle(hNet)->ChangeAllBlockState(bIsBlock ==
                                                                    TRUE);
}

//! @endcond
