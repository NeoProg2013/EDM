#include "gcode-parser.h"

/// ***************************************************************************
/// @brief  Parse one G-code line into structured command
/// @param  line: input string view
/// @return parsed command or parse error
/// ***************************************************************************
gcode_parser_t::result_t gcode_parser_t::parse_line(std::string_view input) const {
    skip_spaces(input);

    if (input.empty()) {
        return std::unexpected(false);
    }

    char cmd_letter = toupper(input.front());
    if (cmd_letter != 'G' && cmd_letter != 'M') {
        return std::unexpected(false);
    }
    input.remove_prefix(1);

    std::optional<uint32_t> cmd_code = parse_uint32(input);
    if (!cmd_code.has_value()) {
        return std::unexpected(false);
    }

    if (cmd_letter == 'G') {
        return parse_g_command(*cmd_code, input);
    }
    return parse_m_command(*cmd_code, input);
}



//
// protected API
/// ***************************************************************************
/// @brief  Parse one G-command with optional X/Y parameters
/// @param  code: numeric G-code value after letter G
/// @param  line: remaining line tail after command code
/// @return parsed command or parse error
/// ***************************************************************************
gcode_parser_t::result_t gcode_parser_t::parse_g_command(uint32_t code, std::string_view line) const {
    gcode_cmd_t cmd { .code = gcode_code_t::G0 };

    switch (code) {
    case 0:  cmd.code = gcode_code_t::G0;  break;
    case 1:  cmd.code = gcode_code_t::G1;  break;
    case 28: cmd.code = gcode_code_t::G28; break;
    case 90: cmd.code = gcode_code_t::G90; break;
    case 91: cmd.code = gcode_code_t::G91; break;
    case 92: cmd.code = gcode_code_t::G92; break;
    default: return std::unexpected(false);
    }

    while (true) {
        skip_spaces(line);
        if (line.empty()) {
            break;
        }

        char word = toupper(line.front());
        if (word != 'X' && word != 'Y') {
            return std::unexpected(false);
        }

        line.remove_prefix(1);

        std::optional<float> value = parse_float(line);
        if (!value.has_value()) {
            return std::unexpected(false);
        }

        if (word == 'X') {
            if (cmd.x.has_value()) {
                return std::unexpected(false);
            }
            cmd.x = *value;
        } else {
            if (cmd.y.has_value()) {
                return std::unexpected(false);
            }
            cmd.y = *value;
        }
    }

    // Validation
    switch (cmd.code) {
    case gcode_code_t::G0:
    case gcode_code_t::G1:
    case gcode_code_t::G92:
        if (!cmd.x.has_value() && !cmd.y.has_value()) {
            return std::unexpected(false);
        }
        break;

    case gcode_code_t::G28:
    case gcode_code_t::G90:
    case gcode_code_t::G91:
        if (cmd.x.has_value() || cmd.y.has_value()) {
            return std::unexpected(false);
        }
        break;

    default:
        break;
    }

    return cmd;
}

/// ***************************************************************************
/// @brief  Parse one M-command without additional parameters
/// @param  code: numeric M-code value after letter M
/// @param  line: remaining line tail after command code
/// @return parsed command or parse error
/// ***************************************************************************
gcode_parser_t::result_t gcode_parser_t::parse_m_command(uint32_t code, std::string_view line) const {
    gcode_cmd_t cmd { .code = gcode_code_t::M0 };

    switch (code) {
    case 0:  cmd.code = gcode_code_t::M0;  break;
    case 2:  cmd.code = gcode_code_t::M2;  break;
    case 3:  cmd.code = gcode_code_t::M3;  break;
    case 5:  cmd.code = gcode_code_t::M5;  break;
    case 7:  cmd.code = gcode_code_t::M7;  break;
    case 8:  cmd.code = gcode_code_t::M8;  break;
    case 9:  cmd.code = gcode_code_t::M9;  break;
    case 30: cmd.code = gcode_code_t::M30; break;
    default: return std::unexpected(false);
    }

    skip_spaces(line);
    if (!line.empty()) {
        return std::unexpected(false);
    }

    return cmd;
}

/// ***************************************************************************
/// @brief  Skip leading whitespace characters in the input view
/// @param  line: remaining input line to trim in place
/// ***************************************************************************
void gcode_parser_t::skip_spaces(std::string_view& line) const {
    while (!line.empty() && isspace(line.front()) != 0) {
        line.remove_prefix(1);
    }
}

/// ***************************************************************************
/// @brief  Parse unsigned 32-bit integer from the line head
/// @param  line: remaining input line, advanced on success
/// @return parsed value or empty optional on failure
/// ***************************************************************************
std::optional<uint32_t> gcode_parser_t::parse_uint32(std::string_view& line) const {
    if (line.empty() || !isdigit(line.front())) {
        return std::nullopt;
    }

    uint32_t result = 0;
    while (!line.empty() && isdigit(line.front())) {
        // Convert ASCII to digit
        uint32_t digit = static_cast<uint32_t>(line.front() - '0');

        // Append digit place to result
        result = result * 10U + digit;

        line.remove_prefix(1);
    }

    return result;
}

/// ***************************************************************************
/// @brief  Parse floating-point value from the line head
/// @param  line: remaining input line, advanced on success
/// @return parsed value or empty optional on failure
/// ***************************************************************************
std::optional<float> gcode_parser_t::parse_float(std::string_view& line) const {
    if (line.empty()) {
        return std::nullopt;
    }

    uint32_t token_length = 0;

    // Skip sign
    if (line[token_length] == '+' || line[token_length] == '-') {
        ++token_length;
        if (token_length >= line.size()) {
            return std::nullopt;
        }
    }

    // Search end of number
    bool has_digit = false;
    bool has_dot = false;

    while (token_length < line.size()) {
        char ch = line[token_length];
        if (isdigit(ch)) {
            has_digit = true;
            ++token_length;
            continue;
        }

        if (ch == '.' && !has_dot) {
            has_dot = true;
            ++token_length;
            continue;
        }

        break;
    }

    if (!has_digit) {
        return std::nullopt;
    }

    // Copy token to local buffer
    char buffer[32] { };
    if (token_length >= sizeof(buffer)) {
        return std::nullopt;
    }
    memcpy(buffer, line.data(), token_length);

    // Convert string to float
    char* end_ptr = nullptr;
    float value = strtof(buffer, &end_ptr);
    if (end_ptr == buffer) {
        return std::nullopt;
    }

    // Remove parsed token
    line.remove_prefix(token_length);
    
    return value;
}

