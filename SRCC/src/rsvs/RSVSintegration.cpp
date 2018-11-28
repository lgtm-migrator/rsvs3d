#include <iostream>
#include <cmath>
#include <ctime>
#include <vector>
#include <boost/filesystem.hpp>


#include "RSVSintegration.hpp"
#include "snake.hpp"
#include "snakeengine.hpp"
#include "parameters.hpp"
#include "voxel.hpp"
#include "meshrefinement.hpp"
#include "RSVScalc.hpp"
#include "RSVSalgorithm.hpp"
#include "postprocessing.hpp"

int SAFE_ALGO_TestConn(snake &snakein){
	int ret=0;

	if (snakein.Check3D()){
		#ifdef SAFE_ALGO
		ret = snakein.snakeconn.TestConnectivityBiDir();
		#endif //SAFE_ALGO
	}

	return(ret);
}

void SnakeConnectivityUpdate_legacy(snake &snakein,  vector<int> &isImpact){


	int start_s;

	start_s=clock();

	

	start_s=TimeStamp("position: ", start_s);

	snakein.SnaxImpactDetection(isImpact);
	MergeAllContactVertices(snakein, isImpact);
	snakein.PrepareForUse();


	start_s=TimeStamp("Merge: ", start_s);

	CleanupSnakeConnec(snakein);
	

	start_s=TimeStamp("Clean: ", start_s);
	snakein.SnaxImpactDetection(isImpact);
	SpawnArrivedSnaxels(snakein,isImpact);


	

	start_s=TimeStamp("Spawn: ", start_s);

	snakein.SnaxImpactDetection(isImpact);
	snakein.SnaxAlmostImpactDetection(isImpact, 0.01);
	snakein.PrepareForUse();
	SAFE_ALGO_TestConn(snakein);
	MergeAllContactVertices(snakein, isImpact);
	snakein.PrepareForUse();
	SAFE_ALGO_TestConn(snakein);
	

	start_s=TimeStamp("Impact: ", start_s);

	CleanupSnakeConnec(snakein);

	SAFE_ALGO_TestConn(snakein);
	snakein.OrientFaces();

	start_s=TimeStamp("Clean: ", start_s);
}

void SnakeConnectivityUpdate_robust(snake &snakein,  vector<int> &isImpact){
	/*
	Performs the snake step except the movement of the snake.

	This one performs it in two steps:
	 1) Impact Merge Clean
	 2) Impact Spawn Impact Merge Clean

	This function might be better in snakeengine.cpp
	*/
	double impactAlmostRange = 0.2;

	int start_s, start_f;
	start_f=clock();

	//===============================
	// Impact on edge
	// ======================
	// Impact
	SAFE_ALGO_TestConn(snakein);
	snakein.SnaxImpactDetection(isImpact);
	snakein.PrepareForUse();
	start_s=TimeStamp("Impact: ", start_f);
	// ======================
	// Merge
	MergeAllContactVertices(snakein, isImpact);
	snakein.PrepareForUse();
	start_s=TimeStamp("Merge: ", start_s);
	// ======================
	// Clean
	CleanupSnakeConnec(snakein);
	snakein.PrepareForUse();
	SAFE_ALGO_TestConn(snakein);
	snakein.OrientFaces();
	start_s=TimeStamp("Clean: ", start_s);


	//===============================
	// Spawn
	// ======================
	// Impact
	SAFE_ALGO_TestConn(snakein);
	snakein.SnaxImpactDetection(isImpact);
	snakein.SnaxAlmostImpactDetection(isImpact, impactAlmostRange);
	snakein.PrepareForUse();
	start_s=TimeStamp("Impact: ", start_s);
	// ======================
	// Spawn
	SAFE_ALGO_TestConn(snakein);
	snakein.SnaxImpactDetection(isImpact);
	SpawnArrivedSnaxels(snakein,isImpact);
	start_s=TimeStamp("Spawn: ", start_s);
	// ======================
	// Impact
	SAFE_ALGO_TestConn(snakein);
	snakein.SnaxImpactDetection(isImpact);
	snakein.PrepareForUse();
	start_s=TimeStamp("Impact: ", start_s);
	// ======================
	// Merge
	MergeAllContactVertices(snakein, isImpact);
	snakein.PrepareForUse();
	start_s=TimeStamp("Merge: ", start_s);
	// ======================
	// Clean
	CleanupSnakeConnec(snakein);
	snakein.PrepareForUse();
	SAFE_ALGO_TestConn(snakein);
	snakein.OrientFaces();
	start_s=TimeStamp("Clean: ", start_s);

	TimeStamp(" - Connec Update: ", start_f);
}

