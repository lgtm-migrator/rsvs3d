#include <iostream>

#include "triangulate.hpp"
#include "meshprocessing.hpp"
#include "warning.hpp"
#include "voxel.hpp"



void FlattenBoundaryFaces(mesh &meshin){
	/*
	Flattens non flat border faces of a mesh by triangulating them

	*/
	mesh meshout;
	vector<int> subList, vertList;
	std::array<bool, 3> isFlat;
	triangulation triangleRSVS;
	int nSurf;
	double tol=1e-9;

	nSurf = meshin.surfs.size();
	subList.reserve(nSurf);

	triangleRSVS.stattri.clear();
	triangleRSVS.trivert.clear();
	triangleRSVS.PrepareForUse();
	// Find Non flat boundary faces (only checks for one coordinate being the same)
	for (int i = 0; i < nSurf; ++i)
	{
		if(meshin.surfs(i)->isBorder  && meshin.surfs(i)->edgeind.size()>3){
			meshin.surfs(i)->OrderedVerts(&meshin,vertList);
			vertList = meshin.verts.find_list(vertList);

			isFlat.fill(true);
			int count = vertList.size();
			for (int j = 1; j < count; ++j)
			{
				for (int k = 0; k < 3; ++k)
				{
					isFlat[k] = isFlat[k] && (fabs(meshin.verts(vertList[j])->coord[k]-
										meshin.verts(vertList[0])->coord[k])<tol);
				}
			}
			if(!(isFlat[0] || isFlat[1] || isFlat[2])){
				subList.push_back(i);
			}
		}
	}
	// triangulate the container
	if(subList.size()>0){
		TriangulateContainer(meshin,triangleRSVS , 1, subList); 
		triangleRSVS.meshDep = &meshin;


		triangleRSVS.PrepareForUse();
		triangleRSVS.CalcTriVertPos();
		triangleRSVS.PrepareForUse();
		// meshin.displight();
		auto newSurfs = ConcatenateVectorField(meshin.surfs,
			&surf::edgeind, subList);
		// cerr << "Num new surfs " << newSurfs.size()-subList.size() 
		// 	<< " from " << subList.size() << endl;
		MeshTriangulation(meshout,meshin,triangleRSVS.stattri,
			triangleRSVS.trivert);
		meshin=meshout;
		meshin.PrepareForUse();
		// meshin.displight();
		meshin.TightenConnectivity();
		meshin.OrderEdges();
		// meshin.displight();
	}
}


std::vector<int> FindHolesInSnake(const snake &snakein,
	const HashedVector<int, int> &uncertainVert){
	/*
	Resolves the problem of finding points from which to initiate voids
	in a snake.

	Takes in a snake and a vector of indices to which snaxels were considered
	close. These have to be treated separately as there is some uncertainty
	about wether they are inside or outside. uncertain Vert is a list of
	indices where the algorithm is known to be invalid
	*/

	int ii, cntII, jj, cntJJ, kk, cntKK, ll;
	auto& vols = snakein.snakeconn.volus;
	std::vector<bool> vertExplored;
	std::vector<int> holeInds;
	int vertAct, holeInd=-1;
	// const vector<int>  &surfEdgeind=currVertList;
	vertExplored.assign(snakein.isMeshVertIn.size(),false);
	// Go through the volume objects vertices,
		// looking for a 'from vertex'
			// From this vertex check if it is in? 
			// Check if it is in closeVert 
			// -> yes and no, we're done
			// -> yes and yes, explore this vertices neighbours
			// -> no, nothing to see here go to next from vertex
	cntII = snakein.snakeconn.volus.size();
	holeInds.reserve(cntII);
	for (ii = 0; ii< cntII; ++ii){
		cntJJ = vols(ii)->surfind.size();
		for (jj = 0; jj< cntJJ; ++jj){
			auto surfEdgeind = snakein.snakeconn.surfs.isearch(
				vols(ii)->surfind[jj])->edgeind;
			cntKK = surfEdgeind.size();
			for (kk = 0; kk<cntKK; ++kk){
				for (ll = 0; ll<2; ++ll){
					vertAct = snakein.snaxs.isearch( 
						snakein.snakeconn.edges(surfEdgeind[kk])->vertind[ll]
						)->fromvert;
					holeInd = FindVertexHole(vertAct, *(snakein.snakemesh), 
						snakein.isMeshVertIn, uncertainVert, vertExplored);
					if(holeInd!=-1){break;}
				}
				if(holeInd!=-1){break;}
			}
			if(holeInd!=-1){break;}
		}
		if(holeInd!=-1){holeInds.push_back(holeInd);}
	}
	return (holeInds);
}

