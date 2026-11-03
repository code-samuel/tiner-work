/* MIT License
 *
 * Copyright (c) 2026 Vitor Figueiredo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * SPDX-License-Identifier: MIT
 */

/* GtkTreeView/GtkMessageDialog são "deprecated" no GTK 4.10+, mas continuam
 * funcionando. Silenciamos os avisos para manter o código simples. */
#define GDK_DISABLE_DEPRECATION_WARNINGS 1

#include "config.h"
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rh_pim-db.h"
#include "rh_pim-ui.h"

#define CONSULTA_ESCALAS "SELECT id, descricao FROM Escalas ORDER BY id"

#define CONSULTA_CONTRATOS                                                     \
  "SELECT c.id, col.nome, c.matricula "                                        \
  "FROM Contratos c "                                                          \
  "JOIN Colaboradores col ON col.id = c.colaborador_id "                       \
  "ORDER BY col.nome"

#define CONSULTA_COLABORADORES                                                 \
  "SELECT col.id, col.nome, col.cpf, "                                         \
  "       DATE_FORMAT(col.data_nascimento, '%d/%m/%Y'), "                      \
  "       DATE_FORMAT(col.data_admissao, '%d/%m/%Y'), "                        \
  "       col.telefone, c.matricula, c.cargo, "                                \
  "       CONCAT('R$ ', FORMAT(c.salario, 2, 'pt_BR')), "                      \
  "       e.descricao "                                                        \
  "FROM Colaboradores col "                                                    \
  "LEFT JOIN Contratos c ON c.colaborador_id = col.id "                        \
  "LEFT JOIN Escalas e ON e.id = c.escala_id "                                 \
  "ORDER BY col.nome"

#define CONSULTA_PONTO                                                         \
  "SELECT p.id, col.nome, c.matricula, "                                       \
  "       DATE_FORMAT(p.data_registro, '%d/%m/%Y'), "                          \
  "       TIME_FORMAT(p.entrada, '%H:%i'), "                                   \
  "       TIME_FORMAT(p.saida, '%H:%i'), "                                     \
  "       TIME_FORMAT(TIMEDIFF(p.saida, p.entrada), '%H:%i') "                 \
  "FROM Ponto p "                                                              \
  "JOIN Contratos c ON c.id = p.contrato_id "                                  \
  "JOIN Colaboradores col ON col.id = c.colaborador_id "                       \
  "ORDER BY p.data_registro DESC, p.id DESC"

#define CONSULTA_HOLERITES                                                     \
  "SELECT h.id, col.nome, c.matricula, "                                       \
  "       DATE_FORMAT(h.competencia, '%m/%Y'), h.cargo, "                      \
  "       CONCAT('R$ ', FORMAT(h.salario, 2, 'pt_BR')), "                      \
  "       CONCAT('R$ ', FORMAT(h.impostos, 2, 'pt_BR')), "                     \
  "       CONCAT('R$ ', FORMAT(h.descontos, 2, 'pt_BR')), "                    \
  "       CONCAT('R$ ', FORMAT(h.salario - IFNULL(h.impostos, 0) - "           \
  "IFNULL(h.descontos, 0), 2, 'pt_BR')) "                                      \
  "FROM Holerites h "                                                          \
  "JOIN Contratos c ON c.id = h.contrato_id "                                  \
  "JOIN Colaboradores col ON col.id = c.colaborador_id "                       \
  "ORDER BY h.competencia DESC, h.id DESC"

/* INSERTs: cada '?' é preenchido (com escape) por rh_pim_db_executar() */

#define SQL_INSERIR_COLABORADOR                                                \
  "INSERT INTO Colaboradores (nome, cpf, data_nascimento, data_admissao, "     \
  "telefone) "                                                                 \
  "VALUES (?, ?, ?, ?, ?)"

/* Todo colaborador novo ganha um contrato (usa a 1ª empresa cadastrada) */
#define SQL_INSERIR_CONTRATO                                                   \
  "INSERT INTO Contratos (matricula, cargo, data_admissao, salario, "          \
  "colaborador_id, empresa_id, escala_id) "                                    \
  "VALUES (?, ?, ?, ?, LAST_INSERT_ID(), (SELECT MIN(id) FROM Empresas), ?)"

/* A escala vem do contrato do colaborador */
#define SQL_INSERIR_PONTO                                                      \
  "INSERT INTO Ponto (contrato_id, escala_id, data_registro, entrada, saida) " \
  "SELECT id, escala_id, ?, ?, ? FROM Contratos WHERE id = ?"

/* Cargo e salário são copiados do contrato */
#define SQL_INSERIR_HOLERITE                                                   \
  "INSERT INTO Holerites (contrato_id, competencia, cargo, salario, "          \
  "impostos, descontos) "                                                      \
  "SELECT id, ?, cargo, salario, ?, ? FROM Contratos WHERE id = ?"

