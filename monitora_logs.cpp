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

/***************************************************************************
 * Tipo: ContadoresMonitoramento
 * Descricao
 * Guarda os acumuladores internos usados durante a leitura da lista.
 * Campos
 * logs_processados - quantidade de logs validos processados.
 * linhas_ignoradas - quantidade de linhas vazias da lista.
 * logs_ignorados - quantidade de caminhos inexistentes na lista.
 * totais_atualizados - quantidade de totais regravados com sucesso.
 * Assertiva de entrada
 * Nao se aplica.
 * Assertiva de saida
 * Todos os campos devem permanecer maiores ou iguais a zero.
 ***************************************************************************/
struct ContadoresMonitoramento {
  int logs_processados;
  int linhas_ignoradas;
  int logs_ignorados;
  int totais_atualizados;
};

/***************************************************************************
 * Funcao: CriarResultado
 * Descricao
 * Converte o codigo final e os contadores internos no resultado publico.
 * Parametros
 * codigo - codigo final da situacao encontrada.
 * contadores - acumuladores calculados durante o monitoramento.
 * Valor retornado
 * ResultadoMonitoramento preenchido com o codigo e os contadores informados.
 * Assertiva de entrada
 * contadores deve conter apenas valores maiores ou iguais a zero.
 * Assertiva de saida
 * O resultado retornado deve preservar codigo e copiar exatamente os valores
 * de contadores.
 ***************************************************************************/
ResultadoMonitoramento CriarResultado(
    CodigoResultado codigo,
    const ContadoresMonitoramento& contadores) {
  return {codigo, contadores.logs_processados, contadores.linhas_ignoradas,
          contadores.logs_ignorados, contadores.totais_atualizados};
}

/***************************************************************************
 * Funcao: AbrirListaLogs
 * Descricao
 * Tenta abrir o arquivo que contem a lista de caminhos de logs.
 * Parametros
 * caminho_lista_logs - caminho do arquivo de lista.
 * lista_logs - ponteiro para o fluxo que sera aberto.
 * Valor retornado
 * true se o arquivo foi aberto; false caso contrario.
 * Assertiva de entrada
 * lista_logs deve ser diferente de NULL e caminho_lista_logs nao deve ser
 * vazio.
 * Assertiva de saida
 * Se retornar true, lista_logs deve estar aberto para leitura. Se retornar
 * false, o fluxo nao deve ser usado para leitura da lista.
 ***************************************************************************/
bool AbrirListaLogs(const std::string& caminho_lista_logs,
                    std::ifstream* lista_logs) {
  lista_logs->open(caminho_lista_logs);
  return lista_logs->is_open();
}

/***************************************************************************
 * Funcao: LinhaListaVazia
 * Descricao
 * Verifica se uma linha da lista nao contem nenhum caractere.
 * Parametros
 * linha - linha lida do arquivo de lista.
 * Valor retornado
 * true se linha estiver vazia; false caso contrario.
 * Assertiva de entrada
 * linha deve ter sido obtida da lista de logs.
 * Assertiva de saida
 * O retorno deve ser equivalente a linha.empty().
 ***************************************************************************/
bool LinhaListaVazia(const std::string& linha) {
  return linha.empty();
}

/***************************************************************************
 * Funcao: CaminhoLogExiste
 * Descricao
 * Consulta se o caminho indicado por uma linha da lista existe no sistema.
 * Parametros
 * caminho_log - caminho do arquivo de log.
 * Valor retornado
 * true se o caminho existir; false caso contrario.
 * Assertiva de entrada
 * caminho_log nao deve ser vazio.
 * Assertiva de saida
 * O retorno deve refletir a existencia do caminho no momento da consulta.
 ***************************************************************************/
bool CaminhoLogExiste(const std::string& caminho_log) {
  return std::filesystem::exists(caminho_log);
}

/***************************************************************************
 * Funcao: FormatarDataParaOrdenacao
 * Descricao
 * Gera uma representacao textual de data adequada para comparacao lexical.
 * Parametros
 * ano - ano do registro.
 * mes - mes do registro.
 * dia - dia do registro.
 * Valor retornado
 * Data no formato AAAA/MM/DD, preenchida com zeros a esquerda.
 * Assertiva de entrada
 * ano, mes e dia devem representar uma data valida ja verificada.
 * Assertiva de saida
 * O valor retornado deve manter a mesma ordem cronologica em comparacoes de
 * strings.
 ***************************************************************************/
std::string FormatarDataParaOrdenacao(int ano, int mes, int dia) {
  std::ostringstream data;
  data << std::setw(4) << std::setfill('0') << ano << "/" << std::setw(2)
       << std::setfill('0') << mes << "/" << std::setw(2)
       << std::setfill('0') << dia;
  return data.str();
}

