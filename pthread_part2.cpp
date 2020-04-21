#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sched.h>
#include <time.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <unistd.h>
#include <algorithm>


#define CORE_NUM 4
using namespace std;
/*****************************************************
Compiler: g++ -pthread pthread_part2.cpp -o pthread_part2.out
Execute	: ./pthread_part2.out <Input file>
******************************************************/

pthread_mutex_t count_mutex;	// pthread lock variable

class Thread {

public:
	int ID;
	int PID;
	int Core;
	int Set_Core = -1;
	float Utilization;
	int Matrix_Size;	
	float **Matrix;
	float **Single_Result;
	float **Multi_Result;		//Result have to saving in this function

public:
	void Set_Thread_ID(int _ID)
	{
		ID = _ID;
	}
	
	void Set_Thread_Core(int _Core)
	{
		Set_Core = _Core;
	}

	void Set_Thread_Matrix_Size(float _Matrix_Size)
	{
		Matrix_Size = _Matrix_Size;
	}	
	
	void Initial_Thread(void)
	{
		Utilization = float(Matrix_Size/2000.0);	
		//cout << "Matrix Size : " << Matrix_Size << endl;		
		Matrix = new float*[Matrix_Size];
		for( int i = 0; i < Matrix_Size; i++){
			Matrix[i] = new float[Matrix_Size];
		}

		Single_Result = new float*[Matrix_Size];
		for( int i = 0; i < Matrix_Size; i++){
			Single_Result[i] = new float[Matrix_Size];
		}

		Multi_Result = new float*[Matrix_Size];
		for( int i = 0; i < Matrix_Size; i++){
			Multi_Result[i] = new float[Matrix_Size];
		}

		//Initial Matrix Value
		for( int i = 0; i < Matrix_Size; i++){
			for( int j = 0; j < Matrix_Size; j++){	
				Matrix[i][j] = ((float) rand()/ (RAND_MAX));
				//cout <<  Matrix[i][j] << " " ;
			}
			//cout << endl;
		}
		//cout << endl;			
	}

	void Single_Matrix_Multiplication(void)
	{	
		//Print Thread information
		Core = sched_getcpu();
		PID = syscall(SYS_gettid);
		Print_Information();
		//Multiplication
		for( int i = 0 ; i < Matrix_Size; i++){
			for( int j = 0 ; j < Matrix_Size; j++)
			{
				Single_Result[i][j] = 0;
				for( int k = 0 ; k < Matrix_Size; k++)
				{
					Single_Result[i][j] += Matrix[i][k]*Matrix[k][j];
				}	
				//cout << " [" << i << "," << j << "] :" << Single_Result[i][j] << endl;			
			}
		}
		//cout << endl;		
	}

	void* Multi_Matrix_Multiplication(void *args)
	{
		//Print Thread information
		pthread_mutex_lock( &count_mutex );
		Set_CPU(Set_Core);
		Core = sched_getcpu();
		PID = syscall(SYS_gettid);
		Print_Information();
		pthread_mutex_unlock( &count_mutex );
		//Multiplication	
		for( int i = 0 ; i < Matrix_Size; i++){
			for( int j = 0 ; j < Matrix_Size; j++)
			{
				Multi_Result[i][j] = 0;
				for( int k = 0 ; k < Matrix_Size; k++)
				{
					Multi_Result[i][j] += Matrix[i][k]*Matrix[k][j];
				}	
				//cout << " [" << i << "," << j << "] :" << Single_Result[i][j] << endl;	
				if ( Core != sched_getcpu() )
				{
					cout << "The thread " << ID << " PID " << PID << " is moved from CPU " << Core << " to " << sched_getcpu() << endl;
					Core = sched_getcpu();
				}		
			}
		}
		//cout << endl;		
	}

	void Set_CPU( int CPU_NUM )
	{
		if (CPU_NUM == -1)
			return;
		else{
			cpu_set_t set;
			CPU_ZERO(&set);
			CPU_SET(CPU_NUM, &set);
			sched_setaffinity(0, sizeof(set), &set);
			Set_Core = -1;
		}
	}

	void Compare_Result(void)
	{
		for( int i = 0 ; i < Matrix_Size; i++){
			for( int j = 0 ; j < Matrix_Size; j++)
			{
				if( Multi_Result[i][j] != Single_Result[i][j])
				{
					cout << "ID : " << ID << "\tResult Not Same !! " << endl;
					return ;
				} 	
				Multi_Result[i][j] = 0;				
			}
		}
		cout << "ID : " << ID << "\tPASS" << endl;		
	}
		
