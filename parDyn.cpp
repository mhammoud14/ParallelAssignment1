#include <stdio.h>
#include <time.h>
#include <mpi.h>

#define WIDTH 640
#define HEIGHT 480
#define MAX_ITER 255

struct complex {
    double real;
    double imag;
};

int cal_pixel(struct complex c) {
    double z_real = 0;
    double z_imag = 0;
    double z_real2, z_imag2, lengthsq;
    int iter = 0;
    do {
        z_real2 = z_real * z_real;
        z_imag2 = z_imag * z_imag;
        z_imag = 2 * z_real * z_imag + c.imag;
        z_real = z_real2 - z_imag2 + c.real;
        lengthsq = z_real2 + z_imag2;
        iter++;
    } while ((iter < MAX_ITER) && (lengthsq < 4.0));
    return iter;
}

void save_pgm(const char *filename, int image[HEIGHT][WIDTH]) {
    FILE *pgmimg;
    int temp;
    pgmimg = fopen(filename, "wb");
    fprintf(pgmimg, "P2\n"); // Writing Magic Number to the File
    fprintf(pgmimg, "%d %d\n", WIDTH, HEIGHT); // Writing Width and Height
    fprintf(pgmimg, "255\n");                   // Writing the maximum gray value
    int count = 0;

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            temp = image[i][j];
            fprintf(pgmimg, "%d ", temp); // Writing the gray values in the 2D array to the file
        }
        fprintf(pgmimg, "\n");
    }
    fclose(pgmimg);
}

int main() {
    int image[HEIGHT][WIDTH];
    double total_time;
    struct complex c;

    int DATA_TAG = 0  ,RESULT_TAG = 0 ,SRC_TAG =0 ,TER_TAG = 1;

    MPI_Init(NULL, NULL);

    // Get the number of processes and rank
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);


    int colRow [WIDTH +2];
    int i, row , j , slave , count, r ,ter = -1;
    printf("Hello from rank %d \n", world_rank);
    if (world_rank == 0) {
        // Start measuring time
        clock_t start_time = clock();
        count = 0;
        row = 0;

        for (i = 1, row = 0; i < world_size; i++, row ++) {
            MPI_Send(&row, 1, MPI_INT, i, DATA_TAG, MPI_COMM_WORLD);
            count ++;
            row++;
        }

        do{
            MPI_Recv(&colRow,sizeof(int) * (WIDTH +2), MPI_INT, MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            slave = colRow[WIDTH];
            r = colRow[WIDTH+1];

            count --;
            if(row < HEIGHT){
                MPI_Send(&row, 1, MPI_INT, slave, DATA_TAG, MPI_COMM_WORLD);
                row++;
                count++;
            }
            else{

                MPI_Send(&ter, 1, MPI_INT, slave, TER_TAG, MPI_COMM_WORLD);
            }
            for (int k = 0; k < WIDTH; ++k) {
                image[r][k] = colRow[k];
            }
        } while (count > 0);

        // End measuring time
        clock_t end_time = clock();
        total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        printf("Execution time of trial : %f ms \n", total_time*1000);
        save_pgm("mandelbrotParDyn.pgm", image);
    } else {
        MPI_Recv(&row, 1, MPI_INT, 0, SRC_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        while (row != -1){
            c.imag = (row - HEIGHT / 2.0) * 4.0 / HEIGHT;
            for (int k = 0; k < WIDTH; ++k) {
                c.real = (k - WIDTH / 2.0) * 4.0 / WIDTH;
                colRow[k] = cal_pixel(c);
            }
            colRow[WIDTH] = world_rank;
            colRow[WIDTH+1] = row;
            MPI_Send(&colRow,sizeof(int) * (WIDTH +2) , MPI_INT, 0, RESULT_TAG, MPI_COMM_WORLD);
            MPI_Recv(&row, 1, MPI_INT, 0, SRC_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
    MPI_Finalize();
    return 0;
}
