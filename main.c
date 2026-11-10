#include <stdbool.h>
#include <stdio.h>

#include "banco.h"
#include "clientes.h"
#include "exclusoes.h"
#include "equipamentos.h"
#include "entrada.h"
#include "funcionarios.h"
#include "manutencoes.h"
#include "modelos.h"
#include "ordens_servico.h"

int main(void) {
    char erro[256];
    Cliente clientes[MAX];
    Equipamento equipamentos[MAX];
    Funcionario funcionarios[MAX];
    OrdemServico ordens[MAX];
    Manutencao manutencoes[MAX];
    int totalClientes = 0;
    int totalEquipamentos = 0;
    int totalFuncionarios = 0;
    int totalOrdens = 0;
    int totalManutencoes = 0;
    int opcao;
    bool continuar = true;

    if (!bancoConectar(erro, sizeof erro)) {
        fprintf(stderr, "%s\n", erro);
        return 1;
    }
    if (!bancoCarregar(clientes, &totalClientes, equipamentos,
                       &totalEquipamentos, funcionarios, &totalFuncionarios,
                       ordens, &totalOrdens, erro, sizeof erro)) {
        fprintf(stderr, "%s\n", erro);
        bancoDesconectar();
        return 1;
    }
    if (!bancoCarregarManutencoes(manutencoes, &totalManutencoes, erro,
                                  sizeof erro)) {
        fprintf(stderr, "%s\n", erro);
        bancoDesconectar();
        return 1;
    }

    while (continuar) {
        printf("\n=== SIG-InovaLog - Menu Principal ===\n");
        printf("1. Cadastrar Cliente (RF01)\n");
        printf("2. Cadastrar Equipamento (RF02)\n");
        printf("3. Abrir Ordem de Servico (RF04)\n");
        printf("4. Listar Ordens de Servico (RF05)\n");
        printf("5. Cadastrar Funcionario\n");
        printf("6. Registrar Manutencao\n");
        printf("7. Listar Manutencoes\n");
        printf("8. Excluir registro\n");
        printf("9. Atualizar status de OS\n");
        printf("0. Sair\n");

        if (!lerInteiro("Escolha uma opcao: ", 0, 9, &opcao)) {
            break;
        }

        switch (opcao) {
            case 1:
                continuar = cadastrarCliente(clientes, &totalClientes);
                break;
            case 2:
                continuar =
                    cadastrarEquipamento(equipamentos, &totalEquipamentos,
                                         clientes, totalClientes);
                break;
            case 3:
                continuar = abrirOrdemServico(
                    ordens, &totalOrdens, clientes, totalClientes,
                    totalEquipamentos, equipamentos, funcionarios,
                    totalFuncionarios);
                break;
            case 4:
                listarOrdens(ordens, totalOrdens, clientes, totalClientes,
                             equipamentos, totalEquipamentos, funcionarios,
                             totalFuncionarios);
                break;
            case 5:
                continuar =
                    cadastrarFuncionario(funcionarios, &totalFuncionarios);
                break;
            case 6:
                continuar = registrarManutencao(
                    manutencoes, &totalManutencoes, ordens, totalOrdens);
                break;
            case 7:
                listarManutencoes(manutencoes, totalManutencoes);
                break;
            case 8:
                continuar = excluirCadastro(
                    clientes, &totalClientes, equipamentos,
                    &totalEquipamentos, funcionarios, &totalFuncionarios,
                    ordens, &totalOrdens, manutencoes, &totalManutencoes);
                break;
            case 9:
                continuar = atualizarStatusOrdem(ordens, totalOrdens);
                break;
            case 0:
                printf("Encerrando o sistema...\n");
                continuar = false;
                break;
        }
    }

    bancoDesconectar();
    return 0;
}
