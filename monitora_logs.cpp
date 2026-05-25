#include "monitora_logs.hpp"

#include <fstream>

namespace monitora_logs {

namespace {

ResultadoMonitoramento CriarResultado(CodigoResultado codigo,
                                      int logs_processados,
                                      int linhas_ignoradas) {
  return {codigo, logs_processados, linhas_ignoradas};
}

bool AbrirListaLogs(const std::string& caminho_lista_logs,
                    std::ifstream* lista_logs) {
  lista_logs->open(caminho_lista_logs);
  return lista_logs->is_open();
}

bool LinhaListaVazia(const std::string& linha) {
  return linha.empty();
}

}  // namespace

ResultadoMonitoramento MonitorarLogs(const std::string& caminho_lista_logs) {
  std::ifstream lista_logs;
  if (!AbrirListaLogs(caminho_lista_logs, &lista_logs)) {
    return CriarResultado(CodigoResultado::kListaLogsInexistente, 0, 0);
  }

  int linhas_ignoradas = 0;
  std::string linha;
  while (std::getline(lista_logs, linha)) {
    if (LinhaListaVazia(linha)) {
      ++linhas_ignoradas;
    }
  }

  return CriarResultado(CodigoResultado::kSucesso, 0, linhas_ignoradas);
}

}  // namespace monitora_logs