/**
 * @brief      Prepares the snake to be used for CFD, removes duplicate points and 
	triangulates it.
 *
 * @param[in]  snakein      The snakein
 * @param[in]  distanceTol  The distance tolerance
 * @param      meshgeom     The meshgeom
 * @param      holeCoords   The hole coordinates
 */
void PrepareSnakeForCFD(const snake &snakein, double distanceTol,
	mesh &meshgeom, std::vector<double> &holeCoords){
	/*

	Coordinates of holes are also returned.
	*/
	mesh  meshtemp;
	triangulation triRSVS;
	int nHoles;
	// int nPtsHole, kk, count;

	std::vector<int>  holeIndices;

	// tecplotfile tecout;

	// Cleanup the mesh
	// tecout.OpenFile("../TESTOUT/rsvs_tetgen_mesh.plt");
	meshtemp=snakein.snakeconn;
	// tecout.PrintMesh(meshtemp);
	// meshtemp.displight();

	auto groupedVertices = GroupCloseSnaxels(snakein, distanceTol);
	meshtemp.MergeGroupedVertices(groupedVertices);
	meshtemp.RemoveSingularConnectors();

	meshtemp.PrepareForUse();
	meshtemp.TestConnectivityBiDir(__PRETTY_FUNCTION__);
	meshtemp.TightenConnectivity();
	meshtemp.OrderEdges();
	// tecout.PrintMesh(meshtemp);
	// meshtemp.displight();

	// Triangulation preparation
	triRSVS.stattri.clear();
	triRSVS.trivert.clear();
	triRSVS.PrepareForUse();
	TriangulateMesh(meshtemp,triRSVS);

	triRSVS.PrepareForUse();
	triRSVS.CalcTriVertPos();
	MeshTriangulation(meshgeom,meshtemp,triRSVS.stattri,
		triRSVS.trivert);
	meshgeom.PrepareForUse();
	// meshgeom.displight();
	meshgeom.TightenConnectivity();
	meshgeom.OrderEdges();

	// Find number of holes
	holeIndices=FindHolesInSnake(snakein, groupedVertices);
	nHoles = holeIndices.size();

	holeCoords.assign(nHoles*3,0.0);
	// Assign the holes
	for (int i = 0; i < nHoles; ++i){
		for (int j =0; j<3; ++j){
			holeCoords[i*3+j] = snakein.snakemesh->verts.isearch(
				holeIndices[i])->coord[j];
		}
	}
}


HashedVector<int, int> GroupCloseSnaxels(const snake &snakein, double distTol){
	/*
	Merges snaxel which are `distTol` to the end of their carrying edge.
	*/
	HashedVector<int, int> closeVert;
	std::vector<int> sameSnaxs, rmvInds;
	int nSnax;

	nSnax = snakein.snaxs.size();
	closeVert.vec.assign(nSnax,-1);

	// This piece of code is going to be incomplete as it won't test
	// for multiple snaxels very close on the same edge
	// 
	// closeVert is a list of vertices close
	for (int i = 0; i < nSnax; ++i)	{
		if((snakein.snaxs(i)->d<distTol)){ 
			closeVert.vec[i] = snakein.snaxs(i)->fromvert;
		} else if (((1-snakein.snaxs(i)->d)<distTol)){
			closeVert.vec[i] = snakein.snaxs(i)->tovert;
		} 
	}

	closeVert.GenerateHash();
	return closeVert;
}

void TestVertClose(int vertIndIn, std::vector<bool> &isSnaxDone, 
	const mesh &meshin, double distTol,
	std::vector<int> &sameEdges){
	/*
	recursive function for finding edges which are short and should be collapsed
	*/
	int nextvertIndIn;
	if(!isSnaxDone[vertIndIn]){
			isSnaxDone[vertIndIn]=true;
			for (auto eInd : meshin.verts(vertIndIn)->edgeind){
				double d = 0.0;
				for(int i = 0; i< 3; ++i){
					d+=pow((meshin.verts.isearch(
							meshin.edges.isearch(eInd)->vertind[0]
						)->coord[i]-
						meshin.verts.isearch(
							meshin.edges.isearch(eInd)->vertind[1]
							)->coord[i]),2.0);
				}
				if(d<distTol){
					sameEdges.push_back(eInd);
					for(int i = 0; i< 2; ++i){
						nextvertIndIn = meshin.verts.find(
								meshin.edges.isearch(eInd)->vertind[i]
							);
						TestVertClose(nextvertIndIn, isSnaxDone, 
							meshin,  distTol, sameEdges);
					}
				}
				
			}
		}
}

