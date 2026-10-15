#pragma once
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace bitwork
{
inline std::uint32_t mask(int width) { return width == 32 ? UINT32_MAX : (1u << width) - 1u; }
inline std::int64_t signedValue(std::uint32_t value, int width)
{
    return (value & (1u << (width - 1))) ? static_cast<std::int64_t>(value) - (1LL << width) : value;
}

// 各演算を指定幅に切り詰め、C++ と同じ優先順位で評価する。
class Expression
{
public:
    Expression(std::string_view text, int base, int width, bool isSigned)
        : text(text), base(base), width(width), isSigned(isSigned), bits(mask(width)) {}
    std::uint32_t evaluate()
    {
        const auto value = parse(1);
        spaces();
        if (pos != text.size()) fail("Unexpected character or unsupported operator.");
        return value;
    }
private:
    std::string_view text;
    int base, width;
    bool isSigned;
    std::uint32_t bits;
    std::size_t pos = 0;
    int depth = 0;
    [[noreturn]] void fail(const char* message) { throw std::runtime_error(message); }
    void spaces() { while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t')) ++pos; }
    std::int64_t number(std::uint32_t value) const { return isSigned ? signedValue(value, width) : value; }
    std::uint32_t primary()
    {
        spaces();
        if (++depth > 64) fail("Expression nesting is too deep (maximum 64).");
        if (pos == text.size()) fail("A number is required.");
        const char c = text[pos];
        std::uint32_t value = 0;
        if (c == '+' || c == '-' || c == '~')
        {
            ++pos;
            value = primary();
            if (c == '-') value = (0u - value) & bits;
            if (c == '~') value = (~value) & bits;
        }
        else if (c == '(')
        {
            ++pos;
            value = parse(1);
            spaces();
            if (pos == text.size() || text[pos++] != ')') fail("Missing closing parenthesis.");
        }
        else
        {
            int radix = base;
            if (pos + 1 < text.size() && text[pos] == '0')
            {
                if (text[pos + 1] == 'x' || text[pos + 1] == 'X') { radix = 16; pos += 2; }
                else if (text[pos + 1] == 'b' || text[pos + 1] == 'B') { radix = 2; pos += 2; }
            }
            const auto start = pos;
            std::uint64_t literal = 0;
            while (pos < text.size())
            {
                const char digit = text[pos];
                const int n = digit >= '0' && digit <= '9' ? digit - '0' :
                    digit >= 'a' && digit <= 'f' ? digit - 'a' + 10 :
                    digit >= 'A' && digit <= 'F' ? digit - 'A' + 10 : -1;
                if (n < 0 || n >= radix) break;
                literal = literal * radix + n;
                if (literal > UINT32_MAX) fail("Literal exceeds 32 bits.");
                ++pos;
            }
            if (start == pos) fail("Invalid number for this field.");
            value = static_cast<std::uint32_t>(literal) & bits;
        }
        --depth;
        return value;
    }
    std::uint32_t parse(int minimum)
    {
        auto left = primary();
        for (;;)
        {
            spaces();
            if (pos == text.size()) break;
            const auto tail = text.substr(pos);
            std::string_view op = tail.substr(0, 1);
            if (tail.starts_with("<<") || tail.starts_with(">>")) op = tail.substr(0, 2);
            const int priority = op == "|" ? 1 : op == "^" ? 2 : op == "&" ? 3 :
                op == "<<" || op == ">>" ? 4 : op == "+" || op == "-" ? 5 :
                op == "*" || op == "/" || op == "%" ? 6 : 0;
            if (priority < minimum) break;
            pos += op.size();
            const auto right = parse(priority + 1);
            if (op == "+") left += right;
            else if (op == "-") left -= right;
            else if (op == "*") left = static_cast<std::uint32_t>(static_cast<std::uint64_t>(left) * right);
            else if (op == "/" || op == "%")
            {
                if (!right) fail("Division by zero.");
                left = static_cast<std::uint32_t>(op == "/" ? number(left) / number(right) : number(left) % number(right));
            }
            else if (op == "&") left &= right;
            else if (op == "^") left ^= right;
            else if (op == "|") left |= right;
            else
            {
                if (right >= static_cast<std::uint32_t>(width)) fail("Shift count must be smaller than the bit width.");
                if (op == "<<") left <<= right;
                else if (isSigned) left = static_cast<std::uint32_t>(signedValue(left, width) >> right);
                else left >>= right;
            }
            left &= bits;
        }
        return left;
    }
};
}