/* ========================================================================
 *  ESTRUTURAS
 * ======================================================================== */

typedef struct app_rh app_rh_t;

/* Uma aba = uma tabela na tela + a consulta que a preenche */
typedef struct {
  app_rh_t *app;
  GtkWidget *tabela; /* GtkTreeView */
  GtkListStore *armazenamento;
  const char *nome_tabela_sql; /* usado no DELETE */
  const char *consulta;        /* SELECT que preenche a tabela */
  const char *aviso_exclusao;
  int n_colunas;
} aba_t;

struct app_rh {
  GtkWidget *janela;
  GtkWidget *rotulo_status;

  aba_t aba_colaboradores;
  aba_t aba_ponto;
  aba_t aba_holerites;

  /* Aba Colaboradores */
  GtkWidget *c_nome;
  GtkWidget *c_cpf;
  GtkWidget *c_nascimento;
  GtkWidget *c_admissao;
  GtkWidget *c_telefone;
  GtkWidget *c_matricula;
  GtkWidget *c_cargo;
  GtkWidget *c_salario;
  GtkWidget *c_escala; /* GtkDropDown */

  /* Aba Ponto */
  GtkWidget *p_contrato; /* GtkDropDown */
  GtkWidget *p_data;
  GtkWidget *p_entrada;
  GtkWidget *p_saida;

  /* Aba Holerites */
  GtkWidget *h_contrato; /* GtkDropDown */
  GtkWidget *h_competencia;
  GtkWidget *h_impostos;
  GtkWidget *h_descontos;
};

typedef struct {
  aba_t *aba;
  char *id;
} exclusao_t;

/* ========================================================================
 *  FUNÇÕES AUXILIARES (validação e conversão)
 * ======================================================================== */

static bool validar_formato_horario(const char *horario_str, int *horas,
                                    int *minutos) {
  if (strlen(horario_str) != 5 || horario_str[2] != ':')
    return false;
  if (sscanf(horario_str, "%2d:%2d", horas, minutos) != 2)
    return false;
  if (*horas < 0 || *horas > 23 || *minutos < 0 || *minutos > 59)
    return false;
  return true;
}

/* "31/12/2025" -> "2025-12-31" (formato que o MySQL entende) */
static bool converter_data_br(const char *texto, char *saida, size_t tam) {
  int dia, mes, ano;

  if (strlen(texto) != 10 || texto[2] != '/' || texto[5] != '/')
    return false;
  if (sscanf(texto, "%2d/%2d/%4d", &dia, &mes, &ano) != 3)
    return false;
  if (ano < 1900 || ano > 2100 || mes < 1 || mes > 12 || dia < 1 || dia > 31)
    return false;
  if (!g_date_valid_dmy((GDateDay)dia, (GDateMonth)mes, (GDateYear)ano))
    return false;

  g_snprintf(saida, tam, "%04d-%02d-%02d", ano, mes, dia);
  return true;
}

/* "11/2026" -> "2026-11-01" */
static bool converter_competencia(const char *texto, char *saida, size_t tam) {
  int mes, ano;

  if (strlen(texto) != 7 || texto[2] != '/')
    return false;
  if (sscanf(texto, "%2d/%4d", &mes, &ano) != 2)
    return false;
  if (mes < 1 || mes > 12 || ano < 1900 || ano > 2100)
    return false;

  g_snprintf(saida, tam, "%04d-%02d-01", ano, mes);
  return true;
}

/* "3.500,50" não é aceito; use "3500,50" ou "3500.50" -> "3500.50" */
static bool converter_valor(const char *texto, char *saida, size_t tam) {
  char *copia;
  char *fim;
  double valor;
  bool ok;

  copia = g_strdup(texto);
  g_strdelimit(copia, ",", '.');
  valor = g_ascii_strtod(copia, &fim);
  ok = (*copia != '\0' && *fim == '\0' && valor >= 0.0 && valor < 99999999.0);

  if (ok)
    g_ascii_formatd(saida, (gint)tam, "%.2f", valor);

  g_free(copia);
  return ok;
}

static size_t somente_digitos(const char *entrada, char *saida, size_t tam) {
  size_t n = 0;

  for (; *entrada != '\0' && n + 1 < tam; entrada++) {
    if (g_ascii_isdigit(*entrada))
      saida[n++] = *entrada;
  }
  saida[n] = '\0';
  return n;
}

static void texto_hoje(const char *formato, char *saida, size_t tam) {
  GDateTime *agora;
  char *texto;

  agora = g_date_time_new_now_local();
  texto = g_date_time_format(agora, formato);
  g_strlcpy(saida, texto, tam);
  g_free(texto);
  g_date_time_unref(agora);
}

