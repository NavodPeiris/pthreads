#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

// maximum number of threads
#define MAX_THREADS 4

// struct to hold data for each thread
typedef struct {
    int id; // thread id
    int num_threads; // total number of threads
    int** A; // input matrix A
    int** B; // input matrix B
    int** C; // output matrix C
    int m; // number of rows in A
    int n; // number of columns in A (same as number of rows in B)
    int p; // number of columns in B
} ThreadData;

// function to get a matrix from a file
int** read_matrix_from_file(char* filename, int m, int n) {
    int** A = (int**)malloc(m * sizeof(int*));
    for (int i = 0; i < m; i++) {
        A[i] = (int*)malloc(n * sizeof(int));
    }
    FILE* fp = fopen(filename, "r");
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            fscanf(fp, "%d", &A[i][j]);
        }
    }
    fclose(fp);
    return A;
}

// function to generate a random matrix
int** generate_random_matrix(int m, int n) {
    int** A = (int**)malloc(m * sizeof(int*));
    for (int i = 0; i < m; i++) {
        A[i] = (int*)malloc(n * sizeof(int));
        for (int j = 0; j < n; j++) {
            A[i][j] = rand() % 10;
        }
    }
    return A;
}

// function to multiply two matrices using a single thread
void multiply_matrices_single_thread(int** A, int** B, int** C, int m, int n, int p) {
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < p; j++) {
            C[i][j] = 0;
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

// function to multiply two matrices using multiple threads with row-wise division
void* multiply_matrices_thread_row(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int rows_per_thread = (data->m + data->num_threads - 1) / data->num_threads; // round up
    int start_row = data->id * rows_per_thread;
    int end_row = (data->id + 1) * rows_per_thread;
    if (end_row > data->m) {
        end_row = data->m;
    }
    for (int i = start_row; i < end_row; i++) {
        for (int j = 0; j < data->p; j++) {
            data->C[i][j] = 0;
            for (int k = 0; k < data->n; k++) {
                data->C[i][j] += data->A[i][k] * data->B[k][j];
            }
        }
    }
    pthread_exit(NULL);
}

//column-wise division
void* multiply_matrices_thread_col(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int cols_per_thread = (data->p + data->num_threads - 1) / data->num_threads; // round up
    int start_col = data->id * cols_per_thread;
    int end_col = (data->id + 1) * cols_per_thread;
    if (end_col > data->p) {
        end_col = data->p;
    }
    for (int j = start_col; j < end_col; j++) {
        for (int i = 0; i < data->m; i++) {
            data->C[i][j] = 0;
            for (int k = 0; k < data->n; k++) {
                data->C[i][j] += data->A[i][k] * data->B[k][j];
            }
        }
    }
    pthread_exit(NULL);
}

// function to multiply two matrices using multiple threads with block-wise division
void* multiply_matrices_thread_block(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int block_size = (data->m + data->num_threads - 1) / data->num_threads; // round up
    int start_row = (data->id * block_size) % data->m;
    int end_row = start_row + block_size;
    if (end_row > data->m) {
        end_row = data->m;
    }
    int start_col = (data->id * block_size) / data->m;
    int end_col = start_col + block_size;
    if (end_col > data->p) {
        end_col = data->p;
    }
    for (int i = start_row; i < end_row; i++) {
        for (int j = start_col; j < end_col; j++) {
            data->C[i][j] = 0;
            for (int k = 0; k < data->n; k++) {
                data->C[i][j] += data->A[i][k] * data->B[k][j];
            }
        }
    }
    pthread_exit(NULL);
}

// function to print a matrix
void print_matrix(int** A, int m, int n) {
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", A[i][j]);
        }
        printf("\n");
    }
}

// function to measure the time difference between two timevals
double timeval_diff(struct timeval* start, struct timeval* end) {
    return (double)(end->tv_sec - start->tv_sec) + (double)(end->tv_usec - start->tv_usec) / 1000000.0;
}

