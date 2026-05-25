#include "monitora_logs.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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

bool CaminhoLogExiste(const std::string& caminho_log) {
  return std::filesystem::exists(caminho_log);
}

std::string ChaveOrdenacaoRegistro(const std::string& registro) {
  const int dia = std::stoi(registro.substr(0, registro.find('/')));
  const std::size_t inicio_mes = registro.find('/') + 1;
  const std::size_t fim_mes = registro.find('/', inicio_mes);
  const int mes = std::stoi(registro.substr(inicio_mes, fim_mes - inicio_mes));
  const int ano = std::stoi(registro.substr(fim_mes + 1, 4));
  const std::string hora = registro.substr(registro.find(' ') + 1, 8);

  return std::to_string(ano) + "/" + std::to_string(mes) + "/" +
         std::to_string(dia) + " " + hora;
}

void OrdenarRegistros(std::vector<std::string>* registros) {
  std::stable_sort(registros->begin(), registros->end(),
                   [](const std::string& esquerda,
                      const std::string& direita) {
                     return ChaveOrdenacaoRegistro(esquerda) <
                            ChaveOrdenacaoRegistro(direita);
                   });
}

std::vector<std::string> LerRegistrosLog(const std::string& caminho_log) {
  std::ifstream arquivo_log(caminho_log);
  std::vector<std::string> registros;
  std::string registro;
  while (std::getline(arquivo_log, registro)) {
    if (!registro.empty()) {
      registros.push_back(registro);
    }
  }

  OrdenarRegistros(&registros);
  return registros;
}

std::filesystem::path CaminhoTotal(const std::filesystem::path& caminho_lista,
                                   const std::string& caminho_log) {
  const std::filesystem::path nome_log =
      std::filesystem::path(caminho_log).filename();
  return caminho_lista.parent_path() / ("total_" + nome_log.string());
}

void EscreverTotal(const std::filesystem::path& caminho_total,
                   const std::vector<std::string>& registros) {
  std::ofstream total(caminho_total);
  for (const std::string& registro : registros) {
    total << registro << '\n';
  }
}

bool ProcessarLog(const std::filesystem::path& caminho_lista,
                  const std::string& caminho_log) {
  std::vector<std::string> registros;
  try {
    registros = LerRegistrosLog(caminho_log);
    const std::filesystem::path caminho_total =
        CaminhoTotal(caminho_lista, caminho_log);
    if (std::filesystem::exists(caminho_total)) {
      const std::vector<std::string> registros_total =
          LerRegistrosLog(caminho_total.string());
      registros.insert(registros.begin(), registros_total.begin(),
                       registros_total.end());
      OrdenarRegistros(&registros);
    }

    EscreverTotal(caminho_total, registros);
  } catch (const std::exception&) {
    return false;
  }

  return true;
}

}  // namespace

ResultadoMonitoramento MonitorarLogs(const std::string& caminho_lista_logs) {
  std::ifstream lista_logs;
  if (!AbrirListaLogs(caminho_lista_logs, &lista_logs)) {
    return CriarResultado(CodigoResultado::kListaLogsInexistente, 0, 0, 0);
  }

  int linhas_ignoradas = 0;
  int logs_ignorados = 0;
  int logs_processados = 0;
  std::string linha;
  while (std::getline(lista_logs, linha)) {
    if (LinhaListaVazia(linha)) {
      ++linhas_ignoradas;
    } else if (!CaminhoLogExiste(linha)) {
      ++logs_ignorados;
    } else {
      if (!ProcessarLog(caminho_lista_logs, linha)) {
        return CriarResultado(CodigoResultado::kLogInvalido, logs_processados,
                              linhas_ignoradas, logs_ignorados);
      }
      ++logs_processados;
    }
  }

  return CriarResultado(CodigoResultado::kSucesso, logs_processados,
                        linhas_ignoradas, logs_ignorados);
}

}  // namespace monitora_logs
