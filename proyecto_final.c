#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "stb-master/stb_image.h"
#include "stb-master/stb_image_write.h"
#include <math.h>

void obtenerHistograma(int histograma[256], unsigned char* pixeles, int ancho, int alto, int canales){
    for (int i = 0; i < 256; i++) {
        histograma[i] = 0;
    }

    for (int i = 0; i < ancho * alto; i++) {
        histograma[pixeles[i * canales]]++;
    }
}

//CDF: Cumulative Distributive Function ; Función de distribución acumulativa
void generarCDF(int cdf[256], int histograma[256], unsigned char* pixeles, int ancho, int alto){
    cdf[0] = histograma[0];
    for (int i = 1; i < 256; i++) {
        cdf[i] = histograma[i] + cdf[i - 1];
    }
}

int encontrarCDFmin(int cdf[256]){
    int cdf_min = 0;
    for (int i = 0; i < 256; i++) {
        if (cdf[i] != 0) {
            cdf_min = cdf[i];
            break;
        }
    }
    return cdf_min;
}

//Genera el CDF Ecualizado
void generarEqCDF(int eqCdf[256], int cdf[256], int cdf_min, int ancho, int alto){
    for (int i = 0; i < 256; i++) {
        eqCdf[i] = round(((double)(cdf[i] - cdf_min) / ((ancho * alto) - cdf_min)) * 254) + 1;
    }
}

unsigned char* generarNuevaImagen(int eqCdf[256], unsigned char* pixeles, int ancho, int alto, int canales){
    unsigned char *eqImage = (unsigned char *)malloc(ancho * alto * sizeof(unsigned char));
    for (int i = 0; i < ancho * alto; i++) {
        eqImage[i] = eqCdf[pixeles[i* canales]];
    }
    return eqImage;
}

