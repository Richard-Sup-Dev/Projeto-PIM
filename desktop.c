#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0600
#include <windows.h>
#include <commctrl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "banco.h"
#include "modelos.h"

#define APP_CLASS "SIGInovaLogDesktop"
#define SIDEBAR_WIDTH 232
#define ID_NAV_DASHBOARD 101
#define ID_NAV_CLIENTES 102
#define ID_NAV_EQUIPAMENTOS 103
#define ID_NAV_ORDENS 104
#define ID_NAV_FUNCIONARIOS 105
#define ID_NAV_MANUTENCOES 106
#define ID_ACTION_PRIMARY 201
#define ID_ACTION_SECONDARY 202
#define ID_ACTION_TERTIARY 203
#define ID_ACTION_DELETE 204
#define ID_ACTION_STATUS 205
#define ID_STATUS_LABEL 206
#define ID_FIELD_FIRST 211
#define ID_FIELD_SECOND 212
#define ID_FIELD_THIRD 213
#define ID_TABLE 300
#define ID_STATUS_COMBO 310

enum {
    PAGINA_INICIO,
    PAGINA_CLIENTES,
    PAGINA_EQUIPAMENTOS,
    PAGINA_ORDENS,
    PAGINA_FUNCIONARIOS,
    PAGINA_MANUTENCOES
};

static Cliente clientes[MAX];
static Equipamento equipamentos[MAX];
static Funcionario funcionarios[MAX];
static OrdemServico ordens[MAX];
static Manutencao manutencoes[MAX];
static int totalClientes;
static int totalEquipamentos;
static int totalFuncionarios;
static int totalOrdens;
static int totalManutencoes;
static int paginaAtual;
static bool formularioAberto;
static HWND tabela;
static HWND campoPrimeiro;
static HWND campoSegundo;
static HWND campoTerceiro;
static HWND campoStatus;
static HFONT fontePadrao;
static HFONT fonteTitulo;
static HFONT fontePequena;
static HFONT fonteBotao;
static char aviso[180] = "Conectado ao banco PostgreSQL.";

static const COLORREF COR_FUNDO = RGB(246, 248, 251);
static const COLORREF COR_BRANCO = RGB(255, 255, 255);
static const COLORREF COR_TEXTO = RGB(35, 48, 65);
static const COLORREF COR_SECUNDARIA = RGB(111, 125, 143);
static const COLORREF COR_MENU = RGB(25, 39, 57);
static const COLORREF COR_MENU_SELECIONADO = RGB(39, 120, 115);
static const COLORREF COR_DESTAQUE = RGB(26, 142, 132);
static const COLORREF COR_BORDA = RGB(228, 233, 239);

static void desenharTexto(HDC dc, const char *texto, RECT area, HFONT fonte,
                          COLORREF cor, UINT alinhamento) {
    HFONT fonteAnterior = (HFONT)SelectObject(dc, fonte);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, cor);
    DrawTextA(dc, texto, -1, &area, alinhamento | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, fonteAnterior);
}

static void desenharBloco(HDC dc, RECT area, COLORREF cor, COLORREF borda) {
    HBRUSH pincel = CreateSolidBrush(cor);
    HPEN caneta = CreatePen(PS_SOLID, 1, borda);
    HGDIOBJ pincelAnterior = SelectObject(dc, pincel);
    HGDIOBJ canetaAnterior = SelectObject(dc, caneta);

    RoundRect(dc, area.left, area.top, area.right, area.bottom, 14, 14);
    SelectObject(dc, pincelAnterior);
    SelectObject(dc, canetaAnterior);
    DeleteObject(pincel);
    DeleteObject(caneta);
}

static void desenharCartao(HDC dc, int x, int y, int largura, const char *rotulo,
                           int valor, const char *rodape) {
    RECT cartao = {x, y, x + largura, y + 120};
    RECT texto = {x + 20, y + 17, x + largura - 18, y + 43};
    RECT numero = {x + 20, y + 42, x + largura - 18, y + 91};
    RECT detalhe = {x + 20, y + 91, x + largura - 18, y + 112};
    char valorTexto[24];

    desenharBloco(dc, cartao, COR_BRANCO, COR_BORDA);
    desenharTexto(dc, rotulo, texto, fontePequena, COR_SECUNDARIA, DT_LEFT);
    snprintf(valorTexto, sizeof valorTexto, "%d", valor);
    desenharTexto(dc, valorTexto, numero, fonteTitulo, COR_TEXTO, DT_LEFT);
    desenharTexto(dc, rodape, detalhe, fontePequena, COR_SECUNDARIA, DT_LEFT);
}

static void criarBotao(HWND janela, const char *texto, int id, int x, int y,
                       int largura, int altura) {
    HWND botao = CreateWindowA(
        "BUTTON", texto,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, x, y, largura,
        altura, janela, (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);
    SendMessageA(botao, WM_SETFONT, (WPARAM)fontePadrao, TRUE);
}

static HWND criarCampo(HWND janela, const char *rotulo, int id, int x, int y,
                       int largura, int limite) {
    HWND etiqueta = CreateWindowA("STATIC", rotulo,
                                  WS_CHILD | WS_VISIBLE | SS_LEFT, x, y,
                                  largura, 22, janela,
                                  (HMENU)(INT_PTR)(id + 100), NULL, NULL);
    HWND campo = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, x, y + 27,
        largura, 36, janela, (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);

    SendMessageA(etiqueta, WM_SETFONT, (WPARAM)fontePequena, TRUE);
    SendMessageA(campo, EM_LIMITTEXT, (WPARAM)limite, 0);
    SendMessageA(campo, WM_SETFONT, (WPARAM)fontePadrao, TRUE);
    return campo;
}

static HWND criarCampoMultilinha(HWND janela, const char *rotulo, int id,
                                 int x, int y, int largura, int altura) {
    HWND etiqueta = CreateWindowA("STATIC", rotulo,
                                  WS_CHILD | WS_VISIBLE | SS_LEFT, x, y,
                                  largura, 22, janela,
                                  (HMENU)(INT_PTR)(id + 100), NULL, NULL);
    HWND campo = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL |
            WS_VSCROLL,
        x, y + 27, largura, altura, janela, (HMENU)(INT_PTR)id,
        GetModuleHandleA(NULL), NULL);

    SendMessageA(etiqueta, WM_SETFONT, (WPARAM)fontePequena, TRUE);
    SendMessageA(campo, EM_LIMITTEXT, TAM_MANUTENCAO - 1, 0);
    SendMessageA(campo, WM_SETFONT, (WPARAM)fontePadrao, TRUE);
    return campo;
}

static void adicionarColuna(const char *rotulo, int indice, int largura) {
    LVCOLUMNA coluna;

    ZeroMemory(&coluna, sizeof coluna);
    coluna.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    coluna.pszText = (LPSTR)rotulo;
    coluna.cx = largura;
    coluna.iSubItem = indice;
    ListView_InsertColumn(tabela, indice, &coluna);
}

