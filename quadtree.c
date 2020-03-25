#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define MAX_VAL_COLOR 255

typedef struct RGB {
    unsigned char red, green, blue;
} Pixel;

typedef struct {
    unsigned int width, height;
    Pixel **data;
} Image;

typedef struct QuadtreeNode {
    unsigned char blue, green, red;
    uint32_t area;
    int32_t top_left, top_right;
    int32_t bottom_left, bottom_right;
} __attribute__ ((packed)) QuadtreeNode;

typedef struct NodeArray {
    QuadtreeNode **data; //informatia utila din nod
    unsigned int capacity; //capacitatea vectorului
    unsigned int length; //lungimea
} NodeArray;

typedef struct CompressedData {
    uint32_t *numar_culori;
    uint32_t *numar_noduri;
    QuadtreeNode **vector;
} CompressedData;

NodeArray *initNodeArray() {
    NodeArray *array = (NodeArray*)malloc(sizeof(NodeArray));
    array->data = (QuadtreeNode**)malloc(sizeof(QuadtreeNode*));
    array->capacity = 1;
    array->length = 0;

    return array;
}

void freeNodeArray(NodeArray *nodeArray) {
    int i = 0;
    for (i = 0; i < nodeArray->length; i++ ) {
        free(nodeArray->data[i]);
    }
    free(nodeArray->data);
    free(nodeArray);
}

/*functie pentru initializarea matricei de pixeli*/
Image *initImage(QuadtreeNode *root) {
    uint32_t size = (uint32_t)sqrt(root->area);
    Image *img = (Image*)malloc(sizeof(Image));
    img->width = img->height = size;
    img->data = (Pixel**)malloc(size * sizeof(Pixel*));

    int i = 0;
    for(i = 0; i < size; i++) {
        img->data[i] = (Pixel*)malloc(size * sizeof(Pixel));
    }

    return img;
}

void freeImage(Image *image) {
    int i = 0;
    for(i = 0; i < image->width; i++) {
        free(image->data[i]);
    }
    free(image->data);
    free(image);
}

/* functia adauga un nod din arbore in vectorul de structuri*/
void addNode(NodeArray *array, QuadtreeNode *node) {
    if(array->capacity == array->length) {
        array->data = (QuadtreeNode**)realloc(array->data, 2 * array->capacity * sizeof(QuadtreeNode*));
        array->capacity *= 2;
    }
    array->data[array->length] = node;
    array->length++;
}

/* functia citeste dintr-un fisier PPM (binar) si returneaza o structura
 * de date de tipul Image in care este salvata dimensiunea imaginii si pixelii*/
Image *readFile_PPM(char *filename) {
    char buffer[100];
    int RGB_shade;
    Image *img;
    FILE *file;

    file = fopen(filename, "rb");
    if (!file) {
        printf("file '%s' cannot be accessed\n", filename);
        exit(1);
    }

    fgets(buffer, sizeof(buffer), file);
    //verifica daca tipul imaginii este P6
    if (buffer[0] != 'P' || buffer[1] != '6') {
        printf("Image must have type 'P6')\n");
        exit(1);
    }

    img = (Image*)malloc(sizeof(Image));
    if (!img) {
        printf("Error allocating memory\n");
        exit(1);
    }

    if (fscanf(file, "%d %d", &img->width, &img->height) != 2) {
        printf("Invalid image size\n");
        exit(1);
    }

    if (fscanf(file, "%d", &RGB_shade) != 1) {
        printf("Invalid rgb component\n");
        exit(1);
    }

    if (RGB_shade != MAX_VAL_COLOR) {
        printf("Maximum value of a color must be '255'\n");
        exit(1);
    }

    while (fgetc(file) != '\n');

    //citirea imaginii propriu-zise
    Pixel *pixelArray = (Pixel*)malloc(img->width * img->height * sizeof(Pixel));
    if (fread(pixelArray, 3 * img->width, img->height, file) != img->height) {
        printf("Error loading image\n");
        exit(1);
    }

    //alocare pixel data
    img->data = (Pixel**)malloc(img->height * sizeof(Pixel*));
    int i = 0;
    for(i = 0; i < img->height; i++) {
        img->data[i] = pixelArray + img->width * i;
    }

    fclose(file);
    return img;
}