/***************************************************************************
 * Funcao: FormatarHorarioParaOrdenacao
 * Descricao
 * Gera uma representacao textual de horario adequada para ordenacao.
 * Parametros
 * registro - registro com campos de horario ja validados.
 * Valor retornado
 * Horario no formato HH:MM:SS, preenchido com zeros a esquerda.
 * Assertiva de entrada
 * registro deve conter hora entre 0 e 23 e minuto e segundo entre 0 e 59.
 * Assertiva de saida
 * O valor retornado deve manter a ordem cronologica dentro de um mesmo dia.
 ***************************************************************************/
std::string FormatarHorarioParaOrdenacao(const RegistroLog& registro) {
  std::ostringstream horario;
  horario << std::setw(2) << std::setfill('0') << registro.hora << ":"
          << std::setw(2) << std::setfill('0') << registro.minuto << ":"
          << std::setw(2) << std::setfill('0') << registro.segundo;
  return horario.str();
}

/***************************************************************************
 * Funcao: ChaveOrdenacaoRegistro
 * Descricao
 * Monta a chave usada para comparar registros de log por data e hora.
 * Parametros
 * registro - registro de log valido.
 * Valor retornado
 * String composta por data e horario em formatos ordenaveis lexicalmente.
 * Assertiva de entrada
 * registro deve conter data gregoriana valida e horario valido.
 * Assertiva de saida
 * Registros mais antigos devem produzir chaves lexicograficamente menores.
 ***************************************************************************/
std::string ChaveOrdenacaoRegistro(const RegistroLog& registro) {
  return FormatarDataParaOrdenacao(registro.ano, registro.mes, registro.dia) +
         " " + FormatarHorarioParaOrdenacao(registro);
}

/***************************************************************************
 * Funcao: ParsearRegistroObrigatorio
 * Descricao
 * Interpreta uma linha de log e rejeita a linha quando ela nao e valida.
 * Parametros
 * linha - linha completa de log.
 * Valor retornado
 * RegistroLog preenchido quando a linha e valida.
 * Assertiva de entrada
 * linha deve conter um registro candidato no formato esperado.
 * Assertiva de saida
 * Se retornar, o registro deve ser valido. Se a linha for invalida, deve
 * lancar std::invalid_argument e nao retornar registro parcial.
 ***************************************************************************/
RegistroLog ParsearRegistroObrigatorio(const std::string& linha) {
  RegistroLog registro = {};
  if (!ParsearLinhaLog(linha, &registro)) {
    throw std::invalid_argument("registro de log fora do formato esperado");
  }
  return registro;
}

/***************************************************************************
 * Funcao: ValidarFormatoRegistroLog
 * Descricao
 * Verifica se uma linha pode ser aceita como registro de log.
 * Parametros
 * registro - linha de log a ser validada.
 * Valor retornado
 * Nao retorna valor.
 * Assertiva de entrada
 * registro deve conter uma linha candidata de log.
 * Assertiva de saida
 * Se a funcao terminar normalmente, registro e valido. Caso contrario, uma
 * excecao deve indicar que o arquivo contem registro invalido.
 ***************************************************************************/
void ValidarFormatoRegistroLog(const std::string& registro) {
  ParsearRegistroObrigatorio(registro);
}

/***************************************************************************
 * Funcao: OrdenarRegistros
 * Descricao
 * Ordena os registros por data e hora, preservando a ordem relativa dos
 * registros com timestamps iguais.
 * Parametros
 * registros - ponteiro para o vetor de linhas de log.
 * Valor retornado
 * Nao retorna valor.
 * Assertiva de entrada
 * registros deve ser diferente de NULL e todas as linhas devem ser registros
 * validos.
 * Assertiva de saida
 * O vetor deve conter os mesmos elementos, em ordem cronologica estavel.
 ***************************************************************************/
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

/***************************************************************************
 * Funcao: ParsearArquivoRegistros
 * Descricao
 * Le um arquivo de log ou total, valida suas linhas nao vazias e ordena os
 * registros encontrados.
 * Parametros
 * caminho_arquivo - caminho do arquivo a ser lido.
 * Valor retornado
 * Vetor de registros validos ordenados por data e hora.
 * Assertiva de entrada
 * caminho_arquivo deve identificar um arquivo existente e legivel.
 * Assertiva de saida
 * O vetor retornado deve conter apenas registros validos. Linhas vazias sao
 * ignoradas. Linhas invalidas devem provocar excecao.
 ***************************************************************************/
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

