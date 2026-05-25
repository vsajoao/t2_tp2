#include "monitora_logs.hpp"

#include <fstream>

namespace monitora_logs {

namespace {

ResultadoMonitoramento CriarResultado(CodigoResultado codigo,
                                      int logs_processados) {
  return {codigo, logs_processados};
}

}  // namespace

ResultadoMonitoramento MonitorarLogs(const std::string& caminho_lista_logs) {
  std::ifstream lista_logs(caminho_lista_logs);
  if (!lista_logs.is_open()) {
    return CriarResultado(CodigoResultado::kListaLogsInexistente, 0);
  }

  return CriarResultado(CodigoResultado::kSucesso, 0);
}

}  // namespace monitora_logs
