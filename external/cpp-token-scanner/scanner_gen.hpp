 /**  * MIT License
 * 
 * Copyright (c) 2016 Tobias Hoffmann
 *
 * Copyright (c) 2023 Ochawin A.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef JOIN
#define JOIN(x, y) x ## y
#endif

#ifndef TOKEN_DESCRIPTOR_FILE
#error "TOKEN_DESCRIPTOR_FILE not specified."
#else

#ifndef TOKEN_CLASS_NAME
#error "TOKEN_CLASS_NAME not specified."
#endif

#include <iostream>

#ifndef COMPILE_TIME_TRIE_H
#define COMPILE_TIME_TRIE_H

#include <algorithm>
#include <string_view>
#include <tuple>

namespace cpp20scanner
{
namespace cpp20trie
{
    namespace detail
    {
        template <int N>
        struct FixedStringImpl
        {
            constexpr FixedStringImpl(const char (&str)[N]) noexcept { std::copy_n(str, N, val); }
            constexpr auto empty() const noexcept -> const bool { return size() == 0; }
            constexpr auto head() const noexcept -> const char { return val[0]; }
            static constexpr auto size() noexcept -> std::size_t{ return N; };
            constexpr auto tail() const noexcept -> FixedStringImpl<((N != 1) ? N - 1 : 1)>
            {
                char newVal[((N != 1) ? N - 1 : 1)];
                std::copy_n(&val[(N==1?0:1)], ((N != 1) ? N - 1 : 1), newVal);
                return FixedStringImpl<((N != 1) ? N - 1 : 1)>(newVal);
            }

            char val[N];
        };

    } // namespace detail

    /**
     * A 'FixedString' is a stirng which can be instantiated constexpr.
     * 
     * Example:
     * 
     *  using HelloString = cpp20trie::FixedString<"Hello">;
     *  - HelloString::data()       -> "Hello"
     *  - HelloString::head()       -> 'H'
     *  - HelloString::tail::data() -> "ello"
     * 
     * template <class String>
     * constexpr auto print_string(String&& str) -> void
     * {
     *     std::cout << String::data() << '\n';
     * }
     */
    template <detail::FixedStringImpl String>
    struct FixedString
    {
        using _type = decltype(String);
        static constexpr auto size() noexcept -> int { return String.size(); }
        static constexpr auto data() noexcept -> const char* { return String.val; }
        static constexpr auto head() noexcept -> char { return String.head(); }
        static constexpr auto empty() noexcept -> bool { return String.empty(); }
        using tail = FixedString<String.tail()>;
    };

    namespace detail
    {
        template <unsigned int> 
        struct int_c {};

        /**
         * For SFINAE specialization.
         */
        template <bool B>
        using Specialize = typename std::enable_if<B>::type;

        /**
         * For forcing ADL.
         */
        struct nil {};

        template <int Char, typename Next>
        struct Transition {};

        template <typename... Transitions>
        struct TrieNode : Transitions... {};

        constexpr auto check_trie(TrieNode<> trie, std::string_view str, auto&& fne, auto&&... fns) 
        noexcept -> decltype(fne())
        {
            return fne();
        }

        // This case is only true when we have exactly one transition.
        template <int Char, typename Next, typename = Specialize<(Char >= 0)>>
        constexpr auto check_trie(TrieNode<Transition<Char, Next>> trie, std::string_view str, auto&& fne, auto&&... fns) 
        noexcept -> decltype(fne())
        {
            return (!str.empty() && (str[0] == Char))
                 ? check_trie(Next(), str.substr(1), std::move(fne), std::forward<decltype(fns)>(fns)...)
                 : fne();
        }

        template <typename... Transitions>
        constexpr auto check_trie(TrieNode<Transitions...> trie, std::string_view str, auto&& fne, auto&&... fns) 
        noexcept -> decltype(fne())
        {
            return (!str.empty())
                 ? string_switch(str[0], str.substr(1), trie, std::move(fne), std::forward<decltype(fns)>(fns)...)
                 : fne();
        }

        template <unsigned int Index, typename... Transitions>
        constexpr auto check_trie(TrieNode<Transition<-1, int_c<Index>>, Transitions...>, std::string_view str, auto&& fne, auto&&... fns) 
        noexcept -> decltype(fne())
        {
            return str.empty()
                 ? std::get<Index>(std::make_tuple(std::forward<decltype(fns)>(fns)...))()
                 : check_trie(TrieNode<Transitions...>(), str, std::move(fne), std::forward<decltype(fns)>(fns)...); 
        }

        template <int Char0, typename Next0>
        constexpr auto make_character_tuple_helper(TrieNode<Transition<Char0, Next0>>) noexcept
        {
            return std::make_tuple(Char0);
        }

        template <int Char0, typename Next0, int Char1, typename Next1, typename... Transitions>
        constexpr auto make_character_tuple_helper(TrieNode<Transition<Char0, Next0>, Transition<Char1, Next1>, Transitions...>) noexcept
        {
            return std::tuple_cat(
                make_character_tuple_helper<Char0, Next0>(TrieNode<Transition<Char0, Next0>>()),
                make_character_tuple_helper<Char1, Next1>(TrieNode<Transition<Char1, Next1>, Transitions...>()));
        }

        template <typename... Transitions>
        constexpr auto get_transition_characters(TrieNode<Transitions...> t) noexcept
        {
            return make_character_tuple_helper(t);
        }

        template <int Char0, typename Next0>
        constexpr auto make_transition_children_tuple(TrieNode<Transition<Char0, Next0>>) noexcept
        {
            return std::make_tuple(Next0());
        }

        template <int Char0, typename Next0, int Char1, typename Next1, typename... Transitions>
        constexpr auto make_transition_children_tuple(TrieNode<Transition<Char0, Next0>, Transition<Char1, Next1>, Transitions...>) noexcept
        {
            return std::tuple_cat(
                make_transition_children_tuple<Char0, Next0>(TrieNode<Transition<Char0, Next0>>()),
                make_transition_children_tuple<Char1, Next1>(TrieNode<Transition<Char1, Next1>, Transitions...>()));
        }

        template <typename... Transitions>
        constexpr auto get_transition_children(TrieNode<Transitions...> t) noexcept
        {
            return make_transition_children_tuple(t);
        }

        template <std::size_t... Is>
        constexpr auto compile_switch(std::size_t index, auto&& chars, std::index_sequence<Is...>, auto&& func, auto&& default_function)
        noexcept -> decltype(default_function())
        {
            if constexpr (!std::is_same_v<decltype(default_function()), void>)
            {
                auto ret = default_function();
                std::initializer_list<int>({ (index == std::get<Is>(std::move(chars)) ? (ret = func.template operator()<Is>()), 0 : 0)...} );
                return ret;
            }
            else
            {
                auto found = false;
                std::initializer_list<int>({ (index == std::get<Is>(std::move(chars)) ? found=true, (func.template operator()<Is>()), 0 : 0)...} );
                if (!found) default_function();
            }
        }

        template <std::size_t... Cs>
        constexpr auto switch_impl(std::size_t index, auto&& t, auto&& is, std::index_sequence<Cs...> cs, std::string_view str, auto&& fne, auto&&... fns) 
        noexcept -> decltype(fne())
        {
            auto f = [&]<std::size_t I>() -> auto
            {
                return check_trie(std::get<I>(std::move(t)), str, std::move(fne), std::forward<decltype(fns)>(fns)...);
            };
            return compile_switch(index, std::move(is), std::move(cs), std::move(f), fne);
        }

        template <std::size_t Index, class String, typename = Specialize<(String::size() == 1)>>
        constexpr auto transition_add(nil, String str) 
        noexcept -> Transition<-1, int_c<Index>>
        {
            return {};
        }

        template <std::size_t Index, class String, typename = Specialize<(String::size() > 1)>>
        constexpr auto transition_add(nil, String str)
        noexcept -> Transition<String::head(), TrieNode<decltype(transition_add<Index>(nil(), typename String::tail()))>>
        {
            return {};
        }

        // Casse for reaching the end of the string and
        // there is no transition at the current position.
        template <unsigned int Index, class String, typename... Prefixes, typename... Transitions, typename = Specialize<(String::empty() || sizeof...(Transitions) == 0)>>
        constexpr auto insert_sorted(nil, String&& str, TrieNode<Prefixes...>, Transitions...)
        noexcept -> TrieNode<Prefixes..., decltype(transition_add<Index>(nil(), std::move(str))), Transitions...>
        {
            return {};
        }

        template <std::size_t Index, class String, typename... Prefixes, int Ch, typename Next, typename... Transitions, typename = Specialize<(Ch != String::head())>>
        constexpr auto insert_sorted(nil, String&& str, TrieNode<Prefixes...>, Transition<Ch, Next>, Transitions...)
        noexcept -> TrieNode<Prefixes..., decltype(transition_add<Index>(nil(), std::move(str))), Transition<Ch, Next>, Transitions...>
        {
            return {};
        }

        template <std::size_t Index, typename... Transitions>
        constexpr auto trie_add(TrieNode<Transitions...>, auto&& str)
        noexcept -> decltype(insert_sorted<Index>(nil(), std::move(str), TrieNode<>(), Transitions()...))
        {
            return {};
        }

        template <std::size_t Index, class String, typename... Prefixes, int Ch, typename Next, typename = Specialize<(Ch == String::head())>>
        constexpr auto insert_sorted(nil, String&&, TrieNode<Prefixes...>, Transition<Ch, Next>, auto... transitions)
        noexcept -> TrieNode<Prefixes..., Transition<Ch, decltype(trie_add<Index>(Next(), typename String::tail()))>, decltype(transitions)...>
        {
            return {};
        }

        template <typename... Transitions>
        constexpr auto string_switch(unsigned char ch, std::string_view str, TrieNode<Transitions...> t, auto&& fne, auto&&... fns) 
        noexcept -> decltype(fne())
        {
            auto transition_children = detail::get_transition_children(t);
            auto transition_characters = detail::get_transition_characters(t);
            auto character = static_cast<std::size_t>(ch);
            auto index_sequence = std::make_index_sequence<sizeof...(Transitions)>{};

            return detail::switch_impl(character, 
                std::move(transition_children),
                std::move(transition_characters),
                std::move(index_sequence), 
                str, 
                std::move(fne), 
                std::forward<decltype(fns)>(fns)...);
        }

        // Making the trie.
        template <std::size_t I>
        constexpr auto make_trie(nil = nil()) noexcept -> TrieNode<>
        {
            return {};
        }

        template <std::size_t I>
        constexpr auto make_trie(nil, auto&& str, auto&&... strs)
        noexcept -> decltype(trie_add<I>(make_trie<I + 1>(nil(), std::forward<decltype(strs)>(strs)...), std::move(str)))
        {
            return {};
        }

        /**
         * Concept to make sure that all types in a variadic arg is the same.
         */
        template<typename Arg, typename... Args>
        concept args_have_same_type = (std::same_as<std::remove_cvref_t<Arg>, Args> && ...);
        
        /**
         * Catch error.
         */
        template<typename FnE, typename... Fns>
        constexpr auto check_do_trie(auto&& trie, std::string_view str, auto&& fne, auto&&... fns) 
        noexcept -> FnE
        {
            static_assert(args_have_same_type<FnE, Fns...>, "MATCH Error - Return type mismatch.");
            return check_trie( std::move(trie), str, std::move(fne), std::forward<decltype(fns)>(fns)...);
        }

        template <unsigned long... Is, typename Arg, typename... Args>
        constexpr auto do_trie(std::index_sequence<Is...>, std::string_view str, Arg&& argE, Args&&... args)
        noexcept -> decltype(argE())
        {
            return check_do_trie<
                decltype(argE()),
                decltype((std::get<(Is * 2 + 1)>(std::make_tuple(std::forward<Args>(args)...)))())...
            >(
                make_trie<0>(nil(),std::get<(2 * Is)>(std::make_tuple(std::forward<Args>(args)...))...),
                std::move(str), std::move(argE),
                std::get<(Is * 2 + 1)>(std::make_tuple(std::forward<Args>(args)...))...);
        }

    }; // namespace detail

    constexpr auto do_trie(std::string_view str, auto&& argE, auto&&... args) 
    noexcept -> decltype(argE())
    {
        return detail::do_trie(std::make_index_sequence<sizeof...(args) / 2>(), 
                              std::move(str),
                              std::move(argE),            
                              std::forward<decltype(args)>(args)...
        );
    }

    #define MATCH(str) cpp20scanner::cpp20trie::do_trie(str, [&] {
    #define CASE(str) }, cpp20scanner::cpp20trie::FixedString<str>(), [&] {
    #define ENDMATCH });

} // namespace cpp20trie