/***************************************************************************
 * Funcao: ExtrairNomeBase
 * Descricao
 * Extrai o nome do arquivo de log a partir de caminho Unix ou Windows.
 * Parametros
 * caminho_log - caminho completo ou relativo do arquivo de log.
 * Valor retornado
 * Nome base localizado apos o ultimo separador '/' ou '\\'.
 * Assertiva de entrada
 * caminho_log nao deve ser vazio.
 * Assertiva de saida
 * O retorno nao deve conter diretorios e deve ser usado na composicao do
 * nome total_<nome>.
 ***************************************************************************/
std::string ExtrairNomeBase(const std::string& caminho_log) {
  const std::size_t separador = caminho_log.find_last_of("/\\");
  return separador == std::string::npos ? caminho_log
                                        : caminho_log.substr(separador + 1);
}

/***************************************************************************
 * Funcao: CaminhoTotal
 * Descricao
 * Calcula onde o arquivo total de um log deve ser gravado.
 * Parametros
 * caminho_lista - caminho do arquivo de lista de logs.
 * caminho_log - caminho do arquivo de log processado.
 * Valor retornado
 * Caminho no mesmo diretorio da lista, com nome total_<nome_base_do_log>.
 * Assertiva de entrada
 * caminho_log nao deve ser vazio e caminho_lista deve identificar a lista em
 * processamento.
 * Assertiva de saida
 * O retorno deve combinar o diretorio da lista com o prefixo total_ e o nome
 * base do log.
 ***************************************************************************/
std::filesystem::path CaminhoTotal(const std::filesystem::path& caminho_lista,
                                   const std::string& caminho_log) {
  return caminho_lista.parent_path() /
         ("total_" + ExtrairNomeBase(caminho_log));
}

/***************************************************************************
 * Funcao: EscreverTotal
 * Descricao
 * Grava o arquivo total por meio de arquivo temporario e substituicao final.
 * Parametros
 * caminho_total - caminho do arquivo total definitivo.
 * registros - registros que devem compor o total.
 * Valor retornado
 * Nao retorna valor.
 * Assertiva de entrada
 * registros deve conter apenas linhas validas e ja ordenadas.
 * Assertiva de saida
 * Se terminar normalmente, caminho_total deve conter exatamente os registros
 * informados, um por linha.
 ***************************************************************************/
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

/***************************************************************************
 * Funcao: ProcessarLog
 * Descricao
 * Processa um arquivo de log existente e combina seus registros com o total
 * correspondente, quando esse total ja existe.
 * Parametros
 * caminho_lista - caminho da lista usada para definir o diretorio do total.
 * caminho_log - caminho do arquivo de log a processar.
 * Valor retornado
 * kSucesso, kLogInvalido ou kTotalInvalido.
 * Assertiva de entrada
 * caminho_log deve existir e caminho_lista deve representar a lista em uso.
 * Assertiva de saida
 * Se retornar kSucesso, o total correspondente deve estar atualizado e
 * ordenado. Se retornar erro, o total existente nao deve ser regravado.
 ***************************************************************************/
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

/***************************************************************************
 * Funcao: ProcessarLinhaLista
 * Descricao
 * Executa a regra da tabela de decisao para uma linha da lista de logs.
 * Parametros
 * caminho_lista - caminho da lista em processamento.
 * linha - linha lida da lista.
 * contadores - ponteiro para os acumuladores do monitoramento.
 * Valor retornado
 * CodigoResultado da linha processada.
 * Assertiva de entrada
 * contadores deve ser diferente de NULL e conter valores nao negativos.
 * Assertiva de saida
 * Linhas vazias incrementam linhas_ignoradas; logs ausentes incrementam
 * logs_ignorados; logs processados com sucesso incrementam logs_processados
 * e totais_atualizados.
 ***************************************************************************/
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

/***************************************************************************
 * Funcao: PadraoLinhaLog
 * Descricao
 * Fornece a expressao regular que descreve o formato lexical do registro.
 * Parametros
 * Nao ha parametros.
 * Valor retornado
 * Referencia para a regex de linha de log.
 * Assertiva de entrada
 * Nao se aplica.
 * Assertiva de saida
 * A regex deve aceitar dia e mes com um ou dois digitos, ano com quatro
 * digitos, horario HH:MM:SS, dois espacos e mensagem de 1 a 100 caracteres.
 ***************************************************************************/
const std::regex& PadraoLinhaLog() {
  static const std::regex padrao_linha(
      R"(^([0-9]{1,2})/([0-9]{1,2})/([0-9]{4}) ([0-9]{1,2}):([0-9]{2}):([0-9]{2}) {2}(.{1,100})$)");
  return padrao_linha;
}

