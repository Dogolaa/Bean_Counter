#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Definição da estrutura de dados para representar uma imagem PGM
typedef struct {
    int width, height, max_val;
    int **pixels;
} PGM;

// Função para ler uma linha válida de um arquivo, ignorando comentários
char *readValidLine(FILE *file) {
    static char line[1024];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] != '#' && strlen(line) > 1) { // Ignorar linhas que começam com '#' e linhas vazias
            return line;
        }
    }
    return NULL;
}

// Função para ler uma imagem PGM de um arquivo
PGM readPGM(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        exit(1);
    }

    PGM img;
    char line[1024];

    // Ler o formato do arquivo (P2)
    if (!fgets(line, sizeof(line), file)) {
        perror("Failed to read file format");
        exit(1);
    }
    if (strcmp(line, "P2\n") != 0) {
        fprintf(stderr, "Unsupported file format: %s\n", line);
        exit(1);
    }

    // Ler a largura e altura da imagem
    strcpy(line, readValidLine(file));
    sscanf(line, "%d %d", &img.width, &img.height);

    // Ler o valor máximo de intensidade de pixel
    strcpy(line, readValidLine(file));
    sscanf(line, "%d", &img.max_val);

    // Alocar memória para os pixels da imagem
    img.pixels = malloc(img.height * sizeof(int *));
    for (int i = 0; i < img.height; ++i) {
        img.pixels[i] = malloc(img.width * sizeof(int));
    }

    // Ler os pixels da imagem
    for (int i = 0; i < img.height; ++i) {
        for (int j = 0; j < img.width; ++j) {
            if (fscanf(file, "%d", &img.pixels[i][j]) != 1) {
                perror("Failed to read pixel data");
                exit(1);
            }
        }
    }

    fclose(file);
    return img;
}

