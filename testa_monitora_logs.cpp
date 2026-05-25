#include <filesystem>
#include <fstream>

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
