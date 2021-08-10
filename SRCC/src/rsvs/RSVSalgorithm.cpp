#include "RSVSalgorithm.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

#include "snake.hpp"
#include "snakeengine.hpp"
#include "warning.hpp"

using namespace std;

std::vector<int> FindSpawnVerts(const mesh &meshin, vector<int> &vertList, vector<int> &voluOutList, int outerBorder)
{
    // Function which identifies spawn points
    // Spawn points are:
    //  - Any point part of a cell which touches the void
    //    Put that is not part of a surface that is on the void
    //    itself.
    //  - Points That are on the border of two cells one with some VOS
    //    One without.

    int ii, ni, jj, nj, surfSurb, vert_i;
    vector<int> offBorderVert, internalSurf, voluIndIntern;

    if (outerBorder == 1)
    { // spawn at outer borders
        meshin.GetOffBorderVert(offBorderVert, voluOutList, 1);
        meshin.ElmOnParentBound(internalSurf, voluIndIntern, false, true);
    }
    else if (outerBorder == 0)
    {
        meshin.GetOffBorderVert(offBorderVert, voluOutList, 0);
        meshin.ElmOnParentBound(internalSurf, voluIndIntern, false, false);
    }
    else
    {
        RSVS3D_ERROR_ARGUMENT("outerBorder has an unknown value");
    }
    ni = meshin.verts.size();
    vertList.reserve(ni);
    vertList.clear();
    ni = internalSurf.size();

    cout << "Vertices : " << ni << " (interal), ";
    for (ii = 0; ii < ni; ++ii)
    {
        surfSurb = meshin.surfs.find(internalSurf[ii]);
        nj = meshin.surfs(surfSurb)->edgeind.size();
        for (jj = 0; jj < nj; jj++)
        {
            vert_i = meshin.edges.isearch(meshin.surfs(surfSurb)->edgeind[jj])->vertind[0];
            if (!meshin.verts.isearch(vert_i)->isBorder)
            {
                vertList.push_back(meshin.verts.isearch(vert_i)->index);
            }
            vert_i = meshin.edges.isearch(meshin.surfs(surfSurb)->edgeind[jj])->vertind[1];
            if (!meshin.verts.isearch(vert_i)->isBorder)
            {
                vertList.push_back(meshin.verts.isearch(vert_i)->index);
            }
        }
    }

    sort(offBorderVert);
    unique(offBorderVert);
    ni = offBorderVert.size();
    cout << ni << " (border); " << endl;
    for (ii = 0; ii < ni; ++ii)
    {
        vertList.push_back(offBorderVert[ii]);
    }

    sort(vertList);
    unique(vertList);

    ni = voluIndIntern.size();
    for (ii = 0; ii < ni; ++ii)
    {
        voluOutList.push_back(voluIndIntern[ii]);
    }

    sort(voluOutList);
    unique(voluOutList);
    return (offBorderVert);
}

void SpawnSnakeAndMove(snake &snakein, std::vector<int> vertSpawn)
{
    vector<int> isImpact;
    vector<double> dt;
    int nVe, nE, nS, nVo;
    snakein.snakeconn.size(nVe, nE, nS, nVo);
    int nNew = vertSpawn.size();
    // cout << "vertices to output " << ni << endl;
    snakein.reserve(nVe + nNew * 15, nE + nNew * 15, nS + nNew * 15, nVo + nNew * 1);
    for (int ii = 0; ii < nNew; ++ii)
    {
        SpawnAtVertex(snakein, vertSpawn[ii]);
        // cout << ii << " " ;
    }
    // cout << " vertices Dones" << endl;
    // Move to half distances
    int nTotSnax = snakein.snaxs.size();
    for (int ii = 0; ii < nVe; ++ii)
    {
        snakein.snaxs[ii].v = 0.0;
        snakein.snaxs[ii].d = 0.5;
    }
    for (int ii = nVe; ii < nTotSnax; ++ii)
    {
        snakein.snaxs[ii].v = 1.0;
    }
    snakein.CalculateTimeStep(dt, 0.51);
    snakein.UpdateDistance(dt);
    snakein.PrepareForUse();
    snakein.UpdateCoord();
    snakein.PrepareForUse();
    snakein.SnaxImpactDetection(isImpact);
    MergeAllContactVertices(snakein, isImpact);
    snakein.PrepareForUse();
    CleanupSnakeConnec(snakein);
    snakein.PrepareForUse();
}

