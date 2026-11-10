#ifndef MODELOS_H
#define MODELOS_H

#define MAX 500
#define TAM_NOME 101
#define TAM_TELEFONE 20
#define TAM_DESCRICAO 256
#define TAM_TIPO 51
#define TAM_STATUS 31
#define TAM_DATA 11
#define TAM_FUNCAO 51
#define TAM_MANUTENCAO 1024

typedef struct {
    int idCliente;
    char nome[TAM_NOME];
    char telefone[TAM_TELEFONE];
} Cliente;

typedef struct {
    int idEquipamento;
    char descricao[TAM_DESCRICAO];
    char tipo[TAM_TIPO];
    int idCliente;
} Equipamento;

typedef struct {
    int idFuncionario;
    char nome[TAM_NOME];
    char funcao[TAM_FUNCAO];
} Funcionario;

typedef struct {
    int idOS;
    char dataAbertura[TAM_DATA];
    int idCliente;
    int idEquipamento;
    int idFuncionario;
    char status[TAM_STATUS];
} OrdemServico;

typedef struct {
    int idManutencao;
    char descricao[TAM_MANUTENCAO];
    char dataManutencao[TAM_DATA];
    int idOS;
} Manutencao;

#endif
