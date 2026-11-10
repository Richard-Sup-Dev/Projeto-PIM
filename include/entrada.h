#ifndef ENTRADA_H
#define ENTRADA_H

#include <stdbool.h>
#include <stddef.h>

bool lerLinha(const char *mensagem, char *destino, size_t tamanho);
bool lerInteiro(const char *mensagem, int minimo, int maximo, int *valor);

#endif