#ifndef ENUM_BASE_CLASS
#define ENUM_BASE_CLASS

#include <string_view>
#include <type_traits>

/**
 * This enum implementation technique is heaviliy inspired by the Carbon programming language enums.
 */

// Fixing macro pasting issue.
#define JOIN(x, y) x ## y

/**
 * Use to declare a new raw enum class. Note that this also define 
 * a list holding the name of each enum.
 */
#define DECLARE_RAW_ENUM_CLASS(enum_class_name, underlying_type)  \
    namespace _private                                            \
    {                                                             \
    enum class JOIN(enum_class_name, Raw) : underlying_type;      \
    extern const std::string_view JOIN(enum_class_name, Names)[]; \
    }                                                             \
    enum class _private::JOIN(enum_class_name, Raw) : underlying_type

/**
 * Defines a list of strings holding the name for
 * each entry of the enum. Note that this has to 
 * be called after declaring a new enum class.
 */
#define DEFINE_ENUM_CLASS_NAMES(enum_class_name) \
inline constexpr std::string_view _private::JOIN(enum_class_name, Names)[]

/**
 * ENUMERATORS.
 */

// Used to declare each value inside an enum class.
#define RAW_ENUM_ENUMERATOR(name) name,

// Used to declare each name value for each enum value.
#define ENUM_NAME_ENUMERATOR(name) #name,