// Função para escrever uma imagem PGM em um arquivo
void writePGM(const char *filename, const PGM img) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing: %s\n", filename);
        return;
    }
    fprintf(file, "P2\n%d %d\n%d\n", img.width, img.height, img.max_val); // Escrever cabeçalho do arquivo PGM
    for (int i = 0; i < img.height; ++i) {
        for (int j = 0; j < img.width; ++j) {
            fprintf(file, "%d ", img.pixels[i][j]); // Escrever pixels da imagem
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

// Função para aplicar o algoritmo de limiarização de Sauvola
void applySauvolaThreshold(PGM *img, int windowSize, double k, double R) {
    // Alocar memória para os pixels resultantes
    int **resultPixels = malloc(img->height * sizeof(int *));
    for (int i = 0; i < img->height; ++i) {
        resultPixels[i] = malloc(img->width * sizeof(int));
    }

    // Percorrer cada pixel da imagem
    for (int i = 0; i < img->height; ++i) {
        for (int j = 0; j < img->width; ++j) {
            double sum = 0, sumSq = 0, num = 0;
            // Definir os limites da janela deslizante
            int minI = fmax(0, i - windowSize);
            int maxI = fmin(img->height - 1, i + windowSize);
            int minJ = fmax(0, j - windowSize);
            int maxJ = fmin(img->width - 1, j + windowSize);

            // Calcular a soma e soma dos quadrados dos pixels na janela
            for (int ii = minI; ii <= maxI; ++ii) {
                for (int jj = minJ; jj <= maxJ; ++jj) {
                    int pixel = img->pixels[ii][jj];
                    sum += pixel;
                    sumSq += pixel * pixel;
                    num++;
                }
            }

            // Calcular a média e o desvio padrão dos pixels na janela
            double mean = sum / num;
            double stdDev = sqrt(sumSq / num - mean * mean);
            // Calcular o limiar de Sauvola
            double threshold = mean * (1 + k * ((stdDev / R) - 1));

            // Aplicar o limiar aos pixels resultantes
            resultPixels[i][j] = img->pixels[i][j] < threshold ? 0 : 255;
        }
    }

    // Liberar memória dos pixels originais
    for (int i = 0; i < img->height; ++i) {
        free(img->pixels[i]);
    }
    free(img->pixels);
    img->pixels = resultPixels; // Atualizar a imagem com os pixels resultantes
    writePGM("sauvola_thresholded.pgm", *img); // Escrever a imagem resultante em um arquivo
}

// Função para aplicar o algoritmo de preenchimento por inundação (flood fill)
int floodFill(PGM *img, int x, int y, int label, int **labels) {
    int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int stack_size = img->height * img->width;
    int stack_ptr = 0;
    // Alocar memória para a pilha de preenchimento
    int *stack = malloc(2 * stack_size * sizeof(int));

    // Adicionar a posição inicial à pilha
    stack[stack_ptr++] = x;
    stack[stack_ptr++] = y;
    labels[x][y] = label;

    // Loop enquanto a pilha não estiver vazia
    while (stack_ptr > 0) {
        y = stack[--stack_ptr];
        x = stack[--stack_ptr];

        // Percorrer todas as direções vizinhas
        for (int i = 0; i < 8; ++i) {
            int nx = x + dx[i], ny = y + dy[i];
            // Verificar se o pixel vizinho está dentro da imagem e não foi rotulado
            if (nx >= 0 && nx < img->height && ny >= 0 && ny < img->width && img->pixels[nx][ny] == 0 && labels[nx][ny] == 0) {
                labels[nx][ny] = label; // Atribuir o rótulo ao pixel vizinho
                stack[stack_ptr++] = nx; // Adicionar o pixel vizinho à pilha
                stack[stack_ptr++] = ny;
            }
        }
    }

    free(stack); // Liberar memória da pilha
    return label;
}

// Função para rotular os componentes conectados na imagem
int labelComponents(PGM *img) {
    int label = 1;
    // Alocar memória para a matriz de rótulos
    int **labels = malloc(img->height * sizeof(int *));
    for (int i = 0; i < img->height; ++i) {
        labels[i] = calloc(img->width, sizeof(int));
    }

    // Percorrer cada pixel da imagem
    for (int i = 0; i < img->height; ++i) {
        for (int j = 0; j < img->width; ++j) {
            // Se o pixel é parte de um componente e ainda não foi rotulado
            if (img->pixels[i][j] == 0 && labels[i][j] == 0) {
                floodFill(img, i, j, label++, labels); // Aplicar flood fill para rotular o componente
            }
        }
    }

    // Liberar memória das labels
    for (int i = 0; i < img->height; ++i) {
        free(labels[i]);
    }
    free(labels);

    return label - 1; // Retornar o número de componentes rotulados
}

// Função para aplicar o algoritmo de watershed na imagem
void applyWatershed(PGM *img) {
    int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    // Alocar memória para os marcadores
    int **markers = malloc(img->height * sizeof(int *));
    for (int i = 0; i < img->height; ++i) {
        markers[i] = calloc(img->width, sizeof(int));
    }

    // Passo 1: Inicializar os marcadores
    int current_label = 1;
    for (int i = 1; i < img->height - 1; ++i) {
        for (int j = 1; j < img->width - 1; ++j) {
            if (img->pixels[i][j] == 0) { // Se o pixel faz parte de um objeto
                int isBorder = 0;
                // Verificar se é um pixel de borda (vizinho a um pixel de fundo)
                for (int k = 0; k < 8; ++k) {
                    int ni = i + dx[k];
                    int nj = j + dy[k];
                    if (img->pixels[ni][nj] == 255) {
                        isBorder = 1;
                        break;
                    }
                }
                if (isBorder) {
                    markers[i][j] = current_label++; // Atribuir um novo marcador
                }
            }
        }
    }

    // Passo 2: Propagar os marcadores
    int changes;
    do {
        changes = 0;
        for (int i = 1; i < img->height - 1; ++i) {
            for (int j = 1; j < img->width - 1; ++j) {
                if (img->pixels[i][j] == 0 && markers[i][j] == 0) { // Se o pixel é parte de um objeto e não tem marcador
                    int min_label = current_label;
                    // Encontrar o marcador mínimo entre os vizinhos
                    for (int k = 0; k < 8; ++k) {
                        int ni = i + dx[k];
                        int nj = j + dy[k];
                        if (markers[ni][nj] > 0 && markers[ni][nj] < min_label) {
                            min_label = markers[ni][nj];
                        }
                    }
                    if (min_label < current_label) {
                        markers[i][j] = min_label; // Atribuir o marcador mínimo ao pixel
                        changes++;
                    }
                }
            }
        }
    } while (changes > 0); // Repetir até não haver mais mudanças

    // Substituir os valores dos pixels pelos marcadores
    for (int i = 0; i < img->height; ++i) {
        for (int j = 0; j < img->width; ++j) {
            if (markers[i][j] > 0) {
                img->pixels[i][j] = markers[i][j];
            }
        }
    }

    // Liberar memória
    for (int i = 0; i < img->height; ++i) {
        free(markers[i]);
    }
    free(markers);

    writePGM("watershed.pgm", *img); // Escrever a imagem resultante em um arquivo
}

// Função principal
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <image.pgm>\n", argv[0]);
        return 1;
    }

    // Ler a imagem PGM do arquivo fornecido
    PGM img = readPGM(argv[1]);
    int windowSize = 17; // Tamanho da janela pode ser ajustado
    double k = 0.920;    // Fator de sensibilidade
    double R = 128.0;    // Valor de R pode ser ajustado

    // Aplicar algoritmo de watershed na imagem
    applyWatershed(&img);

    // Aplicar limiarização de Sauvola na imagem
    applySauvolaThreshold(&img, windowSize, k, R);

    // Contar os componentes conectados na imagem
    int components = labelComponents(&img);
    printf("#components= %d\n", components);

    // Liberar memória dos pixels da imagem
    for (int i = 0; i < img.height; ++i) {
        free(img.pixels[i]);
    }
    free(img.pixels);

    return 0;
}
