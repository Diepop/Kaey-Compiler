#pragma once

#define KAEY_TOKEN(name, pattern, ch) \
struct name : Token::With<name> \
{ \
    static inline auto&& Pattern = pattern; \
    static constexpr auto TokenChannel = ch; \
    static std::unique_ptr<Token> Create(StringStream* ss, ptrdiff_t position, size_t line, size_t column) { \
        for (auto [i1, i2] = std::make_tuple(Pattern, ss->CurrentIterator()); *i1; ++i1, ++i2) if (*i1 != *i2) \
            return nullptr; \
        return std::make_unique<name>(position, line, column, Pattern); \
    } \
    name(ptrdiff_t position, size_t line, size_t column, std::string text) : \
    ParentClass(position, line, column, ch, move(text), #name) {  } \
}

#define KAEY_REGEX_TOKEN(name, pattern, group, ch) \
struct name : Token::With<name, RegexToken> \
{ \
    static inline auto const Pattern = std::regex(pattern); \
    static constexpr auto TokenChannel = ch; \
    static std::unique_ptr<Token> Create(StringStream* ss, ptrdiff_t position, size_t line, size_t column) { \
        std::match_results<StringStream::iterator> matches; \
        return std::regex_search(ss->CurrentIterator(), ss->end(), matches, Pattern, std::regex_constants::match_continuous) ? \
        std::make_unique<name>(position, line, column, std::move(matches)) : nullptr; \
    } \
    name(ptrdiff_t position, size_t line, size_t column, std::match_results<StringStream::iterator> result) : \
    ParentClass(position, line, column, ch, group, std::move(result), #name) {  } \
}

#define KAEY_RESERVED_IDENTIFIER(name, pattern, ch) \
struct name : Token::With<name> \
{ \
    static inline auto&& Pattern = pattern; \
    static constexpr auto TokenChannel = ch; \
    static std::unique_ptr<Token> Create(StringStream* ss, ptrdiff_t position, size_t line, size_t column) { \
        for (auto [i1, i2] = std::make_tuple(Pattern, ss->CurrentIterator()); *i1; ++i1, ++i2) if (*i1 != *i2) \
            return nullptr; \
        return !std::isalpha(ss->Peek(std::size(Pattern) - 1)) ? std::make_unique<name>(position, line, column, Pattern) : nullptr; \
    } \
    name(ptrdiff_t position, size_t line, size_t column, std::string text) : \
    ParentClass(position, line, column, ch, move(text), #name) {  } \
}

#define KAEY_OPERATOR_TOKEN(name, pattern, ch) \
struct name : Token::With<name, OperatorToken> \
{ \
    static inline auto&& Pattern = pattern; \
    static constexpr auto TokenChannel = ch; \
    static std::unique_ptr<Token> Create(StringStream* ss, ptrdiff_t position, size_t line, size_t column) { \
        for (auto [i1, i2] = std::make_tuple(Pattern, ss->CurrentIterator()); *i1; ++i1, ++i2) if (*i1 != *i2) \
            return nullptr; \
        return std::make_unique<name>(position, line, column, Pattern); \
    } \
    name(ptrdiff_t position, size_t line, size_t column, std::string text) : \
    ParentClass(position, line, column, ch, move(text), #name) {  } \
}
