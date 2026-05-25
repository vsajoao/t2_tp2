#ifndef MONITORA_LOGS_HPP_
#define MONITORA_LOGS_HPP_

#include <string>

namespace monitora_logs {

enum class CodigoResultado {
  kSucesso,
  kListaLogsInexistente,
};

struct ResultadoMonitoramento {
  CodigoResultado codigo;
};

/***************************************************************************
 * Funcao: MonitorarLogs
 * Descricao
 * Executa o monitoramento dos arquivos listados em um arquivo de entrada.
 * Parametros
 * caminho_lista_logs - caminho do arquivo que contem a lista de logs.
 * Valor retornado
 * Resultado do monitoramento, incluindo o codigo da situacao encontrada.
 * Assertiva de entrada
 * caminho_lista_logs nao deve ser vazio.
 * Assertiva de saida
 * O codigo retornado deve representar sucesso ou uma falha prevista pela
 * tabela de decisao.
 ***************************************************************************/
ResultadoMonitoramento MonitorarLogs(const std::string& caminho_lista_logs);

}  // namespace monitora_logs

#endif  // MONITORA_LOGS_HPP_
