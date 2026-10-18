#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  MYSQL *conexao;

  conexao = mysql_init(NULL);
  if (conexao == NULL) {
    fprintf(stderr, "Erro ao inicializar a estrutura do MySQL!\n");
    return 1;
  }

  if (mysql_real_connect(conexao, "127.0.0.1", "root", "", "testeMYSQL", 3306,
                         NULL, 0) == NULL) {
    fprintf(stderr, "Erro de conexao: %s\n", mysql_error(conexao));
    mysql_close(conexao);
    return 1;
  }

  printf("Sucesso! Conectado ao MySQL do Docker com sucesso!\n");

  mysql_close(conexao);

  return 0;
}
