#include <iostream>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <ctime>

#include "snakstruct.hpp"
#include "snakeengine.hpp"
#include "arraystructures.hpp"

using namespace std;



// Snake Spawning at Vertex.
void SpawnAtVertex(snake& snakein,int indVert){

	snake newsnake;
	int subVert,nVert, nEdge,nSurf,nVolu;
	bool is3D;
	vector<int> vertInds,edgeInds,surfInds,voluInds,edgeInds2;
	vector<int> vertSubs,edgeSubs,surfSubs,voluSubs;
	vector<int> vertSubsTemp,edgeSubsTemp,surfSubsTemp,voluSubsTemp;
	vector<int>::iterator itVecInt; 
	unordered_multimap<int,int> hashEdgeInds,hashVoluInds,hashSurfInds,hashVertInds;


	is3D=snakein.snakemesh->volus.size()>0;
	// Extract Data corresponding to vertex from Mesh
	subVert=snakein.snakemesh->verts.find(indVert);

	
	edgeInds=snakein.snakemesh->verts(subVert)->edgeind;
	edgeSubs=snakein.snakemesh->edges.find_list(edgeInds);
	surfInds=ConcatenateVectorField(snakein.snakemesh->edges, &edge::surfind, edgeSubs);
	sort(surfInds);
	unique(surfInds);

	surfSubs=snakein.snakemesh->surfs.find_list(surfInds);
	voluInds=ConcatenateVectorField(snakein.snakemesh->surfs, &surf::voluind, surfSubs);
	sort(voluInds);
	unique(voluInds);
	if(is3D){
		voluSubs=snakein.snakemesh->volus.find_list(voluInds);
	} else {

		voluSubs=snakein.snakemesh->volus.find_list(voluInds);
	}
	//OperArrayStructMethod(snakein.snakemesh->surfs, surfSubs, &surf::isready, 
	//	ii, std::logical_and<bool>());
	nVert=edgeInds.size();
	nEdge=surfInds.size();
	nSurf=voluInds.size();
	nVolu=int(is3D);

	newsnake.Init(snakein.snakemesh,nVert,nEdge,nSurf,nVolu);
	// Generates snaxels and vertices
	SpawnAtVertexVert(newsnake,nVert, indVert,subVert, surfInds,edgeInds,
		edgeSubs,hashSurfInds);
	// Generate snake edges
	SpawnAtVertexEdge(newsnake, nEdge ,surfInds, edgeInds,	voluInds,
		surfSubs, hashEdgeInds,hashVoluInds);
	// Generate Snake surfaces
	if (is3D){
		SpawnAtVertexSurf3D(newsnake,nSurf,surfInds ,voluInds,voluSubs,hashSurfInds);
	// Generate Volume
		SpawnAtVertexVolu(newsnake,nSurf);
	} else {
		SpawnAtVertexSurf2D( newsnake, nEdge, voluInds);
	}
	
	snakein.SetMaxIndexNM();

	snakein.MakeCompatible_inplace(newsnake);

	// DO NOT RUN TO MAITAIN orederedge newsnake.PrepareForUse();
	snakein.Concatenate(newsnake);

}

void SpawnAtVertexVert(snake& newsnake, int nVert,int indVert, int subVert, const vector<int> &surfInds,
	const vector<int> &edgeInds,const vector<int> &edgeSubs, unordered_multimap<int,int> &hashSurfInds){
	int ii,jj;
	vector<int> edgeSubsTemp;

	newsnake.snakeconn.verts.PopulateIndices();
	newsnake.snaxs.PopulateIndices();
	for (ii=0;ii<nVert;++ii){
		// Finds the to vertex
		jj=int(newsnake.snakemesh->edges(edgeSubs[ii])->vertind[0]==indVert);
		newsnake.snaxs[ii].set(newsnake.snaxs(ii)->index,0.0,0.5,indVert,
			newsnake.snakemesh->edges(edgeSubs[ii])->vertind[jj],edgeInds[ii],0,-1);

		edgeSubsTemp=FindSubList(newsnake.snakemesh->edges(edgeSubs[ii])->surfind,
			surfInds,hashSurfInds);
		newsnake.snakeconn.verts[ii].edgeind=edgeSubsTemp;
		newsnake.snakeconn.verts[ii].coord=newsnake.snakemesh->verts(subVert)->coord;
	}
	newsnake.snakeconn.verts.ChangeIndices(0,1,0,0);
}

