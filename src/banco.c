#include "banco.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpq-fe.h>

static PGconn *conexao;

static void definirErro(char *erro, size_t tamanhoErro, const char *contexto,
                        const PGresult *resultado) {
    const char *codigo = resultado == NULL
                             ? NULL
                             : PQresultErrorField(resultado, PG_DIAG_SQLSTATE);
    const char *detalhe = resultado == NULL
                              ? (conexao == NULL ? NULL : PQerrorMessage(conexao))
                              : PQresultErrorMessage(resultado);

    if (erro == NULL || tamanhoErro == 0) {
        return;
    }

    if (codigo != NULL) {
        snprintf(erro, tamanhoErro, "%s (codigo PostgreSQL %s).", contexto,
                 codigo);
    } else if (detalhe != NULL && detalhe[0] != '\0') {
        snprintf(erro, tamanhoErro, "%s %s", contexto, detalhe);
    } else {
        snprintf(erro, tamanhoErro, "%s Verifique a rede e a configuracao do banco.",
                 contexto);
    }
}

static bool executarConsulta(const char *consulta, int quantidade,
                             const char *const valores[], PGresult **resultado,
                             char *erro, size_t tamanhoErro,
                             const char *contexto) {
    *resultado = PQexecParams(conexao, consulta, quantidade, NULL, valores,
                              NULL, NULL, 0);
    if (*resultado == NULL ||
        PQresultStatus(*resultado) != PGRES_TUPLES_OK) {
        definirErro(erro, tamanhoErro, contexto, *resultado);
        if (*resultado != NULL) {
            PQclear(*resultado);
            *resultado = NULL;
        }
        return false;
    }
    return true;
}

static bool lerIdRetornado(PGresult *resultado, int *id, char *erro,
                           size_t tamanhoErro, const char *contexto) {
    bool valido = resultado != NULL &&
                  PQresultStatus(resultado) == PGRES_TUPLES_OK &&
                  PQntuples(resultado) == 1;

    if (valido) {
        *id = atoi(PQgetvalue(resultado, 0, 0));
    } else {
        definirErro(erro, tamanhoErro, contexto, resultado);
    }
    if (resultado != NULL) {
        PQclear(resultado);
    }
    return valido;
}

static bool valorInteiro(PGresult *resultado, int linha, int coluna, int *valor,
                         char *erro, size_t tamanhoErro) {
    const char *texto = PQgetvalue(resultado, linha, coluna);
    char *fim;
    long numero;

    numero = strtol(texto, &fim, 10);
    if (texto == fim || *fim != '\0' || numero < 1 || numero > 2147483647L) {
        snprintf(erro, tamanhoErro, "O banco retornou um identificador invalido.");
        return false;
    }
    *valor = (int)numero;
    return true;
}

static bool copiarCampo(PGresult *resultado, int linha, int coluna,
                        char *destino, size_t tamanho, char *erro,
                        size_t tamanhoErro) {
    int comprimento = PQgetlength(resultado, linha, coluna);

    if (PQgetisnull(resultado, linha, coluna) || comprimento < 0 ||
        (size_t)comprimento >= tamanho) {
        snprintf(erro, tamanhoErro,
                 "Um campo do banco excede o limite definido no programa.");
        return false;
    }
    memcpy(destino, PQgetvalue(resultado, linha, coluna),
           (size_t)comprimento);
    destino[comprimento] = '\0';
    return true;
}

static bool excluirRegistro(const char *consulta, int id, char *erro,
                            size_t tamanhoErro) {
    char textoId[16];
    const char *valores[] = {textoId};
    PGresult *resultado;
    const char *codigo;

    if (conexao == NULL) {
        snprintf(erro, tamanhoErro, "O banco de dados nao esta conectado.");
        return false;
    }

    snprintf(textoId, sizeof textoId, "%d", id);
    resultado = PQexecParams(conexao, consulta, 1, NULL, valores, NULL, NULL,
                             0);
    if (resultado == NULL ||
        PQresultStatus(resultado) != PGRES_TUPLES_OK) {
        codigo = resultado == NULL
                     ? NULL
                     : PQresultErrorField(resultado, PG_DIAG_SQLSTATE);
        if (codigo != NULL && strcmp(codigo, "23503") == 0) {
            snprintf(erro, tamanhoErro,
                     "Nao foi possivel excluir: existem registros vinculados. "
                     "Exclua primeiro os registros dependentes.");
        } else {
            definirErro(erro, tamanhoErro, "Falha ao excluir registro.",
                        resultado);
        }
        if (resultado != NULL) {
            PQclear(resultado);
        }
        return false;
    }

    if (PQntuples(resultado) != 1) {
        snprintf(erro, tamanhoErro,
                 "O registro nao foi encontrado no banco de dados.");
        PQclear(resultado);
        return false;
    }

    PQclear(resultado);
    return true;
}

