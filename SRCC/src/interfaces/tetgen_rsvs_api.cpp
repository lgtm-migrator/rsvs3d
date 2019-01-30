#include <iostream>
#include <vector>

#include "tetgen_rsvs_api.hpp"
#include "tetgen.h"
#include "mesh.hpp"
#include "snake.hpp"
#include "snakevel.hpp"
#include "voxel.hpp"
#include "arraystructures.hpp"
#include "postprocessing.hpp"
// void tetrahedralize(char *switches, tetgenio *in, tetgenio *out, 
//                     tetgenio *addin, tetgenio *bgmin)

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// tetcall.cxx                                                               //
//                                                                           //
// An example of how to call TetGen from another program by using the data   //
// type "tetgenio" and function "tetrahedralize()" of TetGen libaray.        //
//                                                                           //
// In order to run this example, you need the library of TetGen, you can get //
// the source code as well as the user's manul of TetGen from:               //
//                                                                           //
//            http://tetgen.berlios.de/index.html                            //
//                                                                           //
// Section 2 of the user's manual contains the information of how to compile //
// TetGen into a libaray.                                                    //
//                                                                           //
// The geometry used in this example (illustrated in Section 3.3 .1, Figure  //
// 12 of the user's manual) is a rectangluar bar consists of 8 points and 6  //
// facets (which are all rectangles). In additional, there are two boundary  //
// markers defined on its facets.                                            //
//                                                                           //
// This code illustrates the following basic steps:                          //
//   - at first create an input object "in", and set data of the geometry    //
//     into it.                                                              //
//   - then call function "tetrahedralize()" to create a quality mesh of the //
//     geometry with output in another object "out".                         //
// In addition, It outputs the geometry in the object "in" into two files    //
// (barin.node and barin.poly), and outputs the mesh in the object "out"     //
// into three files (barout.node, barout.ele, and barout.face).  These files //
// can be visualized by TetView.                                             //
//                                                                           //
// To compile this code into an executable program, do the following steps:  //
//   - compile TetGen into a library named "libtet.a" (see Section 2.1 of    //
//     the user's manula for compiling);                                     //
//   - Save this file into the same directory in which you have the files    //
//     "tetgen.h" and "libtet.a";                                            //
//   - compile it using the following command:                               //
//                                                                           //
//     g++ -o test tetcall.cxx -L./ -ltet                                    //
//                                                                           //
//     which will result an executable program named "test".                 //
//                                                                           //
// Please send your quesions, comments to Hang Si <si@wias-berlin.de>        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// main()  Create and refine a mesh using TetGen library.                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



void load_tetgen_testdata(mesh &snakeMesh, mesh &voluMesh, snake &snakein, mesh &triMesh){
	/*
	Loads data for tetgen testing
	*/

	triangulation triRSVS;

	snakeMesh.read("../TESTOUT/testtetgen/SnakeMesh_181205T193158_sphere2.msh");
	voluMesh.read("../TESTOUT/testtetgen/VoluMesh_181205T193158_sphere2.msh");
	snakein.read("../TESTOUT/testtetgen/Snake_181205T193158_sphere2.3snk");
	// snakein.snakeconn.read("../TESTOUT/testtetgen/SnakeConn_181205T193158_sphere2.msh");
	snakein.snakemesh = &snakeMesh;
	
	snakeMesh.PrepareForUse();
	snakeMesh.displight();
	
	voluMesh.PrepareForUse();
	voluMesh.displight();

	snakein.PrepareForUse();
	snakein.displight();

	snakeMesh.AddParent(&voluMesh);

	triRSVS.stattri.clear();
	triRSVS.trivert.clear();
	triRSVS.PrepareForUse();
	TriangulateMesh(snakeMesh,triRSVS);
	TriangulateSnake(snakein,triRSVS);
	triRSVS.PrepareForUse();
	triRSVS.CalcTriVertPos();
	MaintainTriangulateSnake(triRSVS);

	MeshTriangulation(triMesh,snakein.snakeconn,triRSVS.dynatri, triRSVS.trivert);
	triMesh.displight();
	triMesh.PrepareForUse();
	triMesh.OrderEdges();
	triMesh.TestConnectivityBiDir();

	// triMesh is the triangulation in mesh format
	// it is not cleaned up for tiny surfaces
}

