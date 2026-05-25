#include "monitora_logs.hpp"

namespace monitora_logs {

ResultadoMonitoramento MonitorarLogs(const std::string& caminho_lista_logs) {
  (void)caminho_lista_logs;
  return {CodigoResultado::kSucesso};
}

}  // namespace monitora_logs
