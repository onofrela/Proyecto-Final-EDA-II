#include<stdio.h>
#include<stdlib.h>
#include<omp.h>

#define L 256
#define prints 1
int **RandomImage(int ren, int col);
int *Histograma_sec(int **image, int ren, int col);
int* Histograma_par(int **imagen, int ren, int col);

int main(){
    int ren = 5;
    int col = 5;
    int **imagen = RandomImage(ren, col);
    int *histograma = Histograma_sec(imagen, ren, col);
    histograma = Histograma_par(imagen, ren, col);
    /*
    for(i=0; i< NG; i++)
        histo[i] = 0;
    #pragma omp parallel private(histop) num_threads(2)
    {
        for(i=0; i>NG; i++)
            histop[i]=0;
        #pragma omp for private(j)
        for(i=0; i<N; i++)
            for(j=0; j<N; j++)
                histop[IMA[i][j]]++;
        #pragma omp critical
        {
            for(i=0; i<NG; i++)
                histo[i]+=histop[i];
        }
        
    }
    */
   free(imagen);
}

int* Histograma_sec(int **imagen, int ren, int col){
    int histograma[L];

    for(size_t i=0; i<L; i++){
        histograma[i]=0;
    }
    for(size_t i=0; i<ren; i++){
        for(size_t j=0; j<col; j++){
            histograma[imagen[i][j]]++;
        }
    }
    for(size_t i=0; i<L; i++){
        printf("%d: %d\n",i, histograma[i]);
    }
    printf("\n");
    return histograma;
}

int* Histograma_par(int **imagen, int ren, int col){
    int histograma[L];

    #pragma omp parallel for
    for(size_t i=0; i<L; i++){
        histograma[i]=0;
    }

    #pragma omp parallel for reduction(+:histograma) 
    for(size_t i=0; i<ren; i++){
        for(size_t j=0; j<col; j++){
            histograma[imagen[i][j]]++;
        }
    }

    for(size_t i=0; i<L; i++){
        printf("%d: %d\n",i, histograma[i]);
    }
    printf("\n");
    return histograma;
}

int **RandomImage(int ren, int col){
    int **imagen = (int **)malloc(sizeof(int *)*ren);
    for(size_t i = 0; i < ren; i++){
        imagen[i]= (int *)malloc(col*sizeof(int));
    }
    for(size_t i=0;i<ren;i++){
        for(size_t j=0;j<col;j++){
            imagen[i][j]=rand()%L;
            if(prints)
                printf("%d\t", imagen[i][j]);
        }
        if(prints)
            printf("\n");
    }
    if(prints)
        printf("\n");
    return imagen;
}