mesh BuildDomain(const std::vector<double> &lowerB, const std::vector<double> &upperB){
	/*
	Builds a parralelipipede domain stretching from lowerB to upperB

	`lowerB` and `upperB` must be vectors of size 3.
	*/	
	mesh cube;
	int count; 

	if(lowerB.size()!=3 || upperB.size()!=3){
		std::cerr << "Error in " << __PRETTY_FUNCTION__ << " vectors must be"
			" of size 3" << std::endl;
		throw invalid_argument("Input vectors must be of size 3");
	}

	std::array<int, 3> dimGrid={1, 1, 1};
	BuildBlockGrid(dimGrid, cube);

	count = cube.verts.size();
	for (int i = 0; i < count; ++i) {
		for (int j = 0; j < 3; ++j)	{
			cube.verts[i].coord[j] = (cube.verts[i].coord[j]*(upperB[j]-lowerB[j])) + lowerB[j];
		}
	}

	return cube;
}


void MeshData2Tetgenio(const mesh &meshgeom, tetgenio_safe &tetin,
	int facetOffset, int pointOffset, int pointMarker, const std::vector<double> &pointMtrList){
	/*
	Writes meshdata into the tetgenio format for a single mesh
	*/
	vector<int> orderVert;
	int countI, countJ, countK;

	// Set point properties to the appropriate lists
	countI = meshgeom.verts.size();
	countJ = min(int(pointMtrList.size()), tetin.numberofpointmtrs);
	for (int i = 0; i < countI; ++i) {
		// set point coordinates
		for (int j = 0; j < 3; ++j) {
			tetin.pointlist[(i+pointOffset)*3+j] = meshgeom.verts(i)->coord[j];
		}
		// set point metrics
		for (int j = 0; j < countJ; ++j) {
			tetin.pointlist[(i+pointOffset)*countJ+j] =pointMtrList[j];
		}
		// set point markers
		tetin.pointmarkerlist[i] = pointMarker;
	}

	// Set connectivity through facet and polygon
	countI = meshgeom.surfs.size();
	for (int i = 0; i < countI; ++i){

		tetin.allocatefacet(i+facetOffset,1);

		meshgeom.surfs(i)->OrderedVerts(&meshgeom,orderVert);
		tetin.allocatefacetpolygon(i+facetOffset,0,orderVert.size());
		countK = orderVert.size();
		for (int k = 0; k < countK; ++k){
			tetin.facetlist[i+facetOffset].polygonlist[0].vertexlist[k] 
				= meshgeom.verts.find(orderVert[k])+pointOffset;
		}
		tetin.facetmarkerlist[i+facetOffset]=pointMarker;
	}
}


void Mesh2Tetgenio(const mesh &meshgeom, const mesh &meshdomain,
	tetgenio_safe &tetin, int numHoles){
	/*
	Converts a mesh into the safe allocation tetgenio format.
	
	Rules of conversion are: 
		- volu becomes a facet
		- surf becomes a polygon in the facet
		- edges are not logged
		- points come as a list
	*/
	

	tetin.firstnumber=0;

	tetin.numberoffacets = meshgeom.surfs.size() + meshdomain.surfs.size();
	tetin.numberofholes = numHoles;
	tetin.numberofpoints = meshgeom.verts.size() + meshdomain.verts.size();

	tetin.numberofpointmtrs = 0;


	tetin.allocate();

	// MeshData2Tetgenio(meshgeom, tetin, facetOffset, pointOffset, pointMarker, pointMtrList);
	MeshData2Tetgenio(meshgeom, tetin, 0, 0, 1, {});
	MeshData2Tetgenio(meshdomain, tetin, meshgeom.surfs.size(), meshgeom.verts.size(), 2, {});

}