/***************************************************************************
 * Funcao: PreencherRegistroLog
 * Descricao
 * Copia os grupos capturados pela regex para a estrutura RegistroLog.
 * Parametros
 * grupos - grupos capturados por PadraoLinhaLog.
 * registro - ponteiro para receber os campos convertidos.
 * Valor retornado
 * Nao retorna valor.
 * Assertiva de entrada
 * registro deve ser diferente de NULL e grupos deve conter sete capturas
 * validas.
 * Assertiva de saida
 * registro deve conter os campos numericos convertidos e a mensagem copiada.
 ***************************************************************************/
void PreencherRegistroLog(const std::smatch& grupos, RegistroLog* registro) {
  registro->dia = std::stoi(grupos[1].str());
  registro->mes = std::stoi(grupos[2].str());
  registro->ano = std::stoi(grupos[3].str());
  registro->hora = std::stoi(grupos[4].str());
  registro->minuto = std::stoi(grupos[5].str());
  registro->segundo = std::stoi(grupos[6].str());
  registro->mensagem = grupos[7].str();
}

/***************************************************************************
 * Funcao: AnoBissexto
 * Descricao
 * Verifica se um ano segue a regra gregoriana de ano bissexto.
 * Parametros
 * ano - ano a validar.
 * Valor retornado
 * true se o ano for bissexto; false caso contrario.
 * Assertiva de entrada
 * ano deve estar preenchido com quatro digitos parseados da linha de log.
 * Assertiva de saida
 * O retorno deve ser true para anos divisiveis por 400 ou divisiveis por 4 e
 * nao divisiveis por 100.
 ***************************************************************************/
bool AnoBissexto(int ano) {
  return (ano % 4 == 0 && ano % 100 != 0) || ano % 400 == 0;
}

/***************************************************************************
 * Funcao: DiasNoMes
 * Descricao
 * Informa a quantidade de dias de um mes em determinado ano.
 * Parametros
 * mes - mes do ano.
 * ano - ano usado para avaliar fevereiro em ano bissexto.
 * Valor retornado
 * Quantidade maxima de dias do mes informado.
 * Assertiva de entrada
 * mes deve estar entre 1 e 12.
 * Assertiva de saida
 * O retorno deve respeitar a quantidade de dias do calendario gregoriano,
 * incluindo 29 dias em fevereiro de ano bissexto.
 ***************************************************************************/
int DiasNoMes(int mes, int ano) {
  const int dias_por_mes[] = {31, 28, 31, 30, 31, 30,
                              31, 31, 30, 31, 30, 31};
  if (mes == 2 && AnoBissexto(ano)) {
    return 29;
  }
  return dias_por_mes[mes - 1];
}

/***************************************************************************
 * Funcao: DataValida
 * Descricao
 * Verifica se os campos de data de um registro formam data gregoriana valida.
 * Parametros
 * registro - registro com dia, mes e ano preenchidos.
 * Valor retornado
 * true se a data for valida; false caso contrario.
 * Assertiva de entrada
 * registro deve ter sido preenchido a partir da regex de linha de log.
 * Assertiva de saida
 * O retorno deve rejeitar meses fora de 1..12 e dias fora do intervalo
 * permitido para o mes e ano informados.
 ***************************************************************************/
bool DataValida(const RegistroLog& registro) {
  if (registro.mes < 1 || registro.mes > 12) {
    return false;
  }

  const int ultimo_dia = DiasNoMes(registro.mes, registro.ano);

  return registro.dia >= 1 && registro.dia <= ultimo_dia;
}

/***************************************************************************
 * Funcao: HorarioValido
 * Descricao
 * Verifica se os campos de horario de um registro estao nos limites validos.
 * Parametros
 * registro - registro com hora, minuto e segundo preenchidos.
 * Valor retornado
 * true se o horario for valido; false caso contrario.
 * Assertiva de entrada
 * registro deve ter sido preenchido a partir da regex de linha de log.
 * Assertiva de saida
 * O retorno deve aceitar hora em 0..23 e minuto e segundo em 0..59.
 ***************************************************************************/
bool HorarioValido(const RegistroLog& registro) {
  return registro.hora >= 0 && registro.hora <= 23 && registro.minuto >= 0 &&
         registro.minuto <= 59 && registro.segundo >= 0 &&
         registro.segundo <= 59;
}

/***************************************************************************
 * Funcao: RegistroLogValido
 * Descricao
 * Consolida as validacoes semanticas de data e horario do registro.
 * Parametros
 * registro - registro preenchido a partir da linha de log.
 * Valor retornado
 * true se data e horario forem validos; false caso contrario.
 * Assertiva de entrada
 * registro deve ter campos numericos e mensagem preenchidos pela regex.
 * Assertiva de saida
 * O retorno deve ser true somente quando DataValida e HorarioValido forem
 * verdadeiras.
 ***************************************************************************/
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
