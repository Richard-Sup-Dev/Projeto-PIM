#include "manutencoes.h"

#include <stdio.h>

#include "banco.h"
#include "entrada.h"

bool registrarManutencao(Manutencao manutencoes[], int *total,
                         const OrdemServico ordens[], int totalOrdens) {
    Manutencao nova;
    int maiorIdOS;
    int i;
    char erro[200];

    if (*total >= MAX) {
        printf("Limite de manutencoes atingido.\n");
        return true;
    }
    if (totalOrdens == 0) {
        printf("Abra uma ordem de servico antes de registrar uma manutencao.\n");
        return true;
    }

    printf("Ordens de servico disponiveis:\n");
    for (i = 0; i < totalOrdens; i++) {
        printf("%d - %s\n", ordens[i].idOS, ordens[i].status);
    }
    maiorIdOS = ordens[totalOrdens - 1].idOS;
    if (!lerLinha("Descricao do servico realizado: ", nova.descricao,
                  sizeof nova.descricao) ||
        !lerInteiro("ID da ordem de servico: ", 1, maiorIdOS, &nova.idOS)) {
        return false;
    }
    for (i = 0; i < totalOrdens; i++) {
        if (ordens[i].idOS == nova.idOS) {
            break;
        }
    }
    if (i == totalOrdens) {
        printf("Nao existe ordem de servico com esse ID.\n");
        return true;
    }

    if (!bancoRegistrarManutencao(nova.descricao, nova.idOS,
                                  &nova.idManutencao, nova.dataManutencao,
                                  sizeof nova.dataManutencao, erro,
                                  sizeof erro)) {
        printf("%s\n", erro);
        return true;
    }
    manutencoes[*total] = nova;
    (*total)++;
    printf("Manutencao registrada com sucesso. ID: %d\n",
           nova.idManutencao);
    return true;
}

void listarManutencoes(const Manutencao manutencoes[], int total) {
    int i;

    if (total == 0) {
        printf("Nenhuma manutencao registrada.\n");
        return;
    }

    printf("\n--- Manutencoes ---\n");
    for (i = 0; i < total; i++) {
        printf("ID: %d | OS: %d | Data: %s | Descricao: %s\n",
               manutencoes[i].idManutencao, manutencoes[i].idOS,
               manutencoes[i].dataManutencao, manutencoes[i].descricao);
    }
}
