#ifndef MONITORA_LOGS_HPP_
#define MONITORA_LOGS_HPP_

#include <string>

namespace monitora_logs {

/***************************************************************************
 * Tipo: CodigoResultado
 * Descricao
 * Representa o estado final do processamento solicitado.
 * Valores
 * kSucesso - todos os logs existentes e validos foram processados.
 * kListaLogsInexistente - a lista de logs nao pode ser aberta.
 * kLogInvalido - um arquivo de log existente contem registro invalido.
 * kTotalInvalido - um arquivo total existente contem registro invalido.
 * Assertiva de entrada
 * Nao se aplica.
 * Assertiva de saida
 * Todo ResultadoMonitoramento deve usar um destes codigos para indicar a
 * situacao prevista pela tabela de decisao.
 ***************************************************************************/
enum class CodigoResultado {
  kSucesso,
  kListaLogsInexistente,
  kLogInvalido,
  kTotalInvalido,
};

/***************************************************************************
 * Tipo: ResultadoMonitoramento
 * Descricao
 * Agrupa o codigo de resultado e os contadores produzidos pelo monitoramento.
 * Campos
 * codigo - situacao final encontrada no processamento.
 * logs_processados - quantidade de logs existentes e validos processados.
 * linhas_ignoradas - quantidade de linhas vazias ignoradas na lista.
 * logs_ignorados - quantidade de caminhos ausentes ignorados na lista.
 * totais_atualizados - quantidade de arquivos totais escritos com sucesso.
 * Assertiva de entrada
 * Nao se aplica.
 * Assertiva de saida
 * Os contadores devem ser maiores ou iguais a zero. Em caso de sucesso,
 * codigo deve ser kSucesso; em caso de falha, codigo deve indicar a primeira
 * falha prevista encontrada.
 ***************************************************************************/
struct ResultadoMonitoramento {
  CodigoResultado codigo;
  int logs_processados;
  int linhas_ignoradas;
  int logs_ignorados;
  int totais_atualizados;
};

/***************************************************************************
 * Tipo: RegistroLog
 * Descricao
 * Representa os campos extraidos de uma linha valida de log.
 * Campos
 * dia - dia do mes.
 * mes - mes do ano.
 * ano - ano com quatro digitos.
 * hora - hora do dia.
 * minuto - minuto da hora.
 * segundo - segundo do minuto.
 * mensagem - texto do registro, com tamanho entre 1 e 100 caracteres.
 * Assertiva de entrada
 * Nao se aplica.
 * Assertiva de saida
 * Quando produzido por ParsearLinhaLog com retorno true, deve conter data
 * gregoriana valida, horario valido e mensagem nao vazia.
 ***************************************************************************/
struct RegistroLog {
  int dia;
  int mes;
  int ano;
  int hora;
  int minuto;
  int segundo;
  std::string mensagem;
};

/***************************************************************************
 * Funcao: MonitorarLogs
 * Descricao
 * Executa o monitoramento dos arquivos listados em um arquivo de entrada.
 * Parametros
 * caminho_lista_logs - caminho do arquivo que contem a lista de logs.
 * Valor retornado
 * Resultado do monitoramento, incluindo o codigo da situacao encontrada e a
 * quantidade de arquivos de log processados, linhas de lista ignoradas, logs
 * ausentes ignorados e arquivos totais atualizados.
 * Assertiva de entrada
 * caminho_lista_logs nao deve ser vazio.
 * Assertiva de saida
 * O codigo retornado deve representar sucesso ou uma falha prevista pela
 * tabela de decisao, logs_processados deve ser maior ou igual a zero,
 * linhas_ignoradas deve ser maior ou igual a zero, e logs_ignorados deve ser
 * maior ou igual a zero. totais_atualizados deve ser maior ou igual a zero.
 ***************************************************************************/
ResultadoMonitoramento MonitorarLogs(const std::string& caminho_lista_logs);

/***************************************************************************
 * Funcao: ParsearLinhaLog
 * Descricao
 * Verifica e extrai os campos de uma linha de log.
 * Parametros
 * linha - linha completa no formato data, hora, dois espacos e mensagem.
 * registro - ponteiro para receber os campos extraidos quando a linha e
 * valida.
 * Valor retornado
 * true se a linha for valida; false caso contrario.
 * Assertiva de entrada
 * registro deve ser diferente de NULL.
 * Assertiva de saida
 * Se retornar true, registro deve conter data, hora e mensagem extraidas da
 * linha. Se retornar false, a linha nao deve ser aceita como registro valido.
 ***************************************************************************/
bool ParsearLinhaLog(const std::string& linha, RegistroLog* registro);

}  // namespace monitora_logs

#endif  // MONITORA_LOGS_HPP_
