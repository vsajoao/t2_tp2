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

std::vector<std::string> ParsearArquivoRegistros(
    const std::string& caminho_arquivo) {
  std::ifstream arquivo_log(caminho_arquivo);
  std::vector<std::string> registros;
  std::string registro;
  while (std::getline(arquivo_log, registro)) {
    if (!registro.empty()) {
      ChaveOrdenacaoRegistro(registro);
      registros.push_back(registro);
    }
  }

  OrdenarRegistros(&registros);
  return registros;
}

std::string ExtrairNomeBase(const std::string& caminho_log) {
  const std::size_t separador = caminho_log.find_last_of("/\\");
  return separador == std::string::npos ? caminho_log
                                        : caminho_log.substr(separador + 1);
}

std::filesystem::path CaminhoTotal(const std::filesystem::path& caminho_lista,
                                   const std::string& caminho_log) {
  return caminho_lista.parent_path() /
         ("total_" + ExtrairNomeBase(caminho_log));
}

void EscreverTotal(const std::filesystem::path& caminho_total,
                   const std::vector<std::string>& registros) {
  const std::filesystem::path caminho_temporario =
      caminho_total.string() + ".tmp";
  std::ofstream total(caminho_temporario);
  for (const std::string& registro : registros) {
    total << registro << '\n';
  }
  total.close();

  std::filesystem::rename(caminho_temporario, caminho_total);
}

CodigoResultado ProcessarLog(const std::filesystem::path& caminho_lista,
                             const std::string& caminho_log) {
  std::vector<std::string> registros;
  try {
    registros = ParsearArquivoRegistros(caminho_log);
  } catch (const std::exception&) {
    return CodigoResultado::kLogInvalido;
  }

  const std::filesystem::path caminho_total =
      CaminhoTotal(caminho_lista, caminho_log);
  if (std::filesystem::exists(caminho_total)) {
    try {
      const std::vector<std::string> registros_total =
          ParsearArquivoRegistros(caminho_total.string());
      registros.insert(registros.begin(), registros_total.begin(),
                       registros_total.end());
      OrdenarRegistros(&registros);
    } catch (const std::exception&) {
      return CodigoResultado::kTotalInvalido;
    }
  }

  EscreverTotal(caminho_total, registros);
  return CodigoResultado::kSucesso;
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
      const CodigoResultado codigo_log =
          ProcessarLog(caminho_lista_logs, linha);
      if (codigo_log != CodigoResultado::kSucesso) {
        return CriarResultado(codigo_log, logs_processados, linhas_ignoradas,
                              logs_ignorados);
      }
      ++logs_processados;
    }
  }

  return CriarResultado(CodigoResultado::kSucesso, logs_processados,
                        linhas_ignoradas, logs_ignorados);
}

}  // namespace monitora_logs
