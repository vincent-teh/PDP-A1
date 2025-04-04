#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

typedef long NUM;

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

int binary_tree_reduction(int p_processor_size, int rank, NUM local_sum) {
    for (int level = 1; level < p_processor_size; level *= 2) {
        if (rank % (2 * level) == 0) {
            if (rank + level < p_processor_size) {
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

    if (rank == 0) {
        full_array = (NUM *)malloc(n * sizeof(NUM));
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
    local_array = (NUM *) malloc(local_size * sizeof(NUM));
    if (local_array == NULL) {
        printf("Memory allocation failed for local_array on rank %d!\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Scatterv(rank == 0 ? full_array : NULL, send_counts, displacements, MPI_LONG,
                 local_array, local_size, MPI_LONG, 0, MPI_COMM_WORLD);

    NUM local_sum_value = local_sum(local_array, 0, local_size - 1);
    NUM global_sum = binary_tree_reduction(size, rank, local_sum_value);

    if (rank == 0) {
        printf("Final sum is: %ld\n", global_sum);
        free(full_array);
    }

    free(local_array);
    free(send_counts);
    free(displacements);

    MPI_Finalize();
    return 0;
}