#include <iostream>
#include <pthread.h>
#include <stdio.h>	
#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <queue>
#include <cstdlib>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;

#define NUM_THREADS 10
#define MEMORY_SIZE 20

char memory[MEMORY_SIZE];
int thread_message[NUM_THREADS]; 
pthread_mutex_t sharedLock = PTHREAD_MUTEX_INITIALIZER; // mutex
pthread_t server; // server thread handle
sem_t semlist[NUM_THREADS]; // thread semaphores
bool check=true;
struct queueq
{
	int id;
	int size;
	queueq(int a, int b):id(a), size(b){}
};
struct node
{
	int id;
	int size;
	int index;
	node *next;
	node(){};
	node(int ID,int size,int index,node *next):id(ID),size(size),index(index),next(NULL){};
};

node * head = new node(-1,MEMORY_SIZE,0,NULL);//list head initialized
queue<queueq> myqueue;//request queue initialized

void use_mem()//allocated threads sleeps random amount of time.
{
    int sleeptime = rand() %5+1;
	sleep(sleeptime);
}

void my_malloc(int& ID, int& size)//puts the request of the thread to the queue
{
	pthread_mutex_lock(&sharedLock);
	queueq mem(ID, size);
	myqueue.push(mem);
    pthread_mutex_unlock(&sharedLock);
}
void free_mem()//checks and merges two consecutive nodes with same id if there are any.
{
    if(head->next != NULL)
    {
        node* temp1 = head;
        node* temp2 = head->next;
        while(temp2 != NULL)
        {
            if(temp1->id == temp2->id)
            {
                temp1->size = temp1->size + temp2->size;
                temp1->next = temp2->next;
                delete temp2;
                break;
            }
            temp1 = temp1->next;
            temp2 = temp2->next;
        }
    }
}
void release_function()//deletes the list and turns memory back to its initial state.
{
	node *temp = head;
		check=false;

	while(temp != NULL)
	{
		head=head->next;
		delete temp;
		temp = head;
	}

	for(int i=0;i<MEMORY_SIZE;i++)
		memory[i]='X';
	 
}
void dump_memory()//just prints out the list and memory.
{
	if(check){
    node* temp = head;
    cout<<"List:"<<endl;
    while(temp != NULL)
    {
        cout<<"["<<temp->id<<"]"<<"["<<temp->size<<"]"<<"["<<temp->index<<"]";
        if(temp->next != NULL)
        {
            cout<<"---";
        }
        temp = temp->next;
    }
    cout<<endl<<"Memory Dump:"<<endl;
	for(int i = 0; i<MEMORY_SIZE; i++)
    {
        cout<<memory[i];
    }
	cout<<endl<<"*********************************"<<endl;}
}

void * thread_function(void * id) //send requests to my_malloc,then uses memory if allocated, after sleep time updates the memory back.
{
	while(check){
	int* idthread = (int*) id;
	int size = rand() % (MEMORY_SIZE/3)+1;
	my_malloc(*idthread,size);
	sem_wait(&semlist[*idthread]);
	pthread_mutex_lock(&sharedLock);
	if(thread_message[*idthread]!=-1)
	{
		use_mem();
		node* temp=head;
		while(temp!=NULL   &&   temp->id!=(*idthread) && check )
			temp=temp->next;
		if(temp!=NULL && check){
		temp->id=-1;
		for(int i=temp->index;i<temp->index+temp->size;i++)
			memory[i]='X';
		free_mem();
		free_mem();
		free_mem();
		free_mem();
		thread_message[*idthread]=-1;
		dump_memory();}
	}

	pthread_mutex_unlock(&sharedLock);
	}
}