void SpawnAtVertexEdge(snake& newsnake,int nEdge,const vector<int> &surfInds,const vector<int> &edgeInds,
	const vector<int> &voluInds,const vector<int> &surfSubs,unordered_multimap<int,int> &hashEdgeInds, unordered_multimap<int,int> &hashVoluInds){
	int ii,jj,kk;
	vector<int> surfSubsTemp,vertSubsTemp;

	newsnake.snakeconn.edges.PopulateIndices();
	newsnake.snaxedges.PopulateIndices();
	for (ii=0;ii<nEdge;++ii){
		newsnake.snaxedges[ii].surfind=surfInds[ii];

		surfSubsTemp=FindSubList(newsnake.snakemesh->surfs(surfSubs[ii])->voluind,
			voluInds,hashVoluInds);
		newsnake.snakeconn.edges[ii].surfind=surfSubsTemp;

		// Assign vertind (can be done WAY more efficiently the other way round)
		// But liek this we can check the logic
		vertSubsTemp=FindSubList(newsnake.snakemesh->surfs(surfSubs[ii])->edgeind,
			edgeInds,hashEdgeInds);
		kk=0;
		for(jj=0;jj<int(vertSubsTemp.size());++jj){
			if (vertSubsTemp[jj]>=0){
				newsnake.snakeconn.edges[ii].vertind[kk]=vertSubsTemp[jj];
				kk++;
			}
		}
	}
	newsnake.snakeconn.edges.ChangeIndices(1,0,1,0);
	if(!newsnake.Check3D()){
		for (ii=0;ii<nEdge;++ii){
			newsnake.snakeconn.edges[ii].surfind[0]=1;
		}
	}
}
void SpawnAtVertexSurf3D(snake& newsnake,int nSurf,const vector<int> &surfInds, const vector<int> &voluInds,
	const vector<int> &voluSubs,unordered_multimap<int,int> &hashSurfInds){

	int ii,jj;
	vector<int> surfSubsTemp;

	newsnake.snakeconn.surfs.PopulateIndices();
	newsnake.snaxsurfs.PopulateIndices();
	for(ii=0;ii<nSurf;++ii){
		newsnake.snaxsurfs[ii].voluind=voluInds[ii];
		newsnake.snakeconn.surfs[ii].voluind[0]=1;
		// Assign edgeind (can be done WAY more efficiently the other way round)
		// But liek this we can check the logic
		surfSubsTemp=FindSubList(newsnake.snakemesh->volus(voluSubs[ii])->surfind,
			surfInds,hashSurfInds);
		// Needs to be modified to work with 2D (surfSubsTemps does not come out right) 
		for(jj=0;jj<int(surfSubsTemp.size());++jj){
			if (surfSubsTemp[jj]>=0){
				newsnake.snakeconn.surfs[ii].edgeind.push_back(surfSubsTemp[jj]);
				
			}
		}
	}
	newsnake.snakeconn.surfs.ChangeIndices(0,1,0,0);
}

void SpawnAtVertexSurf2D(snake& newsnake,int nEdge, const vector<int> &voluInds){

	int ii,jj;
	vector<int> surfSubsTemp;

	newsnake.snakeconn.surfs.PopulateIndices();
	newsnake.snaxsurfs.PopulateIndices();
	ii=0;
	newsnake.snaxsurfs[ii].voluind=voluInds[ii];
	newsnake.snakeconn.surfs[ii].voluind[0]=0;

	for(jj=0;jj<int(nEdge);++jj){
		newsnake.snakeconn.surfs[ii].edgeind.push_back(jj+1);
	}
	
	//newsnake.snakeconn.surfs.ChangeIndices(0,1,0,0);
}