void RemoveSingularMeshElements(snake &snakein, mesh &meshin){

	double tol = 1e-3;
	std::vector<bool> isSnaxDone;
	HashedVector<int, int> closeVert;
	std::vector<int> sameSnaxs, rmvInds;
	int nSnax, count, countJ, countRep;

	nSnax = snakein.snaxs.size();
	isSnaxDone.assign(nSnax,false);
	closeVert.vec.assign(nSnax,-1);

	count = nSnax;
	// This piece of code is going to be incomplete as it won't test
	// for multiple snaxels very close on the same edge
	// 
	// closeVert is a list of vertices close
	for (int i = 0; i < nSnax; ++i)	{
		if((snakein.snaxs(i)->d<tol)){ 
			closeVert.vec[i] = snakein.snaxs(i)->fromvert;
		} else if (((1-snakein.snaxs(i)->d)<tol)){
			closeVert.vec[i] = snakein.snaxs(i)->tovert;
		} 
	}

	countRep = 0;
	closeVert.GenerateHash();
	rmvInds.reserve(nSnax);
	// Find matching elements and perform the replacement process
	for (int i = 0; i < nSnax; ++i)
	{
		if(!isSnaxDone[i] && closeVert.vec[i]!=-1)
		{
			sameSnaxs=closeVert.findall(closeVert.vec[i]);
			countJ = sameSnaxs.size();
			for (int j = 1; j < countJ; ++j)
			{
				meshin.SwitchIndex(1, meshin.verts(sameSnaxs[j])->index, 
					meshin.verts(sameSnaxs[0])->index);
				countRep++;
				rmvInds.push_back(meshin.verts(sameSnaxs[j])->index);
				isSnaxDone[sameSnaxs[j]]=true;
			}
			isSnaxDone[sameSnaxs[0]]=true;

		} else {
			isSnaxDone[i]=true;
		}
	}
	sort(rmvInds);
	unique(rmvInds);
	FILE *fid;
	fid = fopen("delvert.txt","w");
	for (int i = 0; i < countRep; ++i)
	{	
		for (int j = 0; j < 3; ++j)
		{
			fprintf(fid, "%f ", meshin.verts.isearch(rmvInds[i])->coord[j]);
		}
		fprintf(fid, "\n");
	}
	meshin.verts.remove(rmvInds);
	meshin.verts.PrepareForUse();
	std::cout << "Number of removed vertices " << countRep <<
		 " " << rmvInds.size() << std::endl;
	rmvInds.clear();
	
	// Remove Edges
	count  = meshin.edges.size();
	countRep= 0;
	for (int i = 0; i < count; ++i)
	{
		if(meshin.edges(i)->vertind[0]==meshin.edges(i)->vertind[1]){
			meshin.RemoveIndex(2, meshin.edges(i)->index);
			rmvInds.push_back(meshin.edges(i)->index);
			countRep++;
		}
	}
	std::cout << "Number of removed edges " << countRep << std::endl;
	sort(rmvInds);
	unique(rmvInds);
	meshin.edges.remove(rmvInds);
	meshin.edges.PrepareForUse();
	rmvInds.clear();

	// Remove Surfs
	count  = meshin.surfs.size();
	countRep= 0;
	for (int i = 0; i < count; ++i)
	{
		if(meshin.surfs(i)->edgeind.size()<3){
			meshin.RemoveIndex(3, meshin.surfs(i)->index);
			rmvInds.push_back(meshin.surfs(i)->index);
			countRep++;
		}
	}
	std::cout << "Number of removed surfs " << countRep << std::endl;
	sort(rmvInds);
	unique(rmvInds);
	meshin.surfs.remove(rmvInds);
	meshin.surfs.PrepareForUse();
	rmvInds.clear();

	// Remove Volus
	count  = meshin.volus.size();
	countRep= 0;
	for (int i = 0; i < count; ++i)
	{
		if(meshin.volus(i)->surfind.size()<4){
			meshin.RemoveIndex(3, meshin.volus(i)->index);
			rmvInds.push_back(meshin.volus(i)->index);
			countRep++;
		}
	}
	std::cout << "Number of removed volus " << countRep << std::endl;
	sort(rmvInds);
	unique(rmvInds);
	meshin.volus.remove(rmvInds);
	meshin.volus.PrepareForUse();
	rmvInds.clear();

}