void SpawnRSVS(snake &snakein, int outerBorder)
{
    // Function which handles
    // - spawning
    // - growing the snake
    // - Identifying snaxels to remove
    // - update connectivity:
    //     + find snaxsurfs in invalid snake volus(surfs)
    //     + find snaxedges in these surfs
    //     + find snaxels
    //     * invalid snakevolus=[border cell, empty cell]
    int ii, ni;
    vector<int> vertSpawn, borderVertSpawn;
    vector<int> voluSnaxDelete;
    vector<int> isImpact;
    vector<double> dt;
    snakein.snakemesh()->SetBorders();
    // snakein.snakemesh()->disp();
    borderVertSpawn = FindSpawnVerts(*(snakein.snakemesh()), vertSpawn, voluSnaxDelete, outerBorder);
    ni = vertSpawn.size();
    // cout << "vertices to output " << ni << endl;
    snakein.reserve(ni * 15, ni * 15, ni * 15, ni * 15);
    for (ii = 0; ii < ni; ++ii)
    {
        SpawnAtVertex(snakein, vertSpawn[ii]);
        // cout << ii << " " ;
    }
    // cout << " vertices Dones" << endl;
    // Move to half distances
    ni = snakein.snaxs.size();
    for (ii = 0; ii < ni; ++ii)
    {
        snakein.snaxs[ii].v = 1.0;
    }
    snakein.CalculateTimeStep(dt, 0.6);
    snakein.UpdateDistance(dt);
    snakein.PrepareForUse();
    snakein.UpdateCoord();
    snakein.PrepareForUse();
    // snakein.displight();
    // detect and remove impacts
    snakein.SnaxImpactDetection(isImpact);
    MergeAllContactVertices(snakein, isImpact);
    snakein.PrepareForUse();
    CleanupSnakeConnec(snakein);
    snakein.PrepareForUse();
    // snakein.displight();
    // Remove one of the 'snakes'
    if (snakein.Check3D())
    {
        RemoveSnakeInVolu(snakein, voluSnaxDelete, outerBorder);
    }
    else
    {
        RemoveSnakeInSurf(snakein, voluSnaxDelete, outerBorder);
    }
    snakein.PrepareForUse();
    // snakein.displight();
    // Second spawn phase
    if (outerBorder == 1)
    {
        SpawnSnakeAndMove(snakein, borderVertSpawn);
    }

    snakein.OrientFaces();
    cout << "Initialisation DONE!" << endl;
}

void RemoveSnakeInVolu(snake &snakein, vector<int> &voluInd, int outerBorder)
{
    int ii, ni, jj, nj, nBlocks;
    vector<int> delSurf, delEdge, delSnax, tempSurf, tempEdge, tempSnax, subSurf, subEdge, vertBlocks;
    vector<bool> isBlockDel;

    delSurf.reserve(snakein.snaxsurfs.size());
    delEdge.reserve(snakein.snaxedges.size());
    delSnax.reserve(snakein.snaxs.size());

    ni = voluInd.size();

    for (ii = 0; ii < ni; ++ii)
    {
        tempSurf.clear();
        snakein.snaxsurfs.findsiblings(voluInd[ii], tempSurf);
        tempEdge = ConcatenateVectorField(snakein.snakeconn.surfs, &surf::edgeind, tempSurf);
        subEdge = snakein.snakeconn.edges.find_list(tempEdge);

        tempSnax = ConcatenateVectorField(snakein.snakeconn.edges, &edge::vertind, subEdge);
        nj = tempSnax.size();
        for (jj = 0; jj < nj; ++jj)
        {
            delSnax.push_back(tempSnax[jj]);
        }
    }
    // cout << "nSnax " << delSnax.size() << endl;
    // cout << "Find snax to del" << endl;
    vertBlocks.clear();
    nBlocks = snakein.snakeconn.ConnectedVertex(vertBlocks);
    bool delVertInVolu = outerBorder > 0;
    isBlockDel.assign(nBlocks, !delVertInVolu);
    ni = delSnax.size();
    for (ii = 0; ii < ni; ++ii)
    {
        isBlockDel[vertBlocks[snakein.snakeconn.verts.find(delSnax[ii])] - 1] = delVertInVolu;
    }
    // cout << "Find All snax to del" << endl;
    delSnax.clear();
    delSurf.clear();
    delEdge.clear();
    ni = vertBlocks.size();

    for (ii = 0; ii < ni; ++ii)
    {
        if (isBlockDel[vertBlocks[ii] - 1])
        {
            delSnax.push_back(snakein.snaxs(ii)->index);

            subEdge = snakein.snakeconn.edges.find_list(snakein.snakeconn.verts(ii)->edgeind);
            nj = snakein.snakeconn.verts(ii)->edgeind.size();
            for (jj = 0; jj < nj; ++jj)
            {
                delEdge.push_back(snakein.snakeconn.verts(ii)->edgeind[jj]);
            }

            tempSurf = ConcatenateVectorField(snakein.snakeconn.edges, &edge::surfind, subEdge);
            nj = tempSurf.size();
            for (jj = 0; jj < nj; ++jj)
            {
                delSurf.push_back(tempSurf[jj]);
            }
        }
    }

    ni = delSurf.size();
    for (ii = 0; ii < ni; ++ii)
    {
        snakein.snakeconn.RemoveIndex(3, delSurf[ii]);
    }

    sort(delSnax);
    unique(delSnax);
    sort(delEdge);
    unique(delEdge);
    sort(delSurf);
    unique(delSurf);

    // ni=delEdge.size();
    // for(ii=0; ii<ni; ++ii){snakein.snakeconn.RemoveIndex(2,delEdge[ii]);}

    // snakein.displight();
    snakein.snaxs.remove(delSnax);
    snakein.snaxedges.remove(delEdge);
    snakein.snaxsurfs.remove(delSurf);
    snakein.snakeconn.verts.remove(delSnax);
    snakein.snakeconn.edges.remove(delEdge);
    snakein.snakeconn.surfs.remove(delSurf);

    snakein.snakeconn.TightenConnectivity();
    snakein.HashArray();
    snakein.snakeconn.TestConnectivityBiDir(__PRETTY_FUNCTION__);
    snakein.ForceCloseContainers();
    snakein.PrepareForUse();
    // snakein.displight();
    if (outerBorder > 0)
    {
        snakein.Flip();
    }
    // cout << "Before Assignement of internal verts" << endl;
    snakein.AssignInternalVerts();
    // cout << "After Assignement of internal verts" << endl;
}

