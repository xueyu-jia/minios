#include "../include/slab.h"
#include "../include/buddy.h"
#include "../include/proto.h"

// cache数组下标从0开始，对应对象是8B。
// 该系统最大可存储2048B大小的字节，更大的直接分配一个页。所以 KMEM_CACHES_NUM 最大为 9
kmem_cache_t kmem_caches[KMEM_CACHES_NUM];

PRIVATE bitmap_check_avail(kmem_slab_t *slab);
PRIVATE void *slab_alloc_object(kmem_slab_t *slab);
kmem_slab_t *kmem_cache_new_slab(kmem_cache_t *cache);
PRIVATE void kmem_cache_init(kmem_cache_t *cache, const char *name, u32 objorder);
PRIVATE int slabCheckempty(kmem_slab_t *slab);
PRIVATE int slabCheckfull(kmem_slab_t* slab);
PRIVATE void deleteSlabfromOld(kmem_slab_t *slab);
PRIVATE void *kmem_cache_alloc_obj(kmem_cache_t *cache);
PRIVATE kmem_cache_t *selectCache(int size);
PRIVATE int freeObjectToSlab(kmem_slab_t *slab, u32 obj);
PRIVATE void moveSlab(kmem_slab_t* slab);
PRIVATE int get_index(int objsize);

/*
	系统初始化，针对cache数组
*/
void init_cache() {
	unsigned int order;
	char name[CACHE_NAME_LEN] = "Buffer_";

	for (order = MIN_BUFF_ORDER; order <= MAX_BUFF_ORDER; order++)
	{	
		int index = 7, temp = order;
		// 计算temp的位数
		int digits = 1;
		while (temp >= 10) {
			temp /= 10;
			digits++;
		}
		// 从最后一位开始填充数字
		index += digits;
    	temp = order;
		do {
			name[index] = temp % 10 + '0';
			temp /= 10;
			index--;
		} while(temp > 0);
		name[8 + digits] = '\0';

		kmem_cache_init(&kmem_caches[order - MIN_BUFF_ORDER], name, order);
	}

}

/*
	分配size大小的对象

	传入参数：
		size：要分配的对象大小
	
	输出：
		对象地址
*/
void *kmalloc(u32 size) {
	void *buff = NULL;
	kmem_cache_t *cache = selectCache(size);
	buff = kmem_cache_alloc_obj(cache);
	return K_LIN2PHY(buff);
}

/*
	释放对象

	传入参数：
		object ：要释放的对象的地址

*/
int kfree(u32 object) {
	
	kmem_slab_t *slab = NULL;
	page *page = pfn_to_page(phy_to_pfn(object));
	int objsize = page->cache->objsize;  // 得到对象的大小

	int flag = 0;
	int move=0;

	int order = get_index(objsize);
	kmem_cache_t* cache = &kmem_caches[order];
	object = K_PHY2LIN(object);
	while(!flag){ // 对象所在的链表可能还没有添加到新的链表，循环查找
		for(int i=PARTIAL; i<=FULL && !flag; i++){  // 依次查找partial链和full链
			acquire(&(cache->list[i].lock));  // 先对每一个链上锁
			kmem_slab_t *temp = cache->list[i].head;
			while(temp){
				if((object>=(u32)temp) && 
						(object<(u32)temp+BLOCK_SIZE)){
					slab = temp;
					move = freeObjectToSlab(temp, object);
					flag = 1;   // 找到
					break;
				}
				temp = temp->next;
			}
			release(&(cache->list[i].lock));  // 释放锁
		}
	}

	if(move){
		moveSlab(slab);
	}
	return 0;
}

/*
	检查bitmap返回第一个检查到的空闲object

	传入参数：
		slab ： slab结构指针。从指针指向的slab所管理的page中分配对象
	
	返回值：第一个空闲对象的下标
*/ 
PRIVATE bitmap_check_avail(kmem_slab_t *slab) {
	u32 obj_index, i ,j;
	bitmap_entry_t *bitmap = slab->bitmap;     // 找到位图起始地址
	kmem_cache_t *cache = slab->cache;
	for (i = 0; i < cache->bitmap_length; i++) // 遍历位图的每一个块
	{	 
		if (bitmap[i] != BITMAP_FULL) 			//当前块没有被完全分配
		{
			j = 0;								//j是空闲对象在位图块中的相对地址
			while (bitmap[i] & (1 << j)) j++;

			obj_index = i * BITMAP_ENTRY_BITS + j; // 计算出空闲对象的次序
			break;
		}
	}
	
	return obj_index;

}