	void Print_Information(void)
	{
		//cout << "The thread " << ID << " PID : " << PID << " is on CPU" << Core << endl;	
		cout << "Thread ID : " << ID;
		cout << "\tPID : " << PID;
		cout << "\tCore : " << Core;
		cout << "\tUtilization : " << Utilization;
		cout << "\tMatrix_Size : " << Matrix_Size << endl;	
	}
};

class CPU {

public:
	int CPU_ID;
	int *Thread_list ;
	int Thread_count = 0;
	float Utilization = 0 ;
	int sort_Num = -1;
	
public:
	void Create_CPU(int Thread_Num, int _CPU_ID)
	{
		CPU_ID = _CPU_ID;
		Thread_list = new int[Thread_Num];
		Thread_count = 0;
		Utilization = 0;
	}

	void Push_Thread_To_CPU(int Thread_ID)
	{
		Thread_list[Thread_count] = Thread_ID;
		Thread_count++;
	}
	void Empty_CPU(void)
	{
		for ( int i = 0 ; i < Thread_count; i++)
		{
			Thread_list[i] = 0;
		}
		Thread_count = 0;
		Utilization = 0;
	}
	void Print_CPU_Information()
	{
		cout << "Core Number : " << CPU_ID << endl;
		cout << "[ " ;
		for ( int i = 0 ; i < Thread_count; i++)
		{
			
			cout << Thread_list[i] << ", " ;
		}
		cout << "]" << endl;
		cout << "Total Utilization : " << Utilization << endl;
		cout << endl;
	}
};

bool compare(int a,int b)
{
    return a > b;
}

typedef void * (*THREADFUNCPTR)(void *); //function pointer


