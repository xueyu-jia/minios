// // ??? undefined realloc(line 58), some functions declared by fat32.h but no use
// /**********************************************************
// *	array.c       //added by mingxuan 2019-5-17
// ***********************************************************/

// #include "fat32.h"
// #include "type.h"
// #include "proto.h"

// DArray* InitDArray(UINT initialCapacity,UINT capacityIncrement)
// {
// 	DArray *array=NULL;
// 	//array=(DArray*)K_PHY2LIN(sys_kmalloc(sizeof(DArray)));
// 	//array=(DArray*)K_PHY2LIN(do_kmalloc(sizeof(DArray)));	//modified by mingxuan 2021-3-25
// 	array=(DArray*)kern_kmalloc(sizeof(DArray));	//modified by mingxuan 2021-8-16
// 	if(array==NULL)
// 	{
// 		return NULL;
// 	}
// 	//array->base=(DArrayElem*)K_PHY2LIN(sys_kmalloc(initialCapacity*sizeof(DArrayElem)));
// 	//array->base=(DArrayElem*)K_PHY2LIN(do_kmalloc(initialCapacity*sizeof(DArrayElem)));	//modified by mingxuan 2021-3-25
// 	array->base=(DArrayElem*)kern_kmalloc(initialCapacity*sizeof(DArrayElem));	//modified by mingxuan 2021-8-16
// 	array->used=0;
// 	array->offset=0;
// 	array->capacity=initialCapacity;
// 	array->increment=capacityIncrement;
// 	return array;
// }

// void AddElement(DArray *array,DArrayElem element)
// {
// 	DArrayElem *newbase=NULL;

// 	if(array->used>=array->capacity)
// 	{
// 		newbase=(DArrayElem*)realloc(array->base,(array->capacity+array->increment)*sizeof(DArrayElem));
// 		if(newbase==NULL)
// 		{
// 			exit(0);
// 		}
// 		array->base=newbase;
// 		array->capacity+=array->increment;
// 	}
// 	array->base[(array->used)++]=element;
// }

// DArrayElem* NextElement(DArray *array)
// {
// 	if(array->offset>=array->used)
// 	{
// 		return NULL;
// 	}
// 	return &(array->base[array->offset++]);
// }

// void DestroyDArray(DArray *array)
// {
// 	//sys_free(array->base);
// 	//do_kfree(K_LIN2PHY(array->base));	//modified by mingxuan 2021-3-25
// 	kern_kfree(array->base);	//modified by mingxuan 2021-8-17

// 	//sys_free(array);
// 	//do_kfree(K_LIN2PHY(array));	//modified by mingxuan 2021-3-25
// 	kern_kfree(array);	//modified by mingxuan 2021-8-17
	
// }
