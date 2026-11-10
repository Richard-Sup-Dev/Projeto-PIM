#include "equipamentos.h"

#include <stdio.h>

#include "banco.h"
#include "entrada.h"

bool cadastrarEquipamento(Equipamento equipamentos[], int *total,
                          const Cliente clientes[], int totalClientes) {
    Equipamento novo;
    int idCliente;
    int maiorIdCliente;
    int i;

    if (*total >= MAX) {
        printf("Limite de equipamentos atingido.\n");
        return true;
    }
    if (totalClientes == 0) {
        printf("Cadastre um cliente antes de cadastrar um equipamento.\n");
        return true;
    }

    printf("Clientes disponiveis:\n");
    for (i = 0; i < totalClientes; i++) {
        printf("%d - %s\n", clientes[i].idCliente, clientes[i].nome);
    }
    maiorIdCliente = clientes[totalClientes - 1].idCliente;

    if (!lerLinha("Descricao do equipamento: ", novo.descricao,
                  sizeof novo.descricao) ||
        !lerLinha("Tipo (ex.: Bomba ou Motor Eletrico): ", novo.tipo,
                  sizeof novo.tipo) ||
        !lerInteiro("ID do cliente proprietario: ", 1, maiorIdCliente,
                    &idCliente)) {
        return false;
    }
    for (i = 0; i < totalClientes; i++) {
        if (clientes[i].idCliente == idCliente) {
            break;
        }
    }
    if (i == totalClientes) {
        printf("Nao existe cliente com esse ID.\n");
        return true;
    }

    {
        char erro[200];
        if (!bancoCadastrarEquipamento(novo.descricao, novo.tipo, idCliente,
                                       &novo.idEquipamento, erro,
                                       sizeof erro)) {
            printf("%s\n", erro);
            return true;
        }
    }
    novo.idCliente = idCliente;
    equipamentos[*total] = novo;
    (*total)++;
    printf("Equipamento cadastrado com sucesso. ID: %d\n",
           novo.idEquipamento);
    return true;
}