// Used to generate a named constant for each enum value.
#define ENUM_CONSTANT_DECLARATION_ENUMERATOR(name) static const EnumType name;

// Used to generate the definition for each named constant for each enum value.
#define ENUM_CONSTANT_DEFINITION_ENUMERATOR(enum_class_name, name) \
 constexpr enum_class_name enum_class_name::name = enum_class_name::create(RawEnumType::name);

/**
 * The base class for Enums.
 */
#define ENUM_BASE(enum_class_name) \
 cpp20scanner::EnumBase<enum_class_name, _private::JOIN(enum_class_name, Raw), _private::JOIN(enum_class_name, Names)>

// The TokenBase class is simply a wrapper class for a enum type.
// It utilizes X-macros for ease of extension.
template <typename DerivedT, typename RawT, const std::string_view Names[]>
class EnumBase
{
  public:
    using RawEnumType    = RawT;
    using EnumType       = DerivedT;
    using UnderlyingType = std::underlying_type_t<RawT>;

    // Allow conversion to raw type.
    constexpr operator RawEnumType() const noexcept
    {
        return value;
    }

    [[nodiscard]] auto name() const -> const std::string_view
    {
        return Names[as_int()];
    }

    [[nodiscard]] static constexpr auto from_int(UnderlyingType v) -> EnumType
    {
      return create(static_cast<RawEnumType>(v));
    }

