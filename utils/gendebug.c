/*************************************************************************
@copyright Copyright (c) 2011 David Bunker

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "skiplist.h"
#include "gendebug.h"

// for initialization
int globalInitDebug = 0;

// malloc holder skip list
skipList globalMemHolder;

void compAssert(char *str){

	printf("%s\n",str);
	// cause segfault for easy gdb stack trace (can be removed)
	volatile int i = *(int*)0x4;
	printf("%d\n",i);
	exit(0);
}

void assert(void *nonZero,char *str){

	if(!nonZero){
		compAssert(str);
	}
}

void assertBool(int nonZero,char *str){

	if(!nonZero){
		compAssert(str);
	}
}

void *Malloc(size_t len){

	void *ret = malloc(len);
	if(!ret){
		printf("Malloc Error\n");
		exit(-1);
	}
	return ret;
}

void Free(void *mem){

	free(mem);
}

// other info could be stored here, could potentially get size info
// by the storage info used by the malloc utility above the mallocd space
typedef struct {
	void *loc;
	int size;
	// info about memory
	char info[10];
} memInfo;

// sort by location in memory
int memCompare(void *first,void *second){
	
	memInfo *memF = first;
	memInfo *memS = second;
	assert(memF,"debug first");
	assert(memS,"debug second");

	if(memF->loc < memS->loc)
		return -1;
	if(memF->loc > memS->loc)
		return 1;
	return 0;
}

// initialize library
void InitDebug(){

	globalInitDebug = 1;
	globalMemHolder = createSkip(Malloc,Free,memCompare);
}

// this is used for debugging, can be more fine grained than valgrind
void *MallocInfoL(size_t len,char *info){

	if(!globalInitDebug)
		InitDebug();

	char *mal = Malloc(len);
	
	memInfo	*mem = Malloc(sizeof(memInfo));
	mem->loc = mal;
	mem->size = len;
	strncpy(mem->info,info,9);
	
	// fill with nonsense
	int i;
	for(i=0;i<len;i++)
		mal[i] = 0x4D;

	insertSkip(globalMemHolder,mem);
	return mal;
}

void *MallocL(size_t len){

	return MallocInfoL(len,"None");
}

void *MallocSpecL(size_t len){

	return MallocInfoL(len,"LIST");
}

// free and clear
void FreeL(void *mal){

	if(!globalInitDebug)
		InitDebug();

	memInfo	memS;
	memInfo	*mem = &memS;
	mem->loc = mal;
	
	memInfo	*retMem = removeSkip(globalMemHolder,mem);
	assert(retMem,"Illegal Free");

	char *retMal = retMem->loc;
	int size = retMem->size;

	assertBool((retMal==mal),"Mal Incorrect");

	// clear out memory
	int i;
	for(i=0;i<size;i++)
		retMal[i] = 0x58;

	// doesn't really free the memory to make sure that if 
	// it is called, the result will be that it was freed (0x58)

	Free(retMem);
}

void printInfo(void *data){

	memInfo *mem = data;
	printf("Memory: %p Size: %i Info: %s\n",mem->loc,mem->size,mem->info);
}

// print out memory leaked
void MemInfo(){

	if(!globalInitDebug)
		InitDebug();
	
	printf("Leak Size: %i\n",sizeSkip(globalMemHolder));
	printSkip(globalMemHolder,printInfo);
}

