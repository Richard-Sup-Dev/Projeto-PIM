#include "clientes.h"

#include <stdio.h>

#include "banco.h"
#include "entrada.h"

bool cadastrarCliente(Cliente clientes[], int *total) {
    Cliente novo;

    if (*total >= MAX) {
        printf("Limite de clientes atingido.\n");
        return true;
    }

    if (!lerLinha("Nome do cliente: ", novo.nome, sizeof novo.nome) ||
        !lerLinha("Telefone: ", novo.telefone, sizeof novo.telefone)) {
        return false;
    }

    {
        char erro[200];
        if (!bancoCadastrarCliente(novo.nome, novo.telefone, &novo.idCliente,
                                   erro, sizeof erro)) {
            printf("%s\n", erro);
            return true;
        }
    }

    clientes[*total] = novo;
    (*total)++;
    printf("Cliente cadastrado com sucesso. ID: %d\n", novo.idCliente);
    return true;
}