/* Lê o texto de um GtkEntry já sem espaços nas pontas */
static void ler_campo(GtkWidget *entrada, char *destino, size_t tam) {
  g_strlcpy(destino, gtk_editable_get_text(GTK_EDITABLE(entrada)), tam);
  g_strstrip(destino);
}

static void limpar_campo(GtkWidget *entrada) {
  gtk_editable_set_text(GTK_EDITABLE(entrada), "");
}

/* O dropdown mostra "5 - Maria - MAT001". O número no começo é o ID. */
static bool obter_id_dropdown(GtkWidget *dropdown, char *saida, size_t tam) {
  gpointer item;
  const char *texto;
  long id;

  item = gtk_drop_down_get_selected_item(GTK_DROP_DOWN(dropdown));
  if (item == NULL)
    return false;

  texto = gtk_string_object_get_string(GTK_STRING_OBJECT(item));
  id = strtol(texto, NULL, 10);
  if (id <= 0)
    return false;

  g_snprintf(saida, tam, "%ld", id);
  return true;
}

/* ========================================================================
 *  STATUS
 * ======================================================================== */

static void definir_status(app_rh_t *app, const char *texto) {
  gtk_label_set_text(GTK_LABEL(app->rotulo_status), texto);
}

/* Mostra "Erro ao <acao>: <detalhe do MySQL>" com mensagens amigáveis */
static void definir_status_erro(app_rh_t *app, const char *acao) {
  const char *detalhe;
  char *mensagem;

  switch (rh_pim_db_ultimo_errno()) {
  case 1062:
    detalhe = "já existe um registro com este CPF ou matrícula.";
    break;
  case 1452:
    detalhe = "referência inválida (colaborador ou escala não encontrado).";
    break;
  case 2002:
  case 2003:
    detalhe = "sem conexão com o MySQL. Suba o banco com: docker compose up -d";
    break;
  default:
    detalhe = rh_pim_db_ultimo_erro();
    break;
  }

  mensagem = g_strdup_printf("Erro ao %s: %s", acao, detalhe);
  definir_status(app, mensagem);
  g_free(mensagem);
}

/* ========================================================================
 *  TABELAS (SELECT) E DROPDOWNS
 * ======================================================================== */

static void ao_linha_tabela(char **campos, unsigned int n_campos,
                            gpointer dados) {
  aba_t *aba = (aba_t *)dados;
  GtkTreeIter iter;
  int i;

  gtk_list_store_append(aba->armazenamento, &iter);

  for (i = 0; i < (int)n_campos && i < aba->n_colunas; i++) {
    gtk_list_store_set(aba->armazenamento, &iter, i,
                       (campos[i] != NULL) ? campos[i] : "", -1);
  }
}

static gboolean carregar_aba(aba_t *aba) {
  gtk_list_store_clear(aba->armazenamento);

  if (!rh_pim_db_consultar(aba->consulta, ao_linha_tabela, aba)) {
    definir_status_erro(aba->app, "carregar a lista");
    return FALSE;
  }

  return TRUE;
}

static void ao_linha_dropdown(char **campos, unsigned int n_campos,
                              gpointer dados) {
  GtkStringList *lista = GTK_STRING_LIST(dados);
  GString *texto;
  unsigned int i;

  texto = g_string_new(NULL);

  for (i = 0; i < n_campos; i++) {
    if (i > 0)
      g_string_append(texto, " - ");
    g_string_append(texto, (campos[i] != NULL) ? campos[i] : "");
  }

  gtk_string_list_append(lista, texto->str);
  g_string_free(texto, TRUE);
}

static gboolean preencher_dropdown(app_rh_t *app, GtkWidget *dropdown,
                                   const char *consulta) {
  GtkStringList *lista;
  guint n_itens;

  lista = GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN(dropdown)));
  n_itens = g_list_model_get_n_items(G_LIST_MODEL(lista));
  gtk_string_list_splice(lista, 0, n_itens, NULL);

  if (!rh_pim_db_consultar(consulta, ao_linha_dropdown, lista)) {
    definir_status_erro(app, "carregar as opções");
    return FALSE;
  }

  return TRUE;
}

/* Recarrega as 3 tabelas e os dropdowns */
static gboolean atualizar_tudo(app_rh_t *app) {
  return carregar_aba(&app->aba_colaboradores) &&
         carregar_aba(&app->aba_ponto) && carregar_aba(&app->aba_holerites) &&
         preencher_dropdown(app, app->c_escala, CONSULTA_ESCALAS) &&
         preencher_dropdown(app, app->p_contrato, CONSULTA_CONTRATOS) &&
         preencher_dropdown(app, app->h_contrato, CONSULTA_CONTRATOS);
}

/* ========================================================================
 *  CALLBACKS DOS BOTÕES
 * ======================================================================== */

static void ao_clicar_atualizar(GtkWidget *widget, gpointer dados_usuario) {
  app_rh_t *app = (app_rh_t *)dados_usuario;

  if (atualizar_tudo(app))
    definir_status(app, "Listas atualizadas.");
}

