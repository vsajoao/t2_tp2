#include "monitora_logs.hpp"

#include <filesystem>
#include <fstream>

namespace monitora_logs {

namespace {

ResultadoMonitoramento CriarResultado(CodigoResultado codigo,
                                      int logs_processados,
                                      int linhas_ignoradas,
                                      int logs_ignorados) {
  return {codigo, logs_processados, linhas_ignoradas, logs_ignorados};
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
    return CriarResultado(CodigoResultado::kListaLogsInexistente, 0, 0, 0);
  }

  int linhas_ignoradas = 0;
  int logs_ignorados = 0;
  std::string linha;
  while (std::getline(lista_logs, linha)) {
    if (LinhaListaVazia(linha)) {
      ++linhas_ignoradas;
    } else if (!std::filesystem::exists(linha)) {
      ++logs_ignorados;
    }
  }

  return CriarResultado(CodigoResultado::kSucesso, 0, linhas_ignoradas,
                        logs_ignorados);
}

}  // namespace monitora_logs