    [[nodiscard]] constexpr auto as_int() const noexcept -> UnderlyingType
    {
        return static_cast<UnderlyingType>(value);
    }

    constexpr EnumBase() = default;
  protected:

    static constexpr auto create(RawEnumType v) -> EnumType
    {
        EnumType r;
        r.value = v;
        return r;
    }

  private:
    RawEnumType value;
};

#endif /* ENUM_BASE_CLASS */

#ifndef TOKEN_BASE_CLASS
#define TOKEN_BASE_CLASS

/**
 * Tokens super class.
 */
#define TOKEN_BASE(enum_class_name) \
ENUM_BASE(enum_class_name), public cpp20scanner::TokenBase<enum_class_name>

template <typename EnumT>
class TokenBase 
{
 using BaseEnumType = EnumT;
public:
 [[nodiscard]] constexpr auto is_symbol() const noexcept -> bool
 {
  return _is_symbol[get_index()];
 }

 [[nodiscard]] constexpr auto is_keyword() const noexcept -> bool
 {
  return _is_keyword[get_index()];
 }

 [[nodiscard]] constexpr auto get_symbol() const noexcept -> const std::string_view
 {
  return _symbols[get_index()];
 }

 static const BaseEnumType keyword_tokens[];
 static const BaseEnumType all_tokens[];
protected:
 // Keywords.
 static const bool _is_keyword[];

 // All string representation of symbols.
 static const std::string_view _symbols[];

 // Symbols.
 static const bool _is_symbol[];

private:
 [[nodiscard]] constexpr auto get_index() const noexcept
 {
   return static_cast<const BaseEnumType*>(this)->as_int();
 }
};

/*================================*/
/*                                */
/*          MACROS for            */
/*     Properties generation.     */
/*                                */
/*================================*/

/**
 * Defining keyword tokens.
 */
#define ALL_KEYWORD_TOKENS_DEFINITION(enum_class_name) \
template<> constexpr cpp20scanner::TokenBase<enum_class_name>::BaseEnumType cpp20scanner::TokenBase<enum_class_name>::keyword_tokens[]

/**
 * Defining all available tokens.
 */ 
#define ALL_TOKENS_DEFINITION(enum_class_name) \
template<> constexpr cpp20scanner::TokenBase<enum_class_name>::BaseEnumType cpp20scanner::TokenBase<enum_class_name>::all_tokens[]

/**
 * Used to mark all keyword tokens.
 */
#define KEYWORD_MARKER_DEFINITION(enum_class_name) \
template<> constexpr bool cpp20scanner::TokenBase<enum_class_name>::_is_keyword[]

/**
 * Used to mark all symbol tokens.
 */
#define SYMBOL_MARKER_DEFINITION(enum_class_name) \
template<> constexpr bool cpp20scanner::TokenBase<enum_class_name>::_is_symbol[]

/**
 * Used to declare all symbols (string).
 */
#define SYMBOL_STRING_DEFINITION(enum_class_name) \
template<> constexpr std::string_view cpp20scanner::TokenBase<enum_class_name>::_symbols[]

/**
 * Retrieve the value for the enum class the token class wraps.
 */
#define TOKEN_ENUM_VALUE(enum_class_name, name) \
cpp20scanner::TokenBase<enum_class_name>::BaseEnumType::name,

#endif /* TOKEN_BASE_CLASS */

#ifndef TOKEN_H
#define TOKEN_H

#include <string>

/**
 * A basic token. Used to classify text.
 */
template<typename TokenType>
struct Token
{
    /**
     * METHODS.
     */

    /**
     * Constructor.
     */
    [[nodiscard]] Token(TokenType t, std::string_view l, std::size_t line_num) noexcept
        : type{t}
        , lexeme{l}
        , line{line_num}
    {
    }

    /**
     * Default constructor, return EOF.
     */
    [[nodiscard]] Token() noexcept
        : type{TokenType::EndOfFile}
        , lexeme{"\0"}
    {
    }

    [[nodiscard]] static auto make(TokenType t, std::string_view l) -> Token<TokenType>
    {
        return Token(t, l, 0);
    }

    [[nodiscard]] auto is(TokenType t) const noexcept -> bool
    {
     return type == t;        
    }

    template <std::same_as<TokenType>... TokenTypes>
    [[nodiscard]] auto any_of(TokenTypes... types) const noexcept -> bool
    {
      return (is(types) || ...);
    }

    /**
     * MEMBERS.
     */
    TokenType   type{};
    std::string lexeme{};
    std::size_t line{};
};


#endif /* TOKEN_H */

#ifndef PARSER_BASE_H
#define PARSER_BASE_H

#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>

template <class Scanner, class TokenType>
class BaseParser
{
    enum class Type
    {
        File,
        Source
    };

  public:
    [[nodiscard]] explicit BaseParser(const std::string& input, Type type = Type::File)
    {
        if (type == Type::File)
        {
            if (!scanner.read_source(input))
                has_error = true;
        }
        else
        {
            scanner.set_source(input);
        }
    }

    constexpr BaseParser() {}

    auto error_occured() const noexcept -> bool 
    {
     return has_error;
    }