/* functia citeste din fisierul comprimat (fisierul binar) numarul de culori ale imaginii,
 * numarul total de noduri si informatia utila din ele
 * returneaza o structura de tipul CompressedData unde sunt salvate informatiile citite*/
CompressedData *readFile_compressed(char *filename) {
    CompressedData *compressedData = (CompressedData*)malloc(sizeof(CompressedData));
    compressedData->numar_culori = (uint32_t*)malloc(sizeof(uint32_t));
    compressedData->numar_noduri = (uint32_t*)malloc(sizeof(uint32_t));

    FILE *file = fopen(filename, "rb");
    fread(compressedData->numar_culori, sizeof(uint32_t), 1, file);
    fread(compressedData->numar_noduri, sizeof(uint32_t), 1, file);

    compressedData->vector = (QuadtreeNode**)malloc(*(compressedData->numar_noduri) * sizeof(QuadtreeNode*));
    int i = 0;
    for (i = 0; i < *(compressedData->numar_noduri); i++) {
        compressedData->vector[i] = (QuadtreeNode*)malloc(sizeof(QuadtreeNode));
        fread(compressedData->vector[i], sizeof(QuadtreeNode), 1, file);
    }

    fclose(file);
    return compressedData;
}

/* functia scrie intr-un fisier binar numarul de culori ale imaginii, numarul total de noduri
 * si informatia utila din ele*/
void writeFile_compressed(char *filename, CompressedData *compressedData) {
    FILE *file;
    file = fopen(filename, "wb");
    if (!file) {
        printf("file '%s' cannot be accessed\n", filename);
        exit(1);
    }
    fwrite(compressedData->numar_culori, sizeof(uint32_t), 1, file);
    fwrite(compressedData->numar_noduri, sizeof(uint32_t), 1, file);
    int i = 0;
    for (i = 0; i < *(compressedData->numar_noduri); i++) {
        fwrite(compressedData->vector[i], sizeof(QuadtreeNode), 1, file);
    }

    fclose(file);
}

/*functia creeaza un fisier de tipul PPM*/
void writeFile_PPM(char *filename, Image *image) {
    FILE *file;
    file = fopen(filename, "wb");
    if (!file) {
        printf("file '%s' cannot be accessed\n", filename);
        exit(1);
    }

    fprintf(file, "P6\n");
    fprintf(file, "%d %d\n", image->width, image->height);
    fprintf(file, "%d\n", MAX_VAL_COLOR);

    //imaginea propriu-zisa
    int i = 0;
    for(i = 0; i < image->height; i++) {
        fwrite(image->data[i], 3 * image->width, 1, file);
    }
    fclose(file);
}

/* functia primeste ca parametri o matrice de pixeli, ofset pentru linie si coloana, dimensiunea matricii
 * si realizeaza media pixelilor RGB
 * returneaza o structura in care sunt salvate mediile red, green, blue*/
Pixel *averageRGB(Pixel **imageData, int offsetX, int offsetY, int size) {
    long long sumRed = 0, sumGreen = 0, sumBlue = 0;
    int i = 0, j = 0;

    for(i = offsetY; i < offsetY + size; i++) {
        for(j = offsetX; j < offsetX + size; j++) {
            sumRed += imageData[i][j].red;
            sumGreen += imageData[i][j].green;
            sumBlue += imageData[i][j].blue;
        }
    }
    Pixel* average = (Pixel*)malloc(sizeof(Pixel));
    average->red = (unsigned char)(sumRed / (size*size));
    average->green = (unsigned char)(sumGreen / (size*size));
    average->blue = (unsigned char)(sumBlue / (size*size));

    return average;
}

/* functia calculeaza un grad al similaritatii conform formulei prezentate in cerinta*/
long long similarityRGB(Pixel **imageData, int offsetX, int offsetY, int size, Pixel *average) {
    long long mean = 0;
    int i = 0, j = 0;
    long long red = average->red;
    long long green = average->green;
    long long blue = average->blue;

    for(i = offsetY; i < offsetY + size; i++) {
        for(j = offsetX; j < offsetX + size; j++) {
            mean += pow((red - imageData[i][j].red), 2) + pow((green - imageData[i][j].green), 2) + pow((blue - imageData[i][j].blue), 2);
        }
    }
    mean = mean / (3 * size * size);

    return mean;
}