bool bancoConectar(char *erro, size_t tamanhoErro) {
    const char *url = getenv("SIG_INOVALOG_DATABASE_URL");

    if (url == NULL || url[0] == '\0') {
        snprintf(erro, tamanhoErro,
                 "Configure a variavel SIG_INOVALOG_DATABASE_URL com a URL do Neon.");
        return false;
    }

    conexao = PQconnectdb(url);
    if (conexao == NULL || PQstatus(conexao) != CONNECTION_OK) {
        definirErro(erro, tamanhoErro, "Nao foi possivel conectar ao PostgreSQL.",
                    NULL);
        if (conexao != NULL) {
            PQfinish(conexao);
            conexao = NULL;
        }
        return false;
    }
    if (PQsetClientEncoding(conexao, "WIN1252") != 0) {
        definirErro(erro, tamanhoErro,
                    "Nao foi possivel configurar a codificacao do cliente.",
                    NULL);
        PQfinish(conexao);
        conexao = NULL;
        return false;
    }

    return true;
}

void bancoDesconectar(void) {
    if (conexao != NULL) {
        PQfinish(conexao);
        conexao = NULL;
    }
}

bool bancoCarregar(Cliente clientes[], int *totalClientes,
                   Equipamento equipamentos[], int *totalEquipamentos,
                   Funcionario funcionarios[], int *totalFuncionarios,
                   OrdemServico ordens[], int *totalOrdens,
                   char *erro, size_t tamanhoErro) {
    PGresult *resultado;
    int linha;
    int linhas;

    *totalClientes = 0;
    *totalEquipamentos = 0;
    *totalFuncionarios = 0;
    *totalOrdens = 0;

    if (!executarConsulta(
            "SELECT id_cliente, nome, telefone FROM cliente ORDER BY id_cliente",
            0, NULL, &resultado, erro, tamanhoErro, "Falha ao carregar clientes.")) {
        return false;
    }
    linhas = PQntuples(resultado);
    if (linhas > MAX) {
        snprintf(erro, tamanhoErro,
                 "Ha mais de %d clientes. Aumente MAX antes de carregar os dados.",
                 MAX);
        PQclear(resultado);
        return false;
    }
    for (linha = 0; linha < linhas; linha++) {
        if (!valorInteiro(resultado, linha, 0, &clientes[linha].idCliente,
                          erro, tamanhoErro) ||
            !copiarCampo(resultado, linha, 1, clientes[linha].nome,
                         sizeof clientes[linha].nome, erro, tamanhoErro) ||
            !copiarCampo(resultado, linha, 2, clientes[linha].telefone,
                         sizeof clientes[linha].telefone, erro, tamanhoErro)) {
            PQclear(resultado);
            return false;
        }
    }
    *totalClientes = linhas;
    PQclear(resultado);

    if (!executarConsulta(
            "SELECT id_equipamento, descricao, tipo, id_cliente "
            "FROM equipamento ORDER BY id_equipamento",
            0, NULL, &resultado, erro, tamanhoErro,
            "Falha ao carregar equipamentos.")) {
        return false;
    }
    linhas = PQntuples(resultado);
    if (linhas > MAX) {
        snprintf(erro, tamanhoErro,
                 "Ha mais de %d equipamentos. Aumente MAX antes de carregar os dados.",
                 MAX);
        PQclear(resultado);
        return false;
    }
    for (linha = 0; linha < linhas; linha++) {
        if (!valorInteiro(resultado, linha, 0,
                          &equipamentos[linha].idEquipamento, erro,
                          tamanhoErro) ||
            !copiarCampo(resultado, linha, 1, equipamentos[linha].descricao,
                         sizeof equipamentos[linha].descricao, erro,
                         tamanhoErro) ||
            !copiarCampo(resultado, linha, 2, equipamentos[linha].tipo,
                         sizeof equipamentos[linha].tipo, erro, tamanhoErro) ||
            !valorInteiro(resultado, linha, 3,
                          &equipamentos[linha].idCliente, erro, tamanhoErro)) {
            PQclear(resultado);
            return false;
        }
    }
    *totalEquipamentos = linhas;
    PQclear(resultado);

    if (!executarConsulta(
            "SELECT id_funcionario, nome, funcao FROM funcionario "
            "ORDER BY id_funcionario",
            0, NULL, &resultado, erro, tamanhoErro,
            "Falha ao carregar funcionarios.")) {
        return false;
    }
    linhas = PQntuples(resultado);
    if (linhas > MAX) {
        snprintf(erro, tamanhoErro,
                 "Ha mais de %d funcionarios. Aumente MAX antes de carregar os dados.",
                 MAX);
        PQclear(resultado);
        return false;
    }
    for (linha = 0; linha < linhas; linha++) {
        if (!valorInteiro(resultado, linha, 0,
                          &funcionarios[linha].idFuncionario, erro,
                          tamanhoErro) ||
            !copiarCampo(resultado, linha, 1, funcionarios[linha].nome,
                         sizeof funcionarios[linha].nome, erro, tamanhoErro) ||
            !copiarCampo(resultado, linha, 2, funcionarios[linha].funcao,
                         sizeof funcionarios[linha].funcao, erro, tamanhoErro)) {
            PQclear(resultado);
            return false;
        }
    }
    *totalFuncionarios = linhas;
    PQclear(resultado);

    if (!executarConsulta(
            "SELECT id_os, data_abertura::text, status, id_cliente, "
            "id_equipamento, id_funcionario FROM ordem_servico ORDER BY id_os",
            0, NULL, &resultado, erro, tamanhoErro,
            "Falha ao carregar ordens de servico.")) {
        return false;
    }
    linhas = PQntuples(resultado);
    if (linhas > MAX) {
        snprintf(erro, tamanhoErro,
                 "Ha mais de %d ordens de servico. Aumente MAX para carrega-las.",
                 MAX);
        PQclear(resultado);
        return false;
    }
    for (linha = 0; linha < linhas; linha++) {
        if (!valorInteiro(resultado, linha, 0, &ordens[linha].idOS, erro,
                          tamanhoErro) ||
            !copiarCampo(resultado, linha, 1, ordens[linha].dataAbertura,
                         sizeof ordens[linha].dataAbertura, erro, tamanhoErro) ||
            !copiarCampo(resultado, linha, 2, ordens[linha].status,
                         sizeof ordens[linha].status, erro, tamanhoErro) ||
            !valorInteiro(resultado, linha, 3, &ordens[linha].idCliente, erro,
                          tamanhoErro) ||
            !valorInteiro(resultado, linha, 4, &ordens[linha].idEquipamento,
                          erro, tamanhoErro) ||
            !valorInteiro(resultado, linha, 5, &ordens[linha].idFuncionario,
                          erro, tamanhoErro)) {
            PQclear(resultado);
            return false;
        }
    }
    *totalOrdens = linhas;
    PQclear(resultado);
    return true;
}