void tetgen_RSVS(snake &snakein){

	mesh meshdomain, meshgeom, meshtemp;
	tetgenio_safe tetin, tetout;
	triangulation triRSVS;
	int nHoles, nPtsHole, kk, count;
	tecplotfile tecout;

	tecout.OpenFile("../TESTOUT/rsvs_tetgen_mesh.plt");
	meshtemp=snakein.snakeconn;
	tecout.PrintMesh(meshtemp);
	meshtemp.displight();
	RemoveSingularMeshElements(snakein,meshtemp);
	meshtemp.PrepareForUse();
	meshtemp.TestConnectivityBiDir();
	meshtemp.TightenConnectivity();
	meshtemp.OrderEdges();
	tecout.PrintMesh(meshtemp);
	meshtemp.displight();

	// Triangulation preparation
	snakein.AssignInternalVerts();
	triRSVS.stattri.clear();
	triRSVS.trivert.clear();
	triRSVS.PrepareForUse();
	TriangulateMesh(meshtemp,triRSVS);
	TriangulateSnake(snakein,triRSVS);
	triRSVS.PrepareForUse();
	triRSVS.CalcTriVertPos();
	MeshTriangulation(meshgeom,meshtemp,triRSVS.stattri,
		triRSVS.trivert);
	meshgeom.PrepareForUse();
	meshgeom.displight();
	meshgeom.TightenConnectivity();
	meshgeom.OrderEdges();

	tecout.PrintMesh(meshgeom);


	// Find number of holes
	nHoles = 0;
	// count = snakein.isMeshVertIn.size();
	// for (int i = 0; i < count; ++i) {
	// 	nHoles+=int(snakein.isMeshVertIn[i]);
	// }

	meshdomain = BuildDomain({-2, -2, -2}, {5, 5, 5});
	meshdomain.PrepareForUse();
	Mesh2Tetgenio(meshgeom, meshdomain, tetin, nHoles);

	std::cout<< std::endl << "Number of holes " << nHoles << std::endl;

	// Assign the holes
	// kk = 0;
	// nPtsHole = snakein.snakemesh->verts.size();
	// for (int i = 0; i < nPtsHole; ++i)
	// {
	// 	if(snakein.isMeshVertIn[i]){
	// 		for(int j=0; j<3; ++j){
	// 			tetin.holelist[kk*3+j]=snakein.snakemesh->verts(i)->coord[j];
	// 		}
	// 		kk++;
	// 	}
	// 	if (kk==nHoles){
	// 		break;
	// 	}
	// }


	tecout.PrintMesh(meshdomain);
	tetin.
	tetin.save_nodes("rsvs_3cell_2body");
	tetin.save_poly("rsvs_3cell_2body");

	// // Tetrahedralize the PLC. Switches are chosen to read a PLC (p),
	// //   do quality mesh generation (q) with a specified quality bound
	// //   (1.414), and apply a maximum volume constraint (a0.1).

	// tetrahedralize("pkq1.414a0.1", &tetin, &tetout);

	// tetout.save_nodes("rsvsout_3cell_2body");
	// tetout.save_elements("rsvsout_3cell_2body");
	// tetout.save_faces("rsvsout_3cell_2body");
}

