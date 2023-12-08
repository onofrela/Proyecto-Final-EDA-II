#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "stb-master/stb_image.h"
#include "stb-master/stb_image_write.h"
#include <math.h>

#define CHANNELS 3

int main(int argc, char *argv[]) {
    double start_secuencial, end_secuencial, start_load, end_load, start_generate, end_generate, start_csv, end_csv, start_paralelo, end_paralelo;

    int ancho, alto, canales;
    char *directorio = argv[1];

    start_load = omp_get_wtime();
    unsigned char *pixeles = stbi_load(directorio, &ancho, &alto, &canales, CHANNELS);
    end_load = omp_get_wtime();
    double load_time = end_load - start_load;

    start_secuencial = omp_get_wtime();

    int histogram[256] = {0};
    for (int i = 0; i < ancho * alto; i++) {
        histogram[pixeles[i * CHANNELS]]++;
    }

    int cdf[256];
    cdf[0] = histogram[0];
    for (int i = 1; i < 256; i++) {
        cdf[i] = histogram[i] + cdf[i - 1];
    }

    int cdf_min = 0;
    for (int i = 0; i < 256; i++) {
        if (cdf[i] != 0) {
            cdf_min = cdf[i];
            break;
        }
    }

    int eqCdf[256];
    for (int i = 0; i < 256; i++) {
        eqCdf[i] = round(((double)(cdf[i] - cdf_min) / ((ancho * alto) - cdf_min)) * 254) + 1;
    }

    unsigned char *eqImage = (unsigned char *)malloc(ancho * alto * sizeof(unsigned char));
    for (int i = 0; i < ancho * alto; i++) {
        eqImage[i] = eqCdf[pixeles[i * CHANNELS]];
    }

    int eqHistogram[256] = {0};
    for (int i = 0; i < ancho * alto; i++) {
        eqHistogram[eqImage[i]]++;
    }

    end_secuencial = omp_get_wtime();
    double secuencial_time = end_secuencial - start_secuencial;

    start_paralelo = omp_get_wtime();

    int histogram_par[256] = {0}, eqHistogram_par[256] = {0}, cdf_par[256] = {0}, eqCdf_par[256] = {0};
    int cdf_min_par = 0;
    unsigned char *eqImage_par = (unsigned char *)malloc(ancho * alto * sizeof(unsigned char));

    #pragma omp parallel shared(histogram_par, pixeles, eqCdf, eqImage, eqHistogram_par) num_threads(omp_get_max_threads())
    {
        #pragma omp for 
        for (int i = 0; i < 256; i++) {
            histogram_par[i] = 0;
        }

        #pragma omp barrier

        #pragma omp for
        for (int i = 0; i < ancho * alto; i++) {
            #pragma omp atomic
            histogram_par[pixeles[i * CHANNELS]]++;
        }

        #pragma omp single
        {
            cdf_par[0] = histogram_par[0];
            for (int i = 1; i < 256; i++) {
                cdf_par[i] = histogram_par[i] + cdf_par[i - 1];
            }

            for (int i = 0; i < 256; i++) {
                if (cdf_par[i] != 0) {
                    cdf_min_par = cdf_par[i];
                    break;
                }
            }
        }

        #pragma omp for
        for (int i = 0; i < 256; i++) {
            eqCdf_par[i] = round(((double)(cdf_par[i] - cdf_min_par) / ((ancho * alto) - cdf_min_par)) * 254) + 1;
        }

        #pragma omp for 
        for (int i = 0; i < ancho * alto; i++) {
            eqImage_par[i] = eqCdf_par[pixeles[i * CHANNELS]];
        }

        #pragma omp for 
        for (int i = 0; i < 256; i++) {
            eqHistogram_par[i] = 0;
        }

        #pragma omp for
        for (int i = 0; i < ancho * alto; i++) {
            #pragma omp atomic
            eqHistogram_par[eqImage[i]]++;
        }
    }

    end_paralelo = omp_get_wtime();
    double tiempo_paralelo = end_paralelo - start_paralelo;

    start_generate = omp_get_wtime();
    stbi_write_jpg("output/eq_ima_sec.jpg", ancho, alto, 1, eqImage, 100);
    end_generate = omp_get_wtime();
    double generate_time = end_generate - start_generate;

    start_csv = omp_get_wtime();
    FILE *file = fopen("histo_secuencial.csv", "w");
    if (file != NULL) {
        fprintf(file, "valor,histo,eqHisto\n");
        for (int i = 0; i < 256; i++) {
            fprintf(file, "%d,%d,%d\n", i, histogram[i], eqHistogram[i]);
        }
        fclose(file);
    }
    end_csv = omp_get_wtime();
    double csv_time = end_csv - start_csv;

    stbi_write_jpg("output/eq_ima_par.jpg", ancho, alto, 1, eqImage_par, 100);
    
    FILE *file_par = fopen("histo_paralell.csv", "w");
    if (file_par != NULL) {
        fprintf(file_par, "valor,histo,eqHisto\n");
        for (int i = 0; i < 256; i++) {
            fprintf(file_par, "%d,%d,%d\n", i, histogram_par[i], eqHistogram_par[i]);
        }
        fclose(file_par);
    }

    stbi_image_free(pixeles);
    free(eqImage);
    free(eqImage_par);

    printf("Resolución de la imagen:\n");
    printf("Ancho: %d\nAlto: %d\nCanales: %d\nTamaño: %d\n", ancho, alto, canales, ancho * alto * canales);
    
    int num_processors = omp_get_max_threads();
    double overhead_time = secuencial_time - tiempo_paralelo;

    double speedup = secuencial_time / tiempo_paralelo;
    double efficiency = speedup / num_processors;

    printf("\nTiempos:\n");
    printf("Tiempo de ejecución en serie: %.4f segundos\n", secuencial_time);
    printf("Tiempo de ejecución en paralelo: %.4f segundos\n", tiempo_paralelo);
    printf("Número de procesadores: %d\n", num_processors);
    printf("SpeedUp: %.4f\n", speedup);
    printf("Eficiencia: %.4f\n", efficiency);
    printf("Tiempo de Overhead: %.4f segundos\n", overhead_time);
    printf("Tiempo de carga de imagen: %.4f segundos\n", load_time);
    printf("Tiempo de generación de imagen: %.4f segundos\n", generate_time);
    printf("Tiempo de generación de archivos CSV: %.4f segundos\n", csv_time);

    return 0;
}