/*functia imparte recursiv imaginea patratica in sferturi pana cand gradul similaritatii este mai mic decat un prag dat*/
void divideData(Pixel **imageData, int offsetX, int offsetY, unsigned int size, long limit, QuadtreeNode *root, NodeArray *nodeArray, unsigned int *leafNumber) {
    Pixel *average = averageRGB(imageData, offsetX, offsetY, size); //media fiecarui canal RGB din blocul curent
    long long mean = similarityRGB(imageData, offsetX, offsetY, size, average); //gradul similaritatii blocului curent
    root->red = average->red; //adaug in nodul curent din arbore culoarea medie a blocului pentru canalul RED
    root->blue = average->blue;
    root->green = average->green;
    root->area = size * size; //numarul de pixeli din bloc
    free(average);

    if(mean > limit && size >= 2) {
        QuadtreeNode *topLeftNode = (QuadtreeNode*)malloc(sizeof(QuadtreeNode));
        QuadtreeNode *topRightNode = (QuadtreeNode*)malloc(sizeof(QuadtreeNode));
        QuadtreeNode *bottomRightNode = (QuadtreeNode*)malloc(sizeof(QuadtreeNode));
        QuadtreeNode *bottomLeftNode = (QuadtreeNode*)malloc(sizeof(QuadtreeNode));

        /*adaug in vectorul de structuri un nod din arbore si apelez recursivitatea pentru acest nod*/
        addNode(nodeArray, topLeftNode);
        root->top_left = nodeArray->length-1;
        divideData(imageData, offsetX, offsetY, size / 2, limit, topLeftNode, nodeArray, leafNumber);

        addNode(nodeArray, topRightNode);
        root->top_right = nodeArray->length-1;
        divideData(imageData, offsetX + size / 2, offsetY, size / 2, limit, topRightNode, nodeArray, leafNumber);

        addNode(nodeArray, bottomRightNode);
        root->bottom_right = nodeArray->length-1;
        divideData(imageData, offsetX + size / 2, offsetY + size / 2, size / 2, limit, bottomRightNode, nodeArray, leafNumber);

        addNode(nodeArray, bottomLeftNode);
        root->bottom_left = nodeArray->length-1;
        divideData(imageData, offsetX, offsetY + size / 2, size / 2, limit, bottomLeftNode, nodeArray, leafNumber);
    }
    else { //frunze
        root->top_left = -1;
        root->top_right = -1;
        root->bottom_right = -1;
        root->bottom_left = -1;
        *leafNumber += 1; //adun numarul de frunze
    }
}

/*functia creeaza o matrice de pixeli pe baza informatiilor din vectorul de structuri*/
void collectData(Pixel **imageData, int offsetX, int offsetY, int rootIndex, NodeArray *nodeArray) {
    QuadtreeNode *root = nodeArray->data[rootIndex];
    uint32_t size = (uint32_t)sqrt(root->area);
    int i = 0, j = 0;

    //informatia utila se afla in frunze
    if(root->top_left == -1 && root->top_right == -1 && root->bottom_right == -1 && root->bottom_left == -1) {
        for(i = offsetY; i < offsetY + size; i++) {
            for(j = offsetX; j < offsetX + size; j++) {
                imageData[i][j].red = root->red;
                imageData[i][j].green = root->green;
                imageData[i][j].blue = root->blue;
            }
        }
    }
    else {
        collectData(imageData, offsetX, offsetY, root->top_left, nodeArray);
        collectData(imageData, offsetX + size / 2, offsetY, root->top_right, nodeArray);
        collectData(imageData, offsetX + size / 2, offsetY + size / 2, root->bottom_right, nodeArray);
        collectData(imageData, offsetX, offsetY + size / 2, root->bottom_left, nodeArray);
    }
}

/* functia creeaza o matrice de pixeli pe baza informatiilor din vectorul de structuri
 * respectand cerintele pentru oglindire pe orizontala sau verticala*/