static void adicionarCelula(int linha, int coluna, const char *texto) {
    if (coluna == 0) {
        LVITEMA item;
        ZeroMemory(&item, sizeof item);
        item.mask = LVIF_TEXT;
        item.iItem = linha;
        item.pszText = (LPSTR)texto;
        ListView_InsertItem(tabela, &item);
    } else {
        ListView_SetItemText(tabela, linha, coluna, (LPSTR)texto);
    }
}

static void preencherTabela(void) {
    int i;
    int inicio = 0;
    char id[24];
    char clienteId[TAM_NOME + 24];
    char equipamentoId[TAM_DESCRICAO + 24];

    if (tabela == NULL) {
        return;
    }

    ListView_DeleteAllItems(tabela);
    while (Header_GetItemCount(ListView_GetHeader(tabela)) > 0) {
        ListView_DeleteColumn(tabela, 0);
    }

    if (paginaAtual == PAGINA_CLIENTES) {
        adicionarColuna("ID", 0, 80);
        adicionarColuna("Nome do cliente", 1, 420);
        adicionarColuna("Telefone", 2, 220);
        for (i = 0; i < totalClientes; i++) {
            snprintf(id, sizeof id, "%d", clientes[i].idCliente);
            adicionarCelula(i, 0, id);
            adicionarCelula(i, 1, clientes[i].nome);
            adicionarCelula(i, 2, clientes[i].telefone);
        }
    } else if (paginaAtual == PAGINA_EQUIPAMENTOS) {
        adicionarColuna("ID", 0, 80);
        adicionarColuna("Descricao do equipamento", 1, 390);
        adicionarColuna("Tipo", 2, 220);
        adicionarColuna("Cliente", 3, 240);
        for (i = 0; i < totalEquipamentos; i++) {
            int j;
            const char *nomeCliente = "Cliente nao localizado";
            snprintf(id, sizeof id, "%d", equipamentos[i].idEquipamento);
            for (j = 0; j < totalClientes; j++) {
                if (clientes[j].idCliente == equipamentos[i].idCliente) {
                    nomeCliente = clientes[j].nome;
                    break;
                }
            }
            adicionarCelula(i, 0, id);
            adicionarCelula(i, 1, equipamentos[i].descricao);
            adicionarCelula(i, 2, equipamentos[i].tipo);
            adicionarCelula(i, 3, (char *)nomeCliente);
        }
    } else if (paginaAtual == PAGINA_FUNCIONARIOS) {
        adicionarColuna("ID", 0, 80);
        adicionarColuna("Nome do funcionario", 1, 420);
        adicionarColuna("Funcao", 2, 280);
        for (i = 0; i < totalFuncionarios; i++) {
            snprintf(id, sizeof id, "%d", funcionarios[i].idFuncionario);
            adicionarCelula(i, 0, id);
            adicionarCelula(i, 1, funcionarios[i].nome);
            adicionarCelula(i, 2, funcionarios[i].funcao);
        }
    } else if (paginaAtual == PAGINA_MANUTENCOES) {
        adicionarColuna("ID", 0, 80);
        adicionarColuna("Descricao", 1, 420);
        adicionarColuna("Data", 2, 130);
        adicionarColuna("OS", 3, 100);
        for (i = 0; i < totalManutencoes; i++) {
            snprintf(id, sizeof id, "%d", manutencoes[i].idManutencao);
            adicionarCelula(i, 0, id);
            adicionarCelula(i, 1, manutencoes[i].descricao);
            adicionarCelula(i, 2, manutencoes[i].dataManutencao);
            snprintf(clienteId, sizeof clienteId, "%d", manutencoes[i].idOS);
            adicionarCelula(i, 3, clienteId);
        }
    } else {
        adicionarColuna("OS", 0, 70);
        adicionarColuna("Cliente", 1, 230);
        adicionarColuna("Equipamento", 2, 285);
        adicionarColuna("Tecnico", 3, 200);
        adicionarColuna("Data", 4, 110);
        adicionarColuna("Status", 5, 150);
        if (paginaAtual == PAGINA_INICIO && totalOrdens > 5) {
            inicio = totalOrdens - 5;
        }
        for (i = inicio; i < totalOrdens; i++) {
            int linha = i - inicio;
            int j;
            const char *nomeCliente = "Cliente nao localizado";
            const char *descricaoEquipamento = "Equipamento nao localizado";
            const char *nomeFuncionario = "Tecnico nao localizado";

            for (j = 0; j < totalClientes; j++) {
                if (clientes[j].idCliente == ordens[i].idCliente) {
                    nomeCliente = clientes[j].nome;
                    break;
                }
            }
            for (j = 0; j < totalEquipamentos; j++) {
                if (equipamentos[j].idEquipamento ==
                    ordens[i].idEquipamento) {
                    descricaoEquipamento = equipamentos[j].descricao;
                    break;
                }
            }
            for (j = 0; j < totalFuncionarios; j++) {
                if (funcionarios[j].idFuncionario ==
                    ordens[i].idFuncionario) {
                    nomeFuncionario = funcionarios[j].nome;
                    break;
                }
            }

            snprintf(id, sizeof id, "%d", ordens[i].idOS);
            snprintf(clienteId, sizeof clienteId, "%s (#%d)", nomeCliente,
                     ordens[i].idCliente);
            snprintf(equipamentoId, sizeof equipamentoId, "%s (#%d)",
                     descricaoEquipamento, ordens[i].idEquipamento);
            adicionarCelula(linha, 0, id);
            adicionarCelula(linha, 1, clienteId);
            adicionarCelula(linha, 2, equipamentoId);
            adicionarCelula(linha, 3, (char *)nomeFuncionario);
            adicionarCelula(linha, 4, ordens[i].dataAbertura);
            adicionarCelula(linha, 5, ordens[i].status);
        }
    }
}

static void limparControlesPainel(HWND janela) {
    HWND controle = GetWindow(janela, GW_CHILD);

    while (controle != NULL) {
        HWND proximo = GetWindow(controle, GW_HWNDNEXT);
        int id = GetDlgCtrlID(controle);

        if (id >= 200) {
            DestroyWindow(controle);
        }
        controle = proximo;
    }

    tabela = NULL;
    campoPrimeiro = NULL;
    campoSegundo = NULL;
    campoTerceiro = NULL;
    campoStatus = NULL;
}

