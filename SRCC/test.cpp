#include <iostream>
#include <cstdlib>
#include <string>

#include "arraystructures.hpp"
#include "voxel.hpp"


int main(){
	int errCount,testCount,errFlag;

	errCount=0;
	testCount=0;

	cout << "----------------------------------------------------------------------------------------" << endl;
	cout << "----------------------------------------------------------------------------------------" << endl;
	cout << "      Start testing arraystructures" << endl;
	cout << "----------------------------------------------------------------------------------------" << endl;
	errFlag=Test_ArrayStructures();
	++testCount;
	cout << "----------------------------------------------------------------------------------------" << endl;
	if (errFlag!=0){
		++errCount;
		cout << "Finished testing arraystructures" << endl;
		cout << "                      - Caught Error: " << errFlag << endl;
	} else {
		cout << "Finished testing arraystructures" << endl;
		cout << "                      - No Error" << endl;
	}
	cout << "----------------------------------------------------------------------------------------" << endl;
	cout << "----------------------------------------------------------------------------------------" << endl;
	cout << endl;

	cout << "----------------------------------------------------------------------------------------" << endl;
	cout << "----------------------------------------------------------------------------------------" << endl;
	cout << "      Start testing Voxel" << endl;
	cout << "----------------------------------------------------------------------------------------" << endl;
	errFlag=Test_BuildBlockGrid();
	++testCount;
	cout << "----------------------------------------------------------------------------------------" << endl;
	if (errFlag!=0){
		++errCount;
		cout << "Finished testing Voxel" << endl;
		cout << "                      - Caught Error: " << errFlag << endl;
	} else {
		cout << "Finished testing Voxel" << endl;
		cout << "                      - No Error" << endl;
	}
	cout << "----------------------------------------------------------------------------------------" << endl;
	cout << "----------------------------------------------------------------------------------------" << endl;
	cout << endl;
	
	cout << endl;

	cout << "----------------------------------------------------------------------------------------" << endl;
	cout << "----------------------------------------------------------------------------------------" << endl;
	cout << "      Summary of Tests:" << endl;
	cout << "         " << testCount << " tests completed" << endl;
	cout << "         " << errCount << "  detected errors  " << endl;
	cout << "----------------------------------------------------------------------------------------" << endl;
	cout << "----------------------------------------------------------------------------------------" << endl;


}