bool bancoCarregarManutencoes(Manutencao manutencoes[], int *total,
                             char *erro, size_t tamanhoErro) {
    PGresult *resultado;
    int linha;
    int linhas;

    *total = 0;
    if (!executarConsulta(
            "SELECT id_manutencao, descricao, data_manutencao::text, id_os "
            "FROM manutencao ORDER BY id_manutencao",
            0, NULL, &resultado, erro, tamanhoErro,
            "Falha ao carregar manutencoes.")) {
        return false;
    }

    linhas = PQntuples(resultado);
    if (linhas > MAX) {
        snprintf(erro, tamanhoErro,
                 "Ha mais de %d manutencoes. Aumente MAX antes de carregar os dados.",
                 MAX);
        PQclear(resultado);
        return false;
    }
    for (linha = 0; linha < linhas; linha++) {
        if (!valorInteiro(resultado, linha, 0,
                          &manutencoes[linha].idManutencao, erro,
                          tamanhoErro) ||
            !copiarCampo(resultado, linha, 1, manutencoes[linha].descricao,
                         sizeof manutencoes[linha].descricao, erro,
                         tamanhoErro) ||
            !copiarCampo(resultado, linha, 2,
                         manutencoes[linha].dataManutencao,
                         sizeof manutencoes[linha].dataManutencao, erro,
                         tamanhoErro) ||
            !valorInteiro(resultado, linha, 3, &manutencoes[linha].idOS, erro,
                          tamanhoErro)) {
            PQclear(resultado);
            return false;
        }
    }

    *total = linhas;
    PQclear(resultado);
    return true;
}