HashedVector<int, int> GroupCloseVertices(const mesh &meshin, double distTol){
	/*
	Groups vertices which are `distTol` close to each other.
	*/
	std::vector<bool> isSnaxDone, isEdgeDone;
	HashedVector<int, int> closeVert;
	std::vector<int> sameEdges, rmvInds;
	int nSnax, nEdge, nGroup=1;

	distTol = distTol*distTol;
	nSnax = meshin.verts.size();
	nEdge = meshin.edges.size();
	closeVert.vec.assign(nSnax,-1);
	isSnaxDone.assign(nSnax,false);
	isEdgeDone.assign(nEdge,false);
	sameEdges.reserve(10);
	// This piece of code is going to be incomplete as it won't test
	// for multiple snaxels very close on the same edge
	// 
	// closeVert is a list of vertices close
	
	for (int i = 0; i < nSnax; ++i)	{
		sameEdges.clear();
		TestVertClose(i, isSnaxDone,meshin, distTol,sameEdges);

		if (sameEdges.size()>0){
			for (auto eInd : sameEdges)
			{
				closeVert.vec[meshin.verts.find(
						meshin.edges.isearch(eInd)->vertind[0]
					)]=nGroup;
				// std::cout << meshin.verts.find(
				// 		meshin.edges.isearch(eInd)->vertind[0]
				// 	) << " " << nGroup << "|";
				closeVert.vec[meshin.verts.find(
						meshin.edges.isearch(eInd)->vertind[1]
					)]=nGroup;
				// std::cout << meshin.verts.find(
				// 		meshin.edges.isearch(eInd)->vertind[1]
				// 	) << " " << nGroup << "|";
			}
			// std::cout << "|";
			++nGroup;
		}
	}
	// std::cout << std::endl;

	// std::cout << nGroup << std::endl;
	closeVert.GenerateHash();
	return closeVert;
}

int FindVertexHole(int vertInd, const mesh &meshin, const std::vector<bool> &vertIn,
	const HashedVector<int, int> &uncertainVert, std::vector<bool> &vertExplored){
	/*
	Starting from vertInd look for a vertex which is definitely a hole.

	From this vertex check if it is in? 
	Check if it is in closeVert 
	-> yes and no, we're done
	-> yes and yes, explore this vertices neighbours
	-> no, nothing to see here go to next from vertex
	*/

	std::vector<int> currVertList, nextEdgeList;
	int returnInd=-1;
	int ii, cntII, jj, cntJJ, vertPos, vertFound;

	currVertList.reserve(20);
	nextEdgeList.reserve(20);
	currVertList.push_back(vertInd);

	while(currVertList.size()>0){
		nextEdgeList.clear();
		cntII=currVertList.size();
		for(ii=0; ii<cntII; ++ii){
			vertPos = meshin.verts.find(currVertList[ii]);
			if(!vertExplored[vertPos] && vertIn[vertPos]){
				vertExplored[vertPos]=true;
				vertFound = uncertainVert.find(currVertList[ii]);
				if(vertFound==-1){
					// Found our point!
					returnInd = currVertList[ii];
					// These operations return out of the function
					currVertList.clear();
					break;
				} else {
					// Add the edges to a list
					cntJJ = meshin.verts(vertPos)->edgeind.size();
					for (jj=0; jj< cntJJ; ++jj){
						nextEdgeList.push_back(
							meshin.verts(vertPos)->edgeind[jj]);
					}
				}
			}
		}
		if (returnInd==-1){
			// Assign currVertList
			currVertList.clear();
			if (nextEdgeList.size()>0){
				currVertList = ConcatenateVectorField(meshin.edges,
					&edge::vertind, nextEdgeList);
			}
		}
	}

	return (returnInd);
}


double DomInter(double x, double y1, double y2){

	/*Interpolation function*/
	return x*(y2-y1)+y1;
}


