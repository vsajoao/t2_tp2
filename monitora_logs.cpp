#include "monitora_logs.hpp"

#include <fstream>

namespace monitora_logs {

namespace {

ResultadoMonitoramento CriarResultado(CodigoResultado codigo,
                                      int logs_processados) {
  return {codigo, logs_processados};
}

bool AbrirListaLogs(const std::string& caminho_lista_logs,
                    std::ifstream* lista_logs) {
  lista_logs->open(caminho_lista_logs);
  return lista_logs->is_open();
}

}  // namespace

ResultadoMonitoramento MonitorarLogs(const std::string& caminho_lista_logs) {
  std::ifstream lista_logs;
  if (!AbrirListaLogs(caminho_lista_logs, &lista_logs)) {
    return CriarResultado(CodigoResultado::kListaLogsInexistente, 0);
  }

  return CriarResultado(CodigoResultado::kSucesso, 0);
}

}  // namespace monitora_logs