bool bancoCadastrarCliente(const char *nome, const char *telefone, int *id,
                           char *erro, size_t tamanhoErro) {
    const char *valores[] = {nome, telefone};
    PGresult *resultado;

    if (conexao == NULL) {
        snprintf(erro, tamanhoErro, "O banco de dados nao esta conectado.");
        return false;
    }
    resultado = PQexecParams(
        conexao,
        "INSERT INTO cliente (nome, telefone) VALUES ($1, $2) "
        "RETURNING id_cliente",
        2, NULL, valores, NULL, NULL, 0);
    return lerIdRetornado(resultado, id, erro, tamanhoErro,
                          "Falha ao cadastrar cliente.");
}

bool bancoCadastrarEquipamento(const char *descricao, const char *tipo,
                               int idCliente, int *id, char *erro,
                               size_t tamanhoErro) {
    char clienteId[16];
    const char *valores[] = {descricao, tipo, clienteId};
    PGresult *resultado;

    if (conexao == NULL) {
        snprintf(erro, tamanhoErro, "O banco de dados nao esta conectado.");
        return false;
    }
    snprintf(clienteId, sizeof clienteId, "%d", idCliente);
    resultado = PQexecParams(
        conexao,
        "INSERT INTO equipamento (descricao, tipo, id_cliente) "
        "VALUES ($1, $2, $3::integer) RETURNING id_equipamento",
        3, NULL, valores, NULL, NULL, 0);
    return lerIdRetornado(resultado, id, erro, tamanhoErro,
                          "Falha ao cadastrar equipamento.");
}

bool bancoCadastrarFuncionario(const char *nome, const char *funcao, int *id,
                               char *erro, size_t tamanhoErro) {
    const char *valores[] = {nome, funcao};
    PGresult *resultado;

    if (conexao == NULL) {
        snprintf(erro, tamanhoErro, "O banco de dados nao esta conectado.");
        return false;
    }
    resultado = PQexecParams(
        conexao,
        "INSERT INTO funcionario (nome, funcao) VALUES ($1, $2) "
        "RETURNING id_funcionario",
        2, NULL, valores, NULL, NULL, 0);
    return lerIdRetornado(resultado, id, erro, tamanhoErro,
                          "Falha ao cadastrar funcionario.");
}

bool bancoAbrirOrdem(int idCliente, int idEquipamento, int idFuncionario,
                     int *id, char *dataAbertura, size_t tamanhoData,
                     char *erro, size_t tamanhoErro) {
    char clienteId[16];
    char equipamentoId[16];
    char funcionarioId[16];
    const char *valores[] = {clienteId, equipamentoId, funcionarioId};
    PGresult *resultado;

    if (conexao == NULL) {
        snprintf(erro, tamanhoErro, "O banco de dados nao esta conectado.");
        return false;
    }
    snprintf(clienteId, sizeof clienteId, "%d", idCliente);
    snprintf(equipamentoId, sizeof equipamentoId, "%d", idEquipamento);
    snprintf(funcionarioId, sizeof funcionarioId, "%d", idFuncionario);
    resultado = PQexecParams(
        conexao,
        "INSERT INTO ordem_servico "
        "(data_abertura, status, id_cliente, id_equipamento, id_funcionario) "
        "SELECT CURRENT_DATE, 'Aberta', $1::integer, $2::integer, $3::integer "
        "WHERE EXISTS (SELECT 1 FROM equipamento "
        "WHERE id_equipamento = $2::integer AND id_cliente = $1::integer) "
        "RETURNING id_os, data_abertura::text",
        3, NULL, valores, NULL, NULL, 0);
    if (resultado != NULL &&
        PQresultStatus(resultado) == PGRES_TUPLES_OK &&
        PQntuples(resultado) == 0) {
        snprintf(erro, tamanhoErro,
                 "O equipamento selecionado nao pertence ao cliente informado.");
        PQclear(resultado);
        return false;
    }
    if (resultado == NULL ||
        PQresultStatus(resultado) != PGRES_TUPLES_OK ||
        PQntuples(resultado) != 1) {
        definirErro(erro, tamanhoErro, "Falha ao abrir ordem de servico.",
                    resultado);
        if (resultado != NULL) {
            PQclear(resultado);
        }
        return false;
    }

    *id = atoi(PQgetvalue(resultado, 0, 0));
    if (!copiarCampo(resultado, 0, 1, dataAbertura, tamanhoData, erro,
                     tamanhoErro)) {
        PQclear(resultado);
        return false;
    }
    PQclear(resultado);
    return true;
}