mesh BuildDomain(const std::array<double,3> &lowerB, 
	const std::array<double,3> &upperB, double tolInner){
	/*
	Builds a parralelipipede domain stretching from lowerB to upperB

	`lowerB` and `upperB` must be vectors of size 3. `tolInner` specifies an
	offset expanding the box by a set amount.
	*/	
	mesh cube;
	int count; 

	if(lowerB.size()!=3 || upperB.size()!=3){
		std::cerr << "Error in " << __PRETTY_FUNCTION__ << " vectors must be"
			" of size 3" << std::endl;
		RSVS3D_ERROR_ARGUMENT("Input vectors must be of size 3");
	}

	std::array<int, 3> dimGrid={1, 1, 1};
	BuildBlockGrid(dimGrid, cube);

	count = cube.verts.size();
	for (int i = 0; i < count; ++i) {
		for (int j = 0; j < 3; ++j)	{
			cube.verts[i].coord[j] = (cube.verts[i].coord[j]*((upperB[j]+tolInner)-(lowerB[j]-tolInner))) + (lowerB[j]-tolInner);
		}
	}

	return cube;
}

/**
 * @brief      Builds a series of domains with different edge 
 *  properties controlling the interpolation of the metric.
 *
 * @param[in]  outerLowerB       The outer lower b
 * @param[in]  outerUpperB       The outer upper b
 * @param[in]  innerLowerB       The inner lower b
 * @param[in]  innerUpperB       The inner upper b
 * @param[in]  nSteps            The steps
 * @param      vertPerSubDomain  The vertical per sub domain
 *
 * @return     The cut cell domain.
 * 
	These also serve to avoid having a badly conditioned initial triangulation
	with very small edges.
	
	`nSteps` is the number of total domains.
	0 will return an empty mesh, 1 will return a mesh of the inner bound
	2 will return inner and outer bounds, 

			{
				{DomInter(x, innerLowerB[0], outerLowerB[0]),
				DomInter(x, innerUpperB[0], outerUpperB[0])},
				{DomInter(x, innerLowerB[1], outerLowerB[1]),
				DomInter(x, innerUpperB[1], outerUpperB[1])},
				{DomInter(x, innerLowerB[2], outerLowerB[2]),
				DomInter(x, innerUpperB[2], outerUpperB[2])}
			}

		meshtemp = BuildDomain(
			{DomInter(x, innerLowerB[0], outerLowerB[0]),
				DomInter(x, innerLowerB[1], outerLowerB[1]),
				DomInter(x, innerLowerB[2], outerLowerB[2])},
			{DomInter(x, innerUpperB[0], outerUpperB[0]),
				DomInter(x, innerUpperB[1], outerUpperB[1]),
				DomInter(x, innerUpperB[2], outerUpperB[2])}
			);
 */
mesh BuildCutCellDomain(const std::array<double,3> &outerLowerB, 
	const std::array<double,3> &outerUpperB,
	const std::array<double,3> &innerLowerB, const std::array<double,3> &innerUpperB, int nSteps, 
	std::vector<int> &vertPerSubDomain){

	mesh meshdomain, meshtemp;
	double x;
	std::array<std::array<double, 2>,3> scaleDom;

	// Special case if asked for  0 subdomains
	if(nSteps==0){
		meshdomain.Init(0, 0, 0, 0);
		meshdomain.PrepareForUse();
		return meshdomain;
	}

	// Start from inner bound
	// Then step up 
	vertPerSubDomain.clear();
	meshdomain = BuildDomain(innerLowerB, innerUpperB, 0.1);
	meshdomain.PrepareForUse();
	meshtemp = meshdomain;
	meshtemp.PrepareForUse();
	vertPerSubDomain.push_back(meshtemp.verts.size());

	for (int i = 1; i < nSteps; ++i){
		x =double(i)/double(nSteps-1);
		for (int j=0; j<3;++j){	
			scaleDom[j] ={DomInter(x, innerLowerB[j], outerLowerB[j]),
				DomInter(x, innerUpperB[j], outerUpperB[j])};
		}

		meshtemp.Scale(scaleDom);
		meshtemp.PrepareForUse();
		meshdomain.MakeCompatible_inplace(meshtemp);
		meshdomain.Concatenate(meshtemp);
		meshdomain.PrepareForUse();

		vertPerSubDomain.push_back(meshtemp.verts.size());
	}
	
	return meshdomain;
}