int tetcall()
{
	tetgenio_safe in;
	tetgenio out;
	tetgenio::facet *f;
	tetgenio::polygon *p;
	int i;
	mesh snakeMesh, voluMesh, triMesh;
	snake snakein;

	load_tetgen_testdata(snakeMesh, voluMesh, snakein, triMesh);
	tetgen_RSVS(snakein);
	// All indices start from 1.
	in.firstnumber = 1;
	in.numberofpoints = 8;
	in.numberoffacets = 6;
	in.numberofholes = 0;

	in.allocate();
	// in.pointlist = new REAL[in.numberofpoints * 3];
	// in.facetlist = new tetgenio::facet[in.numberoffacets];
	// in.facetmarkerlist = new int[in.numberoffacets];

	in.pointlist[0]  = 0;  // node 1.
	in.pointlist[1]  = 0;
	in.pointlist[2]  = 0;
	in.pointlist[3]  = 2;  // node 2.
	in.pointlist[4]  = 0;
	in.pointlist[5]  = 0;
	in.pointlist[6]  = 2;  // node 3.
	in.pointlist[7]  = 2;
	in.pointlist[8]  = 0;
	in.pointlist[9]  = 0;  // node 4.
	in.pointlist[10] = 2;
	in.pointlist[11] = 0;
	// Set node 5, 6, 7, 8.
	for (i = 4; i < 8; i++) {
		in.pointlist[i * 3]     = in.pointlist[(i - 4) * 3];
		in.pointlist[i * 3 + 1] = in.pointlist[(i - 4) * 3 + 1];
		in.pointlist[i * 3 + 2] = 12;
	}


	// Facet 1. The leftmost facet.
	f = &in.facetlist[0];
	f->numberofpolygons = 1;
	f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
	f->numberofholes = 0;
	f->holelist = NULL;
	p = &f->polygonlist[0];
	p->numberofvertices = 4;
	p->vertexlist = new int[p->numberofvertices];
	p->vertexlist[0] = 1;
	p->vertexlist[1] = 2;
	p->vertexlist[2] = 3;
	p->vertexlist[3] = 4;

	// Facet 2. The rightmost facet.
	f = &in.facetlist[1];
	f->numberofpolygons = 1;
	f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
	f->numberofholes = 0;
	f->holelist = NULL;
	p = &f->polygonlist[0];
	p->numberofvertices = 4;
	p->vertexlist = new int[p->numberofvertices];
	p->vertexlist[0] = 5;
	p->vertexlist[1] = 6;
	p->vertexlist[2] = 7;
	p->vertexlist[3] = 8;

	// Facet 3. The bottom facet.
	f = &in.facetlist[2];
	f->numberofpolygons = 1;
	f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
	f->numberofholes = 0;
	f->holelist = NULL;
	p = &f->polygonlist[0];
	p->numberofvertices = 4;
	p->vertexlist = new int[p->numberofvertices];
	p->vertexlist[0] = 1;
	p->vertexlist[1] = 5;
	p->vertexlist[2] = 6;
	p->vertexlist[3] = 2;

	// Facet 4. The back facet.
	f = &in.facetlist[3];
	f->numberofpolygons = 1;
	f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
	f->numberofholes = 0;
	f->holelist = NULL;
	p = &f->polygonlist[0];
	p->numberofvertices = 4;
	p->vertexlist = new int[p->numberofvertices];
	p->vertexlist[0] = 2;
	p->vertexlist[1] = 6;
	p->vertexlist[2] = 7;
	p->vertexlist[3] = 3;

	// Facet 5. The top facet.
	f = &in.facetlist[4];
	f->numberofpolygons = 1;
	f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
	f->numberofholes = 0;
	f->holelist = NULL;
	p = &f->polygonlist[0];
	p->numberofvertices = 4;
	p->vertexlist = new int[p->numberofvertices];
	p->vertexlist[0] = 3;
	p->vertexlist[1] = 7;
	p->vertexlist[2] = 8;
	p->vertexlist[3] = 4;

	// Facet 6. The front facet.
	f = &in.facetlist[5];
	f->numberofpolygons = 1;
	f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
	f->numberofholes = 0;
	f->holelist = NULL;
	p = &f->polygonlist[0];
	p->numberofvertices = 4;
	p->vertexlist = new int[p->numberofvertices];
	p->vertexlist[0] = 4;
	p->vertexlist[1] = 8;
	p->vertexlist[2] = 5;
	p->vertexlist[3] = 1;

	// Set 'in.facetmarkerlist'

	in.facetmarkerlist[0] = -1;
	in.facetmarkerlist[1] = -2;
	in.facetmarkerlist[2] = 0;
	in.facetmarkerlist[3] = 0;
	in.facetmarkerlist[4] = 0;
	in.facetmarkerlist[5] = 0;
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wwrite-strings"
	// Output the PLC to files 'barin.node' and 'barin.poly'.
	in.save_nodes("barin");
	in.save_poly("barin");

	// Tetrahedralize the PLC. Switches are chosen to read a PLC (p),
	//   do quality mesh generation (q) with a specified quality bound
	//   (1.414), and apply a maximum volume constraint (a0.1).

	tetrahedralize("pq1.414a0.1", &in, &out);

	// Output mesh to files 'barout.node', 'barout.ele' and 'barout.face'.
	out.save_nodes("barout");
	out.save_elements("barout");
	out.save_faces("barout");
	#pragma GCC diagnostic pop

	return 0;
}


