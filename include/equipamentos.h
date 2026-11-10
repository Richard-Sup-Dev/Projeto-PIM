#ifndef EQUIPAMENTOS_H
#define EQUIPAMENTOS_H

#include <stdbool.h>

#include "modelos.h"

bool cadastrarEquipamento(Equipamento equipamentos[], int *total,
                          const Cliente clientes[], int totalClientes);

#endif