bool bancoRegistrarManutencao(const char *descricao, int idOS, int *id,
                              char *dataManutencao, size_t tamanhoData,
                              char *erro, size_t tamanhoErro) {
    char ordemId[16];
    const char *valores[] = {descricao, ordemId};
    PGresult *resultado;

    if (conexao == NULL) {
        snprintf(erro, tamanhoErro, "O banco de dados nao esta conectado.");
        return false;
    }
    snprintf(ordemId, sizeof ordemId, "%d", idOS);
    resultado = PQexecParams(
        conexao,
        "INSERT INTO manutencao (descricao, data_manutencao, id_os) "
        "VALUES ($1, CURRENT_DATE, $2::integer) "
        "RETURNING id_manutencao, data_manutencao::text",
        2, NULL, valores, NULL, NULL, 0);
    if (resultado == NULL ||
        PQresultStatus(resultado) != PGRES_TUPLES_OK ||
        PQntuples(resultado) != 1) {
        definirErro(erro, tamanhoErro, "Falha ao registrar manutencao.",
                    resultado);
        if (resultado != NULL) {
            PQclear(resultado);
        }
        return false;
    }

    *id = atoi(PQgetvalue(resultado, 0, 0));
    if (!copiarCampo(resultado, 0, 1, dataManutencao, tamanhoData, erro,
                     tamanhoErro)) {
        PQclear(resultado);
        return false;
    }
    PQclear(resultado);
    return true;
}

bool bancoAtualizarStatusOrdem(int id, const char *status, char *erro,
                               size_t tamanhoErro) {
    char textoId[16];
    const char *valores[] = {textoId, status};
    PGresult *resultado;
    const char *statusPermitidos[] = {
        "Aberta", "Em Andamento", "Concluida", "Cancelada"};
    size_t i;
    bool permitido = false;

    for (i = 0; i < sizeof statusPermitidos / sizeof statusPermitidos[0]; i++) {
        if (strcmp(status, statusPermitidos[i]) == 0) {
            permitido = true;
            break;
        }
    }
    if (!permitido) {
        snprintf(erro, tamanhoErro, "O status informado nao e permitido.");
        return false;
    }
    if (conexao == NULL) {
        snprintf(erro, tamanhoErro, "O banco de dados nao esta conectado.");
        return false;
    }

    snprintf(textoId, sizeof textoId, "%d", id);
    resultado = PQexecParams(
        conexao,
        "UPDATE ordem_servico SET status = $2 "
        "WHERE id_os = $1::integer RETURNING id_os",
        2, NULL, valores, NULL, NULL, 0);
    if (resultado == NULL ||
        PQresultStatus(resultado) != PGRES_TUPLES_OK) {
        definirErro(erro, tamanhoErro,
                    "Falha ao atualizar o status da ordem de servico.",
                    resultado);
        if (resultado != NULL) {
            PQclear(resultado);
        }
        return false;
    }
    if (PQntuples(resultado) != 1) {
        snprintf(erro, tamanhoErro,
                 "A ordem de servico nao foi encontrada no banco.");
        PQclear(resultado);
        return false;
    }

    PQclear(resultado);
    return true;
}

bool bancoExcluirCliente(int id, char *erro, size_t tamanhoErro) {
    return excluirRegistro(
        "DELETE FROM cliente WHERE id_cliente = $1::integer "
        "RETURNING id_cliente",
        id, erro, tamanhoErro);
}

bool bancoExcluirEquipamento(int id, char *erro, size_t tamanhoErro) {
    return excluirRegistro(
        "DELETE FROM equipamento WHERE id_equipamento = $1::integer "
        "RETURNING id_equipamento",
        id, erro, tamanhoErro);
}

bool bancoExcluirFuncionario(int id, char *erro, size_t tamanhoErro) {
    return excluirRegistro(
        "DELETE FROM funcionario WHERE id_funcionario = $1::integer "
        "RETURNING id_funcionario",
        id, erro, tamanhoErro);
}

bool bancoExcluirOrdem(int id, char *erro, size_t tamanhoErro) {
    return excluirRegistro(
        "DELETE FROM ordem_servico WHERE id_os = $1::integer RETURNING id_os",
        id, erro, tamanhoErro);
}

bool bancoExcluirManutencao(int id, char *erro, size_t tamanhoErro) {
    return excluirRegistro(
        "DELETE FROM manutencao WHERE id_manutencao = $1::integer "
        "RETURNING id_manutencao",
        id, erro, tamanhoErro);
}
