# SIG-InovaLog

Sistema desktop em C para cadastro de clientes, equipamentos, funcionarios e
ordens de servico. Os registros sao gravados no PostgreSQL e permanecem no
banco depois que o aplicativo e fechado.

## Requisitos

- Windows 64 bits.
- GCC MinGW-w64 64 bits.
- PostgreSQL `libpq` 64 bits e seus arquivos DLL disponiveis no `PATH`.
- Uma instancia PostgreSQL acessivel, como o projeto Neon.

O MySQL Workbench e especifico para MySQL e nao conecta diretamente ao Neon
PostgreSQL. Para administrar esse banco, use o painel SQL do Neon, pgAdmin ou
outro cliente PostgreSQL.

## Estrutura do projeto

```text
projeto PIM/
|-- database/
|   `-- schema.sql
|-- include/
|   |-- banco.h
|   |-- clientes.h
|   |-- exclusoes.h
|   |-- equipamentos.h
|   |-- entrada.h
|   |-- funcionarios.h
|   |-- manutencoes.h
|   |-- modelos.h
|   `-- ordens_servico.h
|-- src/
|   |-- banco.c
|   |-- clientes.c
|   |-- exclusoes.c
|   |-- equipamentos.c
|   |-- entrada.c
|   |-- funcionarios.c
|   |-- manutencoes.c
|   `-- ordens_servico.c
|-- main.c
|-- desktop.c
`-- desktop.manifest
```

`database/schema.sql` contem o esquema PostgreSQL equivalente as tabelas
`CLIENTE`, `EQUIPAMENTO`, `FUNCIONARIO`, `ORDEM_SERVICO` e `MANUTENCAO`.
Execute-o uma vez no banco caso essas tabelas ainda nao existam. Se ja foram
criadas, confira que os nomes e colunas correspondem ao arquivo.

## Configurar a conexao

Crie no Neon uma senha nova para o banco e use a URL PostgreSQL copiada da
pagina de conexao do projeto. Nao grave a URL no codigo ou em arquivos do
projeto. O programa le a URL da variavel de ambiente
`SIG_INOVALOG_DATABASE_URL`.

No PowerShell, o comando abaixo solicita a URL sem mostra-la na tela, usa-a
somente durante a execucao e a remove do ambiente ao finalizar:

```powershell
$segredo = Read-Host "URL PostgreSQL do Neon" -AsSecureString
$ponteiro = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($segredo)
try {
    $env:SIG_INOVALOG_DATABASE_URL = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($ponteiro)
    .\sig_inovalog_desktop.exe
}
finally {
    Remove-Item Env:SIG_INOVALOG_DATABASE_URL -ErrorAction SilentlyContinue
    [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($ponteiro)
}
```

Cadastre funcionarios pela tela **Funcionarios**. A data de abertura das OS e
das manutencoes e gerada pelo PostgreSQL. Clientes e equipamentos podem ser
associados nas telas correspondentes.

## Compilar no Windows

Os comandos abaixo usam o GCC MinGW-w64 64 bits do Strawberry Perl e a
instalacao local de `libpq`. Ajuste os caminhos se o PostgreSQL ou o MinGW
estiverem instalados em outro local.

```powershell
$gcc = "C:\Strawberry\c\bin\x86_64-w64-mingw32-gcc.exe"
$include = "C:\Strawberry\c\include"
$lib = "C:\Strawberry\c\lib"

& $gcc -std=c11 -Wall -Wextra -Wpedantic -Iinclude "-I$include" "-L$lib" `
    desktop.c src\banco.c -lpq -mwindows -lcomctl32 -lgdi32 -luser32 `
    -o sig_inovalog_desktop.exe

Copy-Item desktop.manifest sig_inovalog_desktop.exe.manifest
```

Garanta que a DLL de `libpq` e suas dependencias estejam no `PATH` ao iniciar
o aplicativo. No Strawberry Perl, elas normalmente ficam em
`C:\Strawberry\c\bin`.

Para compilar a versao de terminal, use os mesmos caminhos e inclua os modulos
de cadastro:

```powershell
& $gcc -std=c11 -Wall -Wextra -Wpedantic -Iinclude "-I$include" "-L$lib" `
    main.c src\entrada.c src\clientes.c src\equipamentos.c `
    src\funcionarios.c src\manutencoes.c src\ordens_servico.c `
    src\exclusoes.c src\banco.c -lpq `
    -o sig_inovalog.exe
```

As duas interfaces usam o mesmo banco PostgreSQL e oferecem cadastro de
clientes, equipamentos, funcionarios, ordens de servico e manutencoes.
Na interface desktop, selecione um registro na tabela e clique em
**Excluir selecionado**. No terminal, escolha **8. Excluir registro**. A
exclusao pede confirmacao e e gravada diretamente no PostgreSQL; os dados locais
tambem sao atualizados somente depois do sucesso no banco. Registros que ainda
tenham dependencias (por exemplo, uma OS com manutencoes) nao sao apagados:
exclua primeiro os registros dependentes para preservar o historico.
Na tela de ordens de servico, selecione uma OS, escolha um status e clique em
**Atualizar status**. Os status disponiveis sao **Aberta**, **Em Andamento**,
**Concluida** e **Cancelada**; a alteracao e salva no PostgreSQL. No terminal,
use a opcao **9. Atualizar status de OS**.