int main() {
    int n1, m2, m, n, p;
    int num_threads = MAX_THREADS;
    // get dimensions of matrix A
    printf("Enter the number of rows of matrix A: ");
    scanf("%d", &m);
    printf("Enter the number of columns of matrix A: ");
    scanf("%d", &n1);

    // get dimensions of matrix B
    printf("Enter the number of rows of matrix B: ");
    scanf("%d", &m2);
    printf("Enter the number of columns of matrix B: ");
    scanf("%d", &p);

    // check if matrices are multipliable
    if (n1 != m2) {
        printf("Error: The number of columns of matrix A must be equal to the number of rows of matrix B.\n");
        return 1;
    }

    n = n1;

    // generate or read matrices A and B
    int** A;
    int** B;
    int** C = (int**)malloc(m * sizeof(int*));
    for (int i = 0; i < m; i++) {
        C[i] = (int*)malloc(p * sizeof(int));
    }
    struct timeval start, end;

    int choice;

    printf("Select one of the following options\n");
    printf("  1.Fill the matrix with the values given in a file\n  2.Fill the matrix with random values\n\n");
    printf(" Enter your choice: ");
    scanf("%d", &choice);
    while (!(choice == 1 || choice == 2))
    {
        printf(" Please enter a valid input...");
        scanf("%d", &choice);
    }

    if (choice == 1)
    {
        A = read_matrix_from_file("A.txt", m, n);
        B = read_matrix_from_file("B.txt", n, p);    
    }
    else if (choice == 2)
    {
        A = generate_random_matrix(m, n);
        B = generate_random_matrix(n, p);
    }

    // to print A and B
    printf("\n Matrix A:\n");
    print_matrix(A, m, n);

    printf("\nMatrix B:\n");
    print_matrix(B, n, p);

    // multiply matrices using a single thread
    gettimeofday(&start, NULL);
    multiply_matrices_single_thread(A, B, C, m, n, p);
    gettimeofday(&end, NULL);

    printf("\n Matrix C:\n");  //print result matrix
    print_matrix(C, m, p);

    printf("\n\nSingle-threaded multiplication took %f seconds.\n", timeval_diff(&start, &end));

    // multiply matrices using multiple threads with row-wise division
    gettimeofday(&start, NULL);
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].id = i;
        thread_data[i].num_threads = num_threads;
        thread_data[i].A = A;
        thread_data[i].B = B;
        thread_data[i].C = C;
        thread_data[i].m = m;
        thread_data[i].n = n;
        thread_data[i].p = p;
        pthread_create(&threads[i], NULL, multiply_matrices_thread_row, &thread_data[i]);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    gettimeofday(&end, NULL);
    printf("Row-wise threaded multiplication took %f seconds.\n", timeval_diff(&start, &end));

    // multiply matrices using multiple threads with column-wise division
    gettimeofday(&start, NULL);
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].id = i;
        thread_data[i].num_threads = num_threads;
        thread_data[i].A = A;
        thread_data[i].B = B;
        thread_data[i].C = C;
        thread_data[i].m = m;
        thread_data[i].n = n;
        thread_data[i].p = p;
        pthread_create(&threads[i], NULL, multiply_matrices_thread_col, &thread_data[i]);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    gettimeofday(&end, NULL);
    printf("Column-wise threaded multiplication took %f seconds.\n", timeval_diff(&start, &end));

    // multiply matrices using multiple threads with block-wise division
    gettimeofday(&start, NULL);
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].id = i;
        thread_data[i].num_threads = num_threads;
        thread_data[i].A = A;
        thread_data[i].B = B;
        thread_data[i].C = C;
        thread_data[i].m = m;
        thread_data[i].n = n;
        thread_data[i].p = p;
        pthread_create(&threads[i], NULL, multiply_matrices_thread_block, &thread_data[i]);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    gettimeofday(&end, NULL);
    printf("Block-wise threaded multiplication took %f seconds.\n", timeval_diff(&start, &end));

    return 0;
}