void * server_function(void *)//checks if requests fit the list and uptades the list accordingly.
{

	
	while(check){
	
		while(!myqueue.empty()&&check)
		{
			pthread_mutex_lock(&sharedLock);
			queueq first = myqueue.front();
			myqueue.pop();
				
			if(head != NULL && head->next == NULL)
			{
				node *temp = new node(first.id,first.size,0,NULL);
				temp->next = head;
				for(int i=0;i<first.size;i++)
					memory[i] = '0' + first.id;
				head = temp;
				head->next->index+=first.size;
				head->next->size = head->next->size-first.size;
				free_mem();
				free_mem();free_mem();
				thread_message[first.id]=0;
				dump_memory();
			}
			else if (head != NULL && head->id == -1 && head->size >= first.size)
			{
				if(head->size == first.size)
				{
					head->id=first.id;
					for(int i=0;i<first.size;i++)
					memory[i] = '0' + first.id;
					thread_message[first.id]=0;
					free_mem();free_mem();
					free_mem();
					dump_memory();
				}
				else if (head !=NULL)
				{
				node* temp = head;
				node* newhead = new node(first.id,first.size,0,NULL);
				newhead->next=head;
				temp->size = temp->size - first.size;
				temp->index =+ first.size;
				head = newhead;
					for(int i=0;i<first.size;i++)
					 memory[i] = '0' + first.id;
					thread_message[first.id]=0;
					free_mem();
					free_mem();free_mem();
					dump_memory();
				}
			}
			else
			{
				node* temp = head;
				while(temp!=NULL)
				{
					if(temp->id == -1 && temp->size >= first.size)
						break;
					temp = temp->next;
				}
				if(temp != NULL)
				{
					if(temp->size == first.size)
					{
					temp->id=first.id;
					for(int i=temp->index;i<first.size+temp->index;i++)
						memory[i] = '0' + first.id;
					thread_message[first.id]=0;
					free_mem();
					free_mem();free_mem();
					dump_memory();
					}

					else 
					{
						if(temp->next!=NULL)
						{
							node* newhole = new node (-1,temp->size-first.size,temp->index+first.size,NULL);
							newhole->next = temp->next;
							temp->next=newhole;
							temp->id = first.id;
							temp->size = first.size;
							for(int i=temp->index;i<temp->index+temp->size;i++)
								memory[i] = '0' + first.id;
							thread_message[first.id]=0;
							free_mem();free_mem();
							free_mem();
							dump_memory();
						}
						else
						{
							node* newhole = new node (-1,temp->size-first.size,temp->index+first.size,NULL);
							temp->id = first.id;
							temp->size = first.size;
							temp->next = newhole;
							for(int i=temp->index;i<temp->index+temp->size;i++)
								memory[i] = '0' + first.id;
							thread_message[first.id]=0;
							free_mem();free_mem();
							free_mem();
							dump_memory();

						}
					}
				}
			}
			
			sem_post(&semlist[first.id]);
			pthread_mutex_unlock(&sharedLock);
			
		}
		
	}
}
void init()	 //semaphores and mutexes initialized
{
	pthread_mutex_lock(&sharedLock);	//lock
	for(int i = 0; i < NUM_THREADS; i++) //initialize semaphores
	{sem_init(&semlist[i],0,0);}
   	pthread_create(&server,NULL,server_function,NULL); //start server 
	pthread_mutex_unlock(&sharedLock); //unlock
}


int main(int argc, char *argv[])
{
	for(int i=0;i<MEMORY_SIZE;i++)	
		memory[i]='X';
	 int mythreads[NUM_THREADS];
	 for(int j=0;j<NUM_THREADS;j++)//this array holds the threads' ids
		 mythreads[j]=j;
	 init();
	 for(int i=0;i<NUM_THREADS;i++)
		 thread_message[i]=-1;
	 pthread_t thread[NUM_THREADS];//array of threads
	 
	 for(int i=0;i<NUM_THREADS;i++)//threads created
		 pthread_create(&thread[i],NULL,&thread_function,(void*) &mythreads[i]);
	 sleep(10);
	check=false;
	 
	 pthread_cancel(server);
	 for(int k=0;k<NUM_THREADS;k++)//threads joined 
		 pthread_cancel(thread[k]);
	 
	  release_function();
	 	 printf("\nTerminating...\n");

	
	 return 0;
}
