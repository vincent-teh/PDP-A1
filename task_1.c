#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
// MPI send and recv: https://mpitutorial.com/tutorials/mpi-send-and-receive/
#include <time.h>  // For time measurement

void split_array_indice( int *array ,int n_size, int p_processor, int k, int *start_k, int *end_k) {
    int q= n /p ; // number of elements per processor
    int r= n % p ; // remainder , for the case which the n can not be divided by p evenly
    *start_k = k * q + (k < r ? k : r);; // start index for processor k , start_k=k*(n/p) end_k=(k+1)*(n/p), for general case
    // if this can;t be divided evenly, we need to add 1 for the first r processors, this is based ideal
    *end_k = *start_k + q - 1 + (k < r ? 1 : 0);; // end index for processor k
    printf("Processor %d: Start index = %d, End index = %d\n", k, start_k, end_k);

    // for (int k = 0; k < p_processor; k++)  // loop for each processor  -- no need , we can use MPI_Bcast to send the start and end index to each processor
    // {
    //     // Calculate the start and end indices for each processor

    // }
 
}

int local_sum(int *array, int start_k, int end_k) {
    int local_sum = 0;
    for (int i = start_k; i <= end_k; i++) {
        local_sum += array[i]; // sum the local array
    }
    return local_sum;
}

// MPI_Send(&data, count, type, dest, tag, comm)
// MPI_Recv(&data, count, type, source, tag, comm, &status)
// then we do the global sum, this is the main part of the code, we need to use the MPI_Send and MPI_Recv to do the global sum
int binary_tree_reduction (int p_processor_size, int rank, int local_sum  ){  //  P_1 to P_0, P_2 to P_0, P_3 to P_1, P_4 to P_1, P_5 to P_2, P_6 to P_2, P_7 to P_3
    // use %2 ==0 to find the receiver, use %2 ==1 to find the sender is just for the first level,
    // for the second level, is not same, we need to use the rank to find the sender and receiver, maybe compare!! 
    // the send .level 1, 1 3 5 7, level 2: 2 ,6 , level 3: 4 .  
    // each level, split to different group and hight send to lower rank processor ! 
    for (int step = 1; step < size; step *= 2) {
        if (rank % (2 * step) == 0) {
            // Receiver
            if (rank + step < size) { // this for avoid the precssor not exsit ! so make sure work for the 2 power of processor 
                int recv_val;
                MPI_Recv(&recv_val, 1, MPI_INT, rank + step, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                local_sum += recv_val;
            }
        } else {
            // Sender
            MPI_Send(&local_sum, 1, MPI_INT, rank - step, 0, MPI_COMM_WORLD);
            break; // After sending, you're done!
        }
    }
    return local_sum;

}

int main(int argc, char** argv) {  
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
    MPI_Comm_size(MPI_COMM_WORLD, &size); 

    if (argc < 2) {
        if (rank == 0) {
            printf("Usage: mpirun -np <num_procs> ./program <array_size>\n");
        }
        MPI_Finalize();
        return 1;
    }

    int n = atoi(argv[1]); // Array size
    int *full_array = NULL;
    int *local_array = NULL;

    // Generate the full array on rank 0
    if (rank == 0) {
        full_array = (int *)malloc(n * sizeof(int));
        for (int i = 0; i < n; i++) {
            full_array[i] = i + 1; // Initialize array with values 1 to n
        }
    }

    // Calculate chunk sizes and displacements
    int *send_counts = (int *)malloc(size * sizeof(int));
    int *displacements = (int *)malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        int start_k, end_k;
        split_array_indice(full_array, n, size, i, &start_k, &end_k);
        send_counts[i] = end_k - start_k + 1;
        displacements[i] = start_k;
    }

    // Allocate local array
    int local_size = send_counts[rank];
    local_array = (int *)malloc(local_size * sizeof(int));

    // Distribute chunks of the array to each processor
    MPI_Scatterv(full_array, send_counts, displacements, MPI_INT, local_array, local_size, MPI_INT, 0, MPI_COMM_WORLD);

    // Each rank computes its local sum
    int local_sum_value = local_sum(local_array, 0, local_size - 1);

    // Use tree-based reduction to compute the global sum
    int global_sum = binary_tree_reduction(size, rank, local_sum_value);

    // Rank 0 prints the final result
    if (rank == 0) {
        printf("Final sum is: %d\n", global_sum);
        free(full_array);
    }

    // Free allocated memory
    free(local_array);
    free(send_counts);
    free(displacements);

    MPI_Finalize();
    return 0;
}