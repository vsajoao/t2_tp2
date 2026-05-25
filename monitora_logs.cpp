#include "monitora_logs.hpp"

#include <fstream>

namespace monitora_logs {

namespace {

ResultadoMonitoramento CriarResultado(CodigoResultado codigo) {
  return {codigo};
}

}  // namespace

ResultadoMonitoramento MonitorarLogs(const std::string& caminho_lista_logs) {
  std::ifstream lista_logs(caminho_lista_logs);
  if (!lista_logs.is_open()) {
    return CriarResultado(CodigoResultado::kListaLogsInexistente);
  }

  return CriarResultado(CodigoResultado::kSucesso);
}

}  // namespace monitora_logs
