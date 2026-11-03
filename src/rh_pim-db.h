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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/* Chamada para cada linha de um SELECT. Os campos NULL do banco chegam como NULL. */
typedef void (*RhPimDbLinhaFunc) (char **campos, unsigned int n_campos, gpointer dados);

/* Conecta ao MySQL (se ainda não estiver conectado). */
gboolean     rh_pim_db_conectar     (void);
void         rh_pim_db_desconectar  (void);

/* INSERT / DELETE / etc. Cada '?' do SQL é trocado pelo próximo item de
 * params (já escapado e entre aspas). Texto vazio ou NULL vira NULL no banco. */
gboolean     rh_pim_db_executar     (const char  *sql,
                                     const char **params,
                                     int          n_params);

/* SELECT: chama callback uma vez para cada linha do resultado. */
gboolean     rh_pim_db_consultar    (const char       *sql,
                                     RhPimDbLinhaFunc  callback,
                                     gpointer          dados);

/* Informações sobre a última falha. */
const char  *rh_pim_db_ultimo_erro  (void);
unsigned int rh_pim_db_ultimo_errno (void);

G_END_DECLS