void RemoveSnakeInSurf(snake &snakein, vector<int> &voluInd, int outerBorder)
{
    int ii, ni, jj, nj, nBlocks;
    vector<int> delSurf, delEdge, delSnax, tempSurf, tempEdge, tempSnax, subSurf, subEdge, vertBlocks;
    vector<bool> isBlockDel;

    delSurf.reserve(snakein.snaxsurfs.size());
    delEdge.reserve(snakein.snaxedges.size());
    delSnax.reserve(snakein.snaxs.size());

    ni = voluInd.size();

    for (ii = 0; ii < ni; ++ii)
    {
        tempSurf.clear();
        snakein.snaxedges.findsiblings(voluInd[ii], subEdge);
        // tempEdge=ConcatenateVectorField(snakein.snakeconn.surfs,
        // &surf::edgeind,tempSurf);
        // subEdge=snakein.snakeconn.edges.find_list(tempEdge);

        tempSnax = ConcatenateVectorField(snakein.snakeconn.edges, &edge::vertind, subEdge);
        nj = tempSnax.size();
        for (jj = 0; jj < nj; ++jj)
        {
            delSnax.push_back(tempSnax[jj]);
        }
    }
    // cout << "nSnax " << delSnax.size() << endl;
    // cout << "Find snax to del" << endl;
    vertBlocks.clear();
    nBlocks = snakein.snakeconn.ConnectedVertex(vertBlocks);
    bool delVertInVolu = outerBorder > 0;
    isBlockDel.assign(nBlocks, !delVertInVolu);
    ni = delSnax.size();
    for (ii = 0; ii < ni; ++ii)
    {
        isBlockDel[vertBlocks[snakein.snakeconn.verts.find(delSnax[ii])] - 1] = delVertInVolu;
    }
    // cout << "Find All snax to del" << endl;
    delSnax.clear();
    delSurf.clear();
    delEdge.clear();
    ni = vertBlocks.size();

    for (ii = 0; ii < ni; ++ii)
    {
        if (isBlockDel[vertBlocks[ii] - 1])
        {
            delSnax.push_back(snakein.snaxs(ii)->index);

            subEdge = snakein.snakeconn.edges.find_list(snakein.snakeconn.verts(ii)->edgeind);
            nj = snakein.snakeconn.verts(ii)->edgeind.size();
            for (jj = 0; jj < nj; ++jj)
            {
                delEdge.push_back(snakein.snakeconn.verts(ii)->edgeind[jj]);
            }

            // tempSurf=ConcatenateVectorField(snakein.snakeconn.edges,
            // &edge::surfind,subEdge); nj=tempSurf.size(); for(jj=0; jj<nj; ++jj){
            // 	delSurf.push_back(tempSurf[jj]);
            // }
        }
    }

    ni = delEdge.size();
    for (ii = 0; ii < ni; ++ii)
    {
        snakein.snakeconn.RemoveIndex(2, delEdge[ii]);
    }

    sort(delSnax);
    unique(delSnax);
    sort(delEdge);
    unique(delEdge);
    // sort(delSurf);
    // unique(delSurf);

    // ni=delEdge.size();
    // for(ii=0; ii<ni; ++ii){snakein.snakeconn.RemoveIndex(2,delEdge[ii]);}

    // snakein.displight();
    snakein.snaxs.remove(delSnax);
    snakein.snaxedges.remove(delEdge);
    // snakein.snaxsurfs.remove(delSurf);
    snakein.snakeconn.verts.remove(delSnax);
    snakein.snakeconn.edges.remove(delEdge);
    // snakein.snakeconn.surfs.remove(delSurf);

    snakein.snakeconn.TightenConnectivity();
    snakein.HashArray();
    snakein.snakeconn.TestConnectivityBiDir(__PRETTY_FUNCTION__);
    snakein.ForceCloseContainers();
    snakein.PrepareForUse();
    // snakein.displight();
    if (outerBorder > 0)
    {
        snakein.Flip();
    }
    // cout << "Before Assignement of internal verts" << endl;
    snakein.AssignInternalVerts();
    // cout << "After Assignement of internal verts" << endl;
}
