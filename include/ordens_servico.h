#ifndef ORDENS_SERVICO_H
#define ORDENS_SERVICO_H

#include <stdbool.h>

#include "modelos.h"

bool abrirOrdemServico(OrdemServico ordens[], int *total,
                       const Cliente clientes[],
                       int totalClientes, int totalEquipamentos,
                       const Equipamento equipamentos[],
                       const Funcionario funcionarios[],
                       int totalFuncionarios);
void listarOrdens(const OrdemServico ordens[], int total,
                  const Cliente clientes[], int totalClientes,
                  const Equipamento equipamentos[], int totalEquipamentos,
                  const Funcionario funcionarios[], int totalFuncionarios);
bool atualizarStatusOrdem(OrdemServico ordens[], int total);

#endif
