#include "monitora_logs.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
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

std::string FormatarDataParaOrdenacao(int ano, int mes, int dia) {
  std::ostringstream data;
  data << std::setw(4) << std::setfill('0') << ano << "/" << std::setw(2)
       << std::setfill('0') << mes << "/" << std::setw(2)
       << std::setfill('0') << dia;
  return data.str();
}

std::string FormatarHorarioParaOrdenacao(const RegistroLog& registro) {
  std::ostringstream horario;
  horario << std::setw(2) << std::setfill('0') << registro.hora << ":"
          << std::setw(2) << std::setfill('0') << registro.minuto << ":"
          << std::setw(2) << std::setfill('0') << registro.segundo;
  return horario.str();
}

std::string ChaveOrdenacaoRegistro(const RegistroLog& registro) {
  return FormatarDataParaOrdenacao(registro.ano, registro.mes, registro.dia) +
         " " + FormatarHorarioParaOrdenacao(registro);
}

RegistroLog ParsearRegistroObrigatorio(const std::string& linha) {
  RegistroLog registro = {};
  if (!ParsearLinhaLog(linha, &registro)) {
    throw std::invalid_argument("registro de log fora do formato esperado");
  }
  return registro;
}

void ValidarFormatoRegistroLog(const std::string& registro) {
  ParsearRegistroObrigatorio(registro);
}

void OrdenarRegistros(std::vector<std::string>* registros) {
  std::stable_sort(registros->begin(), registros->end(),
                   [](const std::string& esquerda,
                      const std::string& direita) {
                     return ChaveOrdenacaoRegistro(
                                ParsearRegistroObrigatorio(esquerda)) <
                            ChaveOrdenacaoRegistro(
                                ParsearRegistroObrigatorio(direita));
                   });
}

std::vector<std::string> ParsearArquivoRegistros(
    const std::string& caminho_arquivo) {
  std::ifstream arquivo_log(caminho_arquivo);
  std::vector<std::string> registros;
  std::string registro;
  while (std::getline(arquivo_log, registro)) {
    if (!registro.empty()) {
      ValidarFormatoRegistroLog(registro);
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

const std::regex& PadraoLinhaLog() {
  static const std::regex padrao_linha(
      R"(^([0-9]{1,2})/([0-9]{1,2})/([0-9]{4}) ([0-9]{1,2}):([0-9]{2}):([0-9]{2}) {2}(.{1,100})$)");
  return padrao_linha;
}

void PreencherRegistroLog(const std::smatch& grupos, RegistroLog* registro) {
  registro->dia = std::stoi(grupos[1].str());
  registro->mes = std::stoi(grupos[2].str());
  registro->ano = std::stoi(grupos[3].str());
  registro->hora = std::stoi(grupos[4].str());
  registro->minuto = std::stoi(grupos[5].str());
  registro->segundo = std::stoi(grupos[6].str());
  registro->mensagem = grupos[7].str();
}

bool AnoBissexto(int ano) {
  return (ano % 4 == 0 && ano % 100 != 0) || ano % 400 == 0;
}

int DiasNoMes(int mes, int ano) {
  const int dias_por_mes[] = {31, 28, 31, 30, 31, 30,
                              31, 31, 30, 31, 30, 31};
  if (mes == 2 && AnoBissexto(ano)) {
    return 29;
  }
  return dias_por_mes[mes - 1];
}

bool DataValida(const RegistroLog& registro) {
  if (registro.mes < 1 || registro.mes > 12) {
    return false;
  }

  const int ultimo_dia = DiasNoMes(registro.mes, registro.ano);

  return registro.dia >= 1 && registro.dia <= ultimo_dia;
}

bool HorarioValido(const RegistroLog& registro) {
  return registro.hora >= 0 && registro.hora <= 23 && registro.minuto >= 0 &&
         registro.minuto <= 59 && registro.segundo >= 0 &&
         registro.segundo <= 59;
}

bool RegistroLogValido(const RegistroLog& registro) {
  return DataValida(registro) && HorarioValido(registro);
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
  std::smatch grupos;
  if (!std::regex_match(linha, grupos, PadraoLinhaLog())) {
    return false;
  }

  PreencherRegistroLog(grupos, registro);
  return RegistroLogValido(*registro);
}

}  // namespace monitora_logs
