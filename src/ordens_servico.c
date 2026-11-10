#include "ordens_servico.h"

#include <stdio.h>
#include <string.h>

#include "banco.h"
#include "entrada.h"

bool abrirOrdemServico(OrdemServico ordens[], int *total,
                       const Cliente clientes[], int totalClientes,
                       int totalEquipamentos,
                       const Equipamento equipamentos[],
                       const Funcionario funcionarios[],
                       int totalFuncionarios) {
    OrdemServico nova;
    int maiorIdCliente;
    int maiorIdEquipamento;
    int maiorIdFuncionario;
    int i;

    if (totalClientes == 0 || totalEquipamentos == 0 ||
        totalFuncionarios == 0) {
        printf("Cadastre clientes e equipamentos e registre ao menos um funcionario no banco antes de abrir uma OS.\n");
        return true;
    }
    if (*total >= MAX) {
        printf("Limite de ordens de servico atingido.\n");
        return true;
    }

    printf("Clientes disponiveis:\n");
    for (i = 0; i < totalClientes; i++) {
        printf("%d - %s\n", clientes[i].idCliente, clientes[i].nome);
    }
    printf("Equipamentos disponiveis:\n");
    for (i = 0; i < totalEquipamentos; i++) {
        printf("%d - %s\n", equipamentos[i].idEquipamento,
               equipamentos[i].descricao);
    }
    printf("Funcionarios disponiveis:\n");
    for (i = 0; i < totalFuncionarios; i++) {
        printf("%d - %s\n", funcionarios[i].idFuncionario,
               funcionarios[i].nome);
    }
    maiorIdCliente = clientes[totalClientes - 1].idCliente;
    maiorIdEquipamento = equipamentos[totalEquipamentos - 1].idEquipamento;
    maiorIdFuncionario = funcionarios[totalFuncionarios - 1].idFuncionario;
    if (!lerInteiro("ID do cliente responsavel: ", 1, maiorIdCliente,
                    &nova.idCliente) ||
        !lerInteiro("ID do equipamento: ", 1, maiorIdEquipamento,
                    &nova.idEquipamento) ||
        !lerInteiro("ID do funcionario responsavel: ", 1,
                    maiorIdFuncionario, &nova.idFuncionario)) {
        return false;
    }
    for (i = 0; i < totalClientes; i++) {
        if (clientes[i].idCliente == nova.idCliente) {
            break;
        }
    }
    if (i == totalClientes) {
        printf("Nao existe cliente com esse ID.\n");
        return true;
    }
    for (i = 0; i < totalEquipamentos; i++) {
        if (equipamentos[i].idEquipamento == nova.idEquipamento) {
            break;
        }
    }
    if (i == totalEquipamentos) {
        printf("Nao existe equipamento com esse ID.\n");
        return true;
    }
    if (equipamentos[i].idCliente != nova.idCliente) {
        printf("O equipamento selecionado nao pertence ao cliente informado.\n");
        return true;
    }
    for (i = 0; i < totalFuncionarios; i++) {
        if (funcionarios[i].idFuncionario == nova.idFuncionario) {
            break;
        }
    }
    if (i == totalFuncionarios) {
        printf("Nao existe funcionario com esse ID.\n");
        return true;
    }

    strcpy(nova.status, "Aberta");
    {
        char erro[200];
        if (!bancoAbrirOrdem(nova.idCliente, nova.idEquipamento,
                             nova.idFuncionario, &nova.idOS,
                             nova.dataAbertura, sizeof nova.dataAbertura, erro,
                             sizeof erro)) {
            printf("%s\n", erro);
            return true;
        }
    }
    ordens[*total] = nova;
    (*total)++;
    printf("Ordem de servico aberta com sucesso. ID OS: %d\n", nova.idOS);
    return true;
}

void listarOrdens(const OrdemServico ordens[], int total,
                  const Cliente clientes[], int totalClientes,
                  const Equipamento equipamentos[], int totalEquipamentos,
                  const Funcionario funcionarios[], int totalFuncionarios) {
    int i;

    if (total == 0) {
        printf("Nenhuma ordem de servico registrada.\n");
        return;
    }

    printf("\n--- Ordens de Servico ---\n");
    for (i = 0; i < total; i++) {
        int j;
        const char *nomeCliente = "Cliente nao localizado";
        const char *descricaoEquipamento = "Equipamento nao localizado";
        const char *nomeFuncionario = "Funcionario nao localizado";

        for (j = 0; j < totalClientes; j++) {
            if (clientes[j].idCliente == ordens[i].idCliente) {
                nomeCliente = clientes[j].nome;
                break;
            }
        }

        for (j = 0; j < totalEquipamentos; j++) {
            if (equipamentos[j].idEquipamento == ordens[i].idEquipamento) {
                descricaoEquipamento = equipamentos[j].descricao;
                break;
            }
        }

        for (j = 0; j < totalFuncionarios; j++) {
            if (funcionarios[j].idFuncionario == ordens[i].idFuncionario) {
                nomeFuncionario = funcionarios[j].nome;
                break;
            }
        }

        printf("OS ID: %d | Data: %s | Cliente: %s (ID %d) | Equipamento: %s (ID %d) | Tecnico: %s | Status: %s\n",
               ordens[i].idOS, ordens[i].dataAbertura, nomeCliente,
               ordens[i].idCliente, descricaoEquipamento,
               ordens[i].idEquipamento, nomeFuncionario, ordens[i].status);
    }
}

bool atualizarStatusOrdem(OrdemServico ordens[], int total) {
    static const char *const status[] = {
        "Aberta", "Em Andamento", "Concluida", "Cancelada"};
    int maiorId = 0;
    int id;
    int opcao;
    int i;
    char erro[256];

    if (total == 0) {
        printf("Nao ha ordens de servico cadastradas.\n");
        return true;
    }

    printf("\n--- Atualizar status da OS ---\n");
    for (i = 0; i < total; i++) {
        printf("OS %d - %s\n", ordens[i].idOS, ordens[i].status);
        if (ordens[i].idOS > maiorId) {
            maiorId = ordens[i].idOS;
        }
    }
    if (!lerInteiro("ID da OS: ", 1, maiorId, &id)) {
        return false;
    }
    for (i = 0; i < total; i++) {
        if (ordens[i].idOS == id) {
            break;
        }
    }
    if (i == total) {
        printf("Nao existe OS com esse ID.\n");
        return true;
    }

    printf("1. Aberta\n");
    printf("2. Em Andamento\n");
    printf("3. Concluida\n");
    printf("4. Cancelada\n");
    if (!lerInteiro("Novo status: ", 1, 4, &opcao)) {
        return false;
    }
    if (strcmp(ordens[i].status, status[opcao - 1]) == 0) {
        printf("A OS ja esta com esse status.\n");
        return true;
    }
    if (!bancoAtualizarStatusOrdem(id, status[opcao - 1], erro,
                                   sizeof erro)) {
        printf("%s\n", erro);
        return true;
    }
    snprintf(ordens[i].status, sizeof ordens[i].status, "%s",
             status[opcao - 1]);
    printf("Status da OS %d atualizado para %s.\n", id, ordens[i].status);
    return true;
}
