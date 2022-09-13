#include "stdio.h"

int global = 1;
//char *str = "abcd";
char str[12];

void main(int arg,char *argv[])
{


	//printf("%s\n",str);		//deleted by mingxuan 2021-3-17
//	printf("%s\n","abcd");
//	printf("%d\n", global);	//deleted by mingxuan 2021-3-17
	
	//printf("initial ubuddy\n");
        //test_malloc();
        //test_free();
        //test_alloc_free_page();
        //test_malloc_free_over_4K();        

        //get_pid();

        umount("fat1");

        printf("total_mem_size:%x\n",total_mem_size());

       

        malloc(4096);

        //get_pid();

        printf("total_mem_size:%x\n",total_mem_size());
    
        malloc(4000);

        printf("total_mem_size:%x\n",total_mem_size());


        exit(6);
//        while(1)
//        {
//
//        }

//	printf(" %d\n",get_pid());
//	exit(get_pid());
        
}




/*======================================================================*
                           Malloc Test
added by Wang, 2021.3.18
 *======================================================================*/



void test_malloc()
{


    malloc(4080);
//    udisp_str("-----------vtable-------\n");
//    scan_vtable();

//    udisp_str("-----------malloc_table--------------\n");
//    scan_malloc_table();


    malloc(4000);
//    udisp_str("-----------vtable-------\n");
//    scan_vtable();

//    udisp_str("-----------malloc_table--------------\n");
//    scan_malloc_table();

    malloc(1024);//测试vmalloc中for循环外的if分支
                  //即没有合适的空闲块分配4K页面并建立映射
//    udisp_str("-----------vtable-------\n");
//    scan_vtable();

//    udisp_str("-----------malloc_table--------------\n");
//    scan_malloc_table();

    malloc(1072);//测试vmalloc中for循环内的第一个if分支
                //即空闲表中有合适的分区，并且分区大小大于要分配的空间
    udisp_str("-----------vtable-------\n");
    scan_vtable();

    udisp_str("-----------malloc_table--------------\n");
    scan_malloc_table();

    malloc(2000);//测试vmalloc中for循环内的第二个if分支
                 //即空闲表中有合适的分区，并且分区大小等于要分配的空间,分配后要将该分区删除

    udisp_str("-----------vtable-------\n");
    scan_vtable();

    udisp_str("-----------malloc_table--------------\n");
    scan_malloc_table();

}



void test_free()

{

    int vaddr1,vaddr2,vaddr3,vaddr4,vaddr5,vaddr6,vaddr7,vaddr8;

//    scan_heap_ptr();

    vaddr1=malloc(1024);

    vaddr2=malloc(2000);

    vaddr3=malloc(512);



//    scan_heap_ptr();

    vaddr4=malloc(2048);

    vaddr5=malloc(800);

    vaddr6=malloc(1024);


//    scan_heap_ptr();

    vaddr7=malloc(3200);
    vaddr8=malloc(800);



    udisp_str("-----------vtable-------\n");

    scan_vtable();

    udisp_str("-----------malloc_table--------------\n");
    scan_malloc_table();


    free(vaddr7);//不属于同一页的分区即使连续也不合并

//    udisp_str("-----------vtable-------\n");
//    scan_vtable();

//    udisp_str("-----------malloc_table--------------\n");

//    scan_malloc_table();



    free(vaddr4);//前后都不连续，直接插入

//    udisp_str("-----------vtable-------\n");

//    scan_vtable();

//    udisp_str("-----------malloc_table--------------\n");

//    scan_malloc_table();



    free(vaddr3);//仅和后面内存块连续，合并


//    udisp_str("-----------vtable-------\n");

//    scan_vtable();

//    udisp_str("-----------malloc_table--------------\n");

//    scan_malloc_table();



    free(vaddr5);//仅和前面内存块连续，合并

    udisp_str("-----------vtable-------\n");

    scan_vtable();

    udisp_str("-----------malloc_table--------------\n");

    scan_malloc_table();



    free(vaddr6);//和前面的内存块及后面内存块都连续，两次合并，合并后size=4K,释放页面

    udisp_str("-----------vtable-------\n");

    scan_vtable();

    udisp_str("-----------malloc_table--------------\n");
    scan_malloc_table();


}