/*
	从 slab 中分配对象 

	传入参数：
		slab ： slab结构指针。从指针指向的slab所管理的page中分配对象
	
	返回值：分配的对象的地址
*/ 
PRIVATE void *slab_alloc_object(kmem_slab_t *slab) {

	u32 obj_index;

	void *obj=NULL;
	obj_index = bitmap_check_avail(slab);
	bitmap_set_used(slab->bitmap, obj_index);		// 位图相应位置置位
	slab->used_obj++;
	obj = ptr_offset(slab->objects, obj_index*slab->cache->objsize); // 得到相应位置的地址
	return obj;  // 返回对象地址
}

/*
* @brief 	给定cache分配新的slab
* @param 	cache:某个cache的指针
* @retval 	新创建的slab的指针
* @details	给cache创建新的page。在page中初始化slab、位图、对象。
*/ 
kmem_slab_t *kmem_cache_new_slab(kmem_cache_t *cache) {
	int i;
	kmem_slab_t *slab;

	page *page = alloc_pages(kbud, 0);
	page->cache = cache;

	slab = (kmem_slab_t*)K_PHY2LIN(pfn_to_phy(page_to_pfn(page)));
	slab->cache = cache;
	slab->next = NULL;
	slab->used_obj= 0;
	slab->bitmap = (bitmap_entry_t*)ptr_offset(slab,sizeof(kmem_slab_t));
	slab->objects = ptr_offset(slab->bitmap, (cache->bitmap_length)*sizeof(bitmap_entry_t));
	slab->objects = ROUNDUP(slab->objects, cache->objorder);		//按照size对齐	lirong
	slab->type = EMPTY;
	
	/* 初始化位图 */
	i=0;
	while(i<cache->bitmap_length)
		slab->bitmap[i++] = BITMAP_EMPTY;

	return slab;
}


/*
	对每一个cache初始化

	传入参数：
		cache：要初始化的cache指针
		name: cache名字

*/
PRIVATE void kmem_cache_init(kmem_cache_t *cache, const char *name, u32 objorder) {
	int obj_size = pow(objorder);
	u32 bitmap_size, free;
	unsigned int obj_count;
	free = BLOCK_SIZE - sizeof(kmem_slab_t);

	// bitmap_size： 位图占用字节数
	// obj_count：对象个数
	obj_count = free/obj_size;  // 剩余空间中最多能存放的对象个数
	bitmap_size = calc_bitmap_size(obj_count); // 对象位图的字节数
	while(bitmap_size + obj_count*obj_size > free){
		obj_count --;
		bitmap_size = calc_bitmap_size(obj_count);
	}

	// 给cache命名
	char *temp = cache->name;
	while(*temp++ = *name++);

	cache->objsize = obj_size;
	cache->bitmap_length = bitmap_size / sizeof(bitmap_entry_t); // 位图块数
	cache->obj_per_slab = obj_count;  // 每个slab中对象个数
	//初始化链表和锁
	for(int i=EMPTY; i<=FULL; i++){
		cache->list[i].head = NULL;
		initlock(&(cache->list[i].lock), NULL);
	}
	cache->slab_count[EMPTY] = cache->slab_count[PARTIAL] = cache->slab_count[FULL] = 0; //统计slab数目
	cache->objorder = objorder;
}

/*
	检查slab是否为empty，用在释放对象之后

	传入参数：
		slab ： 已经释放的对象所在的slab的地址
	输出：
		0：不是empty
		1：是empty
*/ 
PRIVATE int slabCheckempty(kmem_slab_t *slab) {
	if(slab->used_obj==0){
		return 1;
	}
	return 0;
}