void mirror(Pixel **imageData, int offsetX, int offsetY, int rootIndex, NodeArray *nodeArray, char *type) {
    QuadtreeNode *root = nodeArray->data[rootIndex];
    uint32_t size = (uint32_t)sqrt(root->area);
    int i = 0, j = 0;

    if(root->top_left == -1 && root->top_right == -1 && root->bottom_right == -1 && root->bottom_left == -1) {
        for(i = offsetY; i < offsetY + size; i++) {
            for(j = offsetX; j < offsetX + size; j++) {
                imageData[i][j].red = root->red;
                imageData[i][j].green = root->green;
                imageData[i][j].blue = root->blue;
            }
        }
    }
    else {
        if(strcmp(type, "h") == 0) { //oglindire pe orizontala
            mirror(imageData, offsetX + size / 2, offsetY, root->top_left, nodeArray, type);
            mirror(imageData, offsetX, offsetY, root->top_right, nodeArray, type);
            mirror(imageData, offsetX, offsetY + size / 2, root->bottom_right, nodeArray, type);
            mirror(imageData, offsetX + size / 2, offsetY + size / 2, root->bottom_left, nodeArray, type);
        }
        if(strcmp(type, "v") == 0) { //oglindire pe verticala
            mirror(imageData, offsetX, offsetY + size / 2, root->top_left, nodeArray, type);
            mirror(imageData, offsetX + size / 2, offsetY + size / 2, root->top_right, nodeArray, type);
            mirror(imageData, offsetX + size / 2, offsetY, root->bottom_right, nodeArray, type);
            mirror(imageData, offsetX, offsetY, root->bottom_left, nodeArray, type);
        }
    }
}

/* functia citeste informatii dintr-un fisier PPM, creeaza arborele de compresie si
 * returneaza o structura de tipul NodeArray*/
NodeArray *compressedImage(char *filename, long prag, unsigned int *leafNumber) {
    Image *img;
    img = readFile_PPM(filename);

    NodeArray *nodeArray = initNodeArray();
    *leafNumber = 0;
    QuadtreeNode *root = (QuadtreeNode*)malloc(sizeof(QuadtreeNode));
    addNode(nodeArray, root); //adaug in vectorul de structuri radacina arborelui
    unsigned int size = img->width;

    divideData(img->data, 0, 0, size, prag, root, nodeArray, leafNumber); //se creeaza arborele de compresie

    free(*(img->data));
    free(img->data);
    free(img);

    return nodeArray;
}

/* functia primeste ca parametri 2 imagini (de aceeasi dimensiune) si realizeaza media aritmetica
 * al fiecarui canal RGB al pixelilor aflati pe aceleasi pozitii in cele 2 matrici de pixeli
 * rezultatul este salvat intr-una din matrici si imaginea respectiva este returnata*/
Image* overlap(Image *inputImg1, Image *inputImg2) {
    long long averageRed = 0;
    long long averageGreen = 0;
    long long averageBlue = 0;
    int i = 0;
    int j = 0;

    if(inputImg1->width == inputImg2->width) { //matrici de dimensiuni egale
        for(i = 0; i < inputImg1->width; i++) {
            for(j = 0; j < inputImg1->width; j++) {
                averageRed = (inputImg1->data[i][j].red + inputImg2->data[i][j].red) / 2;
                averageGreen = (inputImg1->data[i][j].green + inputImg2->data[i][j].green) / 2;
                averageBlue = (inputImg1->data[i][j].blue + inputImg2->data[i][j].blue) / 2;

                //actualizare pixel
                inputImg1->data[i][j].red = (unsigned char)averageRed;
                inputImg1->data[i][j].green = (unsigned char)averageGreen;
                inputImg1->data[i][j].blue = (unsigned char)averageBlue;
            }
        }
    }
    return inputImg1;
}

