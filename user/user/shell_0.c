//#include "stdio.h"
#include "../include/stdio.h"	//modified by mingxuan 2021-8-7

int global=1;   //added by mingxuan 2020-12-21

void main(int arg,char *argv[])
{
    /*
	int stdin = open("dev_tty0",O_RDWR);
	int stdout= open("dev_tty0",O_RDWR);
	int stderr= open("dev_tty0",O_RDWR);
	*/

    char buf[1024];
    int pid;
    int times = 0;
    

    /*
	while(1)
	{
        printf("\nminiOS:/ $ ");
        if(gets(buf) && strlen(buf)!=0 ){
            if(execve(buf)!=0){
                printf("exec failed: file not found!\n");
                continue;
            }
        }
        
	}
    */
   
    /*
   	while(1)
	{
        printf("\nminiOS:/ $ ");
        if(gets(buf) && strlen(buf)!=0 ){
			if(fork()!=0){	//father

                while(1);

			}else{	//child
				if(execve(buf)!=0){
				printf("exec failed: file not found!\n");
                continue;
            }
			}
			
        }     
	}
    */
    

    /*
	if(fork()!=0)
    {//father
        while(1)
	    {
		    int i=10000;
		    while(--i){}
	    }
	}
    else
    {//child
        execve("orange/forkTest.bin");     
	}
    */

   /* // Test4,6
	if(fork()!=0)
    {//father
        udisp_str(" A ");
        while(1);
	}
    else
    {//child
        udisp_str(" B ");
        execve("orange/forkTest.bin");     
	}
    */

    /* // Test5
	if(fork()!=0)
    {//father
        udisp_int(++global);
        while(1);
	}
    else
    {//child
        udisp_int(--global); 
        while(1);   
	}
    */

    //Test7
    /*Correct Runing!*/ 
    /*
	if(fork()!=0)
    {//father
        udisp_str(" A ");
        int i=get_pid();
        udisp_int(i);
       
        while(1);
	}
    else
    {//child
        udisp_str(" B ");
        execve("orange/forkTest.bin");     
        while(1);
	}
    */

    /* //Test8
    if(fork()!=0)
    {//father
        udisp_str(" A ");
        while(1);
	}
    else
    {//child
        execve("orange/forkTest.bin");     
	}
    */

    
    //Test9
    /* 
    if(fork()!=0)
    {//father
        while(1);
	}
    else
    {//child
        execve("orange/forkTest.bin");     
	}
    */

    //Test10
    /*
    if(fork()!=0)
    {//father
        printf("Father ");  
        while(1);
	}
    else
    {//child
        printf("Child ");
        execve("orange/forkTest.bin");     
	}
    */

    //Test11
    /*
    if(fork()!=0)
    {//father
        while(1)
        {
            udisp_int(get_pid());
            udisp_str(" A ");
            int i=10000000;
		    while(--i){}
        }
	}
    else
    {//child
        execve("orange/forkTest.bin");     
	}
    */

    //Test12
    /*
    if(fork()!=0)
    {//father
        printf("%d ",get_pid());
        while(1);
	}
    else
    {//child
        execve("orange/execTest.bin");     
	}
    */

    //Test13
    /*
    if(fork()!=0)
    {//father
        wait_();
        while(1);
	}
    else
    {//child
        execve("orange/execTest.bin");     
	}
    */

    /*
   	while(1)
	{
        printf("\nminiOS:/ $ ");
        if(gets(buf) && strlen(buf)!=0 )
		{
			if(fork()!=0)
			{	//father

                int exit_status;
                exit_status = wait_();				
				printf("exit_status:%d", exit_status);
			}
			else
			{	//child
				if(execve(buf)!=0)
				{
					printf("exec failed: file not found!");
                	continue;
            	}
			}	
   		}
	}
    */

    //wait-exit test
    //Test1
    /*
   	while(1)
	{
        printf("\nminiOS:/ $ ");
        if(gets(buf) && strlen(buf)!=0 )
		{
			if(fork()!=0)
			{	//father

                int exit_status;
                exit_status = wait_();				
				printf("exit_status:%d", exit_status);
			}
			else
			{	//child

                //delay
		        int i=10000;
		        while(--i){}

                printf(" %d ", get_pid());
                exit(12);
			}	
   		}
	}
    */

    //Test2
    /*
   	while(1)
	{
        printf("\nminiOS:/ $ ");
        if(gets(buf) && strlen(buf)!=0 )
		{
			if(fork()!=0)
			{	//father
                
                //delay
		        int i=10000;
		        while(--i){}

                int exit_status;
                exit_status = wait_();				
				printf("exit_status:%d", exit_status);
			}
			else
			{	//child

                printf(" %d ", get_pid());
                exit(12);
			}	
   		}
	}
    */

   //子进程使用exec
   /*
   while(1)
	{
        //printf("\n");
        printf("\nminiOS:/ $ ");
        if(gets(buf) && strlen(buf)!=0 )
		{
			if(fork()!=0)
			{	//father

                int exit_status;
                exit_status = wait_();				
				printf("exit_status:%d", exit_status);
			}
			else
			{	//child

                //delay
		        int i=10000;
		        while(--i){}

                printf(" %d ", get_pid());

                //execve("orange/execTest.bin");
                execve("fat0/test_0.bin"); 
                //exit(12);
			}	
   		}
	}
    */

    //Test3
    /* //两兄弟
    while(1)
	{
        printf("\nminiOS:/ $ ");
        if(gets(buf) && strlen(buf)!=0 )
		{
			if(fork()!=0)
			{	//father
            
                if(fork()!=0)
			    {	//father
                    int exit_status = wait_();				
				    printf("exit_status:%d", exit_status);

                    exit_status = wait_();				
				    printf("exit_status:%d", exit_status);
			    }

			    else
			    {	//child2

                    //delay
		            int i=10000000;
		            while(--i){}

                    printf(" child2:%d ", get_pid());
                    exit(2);
			    }
		    }
		    else
		    {	//child1

                //delay
		        int i=10000000;
		        while(--i){}

                printf(" child1:%d ", get_pid());
                exit(1);
		    }
   		}
	}
    */
    

    //两兄弟都使用exec
    /*
    while(1)
	{
        //printf("\nminiOS:/ $ ");
        printf("\n");
        if(gets(buf) && strlen(buf)!=0 )
		{
			if(fork()!=0)
			{	//father
            
                if(fork()!=0)
			    {	//father
                    int exit_status = wait_();				
				    printf("exit_status:%d", exit_status);

                    exit_status = wait_();				
				    printf("exit_status:%d", exit_status);
			    }

			    else
			    {	//child2

                    //delay
		            //int i=10000000;
                    //int i=100000000; //延迟更多，以实现互斥使用exec
		            int i=500000000; //modified by mingxuan 2021-2-7
                    while(--i){}

                    printf(" child2:%d ", get_pid());
                    //exit(2);
                    //execve("orange/forkTest.bin");
                    //execve("orange/execTest.bin");
                    execve("fat0/test_0.bin");  
			    }
		    }
		    else
		    {	//child1

                //delay
		        int i=10000000;
		        while(--i){}

                printf(" child1:%d ", get_pid());
                //exit(1);
                //execve("orange/execTest.bin");
                execve("fat0/test_0.bin");
		    }
   		}
	}
    */

    /*
    //Test4
    while(1)
	{
        printf("\nminiOS:/ $ ");
        if(gets(buf) && strlen(buf)!=0 )
		{
			if(fork()!=0)
			{	//father
            
                if(fork()!=0)
			    {	//father
                    //delay
		            int i=10000000;
		            while(--i){}
                    
                    int exit_status = wait_();				
				    printf("exit_status:%d", exit_status);

                    //delay
		            i=10000000;
		            while(--i){}

                    exit_status = wait_();				
				    printf("exit_status:%d", exit_status);
			    }

			    else
			    {	//child2

                    printf(" child2:%d ", get_pid());
                    exit(2);
			    }
		    }
		    else
		    {	//child1

                printf(" child1:%d ", get_pid());
                exit(1);
		    }
   		}
	}
    */

    //Test5
    /*
    while(1)
	{
        printf("\nminiOS:/ $ ");
        if(gets(buf) && strlen(buf)!=0 )
		{
			if(fork()!=0)
			{	//father
            
                if(fork()!=0)
			    {	//father

                    if(fork()!=0)
			        {	//father
                    
                        int exit_status = wait_();				
				        printf("exit_status:%d", exit_status);

                        exit_status = wait_();				
				        printf("exit_status:%d", exit_status);

                        exit_status = wait_();				
				        printf("exit_status:%d", exit_status);
			        }

			        else
			        {	//child3
                        
                        //delay
		                int i=10000000;
		                while(--i){}

                        printf(" child2:%d ", get_pid());
                        exit(3);
			        }
			    }

			    else
			    {	//child2

                    //delay
		            int i=10000000;
		            while(--i){}

                    printf(" child2:%d ", get_pid());
                    exit(2);
			    }
		    }
		    else
		    {	//child1

                //delay
		        int i=10000000;
		        while(--i){}

                printf(" child1:%d ", get_pid());
                exit(1);
		    }
   		}
	}
    */

    //三兄弟都使用exec
    /*
    while(1)
	{
        //printf("\nminiOS:/ $ ");
        printf("\n");
        if(gets(buf) && strlen(buf)!=0 )
		{
			if(fork()!=0)
			{	//father
            
                if(fork()!=0)
			    {	//father
                    
                    if(fork()!=0)
			        {	//father
                    
                        int exit_status = wait_();				
				        printf("exit_status:%d", exit_status);

                        exit_status = wait_();				
				        printf("exit_status:%d", exit_status);

                        exit_status = wait_();				
				        printf("exit_status:%d", exit_status);
			        }
			        else
			        {	//child3
                        
                        //delay
		                //int i=50000000;
                        int i=2500000000;
		                while(--i){}

                        printf(" child3:%d ", get_pid());
                        //exit(3);
                        //execve("orange/execTest.bin");
                        execve("fat0/test_2.bin"); 
			        }
			    }
			    else
			    {	//child2

                    //delay
                    //int i=100000000; //延迟更多，以实现互斥使用exec
		            int i=500000000;
                    while(--i){}

                    printf(" child2:%d ", get_pid());
                    //exit(2);
                    //execve("orange/forkTest.bin");
                    execve("fat0/test_2.bin");  
			    }
		    }
		    else
		    {	//child1

                //delay
		        int i=10000000;
		        while(--i){}

                printf(" child1:%d ", get_pid());
                //exit(1);
                execve("fat0/test_2.bin");
		    }
   		}
	}
    */
    //get_pid();
  	while(1)
	{
        printf("\nminiOS:/ $ ");
        if(gets(buf) && strlen(buf)!=0 )
		{
			if(fork()!=0)
			{	//father

                int exit_status;
                //exit_status = wait_();
                wait_(&exit_status);//modified by dongzhangqi 2023.4.20 因wait的调整而修改
				printf("exit_status:%d", exit_status);
			}
			else
			{	//child
				if(execve(buf, NULL, NULL)!=0)
				{
					printf("exec failed: file not found!");
                	continue;
            	}
			}	
            // if(execve(buf)!=0){
            //     printf("exec failed: file not found!");
            //     continue;
            // }
   		}
	}

	return ;

}