    /**
     * Protected methods.
     */
  protected:
    /**
     * Advance the current token.
     */
    auto advance() noexcept -> void
    {
        previous = current;

        while (true)
        {
            current = scanner.scan_token();
            if (current.type != TokenType::Error)
            {
                break;
            }
            report_token_error(current);
        }
    }

    /**
     * Consume the current token if it matches what we expect,
     * else we report error.
     */
    auto consume(TokenType type, std::string_view message) noexcept -> void
    {
        if (current.type == type)
        {
            advance();
            return;
        }
        report_error(message);
    }

    /**
     * Check the current token type, if it matches what expect, we consume.
     * This consumption is optional, an error will not be thrown. Returns
     * true if the token was consumed.
     */
    [[nodiscard]] auto match(TokenType type) noexcept -> bool
    {
        if (current.type == type)
        {
            advance();
            return true;
        }
        return false;
    }

    /**
     * Returns true when the current type is equivalent to the expected type.
     */
    [[nodiscard]] bool check(TokenType type) const noexcept
    {
        return current.type == type;
    }

    /**
     * Report the token which caused an error.
     */
    auto report_token_error(Token<TokenType> token) noexcept -> void
    {
        auto message = "Unexpected token " + token.lexeme + '.';
        report_error(message);
    }

    /**
     * Report a generic error. (Custom error messasge)
     */
    auto report_error(std::string_view message) noexcept -> void
    {
        // Prevevent error message overload for one declaration.
        if (this->panic)
            return;

        this->panic = true;

        std::cout << "ERROR [ (line:" << previous.line << ") " << message << " ]\n";

        this->has_error = true;
    }

    /**
     * Log function, for debugging purposes.
     */
    auto log(std::string_view message) const noexcept -> void
    {
#ifdef DEBUG
        std::cout << "LOG [ " << message << " ]\n";
#else
#endif
    }

    /**
     * Members.
     */
  protected:
    Token<TokenType> current;
    Token<TokenType> previous;
    Scanner          scanner;

    bool panic{false};
    bool has_error{false};
};

#endif /* PARSER_BASE_H */

} // namespace detail

#endif // COMPILE_TIME_TRIE_H

// ---------------------------------------------------
// Generators.
// ---------------------------------------------------

#include <cstdint>

/**
 * Declare new raw enum class and name values.
 */
DECLARE_RAW_ENUM_CLASS(TOKEN_CLASS_NAME, uint8_t) {
 #define TOKEN(name) RAW_ENUM_ENUMERATOR(name)
#ifndef TOKEN
#define TOKEN(name)
#endif
#ifndef KEYWORD_TOKEN
#define KEYWORD_TOKEN(name, keyword) TOKEN(name)
#endif
#ifndef SYMBOL_TOKEN
#define SYMBOL_TOKEN(name, symbol) TOKEN(name)
#endif
#ifndef IGNORE_TOKEN
#define IGNORE_TOKEN(character)
#endif
TOKEN(Error)
TOKEN(Raw)
TOKEN(EndOfFile)
TOKEN(Number)
TOKEN(Identifier)
#include TOKEN_DESCRIPTOR_FILE
#undef SYMBOL_TOKEN
#undef KEYWORD_TOKEN
#undef IGNORE_TOKEN
#undef TOKEN
};

#ifndef SCANNER
#define SCANNER(name) JOIN(name, Scanner)
#endif

struct SCANNER(TOKEN_CLASS_NAME);

/**
 * Subclass from EnumBase.
 */
class TOKEN_CLASS_NAME : public TOKEN_BASE(TOKEN_CLASS_NAME)
{
public:
    using Scanner = SCANNER(TOKEN_CLASS_NAME);
    
/**
 * Generate constant declarations.
 */
#define TOKEN(name) ENUM_CONSTANT_DECLARATION_ENUMERATOR(name)
#ifndef TOKEN
#define TOKEN(name)
#endif
#ifndef KEYWORD_TOKEN
#define KEYWORD_TOKEN(name, keyword) TOKEN(name)
#endif
#ifndef SYMBOL_TOKEN
#define SYMBOL_TOKEN(name, symbol) TOKEN(name)
#endif
#ifndef IGNORE_TOKEN
#define IGNORE_TOKEN(character)
#endif
TOKEN(Error)
TOKEN(Raw)
TOKEN(EndOfFile)
TOKEN(Number)
TOKEN(Identifier)
#include TOKEN_DESCRIPTOR_FILE
#undef SYMBOL_TOKEN
#undef KEYWORD_TOKEN
#undef IGNORE_TOKEN
#undef TOKEN
};

/**
 * Generate constant definitions.
 */
#define TOKEN(name) ENUM_CONSTANT_DEFINITION_ENUMERATOR(TOKEN_CLASS_NAME, name)
#ifndef KEYWORD_TOKEN
#define KEYWORD_TOKEN(name, keyword) TOKEN(name)
#endif
#ifndef SYMBOL_TOKEN
#define SYMBOL_TOKEN(name, symbol) TOKEN(name)
#endif
#ifndef IGNORE_TOKEN
#define IGNORE_TOKEN(character)
#endif
TOKEN(Error)
TOKEN(Raw)
TOKEN(EndOfFile)
TOKEN(Number)
TOKEN(Identifier)
#include TOKEN_DESCRIPTOR_FILE
#undef SYMBOL_TOKEN
#undef KEYWORD_TOKEN
#undef IGNORE_TOKEN
#undef TOKEN