void SpawnAtVertexVolu(snake& newsnake, int nSurf){
	int ii;

	newsnake.snakeconn.volus.PopulateIndices();
	newsnake.snakeconn.volus[0].surfind.reserve(nSurf);
	for (ii=0;ii<nSurf;++ii){
		newsnake.snakeconn.volus[0].surfind.push_back(ii+1);
	}
}

// Merge vertices in contact

void MergeAllContactVertices(snake &fullsnake, vector<int> &isImpact){

	// in isImpact needs to be hashed to rapidly check
	int ii,jj,nImpacts;
	vector<int>  snaxToRemove, vertSameSub,subVelTo0;
	vector<bool> isImpactDone;
	HashedVector<int,snax> impactInd, impactTarg;
	
	nImpacts=isImpact.size()/2;
	impactInd.vec.reserve(nImpacts);
	impactTarg.vec.reserve(nImpacts);
	snaxToRemove.reserve(nImpacts);
	isImpactDone.assign(nImpacts,false);

	for(ii=0;ii<int(isImpact.size());ii=ii+2){
		impactInd.vec.push_back(isImpact[ii]);
	}
	for(ii=1;ii<int(isImpact.size());ii=ii+2){
		impactTarg.vec.push_back(isImpact[ii]);
	}
	impactInd.GenerateHash();
	impactTarg.GenerateHash();


	for(ii=0; ii<nImpacts; ++ii){
		if(!isImpactDone[ii]){
			isImpactDone[ii]=true;
			if(impactTarg.vec[ii]>0){
				fullsnake.snakeconn.SwitchIndex(1,impactInd.vec[ii],impactTarg.vec[ii]);
				subVelTo0.push_back(fullsnake.snaxs.find(impactTarg.vec[ii])); 

				snaxToRemove.push_back(impactInd.vec[ii]);
				vertSameSub=ReturnDataEqualRange(impactTarg.vec[ii], impactInd.hashTable);
				
				for(jj=0;jj< int(vertSameSub.size()) ; jj++){
					isImpactDone[vertSameSub[jj]]=true;
				}
				vertSameSub=ReturnDataEqualRange(impactTarg.vec[ii], impactTarg.hashTable);
				for(jj=0;jj< int(vertSameSub.size()) ; jj++){
					isImpactDone[vertSameSub[jj]]=true;
				}
			}
		}
	}
	for (ii=0;ii<int(subVelTo0.size());ii++){
		fullsnake.snaxs[subVelTo0[ii]].v=0.0; 
	}
	fullsnake.snaxs.remove(snaxToRemove);
	fullsnake.snakeconn.verts.remove(snaxToRemove);
	
}

void SpawnArrivedSnaxels(snake &fullsnake,const vector<int> &isImpact){

	snake fwdSnake, bwdSnake;
	int start_s,stop_s;

	fwdSnake.Init(fullsnake.snakemesh,0,0,0,0);
	bwdSnake.Init(fullsnake.snakemesh,0,0,0,0);

	// Generate fwd spawn
	
	start_s=clock();
	SpawnArrivedSnaxelsDir(fullsnake,bwdSnake,isImpact,-1);
	SpawnArrivedSnaxelsDir(fullsnake,fwdSnake,isImpact,-2);

	stop_s=clock();
	cout << "Spawn: " << double(stop_s-start_s)/double(CLOCKS_PER_SEC)*1000 << "ms  " ;
	start_s=clock();

	bwdSnake.Flip();

	fwdSnake.SetMaxIndexNM();
	fwdSnake.MakeCompatible_inplace(bwdSnake);
	// DO NOT RUN TO MAITAIN orederedge newsnake.PrepareForUse();
	fwdSnake.Concatenate(bwdSnake);

	fullsnake.SetMaxIndexNM();
	fullsnake.MakeCompatible_inplace(fwdSnake);
	// DO NOT RUN TO MAITAIN orederedge newsnake.PrepareForUse();
	fullsnake.Concatenate(fwdSnake);
	stop_s=clock();
	cout << "Concatenate: " << double(stop_s-start_s)/double(CLOCKS_PER_SEC)*1000 << "ms  " ;

	cout << " nSnax " << fwdSnake.snaxs.size() << "  ";
	fullsnake.PrepareForUse();

}