void SnakeConnectivityUpdate(snake &snakein,  vector<int> &isImpact){
	/*
	Performs the snake step except the movement of the snake.
	This one performs it in a 'single' step:
	 Impact Spawn Impact Merge Clean

	This function might be better in snakeengine.cpp
	*/
	double impactAlmostRange = 0.2;

	int start_s, start_f;
	start_f=clock();


	//===============================
	// Spawn
	// ======================
	// Impact
	SAFE_ALGO_TestConn(snakein);
	snakein.SnaxImpactDetection(isImpact);
	snakein.SnaxAlmostImpactDetection(isImpact, impactAlmostRange);
	snakein.PrepareForUse();
	start_s=TimeStamp("Impact: ", start_f);
	// ======================
	// Spawn
	SAFE_ALGO_TestConn(snakein);
	snakein.SnaxImpactDetection(isImpact);
	SpawnArrivedSnaxels(snakein,isImpact);
	start_s=TimeStamp("Spawn: ", start_s);
	// ======================
	// Impact
	SAFE_ALGO_TestConn(snakein);
	snakein.SnaxImpactDetection(isImpact);
	snakein.PrepareForUse();
	start_s=TimeStamp("Impact: ", start_s);
	// ======================
	// Merge
	MergeAllContactVertices(snakein, isImpact);
	snakein.PrepareForUse();
	start_s=TimeStamp("Merge: ", start_s);
	// ======================
	// Clean
	CleanupSnakeConnec(snakein);
	snakein.PrepareForUse();
	SAFE_ALGO_TestConn(snakein);
	snakein.OrientFaces();
	start_s=TimeStamp("Clean: ", start_s);

	TimeStamp(" - Connec Update: ", start_f);
}

void SnakeConnectivityUpdate_2D(snake &snakein,  vector<int> &isImpact){
	/*
	Performs the snake step except the movement of the snake.
	This one performs it in a 'single' step:
	 Impact Spawn Impact Merge Clean

	This function might be better in snakeengine.cpp
	*/
	double impactAlmostRange = 0.2;

	int start_s, start_f;
	start_f=clock();


	//===============================
	// Spawn
	// ======================
	// Impact
	SAFE_ALGO_TestConn(snakein);
	snakein.SnaxImpactDetection(isImpact);
	snakein.SnaxAlmostImpactDetection(isImpact, impactAlmostRange);
	snakein.PrepareForUse();
	start_s=TimeStamp("Impact: ", start_f);
	// ======================
	// Spawn
	SAFE_ALGO_TestConn(snakein);
	snakein.SnaxImpactDetection(isImpact);
	SpawnArrivedSnaxels(snakein,isImpact);
	start_s=TimeStamp("Spawn: ", start_s);
	// ======================
	// Impact
	SAFE_ALGO_TestConn(snakein);
	snakein.SnaxImpactDetection(isImpact);
	snakein.PrepareForUse();
	start_s=TimeStamp("Impact: ", start_s);
	// ======================
	// Merge
	MergeAllContactVertices(snakein, isImpact);
	snakein.PrepareForUse();
	start_s=TimeStamp("Merge: ", start_s);
	// ======================
	// Clean
	CleanupSnakeConnec(snakein);
	snakein.PrepareForUse();
	SAFE_ALGO_TestConn(snakein);
	snakein.OrientFaces();
	start_s=TimeStamp("Clean: ", start_s);

	TimeStamp(" - Connec Update: ", start_f);
}

int TimeStamp(const char* str,int start_s){
	int stop_s=clock();
	#ifdef TIME_EXEC
	cout << str << " " << (stop_s-start_s)/double(CLOCKS_PER_SEC)*1000 << "ms; ";
	#endif
	return(stop_s);
}