/**
 * Generate enum names.
 */
DEFINE_ENUM_CLASS_NAMES(TOKEN_CLASS_NAME) = {
#define TOKEN(name) ENUM_NAME_ENUMERATOR(name)
#ifndef TOKEN
#define TOKEN(name)
#endif
#ifndef KEYWORD_TOKEN
#define KEYWORD_TOKEN(name, keyword) TOKEN(name)
#endif
#ifndef SYMBOL_TOKEN
#define SYMBOL_TOKEN(name, symbol) TOKEN(name)
#endif
#ifndef IGNORE_TOKEN
#define IGNORE_TOKEN(character)
#endif
TOKEN(Error)
TOKEN(Raw)
TOKEN(EndOfFile)
TOKEN(Number)
TOKEN(Identifier)
#include TOKEN_DESCRIPTOR_FILE
#undef SYMBOL_TOKEN
#undef KEYWORD_TOKEN
#undef IGNORE_TOKEN
#undef TOKEN
};

/*================================*/
/*                                */
/*     Properties generation.     */
/*                                */
/*================================*/

// Marking keywords.
ALL_KEYWORD_TOKENS_DEFINITION(TOKEN_CLASS_NAME) = {
#define KEYWORD_TOKEN(name, symbol) TOKEN_ENUM_VALUE(TOKEN_CLASS_NAME, name)
#ifndef TOKEN
#define TOKEN(name)
#endif
#ifndef KEYWORD_TOKEN
#define KEYWORD_TOKEN(name, keyword) TOKEN(name)
#endif
#ifndef SYMBOL_TOKEN
#define SYMBOL_TOKEN(name, symbol) TOKEN(name)
#endif
#ifndef IGNORE_TOKEN
#define IGNORE_TOKEN(character) TOKEN(character)
#endif
TOKEN(Error)
TOKEN(Raw)
TOKEN(EndOfFile)
TOKEN(Number)
TOKEN(Identifier)
#include TOKEN_DESCRIPTOR_FILE
#undef SYMBOL_TOKEN
#undef KEYWORD_TOKEN
#undef IGNORE_TOKEN
#undef TOKEN
};

// Collect all tokens.
ALL_TOKENS_DEFINITION(TOKEN_CLASS_NAME) = {
 #define TOKEN(name) TOKEN_ENUM_VALUE(TOKEN_CLASS_NAME, name)
#ifndef TOKEN
#define TOKEN(name)
#endif
#ifndef KEYWORD_TOKEN
#define KEYWORD_TOKEN(name, keyword) TOKEN(name)
#endif
#ifndef SYMBOL_TOKEN
#define SYMBOL_TOKEN(name, symbol) TOKEN(name)
#endif
#ifndef IGNORE_TOKEN
#define IGNORE_TOKEN(character)
#endif
TOKEN(Error)
TOKEN(Raw)
TOKEN(EndOfFile)
TOKEN(Number)
TOKEN(Identifier)
#include TOKEN_DESCRIPTOR_FILE
#undef SYMBOL_TOKEN
#undef KEYWORD_TOKEN
#undef IGNORE_TOKEN
#undef TOKEN
};

// Defining keywords.
KEYWORD_MARKER_DEFINITION(TOKEN_CLASS_NAME) = {
 #define TOKEN(name) false,
 #define KEYWORD_TOKEN(name, keyword) true,
#ifndef TOKEN
#define TOKEN(name)
#endif
#ifndef KEYWORD_TOKEN
#define KEYWORD_TOKEN(name, keyword) TOKEN(name)
#endif
#ifndef SYMBOL_TOKEN
#define SYMBOL_TOKEN(name, symbol) TOKEN(name)
#endif
#ifndef IGNORE_TOKEN
#define IGNORE_TOKEN(character) TOKEN(character)
#endif
TOKEN(Error)
TOKEN(Raw)
TOKEN(EndOfFile)
TOKEN(Number)
TOKEN(Identifier)
#include TOKEN_DESCRIPTOR_FILE
#undef SYMBOL_TOKEN
#undef KEYWORD_TOKEN
#undef IGNORE_TOKEN
#undef TOKEN
};

// Defining symbols.
SYMBOL_MARKER_DEFINITION(TOKEN_CLASS_NAME) = {
 #define TOKEN(name) false,
 #define SYMBOL_TOKEN(name, symbol) true,
#ifndef TOKEN
#define TOKEN(name)
#endif
#ifndef KEYWORD_TOKEN
#define KEYWORD_TOKEN(name, keyword) TOKEN(name)
#endif
#ifndef SYMBOL_TOKEN
#define SYMBOL_TOKEN(name, symbol) TOKEN(name)
#endif
#ifndef IGNORE_TOKEN
#define IGNORE_TOKEN(character) TOKEN(character)
#endif
TOKEN(Error)
TOKEN(Raw)
TOKEN(EndOfFile)
TOKEN(Number)
TOKEN(Identifier)
#include TOKEN_DESCRIPTOR_FILE
#undef SYMBOL_TOKEN
#undef KEYWORD_TOKEN
#undef IGNORE_TOKEN
#undef TOKEN
};

