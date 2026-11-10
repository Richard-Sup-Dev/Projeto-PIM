#ifndef EXCLUSOES_H
#define EXCLUSOES_H

#include <stdbool.h>

#include "modelos.h"

bool excluirCadastro(Cliente clientes[], int *totalClientes,
                     Equipamento equipamentos[], int *totalEquipamentos,
                     Funcionario funcionarios[], int *totalFuncionarios,
                     OrdemServico ordens[], int *totalOrdens,
                     Manutencao manutencoes[], int *totalManutencoes);

#endif