/* ---- COLABORADORES: INSERT (colaborador + contrato) ---- */

static void ao_clicar_cadastrar_colaborador(GtkWidget *widget,
                                            gpointer dados_usuario) {
  app_rh_t *app = (app_rh_t *)dados_usuario;
  char nome[256], cpf_bruto[64], cpf[16];
  char nascimento_br[32], admissao_br[32], nascimento[16], admissao[16];
  char telefone[64], matricula[64], cargo[128];
  char salario_txt[64], salario[32], escala_id[16];
  const char *params_colaborador[5];
  const char *params_contrato[5];
  gboolean sucesso;

  ler_campo(app->c_nome, nome, sizeof(nome));
  ler_campo(app->c_cpf, cpf_bruto, sizeof(cpf_bruto));
  ler_campo(app->c_nascimento, nascimento_br, sizeof(nascimento_br));
  ler_campo(app->c_admissao, admissao_br, sizeof(admissao_br));
  ler_campo(app->c_telefone, telefone, sizeof(telefone));
  ler_campo(app->c_matricula, matricula, sizeof(matricula));
  ler_campo(app->c_cargo, cargo, sizeof(cargo));
  ler_campo(app->c_salario, salario_txt, sizeof(salario_txt));

  if (nome[0] == '\0') {
    definir_status(app, "Erro: informe o nome do colaborador.");
    return;
  }
  if (somente_digitos(cpf_bruto, cpf, sizeof(cpf)) != 11) {
    definir_status(app, "Erro: o CPF deve ter 11 números.");
    return;
  }
  if (!converter_data_br(nascimento_br, nascimento, sizeof(nascimento))) {
    definir_status(app, "Erro: data de nascimento inválida. Use DD/MM/AAAA.");
    return;
  }
  if (!converter_data_br(admissao_br, admissao, sizeof(admissao))) {
    definir_status(app, "Erro: data de admissão inválida. Use DD/MM/AAAA.");
    return;
  }
  if (matricula[0] == '\0') {
    definir_status(app, "Erro: informe a matrícula.");
    return;
  }
  if (cargo[0] == '\0') {
    definir_status(app, "Erro: informe o cargo.");
    return;
  }
  if (!converter_valor(salario_txt, salario, sizeof(salario))) {
    definir_status(app, "Erro: salário inválido. Exemplo: 3500,00");
    return;
  }
  if (!obter_id_dropdown(app->c_escala, escala_id, sizeof(escala_id))) {
    definir_status(app, "Erro: selecione a escala de trabalho.");
    return;
  }

  params_colaborador[0] = nome;
  params_colaborador[1] = cpf;
  params_colaborador[2] = nascimento;
  params_colaborador[3] = admissao;
  params_colaborador[4] = telefone;

  params_contrato[0] = matricula;
  params_contrato[1] = cargo;
  params_contrato[2] = admissao;
  params_contrato[3] = salario;
  params_contrato[4] = escala_id;

  /* Transação: ou grava colaborador + contrato, ou não grava nada */
  if (!rh_pim_db_executar("START TRANSACTION", NULL, 0)) {
    definir_status_erro(app, "cadastrar colaborador");
    return;
  }

  sucesso =
      rh_pim_db_executar(SQL_INSERIR_COLABORADOR, params_colaborador, 5) &&
      rh_pim_db_executar(SQL_INSERIR_CONTRATO, params_contrato, 5);

  if (!sucesso) {
    rh_pim_db_executar("ROLLBACK", NULL, 0);
    definir_status_erro(app, "cadastrar colaborador");
    return;
  }

  rh_pim_db_executar("COMMIT", NULL, 0);

  limpar_campo(app->c_nome);
  limpar_campo(app->c_cpf);
  limpar_campo(app->c_nascimento);
  limpar_campo(app->c_admissao);
  limpar_campo(app->c_telefone);
  limpar_campo(app->c_matricula);
  limpar_campo(app->c_cargo);
  limpar_campo(app->c_salario);

  if (atualizar_tudo(app))
    definir_status(app, "Colaborador cadastrado com sucesso!");
}

/* ---- PONTO: INSERT ---- */

