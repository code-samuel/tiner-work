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

#include "config.h"
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "rh_pim-application.h"

typedef struct {
    GtkWidget *janela_principal;
    GtkWidget *campo_id_funcionario;
    GtkWidget *campo_horario_entrada;
    GtkWidget *campo_horario_saida;
    GtkWidget *rotulo_status;
    GtkTextBuffer *buffer_historico;
} app_rh_t;

struct _RhPimApplication
{
	GtkApplication parent_instance;
};

G_DEFINE_FINAL_TYPE (RhPimApplication, rh_pim_application, GTK_TYPE_APPLICATION)

// --- LÓGICA DO SISTEMA DE RH ---

static bool validar_formato_horario(const char *horario_str, int *horas, int *minutos) {
    if (strlen(horario_str) != 5 || horario_str[2] != ':') return false;
    if (sscanf(horario_str, "%2d:%2d", horas, minutos) != 2) return false;
    if (*horas < 0 || *horas > 23 || *minutos < 0 || *minutos > 59) return false;
    return true;
}

static void ao_clicar_validar_registrar(GtkWidget *widget, gpointer dados_usuario) {
    app_rh_t *app = (app_rh_t *)dados_usuario;

    // Declarações movidas para o topo para evitar erro C90
    const char *id_texto;
    const char *entrada_texto;
    const char *saida_texto;
    int horas_entrada, minutos_entrada;
    int horas_saida, minutos_saida;
    int total_minutos_entrada, total_minutos_saida;
    int diferenca_minutos, horas_trabalhadas, minutos_trabalhados;
    char mensagem_status[256];
    char linha_historico[256];
    GtkTextIter iter_fim;

    id_texto = gtk_editable_get_text(GTK_EDITABLE(app->campo_id_funcionario));
    entrada_texto = gtk_editable_get_text(GTK_EDITABLE(app->campo_horario_entrada));
    saida_texto = gtk_editable_get_text(GTK_EDITABLE(app->campo_horario_saida));

    if (strlen(id_texto) == 0) {
        gtk_label_set_text(GTK_LABEL(app->rotulo_status), "Erro: Informe o ID ou nome do funcionário.");
        return;
    }

    if (!validar_formato_horario(entrada_texto, &horas_entrada, &minutos_entrada)) {
        gtk_label_set_text(GTK_LABEL(app->rotulo_status), "Erro: Entrada inválida. Use o formato HH:MM.");
        return;
    }

    if (!validar_formato_horario(saida_texto, &horas_saida, &minutos_saida)) {
        gtk_label_set_text(GTK_LABEL(app->rotulo_status), "Erro: Saída inválida. Use o formato HH:MM.");
        return;
    }

    total_minutos_entrada = horas_entrada * 60 + minutos_entrada;
    total_minutos_saida = horas_saida * 60 + minutos_saida;

    if (total_minutos_saida <= total_minutos_entrada) {
        gtk_label_set_text(GTK_LABEL(app->rotulo_status), "Erro: Saída deve ser após a entrada.");
        return;
    }

    diferenca_minutos = total_minutos_saida - total_minutos_entrada;
    horas_trabalhadas = diferenca_minutos / 60;
    minutos_trabalhados = diferenca_minutos % 60;

    snprintf(mensagem_status, sizeof(mensagem_status),
             "Sucesso! Jornada validada: %02dh %02dmin trabalhados.",
             horas_trabalhadas, minutos_trabalhados);
    gtk_label_set_text(GTK_LABEL(app->rotulo_status), mensagem_status);

    snprintf(linha_historico, sizeof(linha_historico),
             "ID: %-10s | Entrada: %02d:%02d | Saída: %02d:%02d | Total: %02dh %02dmin\n",
             id_texto, horas_entrada, minutos_entrada, horas_saida, minutos_saida,
             horas_trabalhadas, minutos_trabalhados);

    gtk_text_buffer_get_end_iter(app->buffer_historico, &iter_fim);
    gtk_text_buffer_insert(app->buffer_historico, &iter_fim, linha_historico, -1);
}

static void ao_clicar_limpar_campos(GtkWidget *widget, gpointer dados_usuario) {
    app_rh_t *app = (app_rh_t *)dados_usuario;

    gtk_editable_set_text(GTK_EDITABLE(app->campo_id_funcionario), "");
    gtk_editable_set_text(GTK_EDITABLE(app->campo_horario_entrada), "");
    gtk_editable_set_text(GTK_EDITABLE(app->campo_horario_saida), "");
    gtk_label_set_text(GTK_LABEL(app->rotulo_status), "Campos limpos com sucesso.");
}

// --- ATIVAÇÃO E INTERFACE ---