void integrate::prepare::Mesh(
	const param::grid &gridconf,
	mesh &snakeMesh,
	mesh &voluMesh
	){
	/*prepares the snake and volume meshes gor the RSVS process*/
	// Local declaration
	int ii;
	auto gridSize = gridconf.voxel.gridsizebackground;
	vector<int> elmMapping, backgroundGrid;
	RSVScalc calcVolus;

	backgroundGrid.reserve(int(gridSize.size()));
	for (int i = 0; i < int(gridSize.size()); ++i)
	{
		backgroundGrid.push_back(gridSize[i]);
		gridSize[i] = gridSize[i] * gridconf.voxel.gridsizesnake[i];
	}

	// Initial build of the grid
	BuildBlockGrid(gridSize, snakeMesh);
	snakeMesh.Scale(gridconf.voxel.domain);
	snakeMesh.PrepareForUse();
	snakeMesh.OrientFaces();

	// map elements to coarse grid
	if(snakeMesh.WhatDim()==3){
		for (ii=0;ii<snakeMesh.volus.size();++ii){
			elmMapping.push_back(1);
		}
	} else if (snakeMesh.WhatDim()==2){
		for (ii=0;ii<snakeMesh.surfs.size();++ii){
			elmMapping.push_back(1);
		}
	} else { 
		throw invalid_argument("Incorrect dimension");
	}
	CartesianMapping(snakeMesh,  elmMapping, backgroundGrid);
	CoarsenMesh(snakeMesh,voluMesh,elmMapping);
	snakeMesh.AddParent(&voluMesh,elmMapping);

	voluMesh.PrepareForUse();
	voluMesh.OrientFaces();
	calcVolus.CalculateMesh(voluMesh);
	calcVolus.ReturnConstrToMesh(voluMesh,&volu::volume);
}

void integrate::prepare::Snake(
	const param::snaking &snakconf, 
	mesh &snakeMesh, // non const as it is passed to the snake as a pointer
	snake &rsvsSnake
	){

	rsvsSnake.snakemesh = &snakeMesh;
	SpawnRSVS(rsvsSnake,snakconf.initboundary);
	rsvsSnake.PrepareForUse();
}

void integrate::prepare::Triangulation(
	mesh &snakeMesh,
	snake &rsvsSnake,
	triangulation &rsvsTri
	){

	rsvsTri.PrepareForUse();
	TriangulateMesh(snakeMesh,rsvsTri);
	rsvsTri.PrepareForUse();
	TriangulateSnake(rsvsSnake,rsvsTri);
	rsvsTri.PrepareForUse();
	rsvsTri.CalcTriVertPos();
	rsvsTri.PrepareForUse();
	MaintainTriangulateSnake(rsvsTri);
	rsvsTri.PrepareForUse();
}

void integrate::prepare::PostProcessing(){
	tecplotfile outSnake;
	param::files fileconf;
	// Define 
	fileconf.PrepareForUse();
	if (fileconf.ioout.outdir.size()!=0){
		boost::filesystem::create_directories(fileconf.ioout.outdir);

	}
}

// ===================
// Tests
// ===================

int integrate::test::Prepare(){
	param::parameters paramconf;
	mesh snakeMesh;
	mesh voluMesh;
	snake rsvsSnake;
	triangulation rsvsTri;

	try {
		integrate::prepare::Mesh(paramconf.grid, snakeMesh, voluMesh);
	} catch (exception const& ex) { 
		cerr << "integrate::prepare::Mesh(paramconf.grid, snakeMesh, voluMesh);" << endl;
		cerr << "Exception: " << ex.what() <<endl; 
		return -1;
	} 
	
	try {

		integrate::prepare::Snake(paramconf.snak, snakeMesh, rsvsSnake);
	} catch (exception const& ex) { 
		cerr << "integrate::prepare::Snake(paramconf.snak, snakeMesh, rsvsSnake);" << endl;
		cerr << "Exception: " << ex.what() <<endl; 
		return -1;
	} 

	try {

		integrate::prepare::Triangulation(snakeMesh, rsvsSnake, rsvsTri);
	} catch (exception const& ex) { 
		cerr << "integrate::prepare::Triangulation" << endl;

		cerr << "Exception: " << ex.what() <<endl; 
		return -1;
	} 

	try {

		integrate::prepare::PostProcessing();
	} catch (exception const& ex) { 
		cerr << "integrate::prepare::PostProcessing" << endl;

		cerr << "Exception: " << ex.what() <<endl; 
		return -1;
	} 


	return(0);
}