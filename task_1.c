#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>


void split_array_indice(int n_size, int p_processor, int k, int *start_k, int *end_k) {
    int q = n_size / p_processor;
    int r = n_size % p_processor;
    *start_k = k * q + (k < r ? k : r);
    *end_k = *start_k + q - 1 + (k < r ? 1 : 0);
}


int local_sum(int *array, int start_k, int end_k) {
    int local_sum = 0;
    for (int i = start_k; i <= end_k; i++) {
        local_sum += array[i];
    }
    return local_sum;
}


/**
 * @brief
 * Use %2 ==0 to find the receiver, use %2 == 1 to find the sender is just for the first level,
 * for the second level, is not same, we need to use the rank to find the sender and receiver, maybe compare!!
 * the send .level 1, 1 3 5 7, level 2: 2 ,6 , level 3: 4 .
 * each level, split to different group and hight send to lower rank processor !
 */
int binary_tree_reduction(int p_processor_size, int rank, int local_sum) {
    for (int level = 1; level < p_processor_size; level *= 2) {
        if (rank % (2 * level) == 0) {
            if (rank + level < p_processor_size) {
                int recv_val;
                MPI_Recv(&recv_val, 1, MPI_INT, rank + level, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                local_sum += recv_val;
            }
        } else {
            MPI_Send(&local_sum, 1, MPI_INT, rank - level, 0, MPI_COMM_WORLD);
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
    int *full_array = NULL;
    int *local_array = NULL;

    if (rank == 0) {
        full_array = (int *)malloc(n * sizeof(int));
        if (full_array == NULL) {
            printf("Memory allocation failed for full_array!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        for (int i = 0; i < n; i++) {
            full_array[i] = rand() % 10;
        }
    }

    int *send_counts = (int *)malloc(size * sizeof(int));
    int *displacements = (int *)malloc(size * sizeof(int));
    if (send_counts == NULL || displacements == NULL) {
        printf("Memory allocation failed for send_counts or displacements on rank %d!\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (int i = 0; i < size; i++) {
        int start_k, end_k;
        split_array_indice(n, size, i, &start_k, &end_k);
        send_counts[i] = end_k - start_k + 1;
        displacements[i] = start_k;
    }

    int local_size = send_counts[rank];
    local_array = (int *) malloc(local_size * sizeof(int));
    if (local_array == NULL) {
        printf("Memory allocation failed for local_array on rank %d!\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Scatterv(rank == 0 ? full_array : NULL, send_counts, displacements, MPI_INT,
                 local_array, local_size, MPI_INT, 0, MPI_COMM_WORLD);

    int local_sum_value = local_sum(local_array, 0, local_size - 1);
    int global_sum = binary_tree_reduction(size, rank, local_sum_value);

    if (rank == 0) {
        printf("Final sum is: %d\n", global_sum);
        free(full_array);
    }

    free(local_array);
    free(send_counts);
    free(displacements);

    MPI_Finalize();
    return 0;
}
