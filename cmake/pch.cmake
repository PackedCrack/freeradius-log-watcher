SET(STD_PCH <cstdint>
          <vector>
          <array>
          <chrono>
        <stdexcept>
        <string>
        <cstring>
        <string_view>
        <expected>
        <format>
        <cerrno>
        <filesystem>
        <functional>
        <unordered_set>
        <utility>
        <type_traits>
        <fstream>
        <thread>
        <concepts>
)

# --- functions --- #
function(use_pch PROJ)
  target_precompile_headers(${PROJ}
    PRIVATE
    ${STD_PCH}
  )
endfunction()