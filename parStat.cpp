#include <stdio.h>
#include <time.h>
#include <mpi.h>

#define WIDTH 640
#define HEIGHT 480
#define MAX_ITER 255

struct complex {
    double real;
    double imag;
    int col;
    int x;
    int y;
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

    int MSG_TAG = 0;

    MPI_Init(NULL, NULL);

    // Get the number of processes and rank
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Create MPI datatype for struct complex
    MPI_Datatype mpi_complex;
    int block_lengths[5] = {1, 1, 1, 1, 1};
    MPI_Aint displacements[5];
    MPI_Datatype types[5] = {MPI_DOUBLE, MPI_DOUBLE, MPI_INT, MPI_INT, MPI_INT};

    MPI_Get_address(&c.real, &displacements[0]);
    MPI_Get_address(&c.imag, &displacements[1]);
    MPI_Get_address(&c.col, &displacements[2]);
    MPI_Get_address(&c.x, &displacements[3]);
    MPI_Get_address(&c.y, &displacements[4]);

    MPI_Aint base = displacements[0];
    for (int i = 0; i < 5; i++) {
        displacements[i] -= base;
    }

    MPI_Type_create_struct(5, block_lengths, displacements, types, &mpi_complex);
    MPI_Type_commit(&mpi_complex);

    int i, row , j , step = (HEIGHT / (world_size-1));
    printf("Hello from rank %d \n", world_rank);
    if (world_rank == 0) {
        // Start measuring time
        clock_t start_time = clock();
        for (i = 1, row = 0 ; i < world_size; i++, row += step) {
            MPI_Send(&row, 1, MPI_INT, i, MSG_TAG, MPI_COMM_WORLD);
        }
        for (i = 0; i < HEIGHT * WIDTH; ++i) {
            MPI_Recv(&c, 1, mpi_complex, MPI_ANY_SOURCE, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            image[c.y][c.x] = c.col;
        }

        // End measuring time
        clock_t end_time = clock();
        total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        printf("Execution time of trial : %f ms \n", total_time*1000);
        save_pgm("mandelbrotParStat.pgm", image);
    } else {
        MPI_Recv(&row, 1, MPI_INT, 0, MSG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        int col;
        for (i = row; i < row+step; i++) {
            for (j = 0; j < WIDTH; j++) {
                c.real = (j - WIDTH / 2.0) * 4.0 / WIDTH;
                c.imag = (i - HEIGHT / 2.0) * 4.0 / HEIGHT;
                c.x = j;
                c.y = i;
                col = cal_pixel(c);
                c.col = col;
                MPI_Send(&c, 1, mpi_complex, 0, MSG_TAG, MPI_COMM_WORLD);
            }
        }
    }

    MPI_Type_free(&mpi_complex);
    MPI_Finalize();
    return 0;
}