static void ao_clicar_registrar_ponto(GtkWidget *widget,
                                      gpointer dados_usuario) {
  app_rh_t *app = (app_rh_t *)dados_usuario;
  char contrato_id[16], data_br[32], data_sql[16];
  char entrada_txt[16], saida_txt[16], entrada_sql[16], saida_sql[16];
  int horas_entrada, minutos_entrada, horas_saida, minutos_saida;
  int total_entrada, total_saida, diferenca;
  const char *params[4];
  char mensagem[128];

  if (!obter_id_dropdown(app->p_contrato, contrato_id, sizeof(contrato_id))) {
    definir_status(app, "Erro: selecione o colaborador (cadastre um na aba "
                        "Colaboradores se a lista estiver vazia).");
    return;
  }

  ler_campo(app->p_data, data_br, sizeof(data_br));
  ler_campo(app->p_entrada, entrada_txt, sizeof(entrada_txt));
  ler_campo(app->p_saida, saida_txt, sizeof(saida_txt));

  if (!converter_data_br(data_br, data_sql, sizeof(data_sql))) {
    definir_status(app, "Erro: data inválida. Use DD/MM/AAAA.");
    return;
  }
  if (!validar_formato_horario(entrada_txt, &horas_entrada, &minutos_entrada)) {
    definir_status(app, "Erro: entrada inválida. Use o formato HH:MM.");
    return;
  }
  if (!validar_formato_horario(saida_txt, &horas_saida, &minutos_saida)) {
    definir_status(app, "Erro: saída inválida. Use o formato HH:MM.");
    return;
  }

  total_entrada = horas_entrada * 60 + minutos_entrada;
  total_saida = horas_saida * 60 + minutos_saida;

  if (total_saida <= total_entrada) {
    definir_status(app, "Erro: a saída deve ser depois da entrada.");
    return;
  }

  g_snprintf(entrada_sql, sizeof(entrada_sql), "%02d:%02d:00", horas_entrada,
             minutos_entrada);
  g_snprintf(saida_sql, sizeof(saida_sql), "%02d:%02d:00", horas_saida,
             minutos_saida);

  params[0] = data_sql;
  params[1] = entrada_sql;
  params[2] = saida_sql;
  params[3] = contrato_id;

  if (!rh_pim_db_executar(SQL_INSERIR_PONTO, params, 4)) {
    definir_status_erro(app, "registrar ponto");
    return;
  }

  limpar_campo(app->p_entrada);
  limpar_campo(app->p_saida);

  diferenca = total_saida - total_entrada;
  g_snprintf(mensagem, sizeof(mensagem),
             "Ponto registrado! Jornada: %02dh %02dmin trabalhados.",
             diferenca / 60, diferenca % 60);

  if (carregar_aba(&app->aba_ponto))
    definir_status(app, mensagem);
}

/* ---- HOLERITES: INSERT ---- */

static void ao_clicar_gerar_holerite(GtkWidget *widget,
                                     gpointer dados_usuario) {
  app_rh_t *app = (app_rh_t *)dados_usuario;
  char contrato_id[16], competencia_txt[32], competencia[16];
  char impostos_txt[64], descontos_txt[64], impostos[32], descontos[32];
  const char *params[4];

  if (!obter_id_dropdown(app->h_contrato, contrato_id, sizeof(contrato_id))) {
    definir_status(app, "Erro: selecione o colaborador (cadastre um na aba "
                        "Colaboradores se a lista estiver vazia).");
    return;
  }

  ler_campo(app->h_competencia, competencia_txt, sizeof(competencia_txt));
  ler_campo(app->h_impostos, impostos_txt, sizeof(impostos_txt));
  ler_campo(app->h_descontos, descontos_txt, sizeof(descontos_txt));

  if (!converter_competencia(competencia_txt, competencia,
                             sizeof(competencia))) {
    definir_status(app, "Erro: competência inválida. Use MM/AAAA.");
    return;
  }

  if (impostos_txt[0] == '\0')
    g_strlcpy(impostos, "0.00", sizeof(impostos));
  else if (!converter_valor(impostos_txt, impostos, sizeof(impostos))) {
    definir_status(app, "Erro: valor de impostos inválido. Exemplo: 350,00");
    return;
  }

  if (descontos_txt[0] == '\0')
    g_strlcpy(descontos, "0.00", sizeof(descontos));
  else if (!converter_valor(descontos_txt, descontos, sizeof(descontos))) {
    definir_status(app, "Erro: valor de descontos inválido. Exemplo: 120,00");
    return;
  }

  params[0] = competencia;
  params[1] = impostos;
  params[2] = descontos;
  params[3] = contrato_id;

  if (!rh_pim_db_executar(SQL_INSERIR_HOLERITE, params, 4)) {
    definir_status_erro(app, "gerar holerite");
    return;
  }

  limpar_campo(app->h_impostos);
  limpar_campo(app->h_descontos);

  if (carregar_aba(&app->aba_holerites))
    definir_status(app, "Holerite gerado com sucesso!");
}

/* ---- DELETE (serve para as 3 abas) ---- */

static char *obter_id_selecionado(aba_t *aba) {
  GtkTreeSelection *selecao;
  GtkTreeModel *modelo;
  GtkTreeIter iter;
  char *id = NULL;

  selecao = gtk_tree_view_get_selection(GTK_TREE_VIEW(aba->tabela));

  if (gtk_tree_selection_get_selected(selecao, &modelo, &iter))
    gtk_tree_model_get(modelo, &iter, 0, &id, -1);

  return id; /* quem chama faz g_free() */
}

