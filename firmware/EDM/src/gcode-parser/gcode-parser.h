#ifndef GCODE_PARSER_H
#define GCODE_PARSER_H
#include "core.h"

enum class gcode_code_t {
    G0, G1, G28, G90, G91, G92, 
    M0, M2, M3, M5, M7, M8, M9, M30,
};

struct gcode_cmd_t {
    gcode_code_t code;
    std::optional<float> x { };
    std::optional<float> y { };
};

class gcode_parser_t {
public:
    using result_t = std::expected<gcode_cmd_t, bool>;

    /// ***************************************************************************
    /// @brief  Parse one G-code line into structured command
    /// @param  line: input string view
    /// @return parsed command or parse error
    /// ***************************************************************************
    result_t parse_line(std::string_view line) const;

protected:
    /// ***************************************************************************
    /// @brief  Parse one G-command with optional X/Y parameters
    /// @param  code: numeric G-code value after letter G
    /// @param  line: remaining line tail after command code
    /// @return parsed command or parse error
    /// ***************************************************************************
    result_t parse_g_command(uint32_t code, std::string_view line) const;

    /// ***************************************************************************
    /// @brief  Parse one M-command without additional parameters
    /// @param  code: numeric M-code value after letter M
    /// @param  line: remaining line tail after command code
    /// @return parsed command or parse error
    /// ***************************************************************************
    result_t parse_m_command(uint32_t code, std::string_view line) const;

    /// ***************************************************************************
    /// @brief  Skip leading whitespace characters in the input view
    /// @param  line: remaining input line to trim in place
    /// ***************************************************************************
    void skip_spaces(std::string_view& line) const;

    /// ***************************************************************************
    /// @brief  Parse unsigned 32-bit integer from the line head
    /// @param  line: remaining input line, advanced on success
    /// @return parsed value or empty optional on failure
    /// ***************************************************************************
    std::optional<uint32_t> parse_uint32(std::string_view& line) const;

    /// ***************************************************************************
    /// @brief  Parse floating-point value from the line head
    /// @param  line: remaining input line, advanced on success
    /// @return parsed value or empty optional on failure
    /// ***************************************************************************
    std::optional<float> parse_float(std::string_view& line) const;
};

#endif // GCODE_PARSER_H