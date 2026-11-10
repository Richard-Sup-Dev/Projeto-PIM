#ifndef BANCO_H
#define BANCO_H

#include <stdbool.h>
#include <stddef.h>

#include "modelos.h"

bool bancoConectar(char *erro, size_t tamanhoErro);
void bancoDesconectar(void);
bool bancoCarregar(Cliente clientes[], int *totalClientes,
                  Equipamento equipamentos[], int *totalEquipamentos,
                  Funcionario funcionarios[], int *totalFuncionarios,
                  OrdemServico ordens[], int *totalOrdens,
                  char *erro, size_t tamanhoErro);
bool bancoCarregarManutencoes(Manutencao manutencoes[], int *total,
                             char *erro, size_t tamanhoErro);
bool bancoCadastrarCliente(const char *nome, const char *telefone, int *id,
                           char *erro, size_t tamanhoErro);
bool bancoCadastrarEquipamento(const char *descricao, const char *tipo,
                               int idCliente, int *id, char *erro,
                               size_t tamanhoErro);
bool bancoCadastrarFuncionario(const char *nome, const char *funcao, int *id,
                               char *erro, size_t tamanhoErro);
bool bancoAbrirOrdem(int idCliente, int idEquipamento, int idFuncionario,
                     int *id, char *dataAbertura, size_t tamanhoData,
                     char *erro, size_t tamanhoErro);
bool bancoRegistrarManutencao(const char *descricao, int idOS, int *id,
                              char *dataManutencao, size_t tamanhoData,
                              char *erro, size_t tamanhoErro);
bool bancoAtualizarStatusOrdem(int id, const char *status, char *erro,
                               size_t tamanhoErro);
bool bancoExcluirCliente(int id, char *erro, size_t tamanhoErro);
bool bancoExcluirEquipamento(int id, char *erro, size_t tamanhoErro);
bool bancoExcluirFuncionario(int id, char *erro, size_t tamanhoErro);
bool bancoExcluirOrdem(int id, char *erro, size_t tamanhoErro);
bool bancoExcluirManutencao(int id, char *erro, size_t tamanhoErro);

#endif