static void ao_responder_exclusao(GtkDialog *dialogo, int resposta,
                                  gpointer dados_usuario) {
  exclusao_t *exclusao = (exclusao_t *)dados_usuario;
  app_rh_t *app = exclusao->aba->app;
  const char *params[1];
  char *sql;

  if (resposta == GTK_RESPONSE_YES) {
    sql = g_strdup_printf("DELETE FROM %s WHERE id = ?",
                          exclusao->aba->nome_tabela_sql);
    params[0] = exclusao->id;

    if (rh_pim_db_executar(sql, params, 1)) {
      if (atualizar_tudo(app))
        definir_status(app, "Registro excluído com sucesso.");
    } else {
      definir_status_erro(app, "excluir registro");
    }

    g_free(sql);
  }

  g_free(exclusao->id);
  g_free(exclusao);
  gtk_window_destroy(GTK_WINDOW(dialogo));
}

static void ao_clicar_excluir(GtkWidget *widget, gpointer dados_usuario) {
  aba_t *aba = (aba_t *)dados_usuario;
  exclusao_t *exclusao;
  GtkWidget *dialogo;
  char *id;

  id = obter_id_selecionado(aba);
  if (id == NULL) {
    definir_status(aba->app, "Selecione uma linha da tabela para excluir.");
    return;
  }

  exclusao = g_new0(exclusao_t, 1);
  exclusao->aba = aba;
  exclusao->id = id;

  dialogo = gtk_message_dialog_new(
      GTK_WINDOW(gtk_widget_get_root(widget)),
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
      GTK_BUTTONS_YES_NO, "Excluir o registro %s?", id);
  gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialogo), "%s",
                                           aba->aviso_exclusao);

  g_signal_connect(dialogo, "response", G_CALLBACK(ao_responder_exclusao),
                   exclusao);
  gtk_window_present(GTK_WINDOW(dialogo));
}

/* ========================================================================
 *  CONSTRUÇÃO DA INTERFACE
 * ======================================================================== */

static GtkWidget *criar_entrada(const char *dica) {
  GtkWidget *entrada = gtk_entry_new();

  if (dica != NULL)
    gtk_entry_set_placeholder_text(GTK_ENTRY(entrada), dica);

  return entrada;
}

static GtkWidget *criar_dropdown(void) {
  static const char *const vazio[] = {NULL};

  return gtk_drop_down_new_from_strings(vazio);
}

static GtkWidget *criar_grade(void) {
  GtkWidget *grade = gtk_grid_new();

  gtk_grid_set_row_spacing(GTK_GRID(grade), 8);
  gtk_grid_set_column_spacing(GTK_GRID(grade), 10);
  return grade;
}

/* bloco 0 = coluna da esquerda do formulário, bloco 1 = coluna da direita */
static void adicionar_campo(GtkWidget *grade, int bloco, int linha,
                            const char *texto, GtkWidget *campo) {
  GtkWidget *rotulo = gtk_label_new(texto);

  gtk_label_set_xalign(GTK_LABEL(rotulo), 0.0f);
  if (bloco > 0)
    gtk_widget_set_margin_start(rotulo, 24);
  gtk_widget_set_hexpand(campo, TRUE);

  gtk_grid_attach(GTK_GRID(grade), rotulo, bloco * 2, linha, 1, 1);
  gtk_grid_attach(GTK_GRID(grade), campo, bloco * 2 + 1, linha, 1, 1);
}

static GtkWidget *criar_tabela(aba_t *aba, const char *const *titulos,
                               int n_colunas) {
  GType *tipos;
  GtkWidget *rolagem;
  GtkCellRenderer *renderizador;
  GtkTreeViewColumn *coluna;
  int i;

  aba->n_colunas = n_colunas;

  /* Todas as colunas guardam texto */
  tipos = g_new(GType, n_colunas);
  for (i = 0; i < n_colunas; i++)
    tipos[i] = G_TYPE_STRING;
  aba->armazenamento = gtk_list_store_newv(n_colunas, tipos);
  g_free(tipos);

  aba->tabela =
      gtk_tree_view_new_with_model(GTK_TREE_MODEL(aba->armazenamento));
  g_object_unref(aba->armazenamento); /* a tabela passa a ser a dona */

  for (i = 0; i < n_colunas; i++) {
    renderizador = gtk_cell_renderer_text_new();
    coluna = gtk_tree_view_column_new_with_attributes(titulos[i], renderizador,
                                                      "text", i, NULL);
    gtk_tree_view_column_set_resizable(coluna, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(aba->tabela), coluna);
  }

  rolagem = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(rolagem, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(rolagem), aba->tabela);
  return rolagem;
}

