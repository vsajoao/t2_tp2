#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "monitora_logs.hpp"

TEST_CASE("TD01 lista de logs inexistente retorna erro", "[td01]") {
  const monitora_logs::ResultadoMonitoramento resultado =
      monitora_logs::MonitorarLogs("arquivo_inexistente_logs.txt");

  REQUIRE(resultado.codigo ==
          monitora_logs::CodigoResultado::kListaLogsInexistente);
}

TEST_CASE("TD02 lista vazia nao gera arquivos totais", "[td02]") {
  const std::filesystem::path diretorio_teste =
      std::filesystem::temp_directory_path() / "monitora_logs_td02";
  std::filesystem::create_directories(diretorio_teste);

  const std::filesystem::path caminho_lista =
      diretorio_teste / "logs_vazio.txt";
  std::ofstream(caminho_lista).close();

  const monitora_logs::ResultadoMonitoramento resultado =
      monitora_logs::MonitorarLogs(caminho_lista.string());

  REQUIRE(resultado.codigo == monitora_logs::CodigoResultado::kSucesso);
  REQUIRE(resultado.logs_processados == 0);
  REQUIRE_FALSE(std::filesystem::exists(diretorio_teste / "total_log1.txt"));
}

TEST_CASE("TD03 linha vazia na lista e ignorada", "[td03]") {
  const std::filesystem::path diretorio_teste =
      std::filesystem::temp_directory_path() / "monitora_logs_td03";
  std::filesystem::create_directories(diretorio_teste);

  const std::filesystem::path caminho_lista = diretorio_teste / "logs.txt";
  std::ofstream lista(caminho_lista);
  lista << "\n";
  lista.close();

  const monitora_logs::ResultadoMonitoramento resultado =
      monitora_logs::MonitorarLogs(caminho_lista.string());

  REQUIRE(resultado.codigo == monitora_logs::CodigoResultado::kSucesso);
  REQUIRE(resultado.logs_processados == 0);
  REQUIRE(resultado.linhas_ignoradas == 1);
  REQUIRE_FALSE(std::filesystem::exists(diretorio_teste / "total_log1.txt"));
}

TEST_CASE("TD04 log listado inexistente e ignorado", "[td04]") {
  const std::filesystem::path diretorio_teste =
      std::filesystem::temp_directory_path() / "monitora_logs_td04";
  std::filesystem::create_directories(diretorio_teste);

  const std::filesystem::path caminho_log_inexistente =
      diretorio_teste / "log1.txt";
  const std::filesystem::path caminho_lista = diretorio_teste / "logs.txt";
  std::ofstream lista(caminho_lista);
  lista << caminho_log_inexistente.string() << "\n";
  lista.close();

  const monitora_logs::ResultadoMonitoramento resultado =
      monitora_logs::MonitorarLogs(caminho_lista.string());

  REQUIRE(resultado.codigo == monitora_logs::CodigoResultado::kSucesso);
  REQUIRE(resultado.logs_processados == 0);
  REQUIRE(resultado.logs_ignorados == 1);
  REQUIRE_FALSE(std::filesystem::exists(diretorio_teste / "total_log1.txt"));
}

TEST_CASE("TD05 cria total para log valido sem total anterior", "[td05]") {
  const std::filesystem::path diretorio_teste =
      std::filesystem::temp_directory_path() / "monitora_logs_td05";
  std::filesystem::remove_all(diretorio_teste);
  std::filesystem::create_directories(diretorio_teste);

  const std::filesystem::path caminho_log = diretorio_teste / "log1.txt";
  std::ofstream log(caminho_log);
  log << "20/1/2026 17:45:38  Segundo registro\n";
  log << "16/1/2026 13:27:46  Primeiro registro\n";
  log.close();

  const std::filesystem::path caminho_lista = diretorio_teste / "logs.txt";
  std::ofstream lista(caminho_lista);
  lista << caminho_log.string() << "\n";
  lista.close();

  const monitora_logs::ResultadoMonitoramento resultado =
      monitora_logs::MonitorarLogs(caminho_lista.string());

  const std::filesystem::path caminho_total =
      diretorio_teste / "total_log1.txt";
  REQUIRE(resultado.codigo == monitora_logs::CodigoResultado::kSucesso);
  REQUIRE(resultado.logs_processados == 1);
  REQUIRE(std::filesystem::exists(caminho_total));

  std::ifstream total(caminho_total);
  std::string primeira_linha;
  std::string segunda_linha;
  std::getline(total, primeira_linha);
  std::getline(total, segunda_linha);

  REQUIRE(primeira_linha == "16/1/2026 13:27:46  Primeiro registro");
  REQUIRE(segunda_linha == "20/1/2026 17:45:38  Segundo registro");
}

TEST_CASE("TD06 faz merge com total existente valido", "[td06]") {
  const std::filesystem::path diretorio_teste =
      std::filesystem::temp_directory_path() / "monitora_logs_td06";
  std::filesystem::remove_all(diretorio_teste);
  std::filesystem::create_directories(diretorio_teste);

  const std::filesystem::path caminho_total =
      diretorio_teste / "total_log1.txt";
  std::ofstream total_existente(caminho_total);
  total_existente << "17/1/2026 14:17:46  Registro ja totalizado\n";
  total_existente << "20/1/2026 17:45:38  Registro total mesmo instante\n";
  total_existente << "21/1/2026 18:55:38  Registro final\n";
  total_existente.close();

  const std::filesystem::path caminho_log = diretorio_teste / "log1.txt";
  std::ofstream log(caminho_log);
  log << "20/1/2026 17:45:38  Registro novo mesmo instante\n";
  log << "16/1/2026 13:27:46  Registro inicial\n";
  log.close();

  const std::filesystem::path caminho_lista = diretorio_teste / "logs.txt";
  std::ofstream lista(caminho_lista);
  lista << caminho_log.string() << "\n";
  lista.close();

  const monitora_logs::ResultadoMonitoramento resultado =
      monitora_logs::MonitorarLogs(caminho_lista.string());

  REQUIRE(resultado.codigo == monitora_logs::CodigoResultado::kSucesso);
  REQUIRE(resultado.logs_processados == 1);

  std::ifstream total_final(caminho_total);
  std::vector<std::string> registros;
  std::string registro;
  while (std::getline(total_final, registro)) {
    registros.push_back(registro);
  }

  REQUIRE(registros.size() == 5);
  REQUIRE(registros.front() == "16/1/2026 13:27:46  Registro inicial");
  REQUIRE(registros.back() == "21/1/2026 18:55:38  Registro final");
  REQUIRE(registros[2] == "20/1/2026 17:45:38  Registro total mesmo instante");
  REQUIRE(registros[3] == "20/1/2026 17:45:38  Registro novo mesmo instante");
}