static void
rh_pim_application_activate (GApplication *app)
{
    // Declarações no topo
    GtkWindow *window;
    app_rh_t *dados_app;
    GtkWidget *caixa_principal;
    GtkWidget *grade_formulario;
    GtkWidget *rotulo_id;
    GtkWidget *rotulo_entrada;
    GtkWidget *rotulo_saida;
    GtkWidget *caixa_botoes;
    GtkWidget *botao_validar;
    GtkWidget *botao_limpar;
    GtkWidget *rotulo_historico;
    GtkWidget *janela_rolagem;
    GtkWidget *visao_texto;

	g_assert (RH_PIM_IS_APPLICATION (app));

    window = gtk_application_get_active_window (GTK_APPLICATION (app));

    if (window == NULL) {
        window = GTK_WINDOW(gtk_application_window_new(GTK_APPLICATION(app)));
        gtk_window_set_title(window, "Sistema de RH - Consulta e Ponto");
        gtk_window_set_default_size(window, 520, 480);
        gtk_application_add_window(GTK_APPLICATION(app), window);

        dados_app = g_new0(app_rh_t, 1);
        dados_app->janela_principal = GTK_WIDGET(window);
        g_object_set_data_full(G_OBJECT(window), "app-data", dados_app, g_free);

        caixa_principal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_margin_top(caixa_principal, 15);
        gtk_widget_set_margin_bottom(caixa_principal, 15);
        gtk_widget_set_margin_start(caixa_principal, 15);
        gtk_widget_set_margin_end(caixa_principal, 15);
        gtk_window_set_child(window, caixa_principal);

        grade_formulario = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grade_formulario), 8);
        gtk_grid_set_column_spacing(GTK_GRID(grade_formulario), 10);
        gtk_box_append(GTK_BOX(caixa_principal), grade_formulario);

        rotulo_id = gtk_label_new("ID / Nome do Funcionário:");
        dados_app->campo_id_funcionario = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grade_formulario), rotulo_id, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grade_formulario), dados_app->campo_id_funcionario, 1, 0, 1, 1);

        rotulo_entrada = gtk_label_new("Horário de Entrada (HH:MM):");
        dados_app->campo_horario_entrada = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(dados_app->campo_horario_entrada), "08:00");
        gtk_grid_attach(GTK_GRID(grade_formulario), rotulo_entrada, 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grade_formulario), dados_app->campo_horario_entrada, 1, 1, 1, 1);

        rotulo_saida = gtk_label_new("Horário de Saída (HH:MM):");
        dados_app->campo_horario_saida = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(dados_app->campo_horario_saida), "17:00");
        gtk_grid_attach(GTK_GRID(grade_formulario), rotulo_saida, 0, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(grade_formulario), dados_app->campo_horario_saida, 1, 2, 1, 1);

        caixa_botoes = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        botao_validar = gtk_button_new_with_label("Validar e Registrar");
        botao_limpar = gtk_button_new_with_label("Limpar Campos");

        gtk_widget_set_hexpand(botao_validar, TRUE);
        gtk_widget_set_hexpand(botao_limpar, TRUE);

        gtk_box_append(GTK_BOX(caixa_botoes), botao_validar);
        gtk_box_append(GTK_BOX(caixa_botoes), botao_limpar);
        gtk_box_append(GTK_BOX(caixa_principal), caixa_botoes);

        dados_app->rotulo_status = gtk_label_new("Aguardando dados para validação.");
        gtk_box_append(GTK_BOX(caixa_principal), dados_app->rotulo_status);

        rotulo_historico = gtk_label_new("Registros Validados:");
        gtk_label_set_xalign(GTK_LABEL(rotulo_historico), 0.0);
        gtk_box_append(GTK_BOX(caixa_principal), rotulo_historico);

        janela_rolagem = gtk_scrolled_window_new();
        gtk_widget_set_vexpand(janela_rolagem, TRUE);

        visao_texto = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(visao_texto), FALSE);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(visao_texto), TRUE);
        dados_app->buffer_historico = gtk_text_view_get_buffer(GTK_TEXT_VIEW(visao_texto));

        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(janela_rolagem), visao_texto);
        gtk_box_append(GTK_BOX(caixa_principal), janela_rolagem);

        g_signal_connect(botao_validar, "clicked", G_CALLBACK(ao_clicar_validar_registrar), dados_app);
        g_signal_connect(botao_limpar, "clicked", G_CALLBACK(ao_clicar_limpar_campos), dados_app);
    }

    gtk_window_present (window);
}

// --- AÇÕES (ABOUT E QUIT) ---

static void
rh_pim_application_about_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	static const char *authors[] = {"Vitor Figueiredo", NULL};
	RhPimApplication *self = user_data;
	GtkWindow *window;
    GtkWidget *about;

	g_assert (RH_PIM_IS_APPLICATION (self));

	window = gtk_application_get_active_window (GTK_APPLICATION (self));

    about = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), "Rh_pim");
    gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about), "org.gnome.Example");
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), "0.1.0");
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), "© 2026 Vitor Figueiredo");
    gtk_window_set_transient_for(GTK_WINDOW(about), window);
    gtk_window_set_modal(GTK_WINDOW(about), TRUE);
    gtk_window_present(GTK_WINDOW(about));
}

static void
rh_pim_application_quit_action (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
	RhPimApplication *self = user_data;
	g_assert (RH_PIM_IS_APPLICATION (self));
	g_application_quit (G_APPLICATION (self));
}

static const GActionEntry app_actions[] = {
	{ "quit", rh_pim_application_quit_action },
	{ "about", rh_pim_application_about_action },
};

// --- INICIALIZAÇÃO DA CLASSE ---

static void
rh_pim_application_class_init (RhPimApplicationClass *klass)
{
	GApplicationClass *app_class = G_APPLICATION_CLASS (klass);
	app_class->activate = rh_pim_application_activate;
}

static void
rh_pim_application_init (RhPimApplication *self)
{
	g_action_map_add_action_entries (G_ACTION_MAP (self),
	                                 app_actions,
	                                 G_N_ELEMENTS (app_actions),
	                                 self);
	gtk_application_set_accels_for_action (GTK_APPLICATION (self),
	                                       "app.quit",
	                                       (const char *[]) { "<control>q", NULL });
}

RhPimApplication *
rh_pim_application_new (const char *application_id, GApplicationFlags flags)
{
	g_return_val_if_fail (application_id != NULL, NULL);

	return g_object_new (RH_PIM_TYPE_APPLICATION,
	                     "application-id", application_id,
	                     "flags", flags,
	                     "resource-base-path", "/org/gnome/Example",
	                     NULL);
}
