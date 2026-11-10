#ifndef MANUTENCOES_H
#define MANUTENCOES_H

#include <stdbool.h>

#include "modelos.h"

bool registrarManutencao(Manutencao manutencoes[], int *total,
                         const OrdemServico ordens[], int totalOrdens);
void listarManutencoes(const Manutencao manutencoes[], int total);

#endif