int main(int argc, char *argv[]) {

    /*Declaración de variables de tiempo*/

    //Variables de tiempo de ejecución secuencial
    double start_secuencial, end_secuencial;

    //Variables de tiempo de ejecución paralelo
    double start_paralelo, end_paralelo;

    //Variables de tiempo de carga de imagen
    double start_load, end_load;

    //Variables de tiempo de generación de imagen
    double start_generate, end_generate;

    //Variables de tiempo de generación de archivo CSV
    double start_csv, end_csv;

    /*Carga de la imagen*/

    int ancho, alto, canales;
    char *directorio = argv[1];

    start_load = omp_get_wtime();
    unsigned char *pixeles = stbi_load(directorio, &ancho, &alto, &canales, 0);
    end_load = omp_get_wtime();

    //Calculación del tiempo de carga
    double load_time = end_load - start_load;

    if (pixeles == NULL) {
        printf("Error al cargar la imagen.\n");
        return -1;
    }

    /*Fin de la carga de la imagen*/

    /*Ejecución en serie*/

    start_secuencial = omp_get_wtime();

    int histogram[256];
    obtenerHistograma(histogram, pixeles, ancho, alto, canales);

    int cdf[256];
    generarCDF(cdf, histogram, pixeles, ancho, alto);

    int cdf_min = encontrarCDFmin(cdf);

    int eqCdf[256];
    generarEqCDF(eqCdf, cdf, cdf_min, ancho, alto);

    unsigned char *eqImage = generarNuevaImagen(eqCdf, pixeles, ancho, alto, canales);

    int eqHistogram[256] ;
    obtenerHistograma(eqHistogram, eqImage, ancho, alto, 1);

    end_secuencial = omp_get_wtime();

    double tiempo_secuencial = end_secuencial - start_secuencial;

    /*Fin de ejecución en serie*/

    /*Ejecución en paralelo*/

    start_paralelo = omp_get_wtime();

    int histogram_par[256], eqHistogram_par[256], cdf_par[256], eqCdf_par[256];
    int cdf_min_par;
    unsigned char *eqImage_par = (unsigned char *)malloc(ancho * alto * sizeof(unsigned char));

    #pragma omp parallel shared(histogram_par, pixeles, eqCdf, eqImage, eqHistogram_par) num_threads(omp_get_max_threads())
    {
        //Inicialización del histograma en 0's
        #pragma omp for 
        for (int i = 0; i < 256; i++) {
            histogram_par[i] = 0;
        }

        #pragma omp barrier

        //Obtención del histograma
        #pragma omp for
        for (int i = 0; i < ancho * alto; i++) {
            #pragma omp atomic
            histogram_par[pixeles[i*canales]]++;
        }

        //Generación del CDF y obtención del mínimo del CDF
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

        //Generación del CDF ecualizado
        #pragma omp for
        for (int i = 0; i < 256; i++) {
            eqCdf_par[i] = round(((double)(cdf_par[i] - cdf_min_par) / ((ancho * alto) - cdf_min_par)) * 254) + 1;
        }

        //Generación de la nueva imagen
        #pragma omp for 
        for (int i = 0; i < ancho * alto; i++) {
            eqImage_par[i] = eqCdf_par[pixeles[i * canales]];
        }

        //Inicialización del histograma en 0's
        #pragma omp for 
        for (int i = 0; i < 256; i++) {
            eqHistogram_par[i] = 0;
        }

        //Obtención del histograma de la imagen ecualizada
        #pragma omp for
        for (int i = 0; i < ancho * alto; i++) {
            #pragma omp atomic
            eqHistogram_par[eqImage[i]]++;
        }
    }

    end_paralelo = omp_get_wtime();

    double tiempo_paralelo = end_paralelo - start_paralelo;

    /*Fin de ejecución en paralelo*/

    /*Generación de la imagen*/

    start_generate = omp_get_wtime();
    stbi_write_jpg("output/eq_ima_sec.jpg", ancho, alto, 1, eqImage, 100);
    end_generate = omp_get_wtime();
    double generate_time = end_generate - start_generate;

    /*Fin de la generación de la imagen*/

    /*Generación de los archivos csv tanto secuencial como paralelo*/

    start_csv = omp_get_wtime();
    FILE *file = fopen("histo_secuencial.csv", "w");
    if (file != NULL) {
        fprintf(file, "valor,histograma,histogramaEcualizado\n");
        for (int i = 0; i < 256; i++) {
            fprintf(file, "%d,%d,%d\n", i, histogram[i], eqHistogram[i]);
        }
        fclose(file);
    }

    stbi_write_jpg("output/eq_ima_par.jpg", ancho, alto, 1, eqImage_par, 100);
    
    FILE *file_par = fopen("histo_paralell.csv", "w");
    if (file_par != NULL) {
        fprintf(file, "valor,histograma,histogramaEcualizado\n");
        for (int i = 0; i < 256; i++) {
            fprintf(file_par, "%d,%d,%d\n", i, histogram_par[i], eqHistogram_par[i]);
        }
        fclose(file_par);
    }

    end_csv = omp_get_wtime();
    double csv_time = end_csv - start_csv;

    /*Fin de la generación de los archivos csv tanto secuencial como paralelo*/

    //Liberación de memoria
    stbi_image_free(pixeles);
    free(eqImage);
    free(eqImage_par);


    //Cálculo e impresión de las métricas del desempeño del algoritmo paralelo vs algoritmo serial
    printf("Resolución de la imagen:\n");
    printf("Ancho: %d\nAlto: %d\nCanales: %d\nTamaño: %d\n", ancho, alto, canales, ancho * alto * canales);
    
    int num_procesasdores = omp_get_max_threads();
    double overhead_time = tiempo_secuencial - tiempo_paralelo;

    double speedup = tiempo_secuencial / tiempo_paralelo;
    double eficiencia = speedup / num_procesasdores;

    printf("\nTiempos:\n");
    printf("Tiempo de ejecución en serie: %.4f segundos\n", tiempo_secuencial);
    printf("Tiempo de ejecución en paralelo: %.4f segundos\n", tiempo_paralelo);
    printf("Número de procesadores: %d\n", num_procesasdores);
    printf("SpeedUp: %.4f\n", speedup);
    printf("Eficiencia: %.4f\n", eficiencia);
    printf("Tiempo de Overhead: %.4f segundos\n", overhead_time);
    printf("Tiempo de carga de imagen: %.4f segundos\n", load_time);
    printf("Tiempo de generación de imagen: %.4f segundos\n", generate_time);
    printf("Tiempo de generación de archivos CSV: %.4f segundos\n", csv_time);

    return 0;
}