void SpawnArrivedSnaxelsDir(const snake &fullsnake,snake &partSnake,const vector<int> &isImpact,int dir){

	int nVert, nEdge, nSurf, nVolu,ii,jj,kk;
	vector<int> vertSpawn,subList;
	nVert=0; nEdge=0; nSurf=0; nVolu=0;jj=-1;
	if(dir==-1){
		for(ii=0;ii<int(isImpact.size());ii=ii+2){
			if(isImpact[ii+1]==dir){
				if(!fullsnake.snakemesh->verts.isearch(fullsnake.snaxs(jj)->fromvert)->isBorder){
					jj=fullsnake.snaxs.find(isImpact[ii]);
					vertSpawn.push_back(fullsnake.snaxs(jj)->fromvert);
					nVolu++;
					nVert=nVert+fullsnake.snakemesh->verts(jj)->edgeind.size();


					subList=fullsnake.snakemesh->edges.find_list(fullsnake.snakemesh->verts(jj)->edgeind);

					for(kk=0;kk<int(subList.size());kk++){
						nEdge=nEdge+fullsnake.snakemesh->edges(subList[kk])->surfind.size();
					}
				}

			}
		}
	}

	if(dir==-2){
		for(ii=0;ii<int(isImpact.size());ii=ii+2){
			if(isImpact[ii+1]==dir){
				jj=fullsnake.snaxs.find(isImpact[ii]);
				if(!fullsnake.snakemesh->verts.isearch(fullsnake.snaxs(jj)->tovert)->isBorder){
					vertSpawn.push_back(fullsnake.snaxs(jj)->tovert);
					nVolu++;
					nVert=nVert+fullsnake.snakemesh->verts(jj)->edgeind.size();


					subList=fullsnake.snakemesh->edges.find_list(fullsnake.snakemesh->verts(jj)->edgeind);

					for(kk=0;kk<int(subList.size());kk++){
						nEdge=nEdge+fullsnake.snakemesh->edges(subList[kk])->surfind.size();
					}
				}

			}
		}
	}

	nSurf=nEdge/2;
	partSnake.reserve(nVert, nEdge, nSurf, nVolu);

	sort(vertSpawn);
	unique(vertSpawn);

	for (ii=0;ii<int(vertSpawn.size());ii++){
		SpawnAtVertex(partSnake,vertSpawn[ii]);
	}

}



void CleanupSnakeConnec(snake snakein){

	vector<ConnecRemv> connecEdit;
	int ii,jj,nEdgeConn,nSurfConn;
	snakein.PrepareForUse();
	IdentifyMergEdgeConnec(snakein, connecEdit);

	nEdgeConn=int(connecEdit.size());
	for(ii=0; ii < nEdgeConn;++ii){
		for(jj=0; jj < int(connecEdit[ii].rmvind.size());++jj){
			snakein.snakeconn.SwitchIndex(connecEdit[ii].typeobj,connecEdit[ii].rmvind[jj],
				connecEdit[ii].keepind,connecEdit[ii].scopeind);
		}
	}

	IdentifyMergSurfConnec(snakein, connecEdit);
	
	nSurfConn=int(connecEdit.size());
	for(ii=nEdgeConn; ii < nSurfConn;++ii){
		for(jj=0; jj < int(connecEdit[ii].rmvind.size());++jj){
			snakein.snakeconn.SwitchIndex(connecEdit[ii].typeobj,connecEdit[ii].rmvind[jj],
				connecEdit[ii].keepind,connecEdit[ii].scopeind);
		}
	}
}