int main(int argc, char *argv[]) {
    long prag = 0;
    char *inputFile = argv[argc - 2];
    char *outputFile = argv[argc - 1];

    if(strcmp(argv[1], "-c") == 0) { //programul va rezolva doar cerinta 1 - compresia
        prag = atoi(argv[2]);
        unsigned  int *leafNumber = (unsigned int*)malloc(sizeof(unsigned int));

        NodeArray *nodeArray = compressedImage(inputFile, prag, leafNumber);
        CompressedData *compressedData = (CompressedData*)malloc(sizeof(CompressedData));
        compressedData->numar_culori = (uint32_t*)malloc(sizeof(uint32_t));
        compressedData->numar_noduri = (uint32_t*)malloc(sizeof(uint32_t));

        *(compressedData->numar_culori) = *leafNumber;
        free(leafNumber);
        *(compressedData->numar_noduri) = nodeArray->length;
        compressedData->vector = nodeArray->data;
        writeFile_compressed(outputFile, compressedData); //scrierea informatiilor in fisierul comprimat

        int i = 0;
        for (i = 0; i < *(compressedData->numar_noduri); i++ ) {
            free(compressedData->vector[i]);
        }
        free(compressedData->vector);
        free(compressedData->numar_culori);
        free(compressedData->numar_noduri);
        free(compressedData);
        free(nodeArray);
    }
    if(strcmp(argv[1], "-d") == 0) { //programul va rezolva doar cerinta 2 - decompresia
        CompressedData* compressedData = readFile_compressed(inputFile);
        NodeArray *nodeArray = (NodeArray*)malloc(sizeof(NodeArray));

        nodeArray->data = compressedData->vector;
        nodeArray->length = *(compressedData->numar_noduri);

        Image *decompressedImage = initImage(nodeArray->data[0]); //initializarea matricei de pixeli
        collectData(decompressedImage->data, 0, 0, 0, nodeArray); //crearea matricei de pixeli
        writeFile_PPM(outputFile, decompressedImage);

        freeImage(decompressedImage);
        int i = 0;
        for (i = 0; i < *(compressedData->numar_noduri); i++ ) {
            free(compressedData->vector[i]);
        }
        free(compressedData->vector);
        free(compressedData->numar_culori);
        free(compressedData->numar_noduri);
        free(compressedData);
        free(nodeArray);
    }
    if(strcmp(argv[1], "-m") == 0) { //programul va rula pentru cerinta 3 - oglindirea
        unsigned  int *leafNumber = (unsigned int*)malloc(sizeof(unsigned int));
        char *type = argv[2];
        prag = atoi(argv[3]);

        NodeArray *nodeArray = compressedImage(inputFile, prag, leafNumber);
        Image *mirrorImage = initImage(nodeArray->data[0]); //initializare matrice de pixeli
        mirror(mirrorImage->data, 0, 0, 0, nodeArray, type); //creeaza matrice de pixeli
        writeFile_PPM(outputFile, mirrorImage); //creeaza fisierul de output de tipul PPM

        freeImage(mirrorImage);
        freeNodeArray(nodeArray);
        free(leafNumber);
    }

    if(strcmp(argv[1], "-o") == 0) { //programul ruleaza pentru cerinta bonus (suprapunerea a 2 imagini)
        char *inputFile1 = argv[argc - 3];
        char *inputFile2 = argv[argc - 2];
        unsigned  int *leafNumber1 = (unsigned int*)malloc(sizeof(unsigned int));
        unsigned  int *leafNumber2 = (unsigned int*)malloc(sizeof(unsigned int));
        prag = atoi(argv[2]);

        NodeArray *nodeArray1 = compressedImage(inputFile1, prag, leafNumber1);
        NodeArray *nodeArray2 = compressedImage(inputFile2, prag, leafNumber2);
        Image *img1 = initImage(nodeArray1->data[0]);
        Image *img2 = initImage(nodeArray2->data[0]);
        collectData(img1->data, 0, 0, 0, nodeArray1);
        collectData(img2->data, 0, 0, 0, nodeArray2);

        img1 = overlap(img1, img2); //suprapune img1 cu img2
        writeFile_PPM(outputFile, img1);

        freeImage(img1);
        freeImage(img2);
        freeNodeArray(nodeArray1);
        freeNodeArray(nodeArray2);
        free(leafNumber1);
        free(leafNumber2);
    }

    return 0;
}