void tetgenio_safe::allocate(){

	// Allocation of points and associated attributes
	if (this->numberofpoints>0){
		this->pointlist = new REAL[this->numberofpoints * 3];
		this->pointattributelist = new REAL[this->numberofpoints * this->numberofpointattributes];
		this->pointmtrlist = new REAL[this->numberofpoints * this->numberofpointmtrs];
		this->pointmarkerlist = new int[this->numberofpoints];
	}
	// Allocate tetrahedron
	if(this->numberoftetrahedra>0){
		this->tetrahedronlist = new int[this->numberoftetrahedra*this->numberofcorners];
		this->tetrahedronattributelist = new REAL[this->numberoftetrahedra];
		this->tetrahedronvolumelist = new REAL[this->numberoftetrahedra
			*this->numberoftetrahedronattributes];
		// this->neighborlist = new int[this->numberoftetrahedra*4]; //output only
	}
	// Allocation of facets
	if(this->numberoffacets>0){
		this->facetlist = new tetgenio::facet[this->numberoffacets];
		this->facetmarkerlist = new int[this->numberoffacets];
		for (int i = 0; i < this->numberoffacets; ++i){
			this->facetlist[i].numberofpolygons=0;
			this->facetlist[i].numberofholes=0;
		}
	}

	// Allocation of holes (a set of points)
	if(this->numberofholes>0){
		this->holelist = new REAL[this->numberofholes*3];
	}
	// Allocation of regions (a set of points with attributes)
	// X, Y, Z, region attribute at index, maximum volume at index
	if(this->numberofregions>0){
		this->regionlist = new REAL[this->numberofregions*5];
	}
	// Allocate facet
	if(this->numberoffacetconstraints>0){
		this->facetconstraintlist = new REAL[this->numberoffacetconstraints*2];
	}
	// Allocate constraint
	if(this->numberofsegmentconstraints>0){
		this->segmentconstraintlist = new REAL[this->numberofsegmentconstraints*3];
	}
	// Allocate triangles
	if(this->numberoftrifaces>0){
		this->trifacelist = new int[this->numberoftrifaces*3];
		this->trifacemarkerlist = new int[this->numberoftrifaces];
		this->o2facelist = new int[this->numberoftrifaces*3];
		this->adjtetlist = new int[this->numberoftrifaces*2];
	}

	// Allocate edges
	if(this->numberofedges>0){
		this->edgelist = new int[this->numberofedges*2];
		this->edgemarkerlist = new int[this->numberofedges];
		this->o2edgelist = new int[this->numberofedges];
		this->edgeadjtetlist = new int[this->numberofedges];
	}

	// Voronoi implementation
	if(this->numberofvedges || this->numberofvpoints 
		|| this->numberofvcells || this->numberofvfacets){
		std::cerr << "Warning : tetgenio_safe::allocate() does not support "
			"Voronoi variables" << std::endl; 
	}
}

void tetgenio_safe::allocatefacet(int fIndex){
	if (fIndex<this->numberoffacets){
		if(this->facetlist[fIndex].numberofpolygons>0){
			this->facetlist[fIndex].polygonlist 
				= new tetgenio::polygon[this->facetlist[fIndex].numberofpolygons];
		}
		if(this->facetlist[fIndex].numberofholes>0){
			this->facetlist[fIndex].holelist 
				= new REAL[this->facetlist[fIndex].numberofholes];
		}

	} else {
		std::cerr << "Error: Index passed to facet allocation out "
			"of range" << std::endl;
	}
}
void tetgenio_safe::allocatefacetpolygon(int fIndex, int pIndex){
	if (fIndex<this->numberoffacets){
		if (pIndex<this->facetlist[fIndex].numberofpolygons){
			if(this->facetlist[fIndex].polygonlist[pIndex].numberofvertices>0){
				this->facetlist[fIndex].polygonlist[pIndex].vertexlist 
					= new int[this->facetlist[fIndex].polygonlist[pIndex].numberofvertices];
			}

		} else {
			std::cerr << "Error: Index passed to polygon allocation out "
				"of range" << std::endl;
		}
	} else {
		std::cerr << "Error: Index passed to polygon allocation  "
			"for  facet out of range" << std::endl;
	}
}
void tetgenio_safe::allocatefacet(int fIndex, int numPoly){
	if (fIndex<this->numberoffacets){
		this->facetlist[fIndex].numberofpolygons=numPoly;
		this->allocatefacet(fIndex);

	} else {
		std::cerr << "Error: Index passed to facet allocation out "
			"of range" << std::endl;
	}
}

void tetgenio_safe::allocatefacetpolygon(int fIndex, int pIndex, int numVerts){
	if (fIndex<this->numberoffacets){
		if (pIndex<this->facetlist[fIndex].numberofpolygons){
			this->facetlist[fIndex].polygonlist[pIndex].numberofvertices = numVerts;
			this->allocatefacetpolygon(fIndex,pIndex);
		} else {
			std::cerr << "Error: Index passed to polygon allocation out "
				"of range" << std::endl;
		}
	} else {
		std::cerr << "Error: Index passed to polygon allocation  "
			"for  facet out of range" << std::endl;
	}
}

// Test code
int test_tetgenapi(){

	tetcall();

	return(0);
}