void IdentifyMergEdgeConnec(snake &snakein, vector<ConnecRemv> &connecEdit){

	vector<bool> isObjDone;
	vector<int> tempSub,tempSub2, tempCount;
	HashedVector<int,int> tempIndHash; 
	//vector<int> objSub;
	int nSnaxEdge, ii,nParent; //nSnax, nSnaxSurf,
	ConnecRemv tempConnec, tempConnec2;

	//nSnax=snakein.snaxs.size();
	nSnaxEdge=snakein.snaxedges.size();
	//nSnaxSurf=snakein.snaxsurfs.size();

	isObjDone.reserve(nSnaxEdge);


	isObjDone.assign(nSnaxEdge,false);
	for(ii=0; ii<nSnaxEdge ; ++ii){
		if(!isObjDone[ii]){
			nParent=snakein.snaxedges.countparent(snakein.snaxedges(ii)->KeyParent());
			if(nParent>1){
				snakein.snaxedges.findsiblings(snakein.snaxedges(ii)->KeyParent(),tempSub);
				IdentifyMergeEdgeGeneral(snakein, isObjDone,connecEdit, tempConnec,  tempConnec2,tempSub,tempSub2, tempCount,tempIndHash);
			}
			isObjDone[ii]=true;
		}
	}

}

void IdentifyMergeEdgeGeneral(const snake &snakein, vector<bool> &isObjDone,vector<ConnecRemv> &connecEdit, ConnecRemv &tempConnec,  ConnecRemv &tempConnec2,vector<int> &tempSub,vector<int> &tempSub2, vector<int> &tempCount, HashedVector<int,int> &tempIndHash) 
{

	int jj,jjNext,jjStart, nTemp;

	
	// check if the edges are connected
	tempIndHash.vec.clear();
	tempCount.clear();
	tempIndHash.vec=ConcatenateVectorField(snakein.snakeconn.edges, &edge::vertind,tempSub);
	tempIndHash.GenerateHash();
	tempCount=tempIndHash.count(tempIndHash.vec);
	nTemp=tempCount.size();
	for (jj=0;jj<int(tempSub.size());++jj){
		tempConnec2.scopeind.push_back(snakein.snaxedges(tempSub[jj])->index);

	}

	jjStart=0;
	while (tempCount[jjStart]!=1 && jjStart<nTemp){jjStart++;}
	if (jjStart>=nTemp){ 
	// if all 2s

		tempConnec.rmvind.clear();
		tempConnec.keepind=snakein.snaxedges(tempSub[0])->index;
		tempConnec.typeobj=2;
		for (jj=0;jj<int(tempSub.size());++jj){
			tempConnec.rmvind.push_back(snakein.snaxedges(tempSub[jj])->index);
			isObjDone[tempSub[jj]]=true;
			#ifdef SAFE_ALGO
			if (tempCount[jj*2]!=2 && tempCount[jj*2+1]!=2){
				cerr << "Error: Unexpected  behaviour "<< endl;
				cerr << " jjStart not found but vertex does not have 2 connections "<< endl;
				cerr << "	in function:" <<  __PRETTY_FUNCTION__ << endl;
				throw invalid_argument ("Unexpected algorithmic behaviour"); 
			}
			#endif //SAFE_ALGO
		}
		connecEdit.push_back(tempConnec);

	} else {
		do{ 
		// group edges by connection groups
			jj=jjStart;
			tempConnec.rmvind.clear();
			tempConnec2.rmvind.clear();
			tempConnec.typeobj=2;
			tempConnec2.typeobj=5;

			tempCount[jjStart]=0;
			tempConnec.keepind=snakein.snaxedges(tempSub[jjStart/2])->index;
			isObjDone[tempSub[jjStart/2]]=true;
						// if second part of an edge check the other part
			jjNext=jj+(1-((jj%2)*2)); // equivalend of jj+ (jj%2 ? -1 : 1) 
			while(tempCount[jjNext]>1){ 
			// Builds one group

				#ifdef SAFE_ALGO
				if (tempCount[jjNext]>2){
					cerr << "Error: Algorithm not conceived for this case "<< endl;
					cerr << " snake has more than 2 edges connected to the same snaxel inside the same surface "<< endl;
					cerr << "	in function:" <<  __PRETTY_FUNCTION__ << endl;
					throw invalid_argument ("Unexpected algorithmic behaviour");
				}
				#endif // SAFE_ALGO

				tempConnec2.rmvind.push_back(tempIndHash.vec[jjNext]);
				tempCount[jjNext]=0;
				tempSub2=tempIndHash.findall(tempIndHash.vec[jjNext]);
				jj=0;
				while(tempSub2[jj]==jjNext && jj<4){++jj;}

				#ifdef SAFE_ALGO
				if (jj>2 || jj<0){
					cerr << "Error: Algorithm not conceived for this case "<< endl;
					cerr << " jj>3 Unsafe read has happened "<< endl;
					cerr << "	in function:" <<  __PRETTY_FUNCTION__ << endl;
					throw invalid_argument ("Unexpected algorithmic behaviour");
				}
				#endif // SAFE_ALGO

				jj=tempSub2[jj];
				tempCount[jj]=0;
				tempConnec.rmvind.push_back(snakein.snaxedges(tempSub[jj/2])->index);
				isObjDone[tempSub[jj/2]]=true;

				jjNext=jj+(1-((jj%2)*2));

			}
			tempConnec2.keepind=tempIndHash.vec[jjNext];
			if (jj!=jjStart){
				connecEdit.push_back(tempConnec);
				connecEdit.push_back(tempConnec2);
			}

			jjStart=0;
			while (tempCount[jjStart]!=1 && jjStart<nTemp){jjStart++;}
		} while (jjStart<nTemp);
	}
}


