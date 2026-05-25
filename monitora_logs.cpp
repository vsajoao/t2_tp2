#include "monitora_logs.hpp"

#include <fstream>

namespace monitora_logs {

ResultadoMonitoramento MonitorarLogs(const std::string& caminho_lista_logs) {
  std::ifstream lista_logs(caminho_lista_logs);
  if (!lista_logs.is_open()) {
    return {CodigoResultado::kListaLogsInexistente};
  }

  return {CodigoResultado::kSucesso};
}

}  // namespace monitora_logs