static void configurarTabela(HWND janela, int x, int y, int largura,
                             int altura) {
    tabela = CreateWindowExA(
        WS_EX_CLIENTEDGE, WC_LISTVIEWA, "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL |
            LVS_SHOWSELALWAYS,
        x, y, largura, altura, janela, (HMENU)(INT_PTR)ID_TABLE,
        GetModuleHandleA(NULL), NULL);
    ListView_SetExtendedListViewStyle(
        tabela, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    SendMessageA(tabela, WM_SETFONT, (WPARAM)fontePadrao, TRUE);
    preencherTabela();
}

static void criarCombo(HWND janela, const char *rotulo, int id, int x, int y,
                       int largura) {
    HWND etiqueta = CreateWindowA("STATIC", rotulo,
                                  WS_CHILD | WS_VISIBLE | SS_LEFT, x, y,
                                  largura, 22, janela,
                                  (HMENU)(INT_PTR)(id + 100), NULL, NULL);
    HWND combo = CreateWindowA(
        "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        x, y + 27, largura, 180, janela, (HMENU)(INT_PTR)id,
        GetModuleHandleA(NULL), NULL);
    int i;
    char opcao[TAM_DESCRICAO + 24];

    SendMessageA(etiqueta, WM_SETFONT, (WPARAM)fontePequena, TRUE);
    SendMessageA(combo, WM_SETFONT, (WPARAM)fontePadrao, TRUE);
    if (paginaAtual == PAGINA_EQUIPAMENTOS && id == ID_FIELD_THIRD) {
        for (i = 0; i < totalClientes; i++) {
            snprintf(opcao, sizeof opcao, "%d - %s", clientes[i].idCliente,
                     clientes[i].nome);
            SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)opcao);
        }
    } else if (paginaAtual == PAGINA_ORDENS && id == ID_FIELD_FIRST) {
        for (i = 0; i < totalClientes; i++) {
            snprintf(opcao, sizeof opcao, "%d - %s", clientes[i].idCliente,
                     clientes[i].nome);
            SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)opcao);
        }
    } else if (paginaAtual == PAGINA_ORDENS && id == ID_FIELD_SECOND) {
        for (i = 0; i < totalEquipamentos; i++) {
            snprintf(opcao, sizeof opcao, "%d - %s",
                     equipamentos[i].idEquipamento,
                     equipamentos[i].descricao);
            SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)opcao);
        }
    } else if (paginaAtual == PAGINA_ORDENS && id == ID_FIELD_THIRD) {
        for (i = 0; i < totalFuncionarios; i++) {
            snprintf(opcao, sizeof opcao, "%d - %s",
                     funcionarios[i].idFuncionario, funcionarios[i].nome);
            SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)opcao);
        }
    } else if (paginaAtual == PAGINA_MANUTENCOES) {
        for (i = 0; i < totalOrdens; i++) {
            snprintf(opcao, sizeof opcao, "%d - OS %d (%s)",
                     ordens[i].idOS, ordens[i].idOS, ordens[i].status);
            SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)opcao);
        }
    } else {
        for (i = 0; i < totalFuncionarios; i++) {
            snprintf(opcao, sizeof opcao, "%d - %s",
                     funcionarios[i].idFuncionario, funcionarios[i].nome);
            SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)opcao);
        }
    }
    if (SendMessageA(combo, CB_GETCOUNT, 0, 0) > 0) {
        SendMessageA(combo, CB_SETCURSEL, 0, 0);
    }

    if (id == ID_FIELD_FIRST) {
        campoPrimeiro = combo;
    } else if (id == ID_FIELD_SECOND) {
        campoSegundo = combo;
    } else {
        campoTerceiro = combo;
    }
}

static void criarFormulario(HWND janela) {
    int largura;
    RECT area;

    GetClientRect(janela, &area);
    largura = area.right - SIDEBAR_WIDTH - 72;
    if (largura > 670) {
        largura = 670;
    }

    if (paginaAtual == PAGINA_CLIENTES) {
        campoPrimeiro = criarCampo(janela, "Nome do cliente",
                                   ID_FIELD_FIRST, 280, 228, largura, 100);
        campoSegundo = criarCampo(janela, "Telefone", ID_FIELD_SECOND, 280,
                                  328, largura, 20);
    } else if (paginaAtual == PAGINA_FUNCIONARIOS) {
        campoPrimeiro = criarCampo(janela, "Nome do funcionario",
                                   ID_FIELD_FIRST, 280, 228, largura, 100);
        campoSegundo = criarCampo(janela, "Funcao", ID_FIELD_SECOND, 280, 328,
                                  largura, 50);
    } else if (paginaAtual == PAGINA_EQUIPAMENTOS) {
        campoPrimeiro = criarCampo(janela, "Descricao do equipamento",
                                   ID_FIELD_FIRST, 280, 228, largura, 255);
        campoSegundo = criarCampo(janela, "Tipo do equipamento",
                                  ID_FIELD_SECOND, 280, 328, largura, 50);
        criarCombo(janela, "Cliente proprietario", ID_FIELD_THIRD, 280, 408,
                   largura);
    } else if (paginaAtual == PAGINA_MANUTENCOES) {
        criarCombo(janela, "Ordem de servico", ID_FIELD_FIRST, 280, 218,
                   largura);
        campoSegundo = criarCampoMultilinha(
            janela, "Descricao da manutencao", ID_FIELD_SECOND, 280, 296,
            largura, 120);
    } else {
        criarCombo(janela, "Cliente responsavel", ID_FIELD_FIRST, 280, 218,
                   largura);
        criarCombo(janela, "Equipamento", ID_FIELD_SECOND, 280, 296, largura);
        criarCombo(janela, "Funcionario responsavel", ID_FIELD_THIRD, 280,
                   374, largura);
    }

    criarBotao(janela,
               paginaAtual == PAGINA_ORDENS ? "Abrir ordem" :
               paginaAtual == PAGINA_MANUTENCOES ? "Registrar manutencao" :
                                                   "Salvar cadastro",
               ID_ACTION_SECONDARY, 280, 490, 160, 42);
    criarBotao(janela, "Cancelar", ID_ACTION_TERTIARY, 454, 490, 120, 42);
}

static void criarControlesPainel(HWND janela) {
    RECT area;
    int largura;

    GetClientRect(janela, &area);
    largura = area.right - SIDEBAR_WIDTH - 56;

    if (formularioAberto) {
        criarFormulario(janela);
        return;
    }

    if (paginaAtual == PAGINA_INICIO) {
        criarBotao(janela, "Novo cliente", ID_ACTION_PRIMARY, 260, 354, 150, 42);
        criarBotao(janela, "Novo equipamento", ID_ACTION_SECONDARY, 425, 354,
                   180, 42);
        criarBotao(janela, "Abrir ordem", ID_ACTION_TERTIARY, 620, 354, 150,
                   42);
        configurarTabela(janela, 260, 444, largura, area.bottom - 466);
    } else if (paginaAtual == PAGINA_ORDENS) {
        HWND etiqueta;
        int i;
        const char *status[] = {"Aberta", "Em Andamento", "Concluida",
                                "Cancelada"};

        criarBotao(janela, "Excluir selecionado", ID_ACTION_DELETE, 260, 100,
                   180, 42);
        criarBotao(janela, "Abrir ordem", ID_ACTION_PRIMARY,
                   area.right - 220, 100, 180, 42);
        etiqueta = CreateWindowA("STATIC", "Novo status da OS",
                                 WS_CHILD | WS_VISIBLE | SS_LEFT, 260, 151,
                                 220, 20, janela,
                                 (HMENU)(INT_PTR)ID_STATUS_LABEL, NULL, NULL);
        SendMessageA(etiqueta, WM_SETFONT, (WPARAM)fontePequena, TRUE);
        campoStatus = CreateWindowA(
            "COMBOBOX", "",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            260, 174, 220, 132, janela, (HMENU)(INT_PTR)ID_STATUS_COMBO,
            GetModuleHandleA(NULL), NULL);
        SendMessageA(campoStatus, WM_SETFONT, (WPARAM)fontePadrao, TRUE);
        for (i = 0; i < (int)(sizeof status / sizeof status[0]); i++) {
            SendMessageA(campoStatus, CB_ADDSTRING, 0, (LPARAM)status[i]);
        }
        criarBotao(janela, "Atualizar status", ID_ACTION_STATUS, 490, 170,
                   180, 42);
        configurarTabela(janela, 260, 220, largura, area.bottom - 245);
    } else {
        criarBotao(janela, "Excluir selecionado", ID_ACTION_DELETE,
                   area.right - 420, 100, 180, 42);
        criarBotao(janela,
                   paginaAtual == PAGINA_ORDENS
                       ? "Abrir ordem"
                       : paginaAtual == PAGINA_MANUTENCOES
                             ? "Registrar manutencao"
                             : "Novo cadastro",
                   ID_ACTION_PRIMARY, area.right - 220, 100, 180, 42);
        configurarTabela(janela, 260, 165, largura, area.bottom - 190);
    }
}