int main(int argc, char** argv)
{
	int Num_Thread = 0;
	Thread* Thread_Set;
	CPU* CPU_Set;
	string line;
	struct timeval start;
    struct timeval end;
	double Time_Use;

	cout << "Input File Name : " << argv[1] << endl ;
/********************************** Input ************************************************************************
******************************************************************************************************************/
	ifstream myfile (argv[1]);
	if (myfile.is_open())
	{
		//Get Number of Thread
		getline (myfile,line);
		Num_Thread = atoi(line.c_str());		
		cout << "Num_Thread : " << Num_Thread << endl;

		//Read file
		int Read_Matrix_Size = 0;
		Thread_Set = new Thread[Num_Thread];
		for ( int i = 0; i<Num_Thread; i++)
		{
			getline (myfile,line);
			Read_Matrix_Size = atoi(line.c_str());
			//cout << read_U << endl;
			Thread_Set[i].Set_Thread_ID(i);
			Thread_Set[i].Set_Thread_Matrix_Size(Read_Matrix_Size);			
		}
		myfile.close();
	}
	else{	
		cout << "Error Input File Name" << endl;
		return 0;
	}
/********************************** Matrix Initialize ************************************************************
******************************************************************************************************************/
	for ( int i = 0; i<Num_Thread; i++)
		Thread_Set[i].Initial_Thread();

	// Print_All_Thread_Information(Thread_Set, Num_Thread);

/********************************** Single Thread Matrix Multiplication *********************************************
******************************************************************************************************************/	
	cout << "\n===========Start Single Thread Matrix Multiplication===========" << endl; 
	gettimeofday(&start, NULL);
	for ( int i = 0; i<Num_Thread; i++)
	{
		Thread_Set[i].Single_Matrix_Multiplication();
	}
	gettimeofday(&end, NULL);
	Time_Use = (end.tv_sec - start.tv_sec) + (double)(end.tv_usec - start.tv_usec)/1000000.0;
	cout << "Single Thread Spend time : " << Time_Use << endl;




//////////////////////////////////////////
	//Initial cpu
	CPU_Set = new CPU[CORE_NUM];
	for( int i = 0; i < CORE_NUM; i++)
	{
		CPU_Set[i].Create_CPU(Num_Thread, i);
	}
//////////////////////////////////////////

/********************************** Partition First-Fit Multi Thread Matrix Multiplication ************************************
******************************************************************************************************************/
	cout << "\n===========Partition First-Fit Multi Thread Matrix Multiplication===========" << endl;
	pthread_t pthread_Thread[Num_Thread]; 

	// First-Fit Scheduling
	for(int thread_index = 0; thread_index < Num_Thread; thread_index++)
	{
		for(int CoreID = 0; CoreID < CORE_NUM; CoreID++)
		{		
			if( CPU_Set[CoreID].Utilization + Thread_Set[thread_index].Utilization <= 1)
			{

				CPU_Set[CoreID].Push_Thread_To_CPU(thread_index);
				CPU_Set[CoreID].Utilization += Thread_Set[thread_index].Utilization;
				Thread_Set[thread_index].Set_Thread_Core(CoreID);
				break;
			}else 
			{
				if (CoreID == CORE_NUM - 1)
				{
					cout << "Thread " << thread_index << " is not push." << endl;
				}					
			}		
		}
	}
	//Print 
	for( int i = 0; i < CORE_NUM; i++)
	{
		cout << "CPU" << i << " : " ;
		CPU_Set[i].Print_CPU_Information();
	}
	


	//Start pthread execution
	gettimeofday(&start, NULL);


	// Creating threads, each evaluating its own part 
    for (int i = 0; i < Num_Thread; i++)
	{		
		pthread_create(&pthread_Thread[i], NULL, (THREADFUNCPTR)&Thread::Multi_Matrix_Multiplication, &Thread_Set[i]);
	}
	// joining and waiting for all threads to complete 
    for (int i = 0; i < Num_Thread; i++)
	{
		pthread_join(pthread_Thread[i], NULL); 
	} 

	gettimeofday(&end, NULL);

	Time_Use = (end.tv_sec - start.tv_sec) + (double)(end.tv_usec - start.tv_usec)/1000000.0;
	cout << "Multi Thread Spend time : " << Time_Use << endl;

	cout << "\n===========Partition First-Fit Multi Thread Compare Result===========" << endl; 
	for ( int i = 0; i<Num_Thread; i++)
	{
		Thread_Set[i].Compare_Result();
	}

/********************************** Partition Best-Fit Multi Thread Matrix Multiplication ************************************
******************************************************************************************************************/
	cout << "\n===========Partition Best-Fit Multi Thread Matrix Multiplication===========" << endl;

	for( int i = 0; i < CORE_NUM; i++)
	{
		CPU_Set[i].Empty_CPU();
	}
	
	// Best-Fit Scheduling
	int Best_CoreID;
	for(int thread_index = 0; thread_index < Num_Thread; thread_index++)
	{
		Best_CoreID = -1;
		// find best Core for thread
		for(int CoreID = 0; CoreID < CORE_NUM; CoreID++)
		{		
			if( CPU_Set[CoreID].Utilization + Thread_Set[thread_index].Utilization <= 1)
			{
				if(Best_CoreID == -1)
				{
					Best_CoreID = CoreID;
				}else if(CPU_Set[Best_CoreID].Utilization < CPU_Set[CoreID].Utilization)
				{
					Best_CoreID = CoreID;
				}
			}												
		}
		if (Best_CoreID != -1)
		{
			CPU_Set[Best_CoreID].Push_Thread_To_CPU(thread_index);
			CPU_Set[Best_CoreID].Utilization += Thread_Set[thread_index].Utilization;
			Thread_Set[thread_index].Set_Thread_Core(Best_CoreID);
		}else
		{
			cout << "Thread " << thread_index << " is not push." << endl;
		}		
	}
	
	//Print 
	for( int i = 0; i < CORE_NUM; i++)
	{
		cout << "CPU" << i << " : " ;
		CPU_Set[i].Print_CPU_Information();
	}
	
	//Start pthread execution
	gettimeofday(&start, NULL);

	// Creating threads, each evaluating its own part 
    for (int i = 0; i < Num_Thread; i++)
	{		
		pthread_create(&pthread_Thread[i], NULL, (THREADFUNCPTR)&Thread::Multi_Matrix_Multiplication, &Thread_Set[i]);
	}
	// joining and waiting for all threads to complete 
    for (int i = 0; i < Num_Thread; i++)
	{
		pthread_join(pthread_Thread[i], NULL); 
	} 

	gettimeofday(&end, NULL);
	Time_Use = (end.tv_sec - start.tv_sec) + (double)(end.tv_usec - start.tv_usec)/1000000.0;
	cout << "Multi Thread Spend time : " << Time_Use << endl;

	cout << "\n===========Partition Best-Fit Multi Thread Compare Result===========" << endl; 
	for ( int i = 0; i<Num_Thread; i++)
	{
		Thread_Set[i].Compare_Result();
	}

/********************************** Partition Worst-Fit Multi Thread Matrix Multiplication ************************************
******************************************************************************************************************/
	cout << "\n===========Partition Worst-Fit Multi Thread Matrix Multiplication===========" << endl;

	for( int i = 0; i < CORE_NUM; i++)
	{
		CPU_Set[i].Empty_CPU();
	}
	
	// Worst-Fit Scheduling
	int Worst_CoreID;
	for(int thread_index = 0; thread_index < Num_Thread; thread_index++)
	{
		Worst_CoreID = -1;
		// find worst Core for thread
		for(int CoreID = 0; CoreID < CORE_NUM; CoreID++)
		{		
			if( CPU_Set[CoreID].Utilization + Thread_Set[thread_index].Utilization <= 1)
			{
				if(Worst_CoreID == -1)
				{
					Worst_CoreID = CoreID;
				}else if(CPU_Set[Worst_CoreID].Utilization > CPU_Set[CoreID].Utilization)
				{
					Worst_CoreID = CoreID;
				}
			}												
		}
		if (Worst_CoreID != -1)
		{
			CPU_Set[Worst_CoreID].Push_Thread_To_CPU(thread_index);
			CPU_Set[Worst_CoreID].Utilization += Thread_Set[thread_index].Utilization;
			Thread_Set[thread_index].Set_Thread_Core(Worst_CoreID);
		}else
		{
			cout << "Thread " << thread_index << " is not push." << endl;
		}		
	}
	//Print 
	for( int i = 0; i < CORE_NUM; i++)
	{
		cout << "CPU" << i << " : " ;
		CPU_Set[i].Print_CPU_Information();
	}

	//Start pthread execution
	gettimeofday(&start, NULL);
	// Creating threads, each evaluating its own part 
    for (int i = 0; i < Num_Thread; i++)
	{		
		pthread_create(&pthread_Thread[i], NULL, (THREADFUNCPTR)&Thread::Multi_Matrix_Multiplication, &Thread_Set[i]);
	}
	// joining and waiting for all threads to complete 
    for (int i = 0; i < Num_Thread; i++)
	{
		pthread_join(pthread_Thread[i], NULL); 
	} 

	gettimeofday(&end, NULL);
	Time_Use = (end.tv_sec - start.tv_sec) + (double)(end.tv_usec - start.tv_usec)/1000000.0;
	cout << "Multi Thread Spend time : " << Time_Use << endl;

	cout << "\n===========Partition Worst-Fit Multi Thread Compare Result===========" << endl; 
	for ( int i = 0; i<Num_Thread; i++)
	{
		Thread_Set[i].Compare_Result();
	}

/********************************** Global Multi Thread Matrix Multiplication ************************************
******************************************************************************************************************/
	cout << "\n===========Start Global Multi Thread Matrix Multiplication===========" << endl;

	for( int i = 0; i < CORE_NUM; i++)
	{
		CPU_Set[i].Empty_CPU();
	}

	//Print 
	for( int i = 0; i < CORE_NUM; i++)
	{
		cout << "CPU" << i << " : " ;
		CPU_Set[i].Print_CPU_Information();
	}

	//Start pthread execution
	gettimeofday(&start, NULL);
	// Creating threads, each evaluating its own part 
    for (int i = 0; i < Num_Thread; i++)
	{		
		pthread_create(&pthread_Thread[i], NULL, (THREADFUNCPTR)&Thread::Multi_Matrix_Multiplication, &Thread_Set[i]);
	}
	// joining and waiting for all threads to complete 
    for (int i = 0; i < Num_Thread; i++)
	{
		pthread_join(pthread_Thread[i], NULL); 
	} 

	gettimeofday(&end, NULL);
	Time_Use = (end.tv_sec - start.tv_sec) + (double)(end.tv_usec - start.tv_usec)/1000000.0;
	cout << "Multi Thread Spend time : " << Time_Use << endl;

	cout << "\n===========Partition Global Multi Thread Compare Result===========" << endl; 
	for ( int i = 0; i<Num_Thread; i++)
	{
		Thread_Set[i].Compare_Result();
	}


/********************************** Output ***********************************************************************
******************************************************************************************************************/	

	cout << "\nSuccess Execution !!" << endl; 
	//sleep(10);
	return 0;
}










