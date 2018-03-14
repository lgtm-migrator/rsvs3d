#include <iostream>
#include <cstdlib>
#include <string>
#include <functional>

#include "test.hpp"
#include "arraystructures.hpp"
#include "voxel.hpp"
#include "postprocessing.hpp"
#include "snakstruct.hpp"

int main(){
	customtest gridTest;
	#ifdef TEST_ALL
	gridTest.Run(Test_ArrayStructures,"arraystructures");
	
	gridTest.Run(Test_BuildBlockGrid_noout,"Voxel");

	gridTest.Run(Test_tecplotfile,"post-processing");

	gridTest.Run(Test_tecplotfile,"post-processing class");

	gridTest.Run(Test_MeshOut,"Mesh output");
	#endif //TEST_ALL
	gridTest.Run(TestTemplate_ArrayStruct<vert>,"TestTemplate_ArrayStruct<vert>");
	gridTest.Run(TestTemplate_ArrayStruct<edge>,"TestTemplate_ArrayStruct<edge>");
	gridTest.Run(TestTemplate_ArrayStruct<surf>,"TestTemplate_ArrayStruct<surf>");
	gridTest.Run(TestTemplate_ArrayStruct<volu>,"TestTemplate_ArrayStruct<volu>");
	gridTest.Run(TestTemplate_ArrayStruct<snax>,"TestTemplate_ArrayStruct<snax>");
	gridTest.Run(TestTemplate_ArrayStruct<snaxedge>,"TestTemplate_ArrayStruct<snaxedge>");
	gridTest.Run(TestTemplate_ArrayStruct<snaxsurf>,"TestTemplate_ArrayStruct<snaxsurf>");

	gridTest.Run(Test_SnakeStructures,"Snake containers");


	gridTest.PrintSummary();
	return(0);
}


