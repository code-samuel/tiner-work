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

#include "config.h"
#include <mysql.h>
#include <string.h>

#include "rh_pim-db.h"

/* --- CONFIGURAÇÃO DA CONEXÃO (combina com o docker-compose.yml) --- */
#define DB_HOST                                                                \
  "127.0.0.1" /* use 127.0.0.1 e não "localhost" (evita socket)               \
               */
#define DB_USUARIO "root"
#define DB_SENHA "" /* MYSQL_ALLOW_EMPTY_PASSWORD: "yes" */
#define DB_NOME "TinerWork"
#define DB_PORTA 3306

static MYSQL *conexao = NULL;
static char ultimo_erro[512] = "";
static unsigned int ultimo_errno = 0;

const char *rh_pim_db_ultimo_erro(void) { return ultimo_erro; }

unsigned int rh_pim_db_ultimo_errno(void) { return ultimo_errno; }

static void guardar_erro(void) {
  if (conexao != NULL) {
    g_strlcpy(ultimo_erro, mysql_error(conexao), sizeof(ultimo_erro));
    ultimo_errno = mysql_errno(conexao);
  } else {
    g_strlcpy(ultimo_erro, "Sem conexão com o banco de dados.",
              sizeof(ultimo_erro));
    ultimo_errno = 0;
  }
}

gboolean rh_pim_db_conectar(void) {
  if (conexao != NULL)
    return TRUE;

  conexao = mysql_init(NULL);
  if (conexao == NULL) {
    g_strlcpy(ultimo_erro, "Não foi possível inicializar o cliente MySQL.",
              sizeof(ultimo_erro));
    ultimo_errno = 0;
    return FALSE;
  }

  mysql_options(conexao, MYSQL_SET_CHARSET_NAME, "utf8mb4");

  if (mysql_real_connect(conexao, DB_HOST, DB_USUARIO, DB_SENHA, DB_NOME,
                         DB_PORTA, NULL, 0) == NULL) {
    guardar_erro();
    mysql_close(conexao);
    conexao = NULL;
    return FALSE;
  }

  return TRUE;
}

void rh_pim_db_desconectar(void) {
  if (conexao != NULL) {
    mysql_close(conexao);
    conexao = NULL;
  }
}

static void tratar_falha(void) {
  guardar_erro();

  if (ultimo_errno == 2006 || ultimo_errno == 2013)
    rh_pim_db_desconectar();
}

static char *montar_sql(const char *sql, const char **params, int n_params) {
  GString *resultado;
  const char *p;
  char *escapado;
  unsigned long tamanho;
  int indice;

  resultado = g_string_new(NULL);
  indice = 0;

  for (p = sql; *p != '\0'; p++) {
    if (*p == '?' && indice < n_params) {
      if (params[indice] == NULL || params[indice][0] == '\0') {
        g_string_append(resultado, "NULL");
      } else {
        tamanho = strlen(params[indice]);
        escapado = g_malloc(tamanho * 2 + 1);
        mysql_real_escape_string(conexao, escapado, params[indice], tamanho);
        g_string_append_printf(resultado, "'%s'", escapado);
        g_free(escapado);
      }
      indice++;
    } else {
      g_string_append_c(resultado, *p);
    }
  }

  return g_string_free(resultado, FALSE);
}

gboolean rh_pim_db_executar(const char *sql, const char **params,
                            int n_params) {
  char *sql_final;
  int resultado;

  if (!rh_pim_db_conectar())
    return FALSE;

  sql_final = montar_sql(sql, params, n_params);
  resultado = mysql_query(conexao, sql_final);
  g_free(sql_final);

  if (resultado != 0) {
    tratar_falha();
    return FALSE;
  }

  return TRUE;
}

gboolean rh_pim_db_consultar(const char *sql, RhPimDbLinhaFunc callback,
                             gpointer dados) {
  MYSQL_RES *resultado;
  MYSQL_ROW linha;
  unsigned int n_campos;

  if (!rh_pim_db_conectar())
    return FALSE;

  if (mysql_query(conexao, sql) != 0) {
    tratar_falha();
    return FALSE;
  }

  resultado = mysql_store_result(conexao);
  if (resultado == NULL) {
    tratar_falha();
    return FALSE;
  }

  n_campos = mysql_num_fields(resultado);

  while ((linha = mysql_fetch_row(resultado)) != NULL)
    callback(linha, n_campos, dados);

  mysql_free_result(resultado);
  return TRUE;
}