static GtkWidget *criar_botoes(const char *texto_cadastrar,
                               GCallback ao_cadastrar, aba_t *aba) {
  GtkWidget *caixa, *botao_cadastrar, *botao_excluir, *botao_atualizar;

  caixa = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  botao_cadastrar = gtk_button_new_with_label(texto_cadastrar);
  botao_excluir = gtk_button_new_with_label("Excluir selecionado");
  botao_atualizar = gtk_button_new_with_label("Atualizar lista");

  gtk_widget_add_css_class(botao_cadastrar, "suggested-action");
  gtk_widget_add_css_class(botao_excluir, "destructive-action");

  g_signal_connect(botao_cadastrar, "clicked", ao_cadastrar, aba->app);
  g_signal_connect(botao_excluir, "clicked", G_CALLBACK(ao_clicar_excluir),
                   aba);
  g_signal_connect(botao_atualizar, "clicked", G_CALLBACK(ao_clicar_atualizar),
                   aba->app);

  gtk_box_append(GTK_BOX(caixa), botao_cadastrar);
  gtk_box_append(GTK_BOX(caixa), botao_excluir);
  gtk_box_append(GTK_BOX(caixa), botao_atualizar);
  return caixa;
}

static GtkWidget *criar_pagina(GtkWidget *grade, GtkWidget *botoes,
                               GtkWidget *rolagem) {
  GtkWidget *caixa = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

  gtk_widget_set_margin_top(caixa, 12);
  gtk_widget_set_margin_bottom(caixa, 12);
  gtk_widget_set_margin_start(caixa, 12);
  gtk_widget_set_margin_end(caixa, 12);

  gtk_box_append(GTK_BOX(caixa), grade);
  gtk_box_append(GTK_BOX(caixa), botoes);
  gtk_box_append(GTK_BOX(caixa), rolagem);
  return caixa;
}

/* ---- Aba 1: Colaboradores ---- */

static GtkWidget *criar_aba_colaboradores(app_rh_t *app) {
  static const char *const titulos[] = {
      "ID",       "Nome",      "CPF",   "Nascimento", "Admissão",
      "Telefone", "Matrícula", "Cargo", "Salário",    "Escala"};
  aba_t *aba = &app->aba_colaboradores;
  GtkWidget *grade, *botoes, *rolagem;

  aba->app = app;
  aba->nome_tabela_sql = "Colaboradores";
  aba->consulta = CONSULTA_COLABORADORES;
  aba->aviso_exclusao =
      "O contrato, os registros de ponto e os holerites deste colaborador "
      "também serão excluídos. Não é possível desfazer.";

  grade = criar_grade();

  app->c_nome = criar_entrada("Maria da Silva");
  app->c_cpf = criar_entrada("000.000.000-00");
  app->c_nascimento = criar_entrada("DD/MM/AAAA");
  app->c_admissao = criar_entrada("DD/MM/AAAA");
  app->c_telefone = criar_entrada("(11) 99999-9999");
  app->c_matricula = criar_entrada("MAT-0001");
  app->c_cargo = criar_entrada("Analista");
  app->c_salario = criar_entrada("3500,00");
  app->c_escala = criar_dropdown();

  adicionar_campo(grade, 0, 0, "Nome:", app->c_nome);
  adicionar_campo(grade, 0, 1, "CPF:", app->c_cpf);
  adicionar_campo(grade, 0, 2, "Data de nascimento:", app->c_nascimento);
  adicionar_campo(grade, 0, 3, "Data de admissão:", app->c_admissao);
  adicionar_campo(grade, 0, 4, "Telefone:", app->c_telefone);
  adicionar_campo(grade, 1, 0, "Matrícula:", app->c_matricula);
  adicionar_campo(grade, 1, 1, "Cargo:", app->c_cargo);
  adicionar_campo(grade, 1, 2, "Salário (R$):", app->c_salario);
  adicionar_campo(grade, 1, 3, "Escala:", app->c_escala);

  botoes = criar_botoes("Cadastrar colaborador",
                        G_CALLBACK(ao_clicar_cadastrar_colaborador), aba);
  rolagem = criar_tabela(aba, titulos, G_N_ELEMENTS(titulos));

  return criar_pagina(grade, botoes, rolagem);
}

/* ---- Aba 2: Ponto ---- */

