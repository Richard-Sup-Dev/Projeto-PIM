#include "entrada.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_ENTRADA 2048

bool lerLinha(const char *mensagem, char *destino, size_t tamanho) {
    char entrada[TAM_ENTRADA];

    for (;;) {
        size_t comprimento;
        int caractere;

        printf("%s", mensagem);
        if (fgets(entrada, sizeof entrada, stdin) == NULL) {
            return false;
        }

        comprimento = strlen(entrada);
        if (comprimento > 0 && entrada[comprimento - 1] == '\n') {
            entrada[--comprimento] = '\0';
            if (comprimento > 0 && entrada[comprimento - 1] == '\r') {
                entrada[--comprimento] = '\0';
            }
        } else if (!feof(stdin)) {
            while ((caractere = getchar()) != '\n' && caractere != EOF) {
            }
            printf("Entrada muito longa. Tente novamente.\n");
            continue;
        }

        if (comprimento == 0) {
            printf("O valor nao pode ficar vazio.\n");
            continue;
        }
        if (comprimento >= tamanho) {
            printf("O valor deve ter no maximo %llu caracteres.\n",
                   (unsigned long long)(tamanho - 1));
            continue;
        }

        memcpy(destino, entrada, comprimento + 1);
        return true;
    }
}

bool lerInteiro(const char *mensagem, int minimo, int maximo, int *valor) {
    char entrada[TAM_ENTRADA];

    for (;;) {
        char *fim;
        long numero;

        if (!lerLinha(mensagem, entrada, sizeof entrada)) {
            return false;
        }

        errno = 0;
        numero = strtol(entrada, &fim, 10);
        while (*fim == ' ' || *fim == '\t') {
            fim++;
        }

        if (entrada == fim || *fim != '\0' || errno == ERANGE ||
            numero < minimo || numero > maximo || numero < INT_MIN ||
            numero > INT_MAX) {
            printf("Digite um numero inteiro entre %d e %d.\n", minimo, maximo);
            continue;
        }

        *valor = (int)numero;
        return true;
    }
}
