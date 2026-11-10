#include "funcionarios.h"

#include <stdio.h>

#include "banco.h"
#include "entrada.h"

bool cadastrarFuncionario(Funcionario funcionarios[], int *total) {
    Funcionario novo;
    char erro[200];

    if (*total >= MAX) {
        printf("Limite de funcionarios atingido.\n");
        return true;
    }

    if (!lerLinha("Nome do funcionario: ", novo.nome, sizeof novo.nome) ||
        !lerLinha("Funcao: ", novo.funcao, sizeof novo.funcao)) {
        return false;
    }
    if (!bancoCadastrarFuncionario(novo.nome, novo.funcao,
                                   &novo.idFuncionario, erro, sizeof erro)) {
        printf("%s\n", erro);
        return true;
    }

    funcionarios[*total] = novo;
    (*total)++;
    printf("Funcionario cadastrado com sucesso. ID: %d\n",
           novo.idFuncionario);
    return true;
}