/*
	检查slab是否为full，用在分配对象之后

	传入参数：
		slab ： 已经分配的对象所在的slab的地址
	输出：
		0：不是full
		1：是full
*/ 
PRIVATE int slabCheckfull(kmem_slab_t* slab) {
	
	if(slab->used_obj==slab->cache->obj_per_slab)
		return 1;
	return 0;
	
}

/*
	将slab从当前的链中删除

	传入参数：
		slab ： 要删除的slab的指针
	
*/ 
PRIVATE void deleteSlabfromOld(kmem_slab_t *slab) {

	kmem_cache_t *cache = slab->cache;
	slab_type_t type = slab->type;
	kmem_slab_t *cur, *prev;

	prev = NULL;
	cur = cache->list[type].head;

	while (cur != slab)
	{
		prev = cur;
		cur = cur->next;
		if (cur == NULL)
			return ;
	}

	if (prev)
	{
		prev->next = cur->next;
	}
	else
	{
		cache->list[type].head = cur->next;
	}

	cache->slab_count[type]--;
}
 
/*
	从cache分配一个对象，返回对象的地址。分配后要对slab的类型进行判断，按照规则将slab从某个链中
	移除，并且插入到新的链中。

	传入参数：
		cache ： 指向cache的指针，从这个指针指向的cache中分配对象
	
	输出：
		对象地址
*/ 
PRIVATE void *kmem_cache_alloc_obj(kmem_cache_t *cache) {
	void *obj = NULL;
	int move = 0;
	int flag = 0;
	kmem_slab_t *slab;
	// 首先从partial类型的slab分配
	acquire(&(cache->list[PARTIAL].lock)); // 先加锁，再判断
	if (cache->slab_count[PARTIAL] > 0)
	{
		slab=cache->list[PARTIAL].head;
		obj = slab_alloc_object(slab);  // 分配一个对象
		flag=1;							// 表示分配对象成功
		if(slabCheckfull(slab)){		// 判断此时page类型，page所有对象是否全部被分配
			deleteSlabfromOld(slab);    // 将page从原来的链中删除
			slab->type = FULL;			// 修改当前的slab的类型
			move = 1;					// 表示之后要将slab插入新的链表中
		} 
		release(&(cache->list[PARTIAL].lock)); // 分配成功，释放锁
	}else{ 
		release(&(cache->list[PARTIAL].lock)); // 没有 PARTIAL 链表，释放锁
	}
	
	
	// 没有partial类型的slab，就从empty类型的slab分配
	if(!flag){
		acquire(&(cache->list[EMPTY].lock));
		if(cache->slab_count[EMPTY] > 0){
			slab=cache->list[EMPTY].head;
			obj = slab_alloc_object(slab);
			deleteSlabfromOld(slab);
			slab->type = PARTIAL;
			move =1;
			flag = 1;
			release(&(cache->list[EMPTY].lock));
		}else{
			release(&(cache->list[EMPTY].lock));
		}
	}
	
	// 两种类型都没有
	if(!flag){
		slab = kmem_cache_new_slab(cache); // 分配一个新的page，初始化slab
		obj = slab_alloc_object(slab);     // 分配对象
		slab->type = PARTIAL;			   // 修改类型
		move = 1;						   // 移动
	}
	

	if(move){
		moveSlab(slab);    // 将slab移动到对应的类型链表上
	}
	
	return obj;
}

/*
	根据要分配的对象大小，选择从哪个cache分配对象

	传入参数：
		size：要分配的对象大小
	
	输出：
		cache的指针
*/
PRIVATE kmem_cache_t *selectCache(int size) {
	int index = get_index(size);
	return &(kmem_caches[index]);
}


/*
	将一个对象释放到它所在的page中。同时判断释放后的slab类型，对满足规则的slab要从原来链表中
	删除，并且修改slab类型为它现在的类型

	输入参数：
		slab : 要释放的对象所在的slab指针
		obj  ：要释放的对象的地址
	返回值：
		0 ：表示slab没有从当前链表中删除
		1 ：表示slab从当前链表中被删除

*/