void test_alloc_free_page()
{
    int *p1,*p2,*p3,*p4,*p5,*p6;



//    udisp_str("-------------vtable-------------------\n");
//    scan_vtable();

    p1=malloc(4000);  //测试alloc_hpage;

    p2=malloc(4032);

    p3=malloc(3960);

    p4=malloc(4048);

    p5=malloc(4080);


//    udisp_str("-------------vtable-------------------\n");
//    scan_vtable();
//    udisp_str("-------------heap_pointer-------------\n");
//    scan_heap_ptr();
//    udisp_str("-------------vpage_list---------------\n");
//    scan_page_list();

    
    free(p3);    //测试free_vpage

//    udisp_str("-------------vtable-------------------\n");
//    scan_vtable();
//    udisp_str("-------------heap_pointer-------------\n");
//    scan_heap_ptr();
//    udisp_str("-------------vpage_list---------------\n");
//    scan_page_list();

    free(p4);    //测试free_vpage
//    udisp_str("-------------vtable-------------------\n");
//    scan_vtable();
//    udisp_str("-------------heap_pointer-------------\n");
//    scan_heap_ptr();
//    udisp_str("-------------vpage_list---------------\n");
//    scan_page_list();

    p6=malloc(3999); //测试alloc_vpage
//    udisp_str("-------------vtable-------------------\n");
//    scan_vtable();
//    udisp_str("-------------heap_pointer-------------\n");
//    scan_heap_ptr();
//    udisp_str("-------------vpage_list---------------\n");
//    scan_page_list();

    free(p6);
    udisp_str("-------------vtable-------------------\n");
    scan_vtable();
    udisp_str("-------------heap_pointer-------------\n");
    scan_heap_ptr();
    udisp_str("-------------vpage_list---------------\n");
    scan_page_list();

    free(p5);    //测试free_ppage
    udisp_str("-------------vtable-------------------\n");
    scan_vtable();
    udisp_str("-------------heap_pointer-------------\n");
    scan_heap_ptr();
    udisp_str("-------------vpage_list---------------\n");
    scan_page_list();
}

void test_malloc_free_over_4K()
{
    int vaddr1,vaddr2;

    vaddr1=malloc(10240);        //10240=10K,非4K对齐
    
    udisp_str("\n\n-------malloc(10240)-----\n\n");

    udisp_str("-----------vtable-------\n");
    scan_vtable();

    udisp_str("-----------malloc_table--------------\n");
    scan_malloc_table();

    udisp_str("-------------heap_pointer-------------\n");
    scan_heap_ptr();


    udisp_str("\n-------malloc(20480)-----\n\n");

    vaddr2=malloc(20480);         //20480=20K,4K对齐
    udisp_str("-----------vtable-------\n");
    scan_vtable();

    udisp_str("-----------malloc_table--------------\n");
    scan_malloc_table();

    udisp_str("-------------heap_pointer-------------\n");
    scan_heap_ptr();
   
    
    free(vaddr1);

    udisp_str("\n\n-------free the 10K block -----\n\n");

    udisp_str("-----------vtable-------\n");
    scan_vtable();

    udisp_str("-----------malloc_table--------------\n");
    scan_malloc_table();

    udisp_str("-------------heap_pointer-------------\n");
    scan_heap_ptr();

    udisp_str("-------------vpage_list---------------\n");
    scan_page_list();

    free(vaddr2);

    udisp_str("\n\n-------free the 20K block -----\n\n");

    udisp_str("-----------vtable-------\n");
    scan_vtable();

    udisp_str("-----------malloc_table--------------\n");
    scan_malloc_table();

    udisp_str("-------------heap_pointer-------------\n");
    scan_heap_ptr();

}



