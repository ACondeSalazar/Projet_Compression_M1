#include <vector>
#include <math.h>


//implémentation naive O((nm)^2) de la DCT trouvé ici : https://www.geeksforgeeks.org/discrete-cosine-transform-algorithm-program/
int dctTransform(std::vector<std::vector<int>> & matrix, std::vector<std::vector<float>> & dct)
{
    int i, j, k, l;

    double pi = 3.14159265358979323846;
    
    int m = matrix.size();
    int n = matrix[0].size();

    m = 8;
    n = 8;
 
    float ci, cj, dct1, sum;
 
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
 
            // ci and cj depends on frequency as well as
            // number of row and columns of specified matrix
            if (i == 0)
                ci = 1 / sqrt(m);
            else
                ci = sqrt(2) / sqrt(m);
            if (j == 0)
                cj = 1 / sqrt(n);
            else
                cj = sqrt(2) / sqrt(n);
 
            // sum will temporarily store the sum of 
            // cosine signals
            sum = 0;
            for (k = 0; k < m; k++) {
                for (l = 0; l < n; l++) {
                    dct1 = (float) matrix[k][l] * 
                           cos((2.0 * k + 1) * i * pi / (2.0 * m)) * 
                           cos((2.0 * l + 1) * j * pi / (2.0 * n));
                    sum = sum + dct1;
                }
            }
            dct[i][j] = ci * cj * sum;
        }
    }
 
    /* for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            printf("%f\t", dct[i][j]);
        }
        printf("\n");
    } */
}