#include "exclusoes.h"

#include <stdio.h>
#include <string.h>

#include "banco.h"
#include "entrada.h"

static bool confirmarExclusao(const char *tipo, int id) {
    int confirmar;

    printf("Confirma a exclusao de %s ID %d? (1 = sim, 0 = nao)\n", tipo, id);
    if (!lerInteiro("Opcao: ", 0, 1, &confirmar)) {
        return false;
    }
    return confirmar == 1;
}

static void removerDoVetor(void *vetor, size_t tamanhoItem, int indice,
                           int *total) {
    char *itens = (char *)vetor;
    int restantes = *total - indice - 1;

    if (restantes > 0) {
        memmove(itens + (size_t)indice * tamanhoItem,
                itens + (size_t)(indice + 1) * tamanhoItem,
                (size_t)restantes * tamanhoItem);
    }
    (*total)--;
}

bool excluirCadastro(Cliente clientes[], int *totalClientes,
                     Equipamento equipamentos[], int *totalEquipamentos,
                     Funcionario funcionarios[], int *totalFuncionarios,
                     OrdemServico ordens[], int *totalOrdens,
                     Manutencao manutencoes[], int *totalManutencoes) {
    int tipo;
    int id;
    int indice;
    char erro[256];
    bool sucesso;

    printf("\n--- Excluir registro ---\n");
    printf("1. Cliente\n");
    printf("2. Equipamento\n");
    printf("3. Funcionario\n");
    printf("4. Ordem de servico\n");
    printf("5. Manutencao\n");
    if (!lerInteiro("Tipo de registro (0 para cancelar): ", 0, 5, &tipo)) {
        return false;
    }
    if (tipo == 0) {
        return true;
    }

    switch (tipo) {
        case 1:
            if (*totalClientes == 0) {
                printf("Nao ha clientes cadastrados.\n");
                return true;
            }
            for (indice = 0; indice < *totalClientes; indice++) {
                printf("ID %d - %s\n", clientes[indice].idCliente,
                       clientes[indice].nome);
            }
            if (!lerInteiro("ID do cliente a excluir: ", 1, 2147483647,
                            &id)) {
                return false;
            }
            for (indice = 0; indice < *totalClientes; indice++) {
                if (clientes[indice].idCliente == id) {
                    break;
                }
            }
            if (indice == *totalClientes) {
                printf("Cliente nao encontrado na lista carregada.\n");
                return true;
            }
            if (!confirmarExclusao("cliente", id)) {
                return true;
            }
            sucesso = bancoExcluirCliente(id, erro, sizeof erro);
            if (sucesso) {
                removerDoVetor(clientes, sizeof clientes[0], indice,
                               totalClientes);
            }
            break;
        case 2:
            if (*totalEquipamentos == 0) {
                printf("Nao ha equipamentos cadastrados.\n");
                return true;
            }
            for (indice = 0; indice < *totalEquipamentos; indice++) {
                printf("ID %d - %s\n", equipamentos[indice].idEquipamento,
                       equipamentos[indice].descricao);
            }
            if (!lerInteiro("ID do equipamento a excluir: ", 1, 2147483647,
                            &id)) {
                return false;
            }
            for (indice = 0; indice < *totalEquipamentos; indice++) {
                if (equipamentos[indice].idEquipamento == id) {
                    break;
                }
            }
            if (indice == *totalEquipamentos) {
                printf("Equipamento nao encontrado na lista carregada.\n");
                return true;
            }
            if (!confirmarExclusao("equipamento", id)) {
                return true;
            }
            sucesso = bancoExcluirEquipamento(id, erro, sizeof erro);
            if (sucesso) {
                removerDoVetor(equipamentos, sizeof equipamentos[0], indice,
                               totalEquipamentos);
            }
            break;
        case 3:
            if (*totalFuncionarios == 0) {
                printf("Nao ha funcionarios cadastrados.\n");
                return true;
            }
            for (indice = 0; indice < *totalFuncionarios; indice++) {
                printf("ID %d - %s\n", funcionarios[indice].idFuncionario,
                       funcionarios[indice].nome);
            }
            if (!lerInteiro("ID do funcionario a excluir: ", 1, 2147483647,
                            &id)) {
                return false;
            }
            for (indice = 0; indice < *totalFuncionarios; indice++) {
                if (funcionarios[indice].idFuncionario == id) {
                    break;
                }
            }
            if (indice == *totalFuncionarios) {
                printf("Funcionario nao encontrado na lista carregada.\n");
                return true;
            }
            if (!confirmarExclusao("funcionario", id)) {
                return true;
            }
            sucesso = bancoExcluirFuncionario(id, erro, sizeof erro);
            if (sucesso) {
                removerDoVetor(funcionarios, sizeof funcionarios[0], indice,
                               totalFuncionarios);
            }
            break;
        case 4:
            if (*totalOrdens == 0) {
                printf("Nao ha ordens de servico cadastradas.\n");
                return true;
            }
            for (indice = 0; indice < *totalOrdens; indice++) {
                printf("ID %d - %s\n", ordens[indice].idOS,
                       ordens[indice].status);
            }
            if (!lerInteiro("ID da ordem de servico a excluir: ", 1,
                            2147483647, &id)) {
                return false;
            }
            for (indice = 0; indice < *totalOrdens; indice++) {
                if (ordens[indice].idOS == id) {
                    break;
                }
            }
            if (indice == *totalOrdens) {
                printf("Ordem de servico nao encontrada na lista carregada.\n");
                return true;
            }
            if (!confirmarExclusao("ordem de servico", id)) {
                return true;
            }
            sucesso = bancoExcluirOrdem(id, erro, sizeof erro);
            if (sucesso) {
                removerDoVetor(ordens, sizeof ordens[0], indice, totalOrdens);
            }
            break;
        case 5:
            if (*totalManutencoes == 0) {
                printf("Nao ha manutencoes cadastradas.\n");
                return true;
            }
            for (indice = 0; indice < *totalManutencoes; indice++) {
                printf("ID %d - OS %d - %s\n",
                       manutencoes[indice].idManutencao,
                       manutencoes[indice].idOS,
                       manutencoes[indice].descricao);
            }
            if (!lerInteiro("ID da manutencao a excluir: ", 1, 2147483647,
                            &id)) {
                return false;
            }
            for (indice = 0; indice < *totalManutencoes; indice++) {
                if (manutencoes[indice].idManutencao == id) {
                    break;
                }
            }
            if (indice == *totalManutencoes) {
                printf("Manutencao nao encontrada na lista carregada.\n");
                return true;
            }
            if (!confirmarExclusao("manutencao", id)) {
                return true;
            }
            sucesso = bancoExcluirManutencao(id, erro, sizeof erro);
            if (sucesso) {
                removerDoVetor(manutencoes, sizeof manutencoes[0], indice,
                               totalManutencoes);
            }
            break;
        default:
            return true;
    }

    if (!sucesso) {
        printf("%s\n", erro);
        return true;
    }
    printf("Registro excluido do banco de dados com sucesso.\n");
    return true;
}