void IdentifyMergSurfConnec(snake &snakein, vector<ConnecRemv> &connecEdit){

	vector<bool> isObjDone;
	vector<int> tempSub,tempSub2, tempCount;
	HashedVector<int,int> tempIndHash,edge2Surf; 
	//vector<int> objSub;
	int nSnaxSurf, ii,nParent; //nSnax, nSnaxSurf,
	ConnecRemv tempConnec, tempConnec2;

	//nSnax=snakein.snaxs.size();
	nSnaxSurf=snakein.snaxsurfs.size();
	//nSnaxSurf=snakein.snaxsurfs.size();

	isObjDone.reserve(nSnaxSurf);


	isObjDone.assign(nSnaxSurf,false);
	for(ii=0; ii<nSnaxSurf ; ++ii){
		if(!isObjDone[ii]){
			nParent=snakein.snaxsurfs.countparent(snakein.snaxsurfs(ii)->KeyParent());
			if(nParent>1){
				snakein.snaxsurfs.findsiblings(snakein.snaxsurfs(ii)->KeyParent(),tempSub);
				IdentifyMergeSurfGeneral(snakein, isObjDone,connecEdit, tempConnec,tempSub,tempSub2, tempCount,edge2Surf,tempIndHash);
			}
			isObjDone[ii]=true;
		}
	}

}



