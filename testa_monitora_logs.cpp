#include <catch2/catch_test_macros.hpp>

#include "monitora_logs.hpp"

TEST_CASE("TD01 lista de logs inexistente retorna erro", "[td01]") {
  const monitora_logs::ResultadoMonitoramento resultado =
      monitora_logs::MonitorarLogs("arquivo_inexistente_logs.txt");

  REQUIRE(resultado.codigo ==
          monitora_logs::CodigoResultado::kListaLogsInexistente);
}
