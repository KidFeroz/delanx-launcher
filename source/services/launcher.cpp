// Delanx — encadenado de NROs con envSetNextLoad (compatible con hbmenu, Sphaira y
// forwarders NSP basados en nx-hbloader).
#include "services/launcher.hpp"

#include <switch.h>

#include <cstdio>
#include <string>

#include "core/paths.hpp"
#include "core/strings.hpp"
#include "platform/switch_platform.hpp"
#include "services/launch_args.hpp"

namespace delanx {

bool can_chain_nro() { return envHasNextLoad(); }

bool request_next_nro(const LaunchTarget& target, std::string* error) {
  const std::string raw = str::trim(target.nro_path);
  if (raw.empty()) {
    if (error != nullptr) *error = "La entrada no tiene ninguna ruta de .nro";
    return false;
  }

  // Sphaira antepone "sdmc:" si falta; el ABI exige que la ruta empiece por "sdmc:/".
  const std::string nro_path = paths::to_sd_path(raw);
  if (nro_path.size() >= kNextLoadPathMaxBytes) {
    if (error != nullptr) *error = "La ruta del .nro es demasiado larga para el loader";
    return false;
  }
  if (!paths::exists(nro_path)) {
    platform::log("launcher: no existe %s", nro_path.c_str());
    if (error != nullptr) *error = "No se encontro " + nro_path;
    return false;
  }

  // envHasNextLoad() ya existe en la libnx instalada (verificado en env.h), pero se
  // mantiene el doble camino por si se compila contra una version anterior: alli
  // envSetNextLoad devuelve LibnxError_NotInitialized cuando el loader no expuso
  // EntryType_NextLoadPath. No se usa como sonda porque no se debe tocar el buffer
  // del loader si el encadenado no va a completarse.
  const bool available = envHasNextLoad();
  if (!available) {
    if (error != nullptr) {
      *error = "El loader que lanzo Delanx no expone EntryType_NextLoadPath, asi que no se pueden "
               "encadenar NROs. Lanza Delanx desde hbmenu o Sphaira (o desde un forwarder NSP basado "
               "en nx-hbloader).";
    }
    return false;
  }

  bool truncated = false;
  const std::string argv_string = build_launch_argv(nro_path, target.args, &truncated);
  if (truncated) platform::log("launcher: argumentos recortados a %zu bytes", argv_string.size());
  if (argv_string.size() >= kNextLoadArgvMaxBytes) {
    if (error != nullptr) *error = "Los argumentos del .nro son demasiado largos para el loader";
    return false;
  }

  const Result rc = envSetNextLoad(nro_path.c_str(), argv_string.c_str());
  if (R_FAILED(rc)) {
    if (error != nullptr) {
      char detail[64];
      snprintf(detail, sizeof(detail), "0x%08x", static_cast<unsigned>(rc));
      *error = std::string("No se pudo preparar el lanzamiento (Result ") + detail + ")";
    }
    return false;
  }

  platform::log("launcher: siguiente NRO %s args=[%s]", nro_path.c_str(), argv_string.c_str());
  return true;
}

std::string boot_origin_description() {
  const std::string origin = platform::boot_origin();
  if (origin == "hbmenu") return "hbmenu";
  if (origin == "sphaira") return "Sphaira";
  if (origin == "forwarder") return "Forwarder NSP";
  if (origin == "album") return "Album (applet)";
  return "Desconocido";
}

}  // namespace delanx
