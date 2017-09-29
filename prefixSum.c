#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"
#define SIZE 16

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  int p;
  MPI_Comm_size(MPI_COMM_WORLD, &p);
  int n = 2*p;
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int base_level[2];
  int x[SIZE];
  int result[SIZE];
  int levels = (int)log2(n);
  int above_levels[levels];
  for(int i=0; i<levels; i++)
  {
    above_levels[i] = 0; // initialize array
  }
  if(!rank)
  {
    for(int i=0; i<SIZE; i++)
    {
      x[i] = i+1;
    }
  }
  MPI_Scatter(x,2,MPI_INT,base_level,2,MPI_INT,0,MPI_COMM_WORLD);

  // Calculate the first level
  for(int i=0; i<2; i++)
  {
    above_levels[0] += base_level[i]; // Each processor has its own result
  }

  // Calculate from the second to top level
  // Only even-numbered processors are invovlved
  for(int level=1; level<levels; level++)
  {
    int remain = (int)pow(2,level-1);
    int num = (int)pow(2,level);

    if(rank%num == 0)
    {
      int num_recv;
      MPI_Recv(&num_recv,1,MPI_INT,rank+remain,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      above_levels[level] = above_levels[level-1]+num_recv;
    }
    else if(rank%num == remain)
    {
      int num_sent = above_levels[level-1];
      MPI_Send(&num_sent,1,MPI_INT,rank-remain,0,MPI_COMM_WORLD);
    }
  }

  for(int level=levels-2; level>=0; level--)
  {
    int remain = (int)pow(2,level);
    int num = (int)pow(2,level+1);
    int num_sent = above_levels[level];
    int num_above_sent = above_levels[level+1];
    int last_node = (pow(2,levels-level-2)-1)*num+remain;
    int num_recv;
    MPI_Status status;

    if(num_above_sent != 0)
    {
      //printf("Value: %d from rank: %d to %d, level: %d\n", num_above_sent, rank, rank+remain, level);

      //A node sends its value to its right child
      MPI_Send(&num_above_sent,1,MPI_INT,rank+remain,0,MPI_COMM_WORLD);
    }

    if(num_sent != 0)
    {
      if(rank%num == remain) // the right child is going to receive the value
      {
        //printf("rank: %d, level: %d\n", rank, level);
        MPI_Recv(&num_recv,1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
        above_levels[level] = num_recv;
      }
    }

    if(num_sent != 0)
    {
      //A node sends its value to the left child of its right sibling
      if(level!=0 && rank!=last_node)
      {
        num_sent = above_levels[level];
        //printf("above_levels[%d]: %d, rank: %d\n", level, above_levels[level], rank);
        MPI_Send(&num_sent,1,MPI_INT,rank+remain,0,MPI_COMM_WORLD);
      }

      if(rank!=0 && rank%num==0) // the left child is going to receive the value
      {
        MPI_Recv(&num_recv,1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
        //printf("above_levels[%d]: %d + num_recv %d, rank: %d,\n", level, above_levels[level], num_recv, rank);
        above_levels[level] += num_recv;
      }
    }

    MPI_Barrier(MPI_COMM_WORLD);
  }

  //continue applying the rules down to base_level
  int num_sent = above_levels[0];
  int num_recv;
  base_level[1] = num_sent;
  if(rank != SIZE/2-1)
  {
    //printf("%d from %d to %d\n", num_sent, rank, rank+1);
    MPI_Send(&num_sent,1,MPI_INT,rank+1,0,MPI_COMM_WORLD);
  }

  if(rank)
  {
    MPI_Recv(&num_recv,1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    base_level[0] += num_recv;
  }

  MPI_Gather(&num_recv,2,MPI_INT,result,2,MPI_INT,0,MPI_COMM_WORLD);

  /*if(!rank)
  {
    for(int i; i<SIZE; i++)
    {
      printf("%d ", result[i]);
    }
  }*/

  for(int i=0; i<4 && above_levels[i]!=0; i++)
  {
    printf("value %d, level: %d, rank: %d\n", above_levels[i], i, rank);
  }

  MPI_Finalize();
}
