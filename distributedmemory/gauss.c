/* Algorithms:
 1) Processor 0 initialize A, B, X
 2) At begining, processor 0 broadcast A, B, X to all other procossors.
 3) Perform interleaving scheduling, each procossor broadcast corresponding rows to other processors, we use rowNum%procs
 4) After all row finishes elimination, we perform back substitution. 

 Notes: 

 1) At beginning, we use processor 0 to send and gather result. Speed is really slow. After changing to interleaving 
broadcast, time reduce from several minutes to several seconds.

 2) If we use static array, when N is large, there might be problems due to limited resources. When change array
 to dynamic array, problem solved. 


 Team members:
 Juan Li jll809
 Yuqing Chen yco664

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>
#include <limits.h>
#include "mpi.h"

/*#include <ulocks.h>
 #include <task.h>
 */

char *ID;

/* Program Parameters */
#define MAXN 5002  /* Max value of N */

int N;  /* Matrix size */
int procs;  /* Number of processors to use */
int myid;

/* Matrices and vectors */
float  *A, *B, *X;
/* A * X = B, solve for X */

/* junk */
#define randm() 4|2[uid]&3

/* Prototype */
void gauss();  /* The function you will provide.
                * It is this routine that is timed.
                * It is called only on the parent.
                */
/* returns a seed for srand based on the time */
unsigned int time_seed() {
    struct timeval t;
    struct timezone tzdummy;
    
    gettimeofday(&t, &tzdummy);
    return (unsigned int)(t.tv_usec);
}

void parameters(int argc, char **argv) {
    int submit = 0;  /* = 1 if submission parameters should be used */
    int seed = 0;  /* Random seed */
    // char uid[L_cuserid + 2]; /*User name */
    char uid[32];
    /* Read command-line arguments */
    //  if (argc != 3) {
    if ( argc == 1 && !strcmp(argv[1], "submit") ) {
        /* Use submission parameters */
        submit = 1;
        N = 4;
        procs = 2;
        //printf("\nSubmission run for \"%s\".\n", cuserid(uid));
        printf("\nSubmission run for \"%s\".\n", uid);
        /*uid = ID;*/
        strcpy(uid,ID);
        srand(randm());
    }
    else {
        if (argc == 2) {
            seed = atoi(argv[1]);
            srand(seed);
            if (myid == 0) printf("\nRandom seed = %i\n", seed);
        }
        else {
            if (myid == 0) printf("Usage: %s <matrix_dimension> <num_procs> [random seed]\n",
                                  argv[0]);
            printf("       %s submit\n", argv[0]);
            exit(0);
        }
    }
    //  }
    /* Interpret command-line args */
    if (!submit) {
        N = atoi(argv[1]);
        if (N < 1 || N > MAXN) {
            printf("N = %i is out of range.\n", N);
            exit(0);
        }
    }
    
    /* Print parameters */
    if (myid == 0) printf("\nMatrix dimension N = %i.\n", N);
    if (myid == 0) printf("Number of processors = %i.\n", procs);
}

/* Initialize A and B (and X to 0.0s) */
void initialize_inputs() {
    int row, col;
    
    printf("\nInitializing...\n");
    for (row = 0; row < N; row++) {
        for (col = 0; col < N; col++) {
            A[col + row*N] = (float)rand() / 32768.0;
        }
        B[row] = (float)rand() / 32768.0;
        X[row] = 0.0;
    }
    
}

/* Print input matrices */
void print_inputs() {
    int row, col;
    
    if (N < 10) {
        printf("\nA =\n\t");
        for (row = 0; row < N; row++) {
            for (col = 0; col < N; col++) {
                printf("%5.2f%s", A[row*N + col], (col < N-1) ? ", " : ";\n\t");
            }
        }
        printf("\nB = [");
        for (col = 0; col < N; col++) {
            printf("%5.2f%s", B[col], (col < N-1) ? "; " : "]\n");
        }
    }
}

void print_X() {
    int row;
    
    if (N < 10) {
        printf("\nX = [");
        for (row = 0; row < N; row++) {
            printf("%5.2f%s", X[row], (row < N-1) ? "; " : "]\n");
        }
    }
}

int main(int argc, char **argv) {
    ID = argv[argc-1];
    argc--;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &procs);
    printf("\nProcess number %d", myid);
    /* Process program parameters */
    parameters(argc, argv);
    
    //alocate memory
    A = (float*)malloc(N*N*sizeof(float));
    B = (float*)malloc(N*sizeof(float));
    X = (float*)malloc(N*sizeof(float));
    
    /* Initialize A and B */
    if (myid == 0) {
        initialize_inputs();
        
        /* Print input matrices */
        print_inputs();
    }
    /* Gaussian Elimination */
    gauss();
    /* Back substitution */
    if (myid == 0) {
        int row, col;
        for (row = N - 1; row >= 0; row--) {
            X[row] = B[row];
            for (col = N-1; col > row; col--) {
                X[row] -= A[row*N + col] * X[col];
            }
            X[row] /= A[row * N + row];
        }
        /* Display output */
        print_X();
    }
    free(A);
    free(B);
    free(X);
    MPI_Finalize();
    return 0;
}

void gauss() {
    // printf("\nBegin gauss...\n");
    int norm, row, col;  /* Normalization row, and zeroing element row and col */
    float multiplier;
    int bid,i,j; /*Broadcast processor id for each iteration*/
    /*Time Variables*/
    double startwtime = 0.0, endwtime;


    if (myid == 0) {
        printf("\nComputing Parallely Using MPI.\n");
        startwtime = MPI_Wtime();
       
       
    }

     /*Broadcast A[][] and B[] to all the processors*/
    MPI_Bcast(&A[0], N*N, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&B[0], N, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    /* Gaussian elimination */
    for (norm = 0; norm < N-1; norm++) {
        /* Broadcast A[norm] row and B[norm] from corresponding processor, which is important in this iteration*/
        bid = norm%procs;
        MPI_Bcast(&A[norm*N], N, MPI_FLOAT, bid, MPI_COMM_WORLD);
        MPI_Bcast(&B[norm], 1, MPI_FLOAT, bid, MPI_COMM_WORLD);


        /*Gaussian elimination using static interleaved scheduling where each processor gets the same rows in each iteration*/
        for (row = myid; row < N; row += procs) {
            /*Only perform gaussian elimination on rows that haven't been done*/
            if (row > norm) {
                multiplier = A[row*N + norm] / A[norm *N +norm];
                for (col = norm; col < N; col++) {
                    A[row *N + col] -= A[norm*N + col] * multiplier;
                }
                B[row] -= B[norm] * multiplier;
            }
        }

    }
    MPI_Bcast(&A[(N-1)*N], N, MPI_FLOAT, (N-1)%procs, MPI_COMM_WORLD);
    MPI_Bcast(&B[N-1], 1, MPI_FLOAT, (N-1)%procs, MPI_COMM_WORLD);
    
    MPI_Barrier(MPI_COMM_WORLD);

    
    if (myid == 0) {
        endwtime = MPI_Wtime();
        printf("\nelapsed time = %f\n", endwtime - startwtime);

    }

}