// List of symbols.
SYMBOL_STRING_DEFINITION(TOKEN_CLASS_NAME) = {
 #define TOKEN(name) "",
 #define SYMBOL_TOKEN(name, symbol) symbol,
 #define KEYWORD_TOKEN(name, symbol) symbol,
#ifndef TOKEN
#define TOKEN(name)
#endif
#ifndef KEYWORD_TOKEN
#define KEYWORD_TOKEN(name, keyword) TOKEN(name)
#endif
#ifndef SYMBOL_TOKEN
#define SYMBOL_TOKEN(name, symbol) TOKEN(name)
#endif
#ifndef IGNORE_TOKEN
#define IGNORE_TOKEN(character) TOKEN(character)
#endif
TOKEN(Error)
TOKEN(Raw)
TOKEN(EndOfFile)
TOKEN(Number)
TOKEN(Identifier)
#include TOKEN_DESCRIPTOR_FILE
#undef SYMBOL_TOKEN
#undef KEYWORD_TOKEN
#undef IGNORE_TOKEN
#undef TOKEN
};


#include <cctype>
#include <fstream>
#include <sstream>
#include <string>


/**
 * The Scanner class is responsible for
 * processing the input source code into
 * tokens.
 */
struct SCANNER(TOKEN_CLASS_NAME)
{
  public:

    SCANNER(TOKEN_CLASS_NAME)() = default;


    /**
     * Convert string to token.
     */
    static auto strtok(const std::string& str) -> cpp20scanner::Token<TOKEN_CLASS_NAME>
    {
        SCANNER(TOKEN_CLASS_NAME) scanner {};
        scanner.set_source(str);
        return scanner.scan_token();
    }
    

    void set_source(const std::string& source)
    {
        reset();
        source_code = source;
    }

    /**
     * Read all the source code from the file.
     */
    bool read_source(const std::string& path)
    {
        reset();

        this->source_code.clear();

        std::ifstream     ifs(path);
        if (ifs.fail()) return false;

        std::stringstream buffer;
        buffer << ifs.rdbuf();
        this->source_code = buffer.str();

        return true;
    }

    /**
     * Keep consuming while the character is a digit.
     */
    [[nodiscard]] cpp20scanner::Token<TOKEN_CLASS_NAME> scan_number() noexcept
    {
        while (std::isdigit(peek())) advance_position();
        return make_token(TOKEN_CLASS_NAME::Number);
    }

    /**
     * Keep consuming while the character is alphanumeric.
     */
    [[nodiscard]] cpp20scanner::Token<TOKEN_CLASS_NAME> scan_identifier() noexcept
    {
        while (std::isalnum(peek()) || peek()=='_') advance_position();
        return make_token(identifier_type());
    }

    /**
     * Return the identifer type. This could return as
     * one of the keyword types.
     */
    [[nodiscard]] auto identifier_type() -> TOKEN_CLASS_NAME 
    {
        const auto word = source_code.substr(start, current-start);
        
        return MATCH(word)
            return TOKEN_CLASS_NAME::Identifier;
        #define KEYWORD_TOKEN(name, symbol) CASE(symbol) return TOKEN_CLASS_NAME::name;
#ifndef TOKEN
#define TOKEN(name)
#endif
#ifndef KEYWORD_TOKEN
#define KEYWORD_TOKEN(name, keyword) TOKEN(name)
#endif
#ifndef SYMBOL_TOKEN
#define SYMBOL_TOKEN(name, symbol) TOKEN(name)
#endif
#ifndef IGNORE_TOKEN
#define IGNORE_TOKEN(character) TOKEN(character)
#endif
TOKEN(Error)
TOKEN(Raw)
TOKEN(EndOfFile)
TOKEN(Number)
TOKEN(Identifier)
#include TOKEN_DESCRIPTOR_FILE
#undef SYMBOL_TOKEN
#undef KEYWORD_TOKEN
#undef IGNORE_TOKEN
#undef TOKEN
        ENDMATCH;
    }

    [[nodiscard]] cpp20scanner::Token<TOKEN_CLASS_NAME> scan_until_character(char token) noexcept
    {
        while (peek()!=token) advance_position();
        return make_token(TOKEN_CLASS_NAME::Raw);
    }

    template<std::same_as<TOKEN_CLASS_NAME> ...Tokens>
    [[nodiscard]] cpp20scanner::Token<TOKEN_CLASS_NAME> scan_until_token(TOKEN_CLASS_NAME token, Tokens ... tokens) noexcept
    {
        const auto start_pos = current;
        cpp20scanner::Token<TOKEN_CLASS_NAME> tok;
        
        // Keep scanning while it's neither of the tokens specified.
        while ((tok = scan_token()).type != TOKEN_CLASS_NAME::EndOfFile 
              && (tok.type != token
              || ((tok.type != tokens) || ...))) 
        {/* Do nothing */}
        
        start = start_pos;
        current -= tok.lexeme.size();
        
        return make_token(TOKEN_CLASS_NAME::Raw);
    }