PRIVATE int freeObjectToSlab(kmem_slab_t *slab, u32 obj) {
	u32 obj_index;  // 要释放的对象在slab中的次序
	u32 start_addr = slab->objects; // 对象在slab中的起始地址
	obj_index = (obj-start_addr) / (slab->cache->objsize);
	if(bitmap_is_free(slab->bitmap, obj_index)){
		return 0;
	}
	bitmap_set_free(slab->bitmap, obj_index);
	slab->used_obj--;

	if(slab->type == PARTIAL){ // slab 释放之前是 PARTIAL 类型
		if(slabCheckempty(slab)){ // 释放后是 EMPTY 类型
			deleteSlabfromOld(slab); 
			slab->type = EMPTY;
			return 1;
		}
	}else if(slab->type == FULL){ // slab 释放之前是 FULL 类型
		// 将slab从原来链中移除，移动到新的slab链中
		deleteSlabfromOld(slab);
		slab->type = PARTIAL;
		return 1;
	}

	return 0;
}

/*
	将slab插入到 slab->type 类型的链表上

	输入：
		slab ：要插入的slab指针
*/
PRIVATE void moveSlab(kmem_slab_t* slab) {
	kmem_cache_t *cache = slab->cache;
	slab_type_t type = slab->type;

	// empty链上的page个数不能超过最大值，多的需要被释放
	if(type == EMPTY){                                 
		if(slab->cache->slab_count[EMPTY] >= MAX_EMPTY) {
			phy_kfree_4k(K_LIN2PHY(slab));
			return ;
		}
	}

	acquire(&(cache->list[type].lock));  // 对要移动的链上锁
	// 头插法
	slab->next = cache->list[type].head;
	cache->list[type].head = slab;

	cache->slab_count[type]++;
	release(&(cache->list[type].lock));
}  

/*
	由对象的大小得到要操作的cache的下标

	传入参数：
		objsize ：对象的大小
		
	输出：
		kmem_caches数组的下标


*/
PRIVATE int get_index(int objsize) {
	int index = 0; 
    int temp = (objsize-1)>>MIN_BUFF_ORDER; 
	// objsize-1 是为了8、16、32、64...这些大小的对象找到对应的index
	// >>MIN_BUFF_ORDER 是为了以 0 为起始
    while(temp){
        index++;
        temp >>= 1;
    }
	return index; 
}


/*
	打印某个cache使用信息

	输入：
		cache：指向cache的指针
*/
void printInfo(kmem_cache_t *cache)
{
	int all_object=0, used_object=0, free_object;
	all_object = cache->obj_per_slab * (cache->slab_count[EMPTY]+cache->slab_count[PARTIAL]+
	cache->slab_count[FULL]);

	kmem_slab_t *slab = cache->list[PARTIAL].head;
	while(slab!=NULL){
		used_object += slab->used_obj;
		slab = slab->next;
	}
	slab = cache->list[FULL].head;
	while(slab != NULL){
		used_object += slab->used_obj;
		slab = slab->next;
	}
	free_object = all_object-used_object;
	// double usage = 0;
	// if(all_object){
	// 	usage = 100 * ((double)used_object / all_object);
	// }
	disp_str("\ncache info\n");
	disp_str("Name: "); disp_str(cache->name);
	disp_str("\nObject size: "); disp_int(cache->objsize);
	disp_str("\nObject num per Slab: "); disp_int(cache->obj_per_slab);
	disp_str("\nslab info\n");
	disp_str("Number of EMPTY    slabs: "); disp_int(cache->slab_count[EMPTY]);
	disp_str("\nNumber of PARTIAL  slabs: "); disp_int(cache->slab_count[PARTIAL]);
	disp_str("\nNumber of FULL     slabs: "); disp_int(cache->slab_count[FULL]);
	disp_str("\nobject info\n");
	disp_str("Number of all  objects: "); disp_int(all_object);
	disp_str("\nNumber of used objects: "); disp_int(used_object);
	disp_str("\nNumber of free objects: "); disp_int(free_object);
	// disp_str("\nUsed space: %.1f%%", usage);
	disp_str("\n\n");
}

/*
	打印所有cache的指针
*/
void printInfoAll(){
	for(int i=MIN_BUFF_ORDER; i<=MAX_BUFF_ORDER; i++){
		printInfo(&kmem_caches[i-MIN_BUFF_ORDER]);
	}
}