static void removerItemVetor(void *vetor, size_t tamanhoItem, int indice,
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

static void excluirSelecionado(HWND janela) {
    char textoId[32] = "";
    char erro[256] = "";
    char confirmacao[160];
    char *fim;
    long id;
    int linha;
    int indice;
    int resposta;
    bool sucesso = false;
    const char *tipo;

    if (tabela == NULL) {
        return;
    }
    linha = ListView_GetNextItem(tabela, -1, LVNI_SELECTED);
    if (linha < 0) {
        MessageBoxA(janela, "Selecione um registro na tabela primeiro.",
                    "Nenhum registro selecionado",
                    MB_OK | MB_ICONINFORMATION);
        return;
    }

    ListView_GetItemText(tabela, linha, 0, textoId, sizeof textoId);
    id = strtol(textoId, &fim, 10);
    if (textoId == fim || *fim != '\0' || id < 1 || id > INT_MAX) {
        MessageBoxA(janela, "O identificador selecionado e invalido.",
                    "Erro ao excluir", MB_OK | MB_ICONERROR);
        return;
    }

    switch (paginaAtual) {
        case PAGINA_CLIENTES:
            tipo = "cliente";
            break;
        case PAGINA_EQUIPAMENTOS:
            tipo = "equipamento";
            break;
        case PAGINA_FUNCIONARIOS:
            tipo = "funcionario";
            break;
        case PAGINA_ORDENS:
            tipo = "ordem de servico";
            break;
        case PAGINA_MANUTENCOES:
            tipo = "manutencao";
            break;
        default:
            return;
    }

    snprintf(confirmacao, sizeof confirmacao,
             "Deseja excluir %s ID %ld do banco de dados? Esta acao nao pode "
             "ser desfeita.",
             tipo, id);
    resposta = MessageBoxA(janela, confirmacao, "Confirmar exclusao",
                          MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    if (resposta != IDYES) {
        return;
    }

    if (paginaAtual == PAGINA_CLIENTES) {
        for (indice = 0; indice < totalClientes; indice++) {
            if (clientes[indice].idCliente == id) {
                sucesso = bancoExcluirCliente((int)id, erro, sizeof erro);
                if (sucesso) {
                    removerItemVetor(clientes, sizeof clientes[0], indice,
                                     &totalClientes);
                }
                break;
            }
        }
    } else if (paginaAtual == PAGINA_EQUIPAMENTOS) {
        for (indice = 0; indice < totalEquipamentos; indice++) {
            if (equipamentos[indice].idEquipamento == id) {
                sucesso =
                    bancoExcluirEquipamento((int)id, erro, sizeof erro);
                if (sucesso) {
                    removerItemVetor(equipamentos, sizeof equipamentos[0],
                                     indice, &totalEquipamentos);
                }
                break;
            }
        }
    } else if (paginaAtual == PAGINA_FUNCIONARIOS) {
        for (indice = 0; indice < totalFuncionarios; indice++) {
            if (funcionarios[indice].idFuncionario == id) {
                sucesso =
                    bancoExcluirFuncionario((int)id, erro, sizeof erro);
                if (sucesso) {
                    removerItemVetor(funcionarios, sizeof funcionarios[0],
                                     indice, &totalFuncionarios);
                }
                break;
            }
        }
    } else if (paginaAtual == PAGINA_ORDENS) {
        for (indice = 0; indice < totalOrdens; indice++) {
            if (ordens[indice].idOS == id) {
                sucesso = bancoExcluirOrdem((int)id, erro, sizeof erro);
                if (sucesso) {
                    removerItemVetor(ordens, sizeof ordens[0], indice,
                                     &totalOrdens);
                }
                break;
            }
        }
    } else if (paginaAtual == PAGINA_MANUTENCOES) {
        for (indice = 0; indice < totalManutencoes; indice++) {
            if (manutencoes[indice].idManutencao == id) {
                sucesso =
                    bancoExcluirManutencao((int)id, erro, sizeof erro);
                if (sucesso) {
                    removerItemVetor(manutencoes, sizeof manutencoes[0],
                                     indice, &totalManutencoes);
                }
                break;
            }
        }
    }

    if (!sucesso) {
        if (erro[0] == '\0') {
            snprintf(erro, sizeof erro,
                     "O registro nao foi encontrado na lista carregada.");
        }
        MessageBoxA(janela, erro, "Nao foi possivel excluir",
                    MB_OK | MB_ICONERROR);
        return;
    }

    snprintf(aviso, sizeof aviso, "Registro excluido do banco de dados.");
    preencherTabela();
    InvalidateRect(janela, NULL, TRUE);
}

static void atualizarStatusSelecionado(HWND janela) {
    char textoId[32] = "";
    char erro[256];
    char *fim;
    long id;
    LRESULT selecao;
    int linha;
    int indice;
    static const char *const status[] = {
        "Aberta", "Em Andamento", "Concluida", "Cancelada"};

    if (paginaAtual != PAGINA_ORDENS || tabela == NULL ||
        campoStatus == NULL) {
        return;
    }
    linha = ListView_GetNextItem(tabela, -1, LVNI_SELECTED);
    if (linha < 0) {
        MessageBoxA(janela, "Selecione uma ordem de servico na tabela.",
                    "Nenhuma OS selecionada",
                    MB_OK | MB_ICONINFORMATION);
        return;
    }
    selecao = SendMessageA(campoStatus, CB_GETCURSEL, 0, 0);
    if (selecao == CB_ERR ||
        selecao >= (LRESULT)(sizeof status / sizeof status[0])) {
        MessageBoxA(janela, "Selecione o novo status da OS.",
                    "Status necessario", MB_OK | MB_ICONINFORMATION);
        return;
    }

    ListView_GetItemText(tabela, linha, 0, textoId, sizeof textoId);
    id = strtol(textoId, &fim, 10);
    if (textoId == fim || *fim != '\0' || id < 1 || id > INT_MAX) {
        MessageBoxA(janela, "O identificador selecionado e invalido.",
                    "Erro ao atualizar status", MB_OK | MB_ICONERROR);
        return;
    }
    for (indice = 0; indice < totalOrdens; indice++) {
        if (ordens[indice].idOS == id) {
            break;
        }
    }
    if (indice == totalOrdens) {
        MessageBoxA(janela, "A ordem selecionada nao foi encontrada.",
                    "Erro ao atualizar status", MB_OK | MB_ICONERROR);
        return;
    }
    if (!bancoAtualizarStatusOrdem((int)id, status[selecao], erro,
                                   sizeof erro)) {
        MessageBoxA(janela, erro, "Falha ao atualizar status",
                    MB_OK | MB_ICONERROR);
        return;
    }

    snprintf(ordens[indice].status, sizeof ordens[indice].status, "%s",
             status[selecao]);
    snprintf(aviso, sizeof aviso, "Status da OS atualizado.");
    preencherTabela();
    InvalidateRect(janela, NULL, TRUE);
}

static void mudarPagina(HWND janela, int pagina) {
    paginaAtual = pagina;
    formularioAberto = false;
    limparControlesPainel(janela);
    criarControlesPainel(janela);
    InvalidateRect(janela, NULL, TRUE);
}

static void abrirFormularioOrdem(HWND janela) {
    if (totalClientes == 0 || totalEquipamentos == 0 ||
        totalFuncionarios == 0) {
        MessageBoxA(
            janela,
            "Cadastre clientes e equipamentos e inclua ao menos um funcionario na tabela funcionario do banco antes de abrir uma OS.",
            "Cadastros necessarios", MB_OK | MB_ICONINFORMATION);
        return;
    }

    paginaAtual = PAGINA_ORDENS;
    formularioAberto = true;
    limparControlesPainel(janela);
    criarControlesPainel(janela);
    InvalidateRect(janela, NULL, TRUE);
}

static void abrirFormularioManutencao(HWND janela) {
    if (totalOrdens == 0) {
        MessageBoxA(janela,
                    "Abra uma ordem de servico antes de registrar uma manutencao.",
                    "Cadastro necessario", MB_OK | MB_ICONINFORMATION);
        return;
    }

    paginaAtual = PAGINA_MANUTENCOES;
    formularioAberto = true;
    limparControlesPainel(janela);
    criarControlesPainel(janela);
    InvalidateRect(janela, NULL, TRUE);
}

static bool textoNaoVazio(const char *texto) {
    while (*texto == ' ' || *texto == '\t') {
        texto++;
    }
    return *texto != '\0';
}

static void salvarFormulario(HWND janela) {
    char primeiro[TAM_MANUTENCAO];
    char segundo[TAM_MANUTENCAO];
    char erro[256];
    LRESULT selecao;
    int clienteIndex;
    int equipamentoIndex;
    int funcionarioIndex;
    int idCliente;
    int idEquipamento;
    int idFuncionario;

    if (campoPrimeiro == NULL || campoSegundo == NULL ||
        ((paginaAtual == PAGINA_ORDENS ||
          paginaAtual == PAGINA_EQUIPAMENTOS) &&
         campoTerceiro == NULL)) {
        return;
    }

    if (paginaAtual == PAGINA_MANUTENCOES) {
        int ordemIndex;

        selecao = SendMessageA(campoPrimeiro, CB_GETCURSEL, 0, 0);
        if (selecao == CB_ERR) {
            MessageBoxA(janela, "Selecione uma ordem de servico.",
                        "Cadastro necessario", MB_OK | MB_ICONINFORMATION);
            return;
        }
        GetWindowTextA(campoSegundo, primeiro, sizeof primeiro);
        if (!textoNaoVazio(primeiro)) {
            MessageBoxA(janela, "Informe a descricao da manutencao.",
                        "Dados incompletos", MB_OK | MB_ICONWARNING);
            return;
        }
        if (totalManutencoes >= MAX) {
            MessageBoxA(janela, "Limite de manutencoes atingido.",
                        "Limite atingido", MB_OK | MB_ICONWARNING);
            return;
        }
        ordemIndex = (int)selecao;
        if (!bancoRegistrarManutencao(
                primeiro, ordens[ordemIndex].idOS,
                &manutencoes[totalManutencoes].idManutencao,
                manutencoes[totalManutencoes].dataManutencao,
                sizeof manutencoes[totalManutencoes].dataManutencao, erro,
                sizeof erro)) {
            MessageBoxA(janela, erro, "Erro ao registrar manutencao",
                        MB_OK | MB_ICONERROR);
            return;
        }
        strcpy(manutencoes[totalManutencoes].descricao, primeiro);
        manutencoes[totalManutencoes].idOS = ordens[ordemIndex].idOS;
        totalManutencoes++;
        strcpy(aviso, "Manutencao registrada com sucesso.");
    } else if (paginaAtual == PAGINA_ORDENS) {
        selecao = SendMessageA(campoPrimeiro, CB_GETCURSEL, 0, 0);
        if (selecao == CB_ERR) {
            MessageBoxA(janela, "Selecione um cliente cadastrado.",
                        "Cadastro necessario", MB_OK | MB_ICONINFORMATION);
            return;
        }
        clienteIndex = (int)selecao;
        selecao = SendMessageA(campoSegundo, CB_GETCURSEL, 0, 0);
        if (selecao == CB_ERR) {
            MessageBoxA(janela, "Selecione um equipamento cadastrado.",
                        "Cadastro necessario", MB_OK | MB_ICONINFORMATION);
            return;
        }
        equipamentoIndex = (int)selecao;
        selecao = SendMessageA(campoTerceiro, CB_GETCURSEL, 0, 0);
        if (selecao == CB_ERR) {
            MessageBoxA(janela, "Cadastre um funcionario na tabela funcionario do banco.",
                        "Cadastro necessario", MB_OK | MB_ICONINFORMATION);
            return;
        }
        funcionarioIndex = (int)selecao;
        if (totalOrdens >= MAX) {
            MessageBoxA(janela, "Limite de ordens de servico atingido.",
                        "Limite atingido", MB_OK | MB_ICONWARNING);
            return;
        }

        idCliente = clientes[clienteIndex].idCliente;
        idEquipamento = equipamentos[equipamentoIndex].idEquipamento;
        idFuncionario = funcionarios[funcionarioIndex].idFuncionario;
        if (equipamentos[equipamentoIndex].idCliente != idCliente) {
            MessageBoxA(
                janela,
                "O equipamento selecionado nao pertence ao cliente informado.",
                "Vinculo invalido", MB_OK | MB_ICONWARNING);
            return;
        }
        if (!bancoAbrirOrdem(
                idCliente, idEquipamento, idFuncionario,
                &ordens[totalOrdens].idOS, ordens[totalOrdens].dataAbertura,
                sizeof ordens[totalOrdens].dataAbertura, erro, sizeof erro)) {
            MessageBoxA(janela, erro, "Erro ao abrir ordem",
                        MB_OK | MB_ICONERROR);
            return;
        }
        ordens[totalOrdens].idCliente = idCliente;
        ordens[totalOrdens].idEquipamento = idEquipamento;
        ordens[totalOrdens].idFuncionario = idFuncionario;
        strcpy(ordens[totalOrdens].status, "Aberta");
        totalOrdens++;
        strcpy(aviso, "Ordem de servico aberta com sucesso.");
    } else {
        GetWindowTextA(campoPrimeiro, primeiro, sizeof primeiro);
        GetWindowTextA(campoSegundo, segundo, sizeof segundo);
        if (!textoNaoVazio(primeiro) || !textoNaoVazio(segundo)) {
            MessageBoxA(janela, "Preencha todos os campos antes de salvar.",
                        "Dados incompletos", MB_OK | MB_ICONWARNING);
            return;
        }

        if (paginaAtual == PAGINA_CLIENTES) {
            if (totalClientes >= MAX) {
                MessageBoxA(janela, "Limite de clientes atingido.",
                            "Limite atingido", MB_OK | MB_ICONWARNING);
                return;
            }
            if (!bancoCadastrarCliente(primeiro, segundo,
                                       &clientes[totalClientes].idCliente,
                                       erro, sizeof erro)) {
                MessageBoxA(janela, erro, "Erro ao cadastrar cliente",
                            MB_OK | MB_ICONERROR);
                return;
            }
            strcpy(clientes[totalClientes].nome, primeiro);
            strcpy(clientes[totalClientes].telefone, segundo);
            totalClientes++;
            strcpy(aviso, "Cliente cadastrado com sucesso.");
        } else if (paginaAtual == PAGINA_FUNCIONARIOS) {
            if (totalFuncionarios >= MAX) {
                MessageBoxA(janela, "Limite de funcionarios atingido.",
                            "Limite atingido", MB_OK | MB_ICONWARNING);
                return;
            }
            if (!bancoCadastrarFuncionario(
                    primeiro, segundo,
                    &funcionarios[totalFuncionarios].idFuncionario, erro,
                    sizeof erro)) {
                MessageBoxA(janela, erro, "Erro ao cadastrar funcionario",
                            MB_OK | MB_ICONERROR);
                return;
            }
            strcpy(funcionarios[totalFuncionarios].nome, primeiro);
            strcpy(funcionarios[totalFuncionarios].funcao, segundo);
            totalFuncionarios++;
            strcpy(aviso, "Funcionario cadastrado com sucesso.");
        } else {
            if (totalEquipamentos >= MAX) {
                MessageBoxA(janela, "Limite de equipamentos atingido.",
                            "Limite atingido", MB_OK | MB_ICONWARNING);
                return;
            }
            selecao = SendMessageA(campoTerceiro, CB_GETCURSEL, 0, 0);
            if (selecao == CB_ERR) {
                MessageBoxA(janela, "Selecione o cliente proprietario.",
                            "Cadastro necessario", MB_OK | MB_ICONINFORMATION);
                return;
            }
            idCliente = clientes[(int)selecao].idCliente;
            if (!bancoCadastrarEquipamento(
                    primeiro, segundo, idCliente,
                    &equipamentos[totalEquipamentos].idEquipamento, erro,
                    sizeof erro)) {
                MessageBoxA(janela, erro, "Erro ao cadastrar equipamento",
                            MB_OK | MB_ICONERROR);
                return;
            }
            strcpy(equipamentos[totalEquipamentos].descricao, primeiro);
            strcpy(equipamentos[totalEquipamentos].tipo, segundo);
            equipamentos[totalEquipamentos].idCliente = idCliente;
            totalEquipamentos++;
            strcpy(aviso, "Equipamento cadastrado com sucesso.");
        }
    }

    formularioAberto = false;
    limparControlesPainel(janela);
    criarControlesPainel(janela);
    InvalidateRect(janela, NULL, TRUE);
}

static void desenharTela(HDC dc, HWND janela) {
    RECT cliente;
    RECT area;
    RECT titulo;
    RECT subtitulo;
    RECT avisoArea;
    RECT logotipo;
    RECT texto;
    RECT cartao;
    int larguraConteudo;
    int larguraCartao;
    const char *tituloPagina;
    const char *subtituloPagina;

    GetClientRect(janela, &cliente);
    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, GetStockObject(DC_BRUSH));
    SetDCBrushColor(dc, COR_FUNDO);
    PatBlt(dc, 0, 0, cliente.right, cliente.bottom, PATCOPY);

    area.left = 0;
    area.top = 0;
    area.right = SIDEBAR_WIDTH;
    area.bottom = cliente.bottom;
    {
        HBRUSH pincel = CreateSolidBrush(COR_MENU);
        FillRect(dc, &area, pincel);
        DeleteObject(pincel);
    }

    logotipo.left = 22;
    logotipo.top = 22;
    logotipo.right = 62;
    logotipo.bottom = 62;
    desenharBloco(dc, logotipo, COR_DESTAQUE, COR_DESTAQUE);
    texto.left = 22;
    texto.top = 22;
    texto.right = 62;
    texto.bottom = 62;
    desenharTexto(dc, "IL", texto, fonteBotao, COR_BRANCO,
                  DT_CENTER | DT_CENTER);
    texto.left = 76;
    texto.top = 21;
    texto.right = 218;
    texto.bottom = 44;
    desenharTexto(dc, "InovaLog", texto, fonteBotao, COR_BRANCO, DT_LEFT);
    texto.top = 43;
    texto.bottom = 63;
    desenharTexto(dc, "GESTAO DE SERVICOS", texto, fontePequena,
                  RGB(176, 190, 204), DT_LEFT);

    if (paginaAtual == PAGINA_INICIO) {
        tituloPagina = "Visao geral";
        subtituloPagina = "Acompanhe os cadastros e as ordens de servico.";
    } else if (paginaAtual == PAGINA_CLIENTES) {
        tituloPagina = formularioAberto ? "Novo cliente" : "Clientes";
        subtituloPagina = formularioAberto
                              ? "Informe os dados para cadastrar um cliente."
                              : "Consulte e cadastre os clientes da InovaLog.";
    } else if (paginaAtual == PAGINA_EQUIPAMENTOS) {
        tituloPagina = formularioAberto ? "Novo equipamento" : "Equipamentos";
        subtituloPagina =
            formularioAberto
                ? "Informe os dados do equipamento atendido."
                : "Consulte e cadastre os equipamentos atendidos.";
    } else if (paginaAtual == PAGINA_ORDENS) {
        tituloPagina = formularioAberto ? "Nova ordem de servico"
                                        : "Ordens de servico";
        subtituloPagina =
            formularioAberto
                ? "Vincule um cliente e um equipamento a esta ordem."
                : "Acompanhe os atendimentos registrados.";
    } else if (paginaAtual == PAGINA_FUNCIONARIOS) {
        tituloPagina = formularioAberto ? "Novo funcionario" : "Funcionarios";
        subtituloPagina =
            formularioAberto
                ? "Informe o nome e a funcao do funcionario."
                : "Cadastre os tecnicos responsaveis pelas ordens.";
    } else {
        tituloPagina =
            formularioAberto ? "Nova manutencao" : "Manutencoes";
        subtituloPagina =
            formularioAberto
                ? "Registre o servico executado em uma ordem."
                : "Consulte o historico de manutencoes realizadas.";
    }

    titulo.left = SIDEBAR_WIDTH + 28;
    titulo.top = 26;
    titulo.right = cliente.right - 28;
    titulo.bottom = 64;
    desenharTexto(dc, tituloPagina, titulo, fonteTitulo, COR_TEXTO, DT_LEFT);
    subtitulo.left = titulo.left;
    subtitulo.top = 66;
    subtitulo.right = titulo.right;
    subtitulo.bottom = 92;
    desenharTexto(dc, subtituloPagina, subtitulo, fontePequena, COR_SECUNDARIA,
                  DT_LEFT);

    if (aviso[0] != '\0' && paginaAtual == PAGINA_INICIO &&
        !formularioAberto) {
        avisoArea.left = SIDEBAR_WIDTH + 28;
        avisoArea.top = 111;
        avisoArea.right = cliente.right - 30;
        avisoArea.bottom = 135;
        desenharTexto(dc, aviso, avisoArea, fontePequena, COR_DESTAQUE,
                      DT_LEFT);
    }

    if (paginaAtual == PAGINA_INICIO && !formularioAberto) {
        larguraConteudo = cliente.right - SIDEBAR_WIDTH - 56;
        larguraCartao = (larguraConteudo - 36) / 3;
        if (larguraCartao > 300) {
            larguraCartao = 300;
        }
        desenharCartao(dc, 260, 158, larguraCartao, "CLIENTES CADASTRADOS",
                       totalClientes, "Registros no sistema");
        desenharCartao(dc, 278 + larguraCartao, 158, larguraCartao,
                       "EQUIPAMENTOS", totalEquipamentos,
                       "Equipamentos atendidos");
        desenharCartao(dc, 296 + larguraCartao * 2, 158, larguraCartao,
                       "ORDENS DE SERVICO", totalOrdens,
                       "Ordens registradas");

        texto.left = 260;
        texto.top = 302;
        texto.right = cliente.right - 30;
        texto.bottom = 335;
        desenharTexto(dc, "Acoes rapidas", texto, fonteBotao, COR_TEXTO,
                      DT_LEFT);
        texto.top = 416;
        texto.bottom = 442;
        desenharTexto(dc, "Ordens recentes", texto, fonteBotao, COR_TEXTO,
                      DT_LEFT);
        texto.top = 442;
        texto.bottom = 462;
        desenharTexto(dc, "Os ultimos atendimentos cadastrados.",
                      texto, fontePequena, COR_SECUNDARIA, DT_LEFT);
    } else if (!formularioAberto) {
        texto.left = SIDEBAR_WIDTH + 28;
        texto.top = paginaAtual == PAGINA_ORDENS ? 200 : 144;
        texto.right = cliente.right - 30;
        texto.bottom = paginaAtual == PAGINA_ORDENS ? 218 : 162;
        desenharTexto(dc, "REGISTROS", texto, fontePequena, COR_SECUNDARIA,
                      DT_LEFT);
    } else {
        cartao.left = 260;
        cartao.top = 165;
        cartao.right = cliente.right - 28;
        cartao.bottom = 520;
        desenharBloco(dc, cartao, COR_BRANCO, COR_BORDA);
        texto.left = 280;
        texto.top = 185;
        texto.right = cliente.right - 50;
        texto.bottom = 218;
        desenharTexto(dc, "Dados do cadastro", texto, fonteBotao, COR_TEXTO,
                      DT_LEFT);
    }

}

static LRESULT CALLBACK procedimentoJanela(HWND janela, UINT mensagem,
                                            WPARAM wParam, LPARAM lParam) {
    switch (mensagem) {
        case WM_CREATE: {
            int i;
            const char *rotulos[] = {"Visao geral", "Clientes", "Equipamentos",
                                     "Ordens de servico", "Funcionarios",
                                     "Manutencoes"};
            const int ids[] = {ID_NAV_DASHBOARD, ID_NAV_CLIENTES,
                               ID_NAV_EQUIPAMENTOS, ID_NAV_ORDENS,
                               ID_NAV_FUNCIONARIOS, ID_NAV_MANUTENCOES};

            fontePadrao = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                                      FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                      DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            fonteTitulo = CreateFontA(28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE,
                                      FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                      DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            fontePequena = CreateFontA(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                                       FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                       DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            fonteBotao = CreateFontA(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE,
                                     FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                     CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                     DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            if (fontePadrao == NULL) {
                fontePadrao = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            }
            if (fonteTitulo == NULL) {
                fonteTitulo = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            }
            if (fontePequena == NULL) {
                fontePequena = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            }
            if (fonteBotao == NULL) {
                fonteBotao = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            }

            for (i = 0; i < 6; i++) {
                criarBotao(janela, rotulos[i], ids[i], 16, 108 + i * 52, 200,
                           42);
            }
            criarControlesPainel(janela);
            return 0;
        }
        case WM_SIZE:
            if (paginaAtual == PAGINA_INICIO && tabela != NULL) {
                MoveWindow(tabela, 260, 444, LOWORD(lParam) - 288,
                           HIWORD(lParam) - 466, TRUE);
            } else if (paginaAtual == PAGINA_ORDENS && tabela != NULL) {
                MoveWindow(tabela, 260, 220, LOWORD(lParam) - 288,
                           HIWORD(lParam) - 245, TRUE);
            } else if (tabela != NULL) {
                MoveWindow(tabela, 260, 165, LOWORD(lParam) - 288,
                           HIWORD(lParam) - 190, TRUE);
            }
            InvalidateRect(janela, NULL, TRUE);
            return 0;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *limites = (MINMAXINFO *)lParam;
            limites->ptMinTrackSize.x = 980;
            limites->ptMinTrackSize.y = 640;
            return 0;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_NAV_DASHBOARD:
                    mudarPagina(janela, PAGINA_INICIO);
                    return 0;
                case ID_NAV_CLIENTES:
                    mudarPagina(janela, PAGINA_CLIENTES);
                    return 0;
                case ID_NAV_EQUIPAMENTOS:
                    mudarPagina(janela, PAGINA_EQUIPAMENTOS);
                    return 0;
                case ID_NAV_ORDENS:
                    mudarPagina(janela, PAGINA_ORDENS);
                    return 0;
                case ID_NAV_FUNCIONARIOS:
                    mudarPagina(janela, PAGINA_FUNCIONARIOS);
                    return 0;
                case ID_NAV_MANUTENCOES:
                    mudarPagina(janela, PAGINA_MANUTENCOES);
                    return 0;
                case ID_ACTION_PRIMARY:
                    if (paginaAtual == PAGINA_INICIO) {
                        paginaAtual = PAGINA_CLIENTES;
                    } else if (paginaAtual == PAGINA_ORDENS) {
                        abrirFormularioOrdem(janela);
                        return 0;
                    } else if (paginaAtual == PAGINA_MANUTENCOES) {
                        abrirFormularioManutencao(janela);
                        return 0;
                    }
                    formularioAberto = true;
                    limparControlesPainel(janela);
                    criarControlesPainel(janela);
                    InvalidateRect(janela, NULL, TRUE);
                    return 0;
                case ID_ACTION_SECONDARY:
                    if (formularioAberto) {
                        salvarFormulario(janela);
                    } else if (paginaAtual == PAGINA_INICIO) {
                        paginaAtual = PAGINA_EQUIPAMENTOS;
                        formularioAberto = true;
                        limparControlesPainel(janela);
                        criarControlesPainel(janela);
                        InvalidateRect(janela, NULL, TRUE);
                    }
                    return 0;
                case ID_ACTION_DELETE:
                    excluirSelecionado(janela);
                    return 0;
                case ID_ACTION_STATUS:
                    atualizarStatusSelecionado(janela);
                    return 0;
                case ID_ACTION_TERTIARY:
                    if (formularioAberto) {
                        formularioAberto = false;
                        limparControlesPainel(janela);
                        criarControlesPainel(janela);
                        InvalidateRect(janela, NULL, TRUE);
                    } else if (paginaAtual == PAGINA_INICIO) {
                        abrirFormularioOrdem(janela);
                    } else if (paginaAtual == PAGINA_MANUTENCOES) {
                        abrirFormularioManutencao(janela);
                    }
                    return 0;
            }
            break;
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT *item = (DRAWITEMSTRUCT *)lParam;
            int id = (int)item->CtlID;
            char rotulo[128];
            bool navegacao =
                id >= ID_NAV_DASHBOARD && id <= ID_NAV_MANUTENCOES;
            bool selecionado =
                (id == ID_NAV_DASHBOARD && paginaAtual == PAGINA_INICIO) ||
                (id == ID_NAV_CLIENTES && paginaAtual == PAGINA_CLIENTES) ||
                (id == ID_NAV_EQUIPAMENTOS &&
                 paginaAtual == PAGINA_EQUIPAMENTOS) ||
                (id == ID_NAV_ORDENS && paginaAtual == PAGINA_ORDENS) ||
                (id == ID_NAV_FUNCIONARIOS &&
                 paginaAtual == PAGINA_FUNCIONARIOS) ||
                (id == ID_NAV_MANUTENCOES &&
                 paginaAtual == PAGINA_MANUTENCOES);
            COLORREF corFundo =
                navegacao
                    ? (selecionado ? COR_MENU_SELECIONADO : COR_MENU)
                    : id == ID_ACTION_DELETE ? RGB(176, 63, 67)
                                              : COR_DESTAQUE;
            HBRUSH pincel = CreateSolidBrush(corFundo);
            HFONT fonte = navegacao ? fontePadrao : fonteBotao;
            RECT area = item->rcItem;

            FillRect(item->hDC, &area, pincel);
            DeleteObject(pincel);
            SetBkMode(item->hDC, TRANSPARENT);
            SetTextColor(item->hDC, COR_BRANCO);
            SelectObject(item->hDC, fonte);
            GetWindowTextA(item->hwndItem, rotulo, sizeof rotulo);
            if (navegacao) {
                area.left += 16;
                DrawTextA(item->hDC, rotulo, -1, &area,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            } else {
                DrawTextA(item->hDC, rotulo, -1, &area,
                          DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            return TRUE;
        }
        case WM_PAINT: {
            PAINTSTRUCT pintura;
            HDC dc = BeginPaint(janela, &pintura);
            desenharTela(dc, janela);
            EndPaint(janela, &pintura);
            return 0;
        }
        case WM_DESTROY:
            bancoDesconectar();
            if (fontePadrao != NULL && fontePadrao !=
                                           (HFONT)GetStockObject(DEFAULT_GUI_FONT)) {
                DeleteObject(fontePadrao);
            }
            if (fonteTitulo != NULL && fonteTitulo !=
                                           (HFONT)GetStockObject(DEFAULT_GUI_FONT)) {
                DeleteObject(fonteTitulo);
            }
            if (fontePequena != NULL && fontePequena !=
                                            (HFONT)GetStockObject(DEFAULT_GUI_FONT)) {
                DeleteObject(fontePequena);
            }
            if (fonteBotao != NULL && fonteBotao !=
                                          (HFONT)GetStockObject(DEFAULT_GUI_FONT)) {
                DeleteObject(fonteBotao);
            }
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(janela, mensagem, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instancia, HINSTANCE instanciaAnterior,
                   LPSTR linhaComando, int mostrar) {
    INITCOMMONCONTROLSEX controles;
    WNDCLASSEXA classe;
    HWND janela;
    MSG mensagem;
    int resultado;
    char erro[256];

    (void)instanciaAnterior;
    (void)linhaComando;

    if (!bancoConectar(erro, sizeof erro)) {
        MessageBoxA(NULL, erro, "Falha na conexao com PostgreSQL",
                    MB_OK | MB_ICONERROR);
        return 1;
    }
    if (!bancoCarregar(clientes, &totalClientes, equipamentos,
                       &totalEquipamentos, funcionarios, &totalFuncionarios,
                       ordens, &totalOrdens, erro, sizeof erro)) {
        MessageBoxA(NULL, erro, "Falha ao carregar os dados",
                    MB_OK | MB_ICONERROR);
        bancoDesconectar();
        return 1;
    }
    if (!bancoCarregarManutencoes(manutencoes, &totalManutencoes, erro,
                                  sizeof erro)) {
        MessageBoxA(NULL, erro, "Falha ao carregar manutencoes",
                    MB_OK | MB_ICONERROR);
        bancoDesconectar();
        return 1;
    }

    controles.dwSize = sizeof controles;
    controles.dwICC = ICC_LISTVIEW_CLASSES;
    if (!InitCommonControlsEx(&controles)) {
        MessageBoxA(NULL, "Nao foi possivel inicializar os controles do sistema.",
                    "Erro de inicializacao", MB_OK | MB_ICONERROR);
        bancoDesconectar();
        return 1;
    }

    ZeroMemory(&classe, sizeof classe);
    classe.cbSize = sizeof classe;
    classe.style = CS_HREDRAW | CS_VREDRAW;
    classe.lpfnWndProc = procedimentoJanela;
    classe.hInstance = instancia;
    classe.hCursor = LoadCursor(NULL, IDC_ARROW);
    classe.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    classe.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    classe.lpszClassName = APP_CLASS;
    if (!RegisterClassExA(&classe)) {
        MessageBoxA(NULL, "Nao foi possivel registrar a janela do aplicativo.",
                    "Erro de inicializacao", MB_OK | MB_ICONERROR);
        bancoDesconectar();
        return 1;
    }

    janela = CreateWindowExA(0, APP_CLASS, "SIG-InovaLog | Gestao de servicos",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             1180, 790, NULL, NULL, instancia, NULL);
    if (janela == NULL) {
        MessageBoxA(NULL, "Nao foi possivel criar a janela do aplicativo.",
                    "Erro de inicializacao", MB_OK | MB_ICONERROR);
        bancoDesconectar();
        return 1;
    }

    ShowWindow(janela, mostrar);
    UpdateWindow(janela);
    while ((resultado = GetMessageA(&mensagem, NULL, 0, 0)) > 0) {
        TranslateMessage(&mensagem);
        DispatchMessageA(&mensagem);
    }

    bancoDesconectar();
    return resultado == -1 ? 1 : (int)mensagem.wParam;
}
