#include "monitora_logs.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

namespace monitora_logs {

namespace {

struct ContadoresMonitoramento {
  int logs_processados;
  int linhas_ignoradas;
  int logs_ignorados;
  int totais_atualizados;
};

ResultadoMonitoramento CriarResultado(
    CodigoResultado codigo,
    const ContadoresMonitoramento& contadores) {
  return {codigo, contadores.logs_processados, contadores.linhas_ignoradas,
          contadores.logs_ignorados, contadores.totais_atualizados};
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

CodigoResultado ProcessarLinhaLista(
    const std::filesystem::path& caminho_lista,
    const std::string& linha,
    ContadoresMonitoramento* contadores) {
  if (LinhaListaVazia(linha)) {
    ++contadores->linhas_ignoradas;
    return CodigoResultado::kSucesso;
  }

  if (!CaminhoLogExiste(linha)) {
    ++contadores->logs_ignorados;
    return CodigoResultado::kSucesso;
  }

  const CodigoResultado codigo_log = ProcessarLog(caminho_lista, linha);
  if (codigo_log == CodigoResultado::kSucesso) {
    ++contadores->logs_processados;
    ++contadores->totais_atualizados;
  }
  return codigo_log;
}

}  // namespace

ResultadoMonitoramento MonitorarLogs(const std::string& caminho_lista_logs) {
  ContadoresMonitoramento contadores = {0, 0, 0, 0};
  std::ifstream lista_logs;
  if (!AbrirListaLogs(caminho_lista_logs, &lista_logs)) {
    return CriarResultado(CodigoResultado::kListaLogsInexistente, contadores);
  }

  std::string linha;
  while (std::getline(lista_logs, linha)) {
    const CodigoResultado codigo_linha =
        ProcessarLinhaLista(caminho_lista_logs, linha, &contadores);
    if (codigo_linha != CodigoResultado::kSucesso) {
      return CriarResultado(codigo_linha, contadores);
    }
  }

  return CriarResultado(CodigoResultado::kSucesso, contadores);
}

bool ParsearLinhaLog(const std::string& linha, RegistroLog* registro) {
  const std::regex padrao_linha(
      R"(^([0-9]{1,2})/([0-9]{1,2})/([0-9]{4}) ([0-9]{1,2}):([0-9]{2}):([0-9]{2}) {2}(.{1,100})$)");
  std::smatch grupos;
  if (!std::regex_match(linha, grupos, padrao_linha)) {
    return false;
  }

  registro->dia = std::stoi(grupos[1].str());
  registro->mes = std::stoi(grupos[2].str());
  registro->ano = std::stoi(grupos[3].str());
  registro->hora = std::stoi(grupos[4].str());
  registro->minuto = std::stoi(grupos[5].str());
  registro->segundo = std::stoi(grupos[6].str());
  registro->mensagem = grupos[7].str();
  return true;
}

}  // namespace monitora_logs
