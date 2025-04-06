#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>


typedef long NUM;

/**
 * @brief for general case: start_k = k*(n/p), end_k = (k+1)*(n/p),
 * if this can't be divided evenly, we need to add 1 for the first r processorsthis is based ideal
 */
void split_array_indice(int n_size, int p_processor, int k, int *start_k, int *end_k) {
    int q = n_size / p_processor;
    int r = n_size % p_processor;
    *start_k = k * q + (k < r ? k : r);
    *end_k = *start_k + q - 1 + (k < r ? 1 : 0);
}


NUM local_sum(NUM array[], int start_k, int end_k) {
    NUM local_sum = 0;
    for (int i = start_k; i <= end_k; i++) {
        local_sum += array[i];
    }
    return local_sum;
}


/**
 * @brief Example: P_1 to P_0, P_2 to P_0, P_3 to P_1, P_4 to P_1, P_5 to P_2, P_6 to P_2, P_7 to P_3
 * First level, %2 == 0 for the receiver, %2 == 1 for the sender.
 * Second level, we need to use the rank to find the sender and receiver, maybe compare the send.
 * level 1, 1 3 5 7, level 2: 2 ,6 , level 3: 4.
 * Each level, split to different group and hight send to lower rank processor !
 */
int binary_tree_reduction(int p_processor_size, int rank, NUM local_sum) {
    for (int level = 1; level < p_processor_size; level *= 2) {
        if (rank % (2 * level) == 0) {
            if (rank + level < p_processor_size) { // Process exist!
                NUM recv_val;
                MPI_Recv(&recv_val, 1, MPI_LONG, rank + level, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                local_sum += recv_val;
            }
        } else {
            MPI_Send(&local_sum, 1, MPI_LONG, rank - level, 0, MPI_COMM_WORLD);
            break;
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

    int n = atoi(argv[1]);
    NUM *full_array = NULL;
    NUM *local_array = NULL;

    // Allocate local array
    int start_k, end_k;
    split_array_indice(n, size, rank, &start_k, &end_k);
    int local_size = end_k - start_k + 1;
    local_array = (NUM *) malloc(local_size * sizeof(NUM));
    if (local_array == NULL) {
        printf("Memory allocation failed for local_array on rank %d!\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    srand(rank);
    for (NUM i=0; i<local_size; i++) {
        local_array[i] = rand() % 10;
    }


    // Compute sum.
    NUM local_sum_value = local_sum(local_array, 0, local_size - 1);
    NUM global_sum = binary_tree_reduction(size, rank, local_sum_value);

    if (rank == 0) {
        printf("Final sum is: %ld\n", global_sum);
        free(full_array);
    }

    free(local_array);

    MPI_Finalize();
    return 0;
}