void IdentifyMergeSurfGeneral(const snake &snakein, vector<bool> &isObjDone,vector<ConnecRemv> &connecEdit, 
	ConnecRemv &tempConnec,vector<int> &tempSub,vector<int> &tempSub2,
	 vector<int> &tempCount,HashedVector<int,int> &edge2Surf, HashedVector<int,int> &tempIndHash) 
{
	// tempSub is the sub of surfaces in snakeconn that are in the same volume
	int ii,jj,jjStart, nTemp;

	
	edge2Surf.vec.clear();
	tempIndHash.vec=ConcatenateVectorField(snakein.snakeconn.surfs, &surf::edgeind,tempSub);
	// tempIndHash is a hashed vector of concatenate (surfs(tempSub).edgeind)
	for(ii=0; ii<int(tempSub.size());++ii){
		for(jj=0; jj <int(snakein.snakeconn.surfs(tempSub[ii])->edgeind.size()); ++jj){
			edge2Surf.vec.push_back(ii);
		}
	}
	// edge2Surf is a hashed vector of the subscripts into tempSub of the surf matching the 
	// edges in tempHashInd

	tempIndHash.GenerateHash();
	edge2Surf.GenerateHash();
	tempCount=tempIndHash.count(tempIndHash.vec);
	// tempCount is the vector counting the number of occurences of each edge at each edges location
	nTemp=tempCount.size();




	jjStart=0;
	while (tempCount[jjStart]<=1 && jjStart<nTemp){jjStart++;}
	// jjStart must start at a point were tempCount[jjStart] > 1 otherwise there 
	// is no merging needed in the cell

	if(jjStart<nTemp){ 
	// if can't find a count above 1 we're done

		tempSub2=tempIndHash.findall(tempIndHash.vec[jjStart]);
		// tempSub2 is the position of edges matching that detected by jjStart
		tempConnec.typeobj=3;
		isObjDone[tempSub[edge2Surf.vec[jjStart]]]=true;
		tempConnec.keepind=snakein.snakeconn.surfs(tempSub[edge2Surf.vec[jjStart]])->index;
		// Kept index is the last surf to be detected as having the edge of jjStart
		tempCount[jjStart]=0; // To ensure this edge is not set again set tempCount to 0

		IdentifyMergeSurfRecursive( snakein,isObjDone, tempCount,edge2Surf, tempIndHash, 
			tempConnec, tempSub, tempSub2, jjStart);
		if (tempConnec.rmvind.size()>0){
			sort(tempConnec.rmvind);
			unique(tempConnec.rmvind);
			connecEdit.push_back(tempConnec);
		}

	}

	// Note:
	// Check for surface collapse
	// if all of the edges are "collapsed edges" the surfaces need to be assembled in a single surface
	// and made a collapsed surface ie marked for deletion.
}

void IdentifyMergeSurfRecursive(const snake &snakein, vector<bool> &isObjDone, vector<int> &tempCount,const HashedVector<int,int> &edge2Surf, const HashedVector<int,int> &tempIndHash, ConnecRemv &tempConnec, const vector<int> &tempSub, const vector<int> &tempSub2, int excludeSub){

	// tempSub2 is the position of edges matching that detected by jjStart
	// excludeSub is used to not recurse into the caller edge
	int ii, jj ;
	vector<int> tempSurf, tempRecur;

	for(ii = 0 ; ii< int(tempSub2.size()); ++ii){
		if(tempSub2[ii]!=excludeSub){
			tempConnec.rmvind.push_back(snakein.snakeconn.surfs(tempSub[edge2Surf.vec[tempSub2[ii]]])->index);

			isObjDone[tempSub[edge2Surf.vec[tempSub2[ii]]]]=true;
			tempCount[tempSub2[ii]]=0; 
			// this edge is explored set to 0;
			// add all edges detected on the same edge as merge targets
			tempSurf=tempIndHash.findall(tempIndHash.vec[tempSub2[ii]]);
			// find all the occurences of that surf
			for (jj=0; jj<int(tempSurf.size()); ++jj){
				if(tempCount[tempSurf[jj]]>1){
					// for each edge of that cell which is tempCount>1 recurs
					tempCount[tempSurf[jj]]=0;
					tempRecur=tempIndHash.findall(tempIndHash.vec[tempSurf[jj]]);
					IdentifyMergeSurfRecursive( snakein,isObjDone, tempCount,edge2Surf, tempIndHash, 
						tempConnec, tempSub, tempRecur, tempSurf[jj]);
				}
			}
		}
	}
}