static GtkWidget *criar_aba_ponto(app_rh_t *app) {
  static const char *const titulos[] = {
      "ID", "Colaborador", "Matrícula", "Data", "Entrada", "Saída", "Total"};
  aba_t *aba = &app->aba_ponto;
  GtkWidget *grade, *botoes, *rolagem;
  char hoje[16];

  aba->app = app;
  aba->nome_tabela_sql = "Ponto";
  aba->consulta = CONSULTA_PONTO;
  aba->aviso_exclusao = "Não é possível desfazer.";

  grade = criar_grade();

  app->p_contrato = criar_dropdown();
  app->p_data = criar_entrada("DD/MM/AAAA");
  app->p_entrada = criar_entrada("08:00");
  app->p_saida = criar_entrada("17:00");

  texto_hoje("%d/%m/%Y", hoje, sizeof(hoje));
  gtk_editable_set_text(GTK_EDITABLE(app->p_data), hoje);

  adicionar_campo(grade, 0, 0, "Colaborador:", app->p_contrato);
  adicionar_campo(grade, 0, 1, "Data:", app->p_data);
  adicionar_campo(grade, 1, 0, "Horário de entrada (HH:MM):", app->p_entrada);
  adicionar_campo(grade, 1, 1, "Horário de saída (HH:MM):", app->p_saida);

  botoes = criar_botoes("Registrar ponto",
                        G_CALLBACK(ao_clicar_registrar_ponto), aba);
  rolagem = criar_tabela(aba, titulos, G_N_ELEMENTS(titulos));

  return criar_pagina(grade, botoes, rolagem);
}

/* ---- Aba 3: Holerites ---- */

static GtkWidget *criar_aba_holerites(app_rh_t *app) {
  static const char *const titulos[] = {
      "ID",      "Colaborador", "Matrícula", "Competência", "Cargo",
      "Salário", "Impostos",    "Descontos", "Líquido"};
  aba_t *aba = &app->aba_holerites;
  GtkWidget *grade, *botoes, *rolagem, *dica;
  char mes_atual[16];

  aba->app = app;
  aba->nome_tabela_sql = "Holerites";
  aba->consulta = CONSULTA_HOLERITES;
  aba->aviso_exclusao = "Não é possível desfazer.";

  grade = criar_grade();

  app->h_contrato = criar_dropdown();
  app->h_competencia = criar_entrada("MM/AAAA");
  app->h_impostos = criar_entrada("0,00");
  app->h_descontos = criar_entrada("0,00");

  texto_hoje("%m/%Y", mes_atual, sizeof(mes_atual));
  gtk_editable_set_text(GTK_EDITABLE(app->h_competencia), mes_atual);

  adicionar_campo(grade, 0, 0, "Colaborador:", app->h_contrato);
  adicionar_campo(grade, 0, 1, "Competência (MM/AAAA):", app->h_competencia);
  adicionar_campo(grade, 1, 0, "Impostos (R$):", app->h_impostos);
  adicionar_campo(grade, 1, 1, "Descontos (R$):", app->h_descontos);

  dica = gtk_label_new(
      "Cargo e salário bruto são copiados do contrato do colaborador.");
  gtk_label_set_xalign(GTK_LABEL(dica), 0.0f);
  gtk_widget_add_css_class(dica, "dim-label");
  gtk_grid_attach(GTK_GRID(grade), dica, 0, 2, 4, 1);

  botoes =
      criar_botoes("Gerar holerite", G_CALLBACK(ao_clicar_gerar_holerite), aba);
  rolagem = criar_tabela(aba, titulos, G_N_ELEMENTS(titulos));

  return criar_pagina(grade, botoes, rolagem);
}

/* ---- Janela principal ---- */

GtkWidget *rh_pim_ui_criar_janela(GtkApplication *aplicacao) {
  app_rh_t *app;
  GtkWidget *janela, *caixa, *caderno;

  app = g_new0(app_rh_t, 1);

  janela = gtk_application_window_new(aplicacao);
  app->janela = janela;
  g_object_set_data_full(G_OBJECT(janela), "app-data", app, g_free);

  gtk_window_set_title(GTK_WINDOW(janela), "Sistema de RH - TinerWork");
  gtk_window_set_default_size(GTK_WINDOW(janela), 1100, 680);

  app->rotulo_status = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(app->rotulo_status), 0.0f);
  gtk_label_set_wrap(GTK_LABEL(app->rotulo_status), TRUE);

  caderno = gtk_notebook_new();
  gtk_widget_set_vexpand(caderno, TRUE);
  gtk_notebook_append_page(GTK_NOTEBOOK(caderno), criar_aba_colaboradores(app),
                           gtk_label_new("Colaboradores"));
  gtk_notebook_append_page(GTK_NOTEBOOK(caderno), criar_aba_ponto(app),
                           gtk_label_new("Ponto"));
  gtk_notebook_append_page(GTK_NOTEBOOK(caderno), criar_aba_holerites(app),
                           gtk_label_new("Holerites"));

  caixa = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_bottom(caixa, 10);
  gtk_widget_set_margin_start(caixa, 12);
  gtk_widget_set_margin_end(caixa, 12);
  gtk_box_append(GTK_BOX(caixa), caderno);
  gtk_box_append(GTK_BOX(caixa), app->rotulo_status);
  gtk_window_set_child(GTK_WINDOW(janela), caixa);

  /* Primeira carga dos dados */
  if (atualizar_tudo(app))
    definir_status(app, "Conectado ao banco TinerWork.");

  return janela;
}
