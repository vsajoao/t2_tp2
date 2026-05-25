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

TEST_CASE("TD07 log invalido gera erro e preserva total", "[td07]") {
  const std::filesystem::path diretorio_teste =
      std::filesystem::temp_directory_path() / "monitora_logs_td07";
  std::filesystem::remove_all(diretorio_teste);
  std::filesystem::create_directories(diretorio_teste);

  const std::filesystem::path caminho_total =
      diretorio_teste / "total_log1.txt";
  const std::string conteudo_total_original =
      "17/1/2026 14:17:46  Registro ja totalizado\n";
  std::ofstream(caminho_total) << conteudo_total_original;

  const std::filesystem::path caminho_log = diretorio_teste / "log1.txt";
  std::ofstream(caminho_log) << "linha invalida\n";

  const std::filesystem::path caminho_lista = diretorio_teste / "logs.txt";
  std::ofstream(caminho_lista) << caminho_log.string() << "\n";

  const monitora_logs::ResultadoMonitoramento resultado =
      monitora_logs::MonitorarLogs(caminho_lista.string());

  REQUIRE(resultado.codigo == monitora_logs::CodigoResultado::kLogInvalido);
  REQUIRE(resultado.logs_processados == 0);

  std::ifstream total_preservado(caminho_total);
  const std::string conteudo_preservado(
      (std::istreambuf_iterator<char>(total_preservado)),
      std::istreambuf_iterator<char>());

  REQUIRE(conteudo_preservado == conteudo_total_original);
}

TEST_CASE("TD08 total existente invalido gera erro", "[td08]") {
  const std::filesystem::path diretorio_teste =
      std::filesystem::temp_directory_path() / "monitora_logs_td08";
  std::filesystem::remove_all(diretorio_teste);
  std::filesystem::create_directories(diretorio_teste);

  const std::filesystem::path caminho_total =
      diretorio_teste / "total_log1.txt";
  const std::string conteudo_total_original = "total invalido\n";
  std::ofstream(caminho_total) << conteudo_total_original;

  const std::filesystem::path caminho_log = diretorio_teste / "log1.txt";
  std::ofstream(caminho_log) << "16/1/2026 13:27:46  Registro novo\n";

  const std::filesystem::path caminho_lista = diretorio_teste / "logs.txt";
  std::ofstream(caminho_lista) << caminho_log.string() << "\n";

  const monitora_logs::ResultadoMonitoramento resultado =
      monitora_logs::MonitorarLogs(caminho_lista.string());

  REQUIRE(resultado.codigo == monitora_logs::CodigoResultado::kTotalInvalido);
  REQUIRE(resultado.logs_processados == 0);

  std::ifstream total_preservado(caminho_total);
  const std::string conteudo_preservado(
      (std::istreambuf_iterator<char>(total_preservado)),
      std::istreambuf_iterator<char>());

  REQUIRE(conteudo_preservado == conteudo_total_original);
}

TEST_CASE("TD09 logs de mesmo nome em diretorios diferentes combinam",
          "[td09]") {
  const std::filesystem::path diretorio_teste =
      std::filesystem::temp_directory_path() / "monitora_logs_td09";
  std::filesystem::remove_all(diretorio_teste);
  std::filesystem::create_directories(diretorio_teste / "origem_unix");

  const std::filesystem::path caminho_log_unix =
      diretorio_teste / "origem_unix" / "log1.txt";
  std::ofstream(caminho_log_unix) << "18/1/2026 11:34:21  Registro Unix\n";

  const std::filesystem::path caminho_log_windows =
      diretorio_teste / "origem_windows\\log1.txt";
  std::ofstream(caminho_log_windows)
      << "16/1/2026 13:27:46  Registro Windows\n";

  const std::filesystem::path caminho_lista = diretorio_teste / "logs.txt";
  std::ofstream lista(caminho_lista);
  lista << caminho_log_unix.string() << "\n";
  lista << caminho_log_windows.string() << "\n";
  lista.close();

  const monitora_logs::ResultadoMonitoramento resultado =
      monitora_logs::MonitorarLogs(caminho_lista.string());

  REQUIRE(resultado.codigo == monitora_logs::CodigoResultado::kSucesso);
  REQUIRE(resultado.logs_processados == 2);

  const std::filesystem::path caminho_total =
      diretorio_teste / "total_log1.txt";
  std::ifstream total(caminho_total);
  std::vector<std::string> registros;
  std::string registro;
  while (std::getline(total, registro)) {
    registros.push_back(registro);
  }

  REQUIRE(registros.size() == 2);
  REQUIRE(registros[0] == "16/1/2026 13:27:46  Registro Windows");
  REQUIRE(registros[1] == "18/1/2026 11:34:21  Registro Unix");
}

TEST_CASE("TD10 logs de nomes diferentes geram totais separados", "[td10]") {
  const std::filesystem::path diretorio_teste =
      std::filesystem::temp_directory_path() / "monitora_logs_td10";
  std::filesystem::remove_all(diretorio_teste);
  std::filesystem::create_directories(diretorio_teste);

  const std::filesystem::path caminho_log_um = diretorio_teste / "log1.txt";
  std::ofstream(caminho_log_um) << "16/1/2026 13:27:46  Registro log 1\n";

  const std::filesystem::path caminho_log_dois = diretorio_teste / "log2.txt";
  std::ofstream(caminho_log_dois) << "17/1/2026 14:17:46  Registro log 2\n";

  const std::filesystem::path caminho_lista = diretorio_teste / "logs.txt";
  std::ofstream lista(caminho_lista);
  lista << caminho_log_um.string() << "\n";
  lista << caminho_log_dois.string() << "\n";
  lista.close();

  const monitora_logs::ResultadoMonitoramento resultado =
      monitora_logs::MonitorarLogs(caminho_lista.string());

  REQUIRE(resultado.codigo == monitora_logs::CodigoResultado::kSucesso);
  REQUIRE(resultado.logs_processados == 2);
  REQUIRE(resultado.totais_atualizados == 2);

  const std::filesystem::path caminho_total_um =
      diretorio_teste / "total_log1.txt";
  const std::filesystem::path caminho_total_dois =
      diretorio_teste / "total_log2.txt";
  REQUIRE(std::filesystem::exists(caminho_total_um));
  REQUIRE(std::filesystem::exists(caminho_total_dois));

  std::ifstream total_um(caminho_total_um);
  std::ifstream total_dois(caminho_total_dois);
  std::string registro_um;
  std::string registro_dois;
  std::getline(total_um, registro_um);
  std::getline(total_dois, registro_dois);

  REQUIRE(registro_um == "16/1/2026 13:27:46  Registro log 1");
  REQUIRE(registro_dois == "17/1/2026 14:17:46  Registro log 2");
}