    [[nodiscard]] cpp20scanner::Token<TOKEN_CLASS_NAME> scan_body(TOKEN_CLASS_NAME head, TOKEN_CLASS_NAME tail) noexcept
    {
        const auto start_pos = current;
        cpp20scanner::Token<TOKEN_CLASS_NAME> tok;

        int scope_count = 1;
        while (scope_count != 0 && !is_at_end())
        {
           tok = scan_token();
           scope_count += (tok.type == head) ? 1 : 0;
           scope_count -= (tok.type == tail) ? 1 : 0;
        }
        start = start_pos;
        current -= tok.lexeme.size();
        return make_token(TOKEN_CLASS_NAME::Raw);
    }

    /**
     * Return the next token.
     */
    [[nodiscard]] cpp20scanner::Token<TOKEN_CLASS_NAME> scan_token() noexcept
    {
        skip_whitespace();
        start = current;

        if (is_at_end()) 
        {
            return make_token(TOKEN_CLASS_NAME::EndOfFile);
        }

        const auto c = advance();

        if (std::isdigit(c))
        {
            return scan_number();
        }
        else if (std::isalpha(c)) 
        {
            return scan_identifier();
        }

        #define CCASE(character, token) break; case character: return make_token(token)
        switch (c)
        {
            #define SYMBOL_TOKEN(name, symbol) break; case symbol[0]: return make_token(TOKEN_CLASS_NAME::name);
#ifndef TOKEN
#define TOKEN(name)
#endif
#ifndef KEYWORD_TOKEN
#define KEYWORD_TOKEN(name, keyword) TOKEN(name)
#endif
#ifndef SYMBOL_TOKEN
#define SYMBOL_TOKEN(name, symbol) TOKEN(name)
#endif
#ifndef IGNORE_TOKEN
#define IGNORE_TOKEN(character) TOKEN(character)
#endif
TOKEN(Error)
TOKEN(Raw)
TOKEN(EndOfFile)
TOKEN(Number)
TOKEN(Identifier)
#include TOKEN_DESCRIPTOR_FILE
#undef SYMBOL_TOKEN
#undef KEYWORD_TOKEN
#undef IGNORE_TOKEN
#undef TOKEN
        }
        #undef CCASE

        return error_token("Unexpected character.");
    }

    /**
     * Returns true when the current position is
     * the same as the source code's length.
     */
    [[nodiscard]] bool is_at_end() noexcept
    {
        return current >= this->source_code.length();
    }
    
    /**
     * Consume all whitespace.
     */
    void skip_whitespace() noexcept
    {
        while (true)
        {
            switch (peek())
            {
#ifndef TOKEN
#define TOKEN(name)
#endif
#ifndef KEYWORD_TOKEN
#define KEYWORD_TOKEN(name, keyword) TOKEN(name)
#endif
#ifndef SYMBOL_TOKEN
#define SYMBOL_TOKEN(name, symbol) TOKEN(name)
#endif
#ifndef IGNORE_TOKEN
#define IGNORE_TOKEN(character) break; case character[0]: { advance_position(); }
#endif
#include TOKEN_DESCRIPTOR_FILE
#undef SYMBOL_TOKEN
#undef KEYWORD_TOKEN
#undef IGNORE_TOKEN
#undef TOKEN
                break; case '/':
                {
                    if (peek(1) == '/')
                    {
                        while(!is_at_end() && peek() != '\n')
                        {
                            advance_position();
                        }
                    }
                    else
                    {
                        return;
                    }
                }
                break; default: { return; }
            }
        }
    }

  private:
    /**
     * Return the current character and advance to the next.
     */
    const char advance() noexcept
    {
        return source_code.at(current++);
    }

    /**
     * Advance the position without returning anything.
     */
    void advance_position() noexcept
    {
        ++current;
    }

    void reset() noexcept
    {
        start = 0;
        current = start;
        line = 1;
    }

    /**
     * Take a peek at the character 'offset' amount away from current.
     * Offset is defaulted to 0, returning the current character without
     * changing the position of the current position.
     */
    const char peek(std::size_t offset = 0) noexcept
    {
        if (current + offset >= this->source_code.length())
            return '\0';
        return source_code.at(current + offset);
    }

    /**
     * Check if the current character matches the expected
     * character. If it does, we return true AND advance to
     * the next character.
     */
    [[nodiscard]] bool match(const char expected) noexcept
    {
        if (is_at_end() || peek() != expected)
            return false;
        ++current;
        return true;
    }

    /**
     * Create a token with the current string slice.
     */
    [[nodiscard]] cpp20scanner::Token<TOKEN_CLASS_NAME> make_token(const TOKEN_CLASS_NAME type) noexcept 
    {
        const std::size_t length = current - start;
        const auto what = this->source_code.substr(start, length);
        return cpp20scanner::Token(type, this->source_code.substr(start, length), this->line);
    }

    /**
     * Create an error token.
     */
    [[nodiscard]] cpp20scanner::Token<TOKEN_CLASS_NAME> error_token(const std::string& message) noexcept 
    {
        return cpp20scanner::Token(TOKEN_CLASS_NAME::Error, message, this->line);
    }

  private:
    std::size_t start{0};
    std::size_t current{start};
    std::string source_code{};
    std::size_t line{1};
};

#undef SCANNER
#endif

#ifdef JOIN
#undef JOIN
#endif

#undef TOKEN_DESCRIPTOR_FILE
#undef TOKEN_CLASS_NAME
