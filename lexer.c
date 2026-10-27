#include <stdio.h>
#include <stdlib.h>
#include "lexical_analyzer.h"

// DFA states 
typedef enum {
    STATE_START,
    STATE_ERROR,
    
    STATE_IDENTIFIER,
    STATE_DIGIT,
    
    STATE_STRING,
    STATE_STRING_END,
    
    STATE_CHAR_START,
    STATE_CHAR_BODY,
    STATE_CHAR_END,
    
    // Comment states
    STATE_BRACKET,
    STATE_NOTE_N, STATE_NOTE_O, STATE_NOTE_T, STATE_NOTE_E, STATE_NOTE_COLON, //note
    STATE_INLINE_COMMENT,
    STATE_GLOSS_G, STATE_GLOSS_L, STATE_GLOSS_O, STATE_GLOSS_S1, STATE_GLOSS_S2, STATE_GLOSS_COLON, //gloss
    STATE_BLOCK_COMMENT,
    STATE_BLOCK_E, STATE_BLOCK_N, STATE_BLOCK_D, STATE_BLOCK_SPACE, STATE_BLOCK_G, STATE_BLOCK_L, STATE_BLOCK_O, STATE_BLOCK_S1, STATE_BLOCK_S2, //end gloss
    
    // Lowercase keyword/operator states - A 
    STATE_A,
    STATE_AL, STATE_ALA, STATE_ALAS,
    STATE_AN, STATE_AND,
    STATE_AY, STATE_AYE,
    STATE_AT, STATE_AT_SPACE, STATE_AT_L, STATE_AT_LE, STATE_AT_LEA, STATE_AT_LEAS, STATE_AT_LEAST,
    STATE_AT_M, STATE_AT_MO, STATE_AT_MOS, STATE_AT_MOST,
    
    // B 
    STATE_B, STATE_BE,
    STATE_BEC, STATE_BECA, STATE_BECAM, STATE_BECAME,
    STATE_BEL, STATE_BELI, STATE_BELIE, STATE_BELIES,
    
    // C 
    STATE_C, STATE_CO, STATE_COM, STATE_COMB, STATE_COMBI, STATE_COMBIN, STATE_COMBINE, STATE_COMBINED,
    
    //E 
    STATE_E, STATE_EX, STATE_EXE, STATE_EXEU, STATE_EXEUN, STATE_EXEUNT,
    
    //F 
    STATE_F, STATE_FR, STATE_FRA, STATE_FRAC, STATE_FRACT, STATE_FRACTI, STATE_FRACTIO, STATE_FRACTION,
    
    //G 
    STATE_G, STATE_GR, STATE_GRE, STATE_GREA, STATE_GREAT, STATE_GREATE, STATE_GREATER,
    STATE_GREATER_SPACE, STATE_GREATER_T, STATE_GREATER_TH, STATE_GREATER_THA, STATE_GREATER_THAN,
    
    //H 
    STATE_H,
    STATE_HAR, STATE_HARK,
    STATE_HEA, STATE_HEAR, STATE_HEARK, STATE_HEARKE, STATE_HEARKEN,
    STATE_HO, STATE_HOL, STATE_HOLD,
    
    //I 
    STATE_I, STATE_IN, STATE_IND, STATE_INDE, STATE_INDEE, STATE_INDEED,
    
    //L 
    STATE_L,
    STATE_LE,
    STATE_LEG, STATE_LEGI, STATE_LEGIO, STATE_LEGION,
    STATE_LES, STATE_LESS, STATE_LESS_SPACE, STATE_LESS_T, STATE_LESS_TH, STATE_LESS_THA, STATE_LESS_THAN,
    STATE_LET,
    STATE_LETT, STATE_LETTE, STATE_LETTER,
    
    //M 
    STATE_M,
    STATE_MA, STATE_MAG, STATE_MAGN, STATE_MAGNI, STATE_MAGNIF, STATE_MAGNIFI, STATE_MAGNIFIE, STATE_MAGNIFIED,
    STATE_MAT, STATE_MATC, STATE_MATCH, STATE_MATCHE, STATE_MATCHES,
    STATE_ME, STATE_MEA, STATE_MEAS, STATE_MEASU, STATE_MEASUR, STATE_MEASURE,
    STATE_MEM, STATE_MEMB, STATE_MEMBE, STATE_MEMBER,
    
    //N 
    STATE_N,
    STATE_NA,
    STATE_NAU, STATE_NAUG, STATE_NAUGH, STATE_NAUGHT,
    STATE_NAY,
    
    //O 
    STATE_O, STATE_OR,
    
    //P 
    STATE_P,
    STATE_PAR, STATE_PART, STATE_PARTE, STATE_PARTED,
    STATE_PE, STATE_PER, STATE_PERC, STATE_PERCH, STATE_PERCHA, STATE_PERCHAN, STATE_PERCHANC, STATE_PERCHANCE,
    STATE_PO, STATE_POI, STATE_POIN, STATE_POINT,
    
    //R 
    STATE_R,
    STATE_RE,
    STATE_REM, STATE_REMN, STATE_REMNA, STATE_REMNAN, STATE_REMNANT,
    STATE_REP, STATE_REPE, STATE_REPEA, STATE_REPEAT,
    
    //S 
    STATE_S, STATE_SP, STATE_SPE, STATE_SPEA, STATE_SPEAK,
    
    //T 
    STATE_T,
    STATE_TA, STATE_TAL, STATE_TALE,
    STATE_TH, STATE_THE,
    STATE_TR, STATE_TRU, STATE_TRUT, STATE_TRUTH,
    
    //U 
    STATE_U, STATE_UN, STATE_UNT, STATE_UNTI, STATE_UNTIL,
    
    //V 
    STATE_V, STATE_VE, STATE_VER, STATE_VERI, STATE_VERIL, STATE_VERILY,
    
    //W 
    STATE_W,
    STATE_WH, STATE_WHI, STATE_WHIL, STATE_WHILS, STATE_WHILST,
    STATE_WI, STATE_WIT, STATE_WITH,
    STATE_WITHO, STATE_WITHOU, STATE_WITHOUT
} State;

static char *read_file(const char *file);
static char peek(Lexer *l);
static char advance(Lexer *l);
static int is_space(char c);
static int is_delimiter(char c);
static char to_lower(char c);
static Token make_token(TokenType type, Lexer *l);

// peek at current char
static char peek(Lexer *l) {
    return l->src[l->pos];
}

// consume and return current character 
static char advance(Lexer *l) {
    return l->src[l->pos++];
}

// check if character is whitespace (NO ctype.h!) 
static int is_space(char c) {
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

// check if character is a delimiter 
static int is_delimiter(char c) {
    return (c == '\0' || c == ' ' || c == '\t' || c == '\r' || c == '\n' || 
            c == '.' || c == ',' || c == '[' || c == '"' || c == '\'');
}

// convert to lowercase (NO ctype.h!) 
static char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

// create token from current lexeme 
static Token make_token(TokenType type, Lexer *l) {
    Token tok;
    tok.type = type;
    size_t len = l->pos - l->start_pos;
    if (len > 255) len = 255;
    
    size_t i;
    for (i = 0; i < len; i++) {
        tok.lexeme[i] = l->src[l->start_pos + i];
    }
    tok.lexeme[i] = '\0';
    
    return tok;
}

// Initialize lexer 
void lexer_init(Lexer *l, const char *source) {
    l->src = source;
    l->pos = 0;
    l->start_pos = 0;
}

// Convert TokenType to string for output 
const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOK_PERIOD: return "TOK_PERIOD";
        case TOK_COMMA: return "TOK_COMMA";
        case TOK_MEASURE_LIT: return "TOK_MEASURE_LIT";
        case TOK_TALE_LIT: return "TOK_TALE_LIT";
        case TOK_LETTER_LIT: return "TOK_LETTER_LIT";
        case TOK_AYE: return "TOK_AYE";
        case TOK_NAUGHT: return "TOK_NAUGHT";
        case TOK_IDENTIFIER: return "TOK_IDENTIFIER";
        case TOK_KW_MEASURE: return "TOK_KW_MEASURE";
        case TOK_KW_FRACTION: return "TOK_KW_FRACTION";
        case TOK_KW_LETTER: return "TOK_KW_LETTER";
        case TOK_KW_TRUTH: return "TOK_KW_TRUTH";
        case TOK_KW_POINT: return "TOK_KW_POINT";
        case TOK_KW_LEGION: return "TOK_KW_LEGION";
        case TOK_KW_TALE: return "TOK_KW_TALE";
        case TOK_KW_HOLD: return "TOK_KW_HOLD";
        case TOK_KW_MEMBER: return "TOK_KW_MEMBER";
        case TOK_KW_ALAS: return "TOK_KW_ALAS";
        case TOK_KW_PERCHANCE: return "TOK_KW_PERCHANCE";
        case TOK_KW_WHILST: return "TOK_KW_WHILST";
        case TOK_KW_REPEAT: return "TOK_KW_REPEAT";
        case TOK_KW_UNTIL: return "TOK_KW_UNTIL";
        case TOK_KW_EXEUNT: return "TOK_KW_EXEUNT";
        case TOK_KW_HEARKEN: return "TOK_KW_HEARKEN";
        case TOK_KW_SPEAK: return "TOK_KW_SPEAK";
        case TOK_KW_LET: return "TOK_KW_LET";
        case TOK_ASSIGN_BE: return "TOK_ASSIGN_BE";
        case TOK_ASSIGN_BECAME: return "TOK_ASSIGN_BECAME";
        case TOK_OP_COMBINED: return "TOK_OP_COMBINED";
        case TOK_OP_WITHOUT: return "TOK_OP_WITHOUT";
        case TOK_OP_MAGNIFIED: return "TOK_OP_MAGNIFIED";
        case TOK_OP_PARTED: return "TOK_OP_PARTED";
        case TOK_OP_REMNANT: return "TOK_OP_REMNANT";
        case TOK_REL_MATCHES: return "TOK_REL_MATCHES";
        case TOK_REL_BELIES: return "TOK_REL_BELIES";
        case TOK_REL_GREATER_THAN: return "TOK_REL_GREATER_THAN";
        case TOK_REL_LESS_THAN: return "TOK_REL_LESS_THAN";
        case TOK_REL_AT_LEAST: return "TOK_REL_AT_LEAST";
        case TOK_REL_AT_MOST: return "TOK_REL_AT_MOST";
        case TOK_LOG_AND: return "TOK_LOG_AND";
        case TOK_LOG_OR: return "TOK_LOG_OR";
        case TOK_LOG_NAY: return "TOK_LOG_NAY";
        case TOK_EOF: return "TOK_EOF";
        case TOK_ERROR: return "TOK_ERROR";
        default: return "UNKNOWN";
    }
}

// Main DFA - Get next token 
Token get_next_token(Lexer *l) {
    State state = STATE_START;
    char c;
    
    while (1) {
        c = peek(l);
        
        switch (state) {
            case STATE_START:
                l->start_pos = l->pos;
                // Skip whitespace 
                while (is_space(c)) {
                    advance(l);
                    c = peek(l);
                    l->start_pos = l->pos;
                }
                
                // EOF
                if (c == '\0') {
                    Token tok;
                    tok.type = TOK_EOF;
                    tok.lexeme[0] = '\0';
                    return tok;
                }
                
                // Period
                if (c == '.') {
                    advance(l);
                    return make_token(TOK_PERIOD, l);
                }
                
                // Comma
                if (c == ',') {
                    advance(l);
                    return make_token(TOK_COMMA, l);
                }
                
                // String literal
                if (c == '"') {
                    advance(l);
                    state = STATE_STRING;
                    break;
                }
                
                // Character literal
                if (c == '\'') {
                    advance(l);
                    state = STATE_CHAR_START;
                    break;
                }
                
                // Comments
                if (c == '[') {
                    advance(l);
                    state = STATE_BRACKET;
                    break;
                }
                
                // Uppercase identifier
                if (c >= 'A' && c <= 'Z') {
                    advance(l);
                    state = STATE_IDENTIFIER;
                    break;
                }
                
                // Digits
                if (c >= '0' && c <= '9') {
                    advance(l);
                    state = STATE_DIGIT;
                    break;
                }
                
                // Lowercase - keywords/operators/noise
                if (c >= 'a' && c <= 'z') {
                    advance(l);
                    if (c == 'a') { state = STATE_A; break; }
                    if (c == 'b') { state = STATE_B; break; }
                    if (c == 'c') { state = STATE_C; break; }
                    if (c == 'e') { state = STATE_E; break; }
                    if (c == 'f') { state = STATE_F; break; }
                    if (c == 'g') { state = STATE_G; break; }
                    if (c == 'h') { state = STATE_H; break; }
                    if (c == 'i') { state = STATE_I; break; }
                    if (c == 'l') { state = STATE_L; break; }
                    if (c == 'm') { state = STATE_M; break; }
                    if (c == 'n') { state = STATE_N; break; }
                    if (c == 'o') { state = STATE_O; break; }
                    if (c == 'p') { state = STATE_P; break; }
                    if (c == 'r') { state = STATE_R; break; }
                    if (c == 's') { state = STATE_S; break; }
                    if (c == 't') { state = STATE_T; break; }
                    if (c == 'u') { state = STATE_U; break; }
                    if (c == 'v') { state = STATE_V; break; }
                    if (c == 'w') { state = STATE_W; break; }
                    state = STATE_ERROR;
                    break;
                }
                
                // Invalid character - prohibited symbol
                state = STATE_ERROR;
                advance(l);
                break;
            
            // Identifier state (uppercase start)
            case STATE_IDENTIFIER:
                c = peek(l);
                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                    advance(l);
                } else if (is_delimiter(c)) {
                    return make_token(TOK_IDENTIFIER, l);
                } else {
                    // Invalid character in identifier
                    state = STATE_ERROR;
                    advance(l);
                }
                break;
            
            // Digit state
            case STATE_DIGIT:
                c = peek(l);
                if (c >= '0' && c <= '9') {
                    advance(l);
                } else if (is_delimiter(c)) {
                    return make_token(TOK_MEASURE_LIT, l);
                } else {    
                    // Invalid: digit followed by non-delimiter (e.g., 1stActor)
                    state = STATE_ERROR;
                    advance(l);
                }
                break;
            
            // String literal
            case STATE_STRING:
                c = peek(l);
                if (c == '\0' || c == '\n') {
                    // Unterminated string
                    return make_token(TOK_ERROR, l);
                } else if (c == '"') {
                    advance(l);
                    return make_token(TOK_TALE_LIT, l);
                } else {
                    advance(l);
                }
                break;
            
            // Character literal
            case STATE_CHAR_START:
                c = peek(l);
                if (c == '\0' || c == '\n' || c == '\'') {
                    return make_token(TOK_ERROR, l);
                } else {
                    advance(l);
                    state = STATE_CHAR_BODY;
                }
                break;
            
            case STATE_CHAR_BODY:
                c = peek(l);
                if (c == '\'') {
                    advance(l);
                    return make_token(TOK_LETTER_LIT, l);
                } else {
                    return make_token(TOK_ERROR, l);
                }
                break;
            
            // Error state - consume until delimiter
            case STATE_ERROR:
                c = peek(l);
                if (is_delimiter(c)) {
                    return make_token(TOK_ERROR, l);
                } else {
                    advance(l);
                }
                break;
            
            // Comment detection
            case STATE_BRACKET:
                c = to_lower(peek(l));
                if (c == 'n') {
                    advance(l);
                    state = STATE_NOTE_N;
                } else if (c == 'g') {
                    advance(l);
                    state = STATE_GLOSS_G;
                } else {
                    // Invalid comment format
                    return make_token(TOK_ERROR, l);
                }
                break;
            
            // Inline comment: [note: ... ] or \n
            case STATE_NOTE_N:
                if (to_lower(peek(l)) == 'o') { advance(l); state = STATE_NOTE_O; }
                else { state = STATE_ERROR; }
                break;
            case STATE_NOTE_O:
                if (to_lower(peek(l)) == 't') { advance(l); state = STATE_NOTE_T; }
                else { state = STATE_ERROR; }
                break;
            case STATE_NOTE_T:
                if (to_lower(peek(l)) == 'e') { advance(l); state = STATE_NOTE_E; }
                else { state = STATE_ERROR; }
                break;
            case STATE_NOTE_E:
                if (peek(l) == ':') { advance(l); state = STATE_INLINE_COMMENT; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_INLINE_COMMENT:
                c = peek(l);
                if (c == '\0') {
                    // Unterminated inline comment at EOF 
                    return make_token(TOK_ERROR, l);
                } else if (c == ']' || c == '\n') {
                    if (c == ']') advance(l);
                    // Comment consumed - reset to START
                    state = STATE_START;
                    l->start_pos = l->pos;
                } else {
                    advance(l);
                }
                break;
            
            // Block comment: [gloss: ... end gloss]
            case STATE_GLOSS_G:
                if (to_lower(peek(l)) == 'l') { advance(l); state = STATE_GLOSS_L; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GLOSS_L:
                if (to_lower(peek(l)) == 'o') { advance(l); state = STATE_GLOSS_O; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GLOSS_O:
                if (to_lower(peek(l)) == 's') { advance(l); state = STATE_GLOSS_S1; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GLOSS_S1:
                if (to_lower(peek(l)) == 's') { advance(l); state = STATE_GLOSS_S2; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GLOSS_S2:
                if (peek(l) == ':') { advance(l); state = STATE_BLOCK_COMMENT; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_BLOCK_COMMENT:
                c = to_lower(peek(l));
                if (c == '\0') {
                    // Unterminated block comment 
                    return make_token(TOK_ERROR, l);
                } else if (c == 'e') {
                    advance(l);
                    state = STATE_BLOCK_E;
                } else {
                    advance(l);
                }
                break;
            
            case STATE_BLOCK_E:
                c = to_lower(peek(l));
                if (c == 'n') { advance(l); state = STATE_BLOCK_N; }
                else { state = STATE_BLOCK_COMMENT; }
                break;
            case STATE_BLOCK_N:
                c = to_lower(peek(l));
                if (c == 'd') { advance(l); state = STATE_BLOCK_D; }
                else { state = STATE_BLOCK_COMMENT; }
                break;
            case STATE_BLOCK_D:
                c = peek(l);
                if (c == ' ' || c == '\t') { advance(l); state = STATE_BLOCK_SPACE; }
                else { state = STATE_BLOCK_COMMENT; }
                break;
            case STATE_BLOCK_SPACE:
                c = peek(l);
                if (c == ' ' || c == '\t') { advance(l); }
                else if (to_lower(c) == 'g') { advance(l); state = STATE_BLOCK_G; }
                else { state = STATE_BLOCK_COMMENT; }
                break;
            case STATE_BLOCK_G:
                if (to_lower(peek(l)) == 'l') { advance(l); state = STATE_BLOCK_L; }
                else { state = STATE_BLOCK_COMMENT; }
                break;
            case STATE_BLOCK_L:
                if (to_lower(peek(l)) == 'o') { advance(l); state = STATE_BLOCK_O; }
                else { state = STATE_BLOCK_COMMENT; }
                break;
            case STATE_BLOCK_O:
                if (to_lower(peek(l)) == 's') { advance(l); state = STATE_BLOCK_S1; }
                else { state = STATE_BLOCK_COMMENT; }
                break;
            case STATE_BLOCK_S1:
                if (to_lower(peek(l)) == 's') { advance(l); state = STATE_BLOCK_S2; }
                else { state = STATE_BLOCK_COMMENT; }
                break;
            case STATE_BLOCK_S2:
                if (peek(l) == ']') {
                    advance(l);
                    // reset to start 
                    state = STATE_START;
                    l->start_pos = l->pos;
                } else {
                    state = STATE_BLOCK_COMMENT;
                }
                break;
            
            // A branch: alas, and, aye, at least, at most
            case STATE_A:
                c = peek(l);
                if (c == 'l') { advance(l); state = STATE_AL; }
                else if (c == 'n') { advance(l); state = STATE_AN; }
                else if (c == 'y') { advance(l); state = STATE_AY; }
                else if (c == 't') { advance(l); state = STATE_AT; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_AL:
                c = peek(l);
                if (c == 'a') { advance(l); state = STATE_ALA; }
                else { state = STATE_ERROR; }
                break;
            case STATE_ALA:
                c = peek(l);
                if (c == 's') { advance(l); state = STATE_ALAS; }
                else { state = STATE_ERROR; }
                break;
            case STATE_ALAS:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_ALAS, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_AN:
                c = peek(l);
                if (c == 'd') { advance(l); state = STATE_AND; }
                else { state = STATE_ERROR; }
                break;
            case STATE_AND:
                if (is_delimiter(peek(l))) { return make_token(TOK_LOG_AND, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_AY:
                c = peek(l);
                if (c == 'e') { advance(l); state = STATE_AYE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_AYE:
                if (is_delimiter(peek(l))) { return make_token(TOK_AYE, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_AT:
                c = peek(l);
                if (c == ' ' || c == '\t') { advance(l); state = STATE_AT_SPACE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_AT_SPACE:
                c = peek(l);
                if (c == ' ' || c == '\t') { advance(l); }
                else if (c == 'l') { advance(l); state = STATE_AT_L; }
                else if (c == 'm') { advance(l); state = STATE_AT_M; }
                else { state = STATE_ERROR; }
                break;
            case STATE_AT_L:
                if (peek(l) == 'e') { advance(l); state = STATE_AT_LE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_AT_LE:
                if (peek(l) == 'a') { advance(l); state = STATE_AT_LEA; }
                else { state = STATE_ERROR; }
                break;
            case STATE_AT_LEA:
                if (peek(l) == 's') { advance(l); state = STATE_AT_LEAS; }
                else { state = STATE_ERROR; }
                break;
            case STATE_AT_LEAS:
                if (peek(l) == 't') { advance(l); state = STATE_AT_LEAST; }
                else { state = STATE_ERROR; }
                break;
            case STATE_AT_LEAST:
                if (is_delimiter(peek(l))) { return make_token(TOK_REL_AT_LEAST, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_AT_M:
                if (peek(l) == 'o') { advance(l); state = STATE_AT_MO; }
                else { state = STATE_ERROR; }
                break;
            case STATE_AT_MO:
                if (peek(l) == 's') { advance(l); state = STATE_AT_MOS; }
                else { state = STATE_ERROR; }
                break;
            case STATE_AT_MOS:
                if (peek(l) == 't') { advance(l); state = STATE_AT_MOST; }
                else { state = STATE_ERROR; }
                break;
            case STATE_AT_MOST:
                if (is_delimiter(peek(l))) { return make_token(TOK_REL_AT_MOST, l); }
                else { state = STATE_ERROR; }
                break;
            
            // B
            case STATE_B:
                c = peek(l);
                if (c == 'e') { advance(l); state = STATE_BE; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_BE:
                c = peek(l);
                if (is_delimiter(c)) { return make_token(TOK_ASSIGN_BE, l); }
                else if (c == 'c') { advance(l); state = STATE_BEC; }
                else if (c == 'l') { advance(l); state = STATE_BEL; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_BEC:
                if (peek(l) == 'a') { advance(l); state = STATE_BECA; }
                else { state = STATE_ERROR; }
                break;
            case STATE_BECA:
                if (peek(l) == 'm') { advance(l); state = STATE_BECAM; }
                else { state = STATE_ERROR; }
                break;
            case STATE_BECAM:
                if (peek(l) == 'e') { advance(l); state = STATE_BECAME; }
                else { state = STATE_ERROR; }
                break;
            case STATE_BECAME:
                if (is_delimiter(peek(l))) { return make_token(TOK_ASSIGN_BECAME, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_BEL:
                if (peek(l) == 'i') { advance(l); state = STATE_BELI; }
                else { state = STATE_ERROR; }
                break;
            case STATE_BELI:
                if (peek(l) == 'e') { advance(l); state = STATE_BELIE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_BELIE:
                if (peek(l) == 's') { advance(l); state = STATE_BELIES; }
                else { state = STATE_ERROR; }
                break;
            case STATE_BELIES:
                if (is_delimiter(peek(l))) { return make_token(TOK_REL_BELIES, l); }
                else { state = STATE_ERROR; }
                break;
            
            // C
            case STATE_C:
                if (peek(l) == 'o') { advance(l); state = STATE_CO; }
                else { state = STATE_ERROR; }
                break;
            case STATE_CO:
                if (peek(l) == 'm') { advance(l); state = STATE_COM; }
                else { state = STATE_ERROR; }
                break;
            case STATE_COM:
                if (peek(l) == 'b') { advance(l); state = STATE_COMB; }
                else { state = STATE_ERROR; }
                break;
            case STATE_COMB:
                if (peek(l) == 'i') { advance(l); state = STATE_COMBI; }
                else { state = STATE_ERROR; }
                break;
            case STATE_COMBI:
                if (peek(l) == 'n') { advance(l); state = STATE_COMBIN; }
                else { state = STATE_ERROR; }
                break;
            case STATE_COMBIN:
                if (peek(l) == 'e') { advance(l); state = STATE_COMBINE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_COMBINE:
                if (peek(l) == 'd') { advance(l); state = STATE_COMBINED; }
                else { state = STATE_ERROR; }
                break;
            case STATE_COMBINED:
                if (is_delimiter(peek(l))) { return make_token(TOK_OP_COMBINED, l); }
                else { state = STATE_ERROR; }
                break;
            
            // E
            case STATE_E:
                if (peek(l) == 'x') { advance(l); state = STATE_EX; }
                else { state = STATE_ERROR; }
                break;
            case STATE_EX:
                if (peek(l) == 'e') { advance(l); state = STATE_EXE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_EXE:
                if (peek(l) == 'u') { advance(l); state = STATE_EXEU; }
                else { state = STATE_ERROR; }
                break;
            case STATE_EXEU:
                if (peek(l) == 'n') { advance(l); state = STATE_EXEUN; }
                else { state = STATE_ERROR; }
                break;
            case STATE_EXEUN:
                if (peek(l) == 't') { advance(l); state = STATE_EXEUNT; }
                else { state = STATE_ERROR; }
                break;
            case STATE_EXEUNT:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_EXEUNT, l); }
                else { state = STATE_ERROR; }
                break;
            
            // F
            case STATE_F:
                if (peek(l) == 'r') { advance(l); state = STATE_FR; }
                else { state = STATE_ERROR; }
                break;
            case STATE_FR:
                if (peek(l) == 'a') { advance(l); state = STATE_FRA; }
                else { state = STATE_ERROR; }
                break;
            case STATE_FRA:
                if (peek(l) == 'c') { advance(l); state = STATE_FRAC; }
                else { state = STATE_ERROR; }
                break;
            case STATE_FRAC:
                if (peek(l) == 't') { advance(l); state = STATE_FRACT; }
                else { state = STATE_ERROR; }
                break;
            case STATE_FRACT:
                if (peek(l) == 'i') { advance(l); state = STATE_FRACTI; }
                else { state = STATE_ERROR; }
                break;
            case STATE_FRACTI:
                if (peek(l) == 'o') { advance(l); state = STATE_FRACTIO; }
                else { state = STATE_ERROR; }
                break;
            case STATE_FRACTIO:
                if (peek(l) == 'n') { advance(l); state = STATE_FRACTION; }
                else { state = STATE_ERROR; }
                break;
            case STATE_FRACTION:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_FRACTION, l); }
                else { state = STATE_ERROR; }
                break;
            
            // G
            case STATE_G:
                if (peek(l) == 'r') { advance(l); state = STATE_GR; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GR:
                if (peek(l) == 'e') { advance(l); state = STATE_GRE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GRE:
                if (peek(l) == 'a') { advance(l); state = STATE_GREA; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GREA:
                if (peek(l) == 't') { advance(l); state = STATE_GREAT; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GREAT:
                if (peek(l) == 'e') { advance(l); state = STATE_GREATE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GREATE:
                if (peek(l) == 'r') { advance(l); state = STATE_GREATER; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GREATER:
                c = peek(l);
                if (c == ' ' || c == '\t') { advance(l); state = STATE_GREATER_SPACE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GREATER_SPACE:
                c = peek(l);
                if (c == ' ' || c == '\t') { advance(l); }
                else if (c == 't') { advance(l); state = STATE_GREATER_T; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GREATER_T:
                if (peek(l) == 'h') { advance(l); state = STATE_GREATER_TH; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GREATER_TH:
                if (peek(l) == 'a') { advance(l); state = STATE_GREATER_THA; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GREATER_THA:
                if (peek(l) == 'n') { advance(l); state = STATE_GREATER_THAN; }
                else { state = STATE_ERROR; }
                break;
            case STATE_GREATER_THAN:
                if (is_delimiter(peek(l))) { return make_token(TOK_REL_GREATER_THAN, l); }
                else { state = STATE_ERROR; }
                break;
            
            // H
            case STATE_H:
                c = peek(l);
                if (c == 'a') { advance(l); state = STATE_HAR; }
                else if (c == 'e') { advance(l); state = STATE_HEA; }
                else if (c == 'o') { advance(l); state = STATE_HO; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_HAR:
                if (peek(l) == 'k') { advance(l); state = STATE_HARK; }
                else { state = STATE_ERROR; }
                break;
            case STATE_HARK:
                // skip and reset to start
                if (is_delimiter(peek(l))) {
                    state = STATE_START;
                    l->start_pos = l->pos;
                } else {
                    state = STATE_ERROR;
                }
                break;
            
            case STATE_HEA:
                if (peek(l) == 'r') { advance(l); state = STATE_HEAR; }
                else { state = STATE_ERROR; }
                break;
            case STATE_HEAR:
                if (peek(l) == 'k') { advance(l); state = STATE_HEARK; }
                else { state = STATE_ERROR; }
                break;
            case STATE_HEARK:
                if (peek(l) == 'e') { advance(l); state = STATE_HEARKE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_HEARKE:
                if (peek(l) == 'n') { advance(l); state = STATE_HEARKEN; }
                else { state = STATE_ERROR; }
                break;
            case STATE_HEARKEN:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_HEARKEN, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_HO:
                if (peek(l) == 'l') { advance(l); state = STATE_HOL; }
                else { state = STATE_ERROR; }
                break;
            case STATE_HOL:
                if (peek(l) == 'd') { advance(l); state = STATE_HOLD; }
                else { state = STATE_ERROR; }
                break;
            case STATE_HOLD:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_HOLD, l); }
                else { state = STATE_ERROR; }
                break;
            
            // Indeed (noise)
            case STATE_I:
                if (peek(l) == 'n') { advance(l); state = STATE_IN; }
                else { state = STATE_ERROR; }
                break;
            case STATE_IN:
                if (peek(l) == 'd') { advance(l); state = STATE_IND; }
                else { state = STATE_ERROR; }
                break;
            case STATE_IND:
                if (peek(l) == 'e') { advance(l); state = STATE_INDE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_INDE:
                if (peek(l) == 'e') { advance(l); state = STATE_INDEE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_INDEE:
                if (peek(l) == 'd') { advance(l); state = STATE_INDEED; }
                else { state = STATE_ERROR; }
                break;
            case STATE_INDEED:
                // indeed is a NOISE WORD - skip and reset to START
                if (is_delimiter(peek(l))) {
                    state = STATE_START;
                    l->start_pos = l->pos;
                } else {
                    state = STATE_ERROR;
                }
                break;
            
            // L
            case STATE_L:
                c = peek(l);
                if (c == 'e') { advance(l); state = STATE_LE; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_LE:
                c = peek(l);
                if (c == 'g') { advance(l); state = STATE_LEG; }
                else if (c == 's') { advance(l); state = STATE_LES; }
                else if (c == 't') { advance(l); state = STATE_LET; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_LEG:
                if (peek(l) == 'i') { advance(l); state = STATE_LEGI; }
                else { state = STATE_ERROR; }
                break;
            case STATE_LEGI:
                if (peek(l) == 'o') { advance(l); state = STATE_LEGIO; }
                else { state = STATE_ERROR; }
                break;
            case STATE_LEGIO:
                if (peek(l) == 'n') { advance(l); state = STATE_LEGION; }
                else { state = STATE_ERROR; }
                break;
            case STATE_LEGION:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_LEGION, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_LES:
                if (peek(l) == 's') { advance(l); state = STATE_LESS; }
                else { state = STATE_ERROR; }
                break;
            case STATE_LESS:
                c = peek(l);
                if (c == ' ' || c == '\t') { advance(l); state = STATE_LESS_SPACE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_LESS_SPACE:
                c = peek(l);
                if (c == ' ' || c == '\t') { advance(l); }
                else if (c == 't') { advance(l); state = STATE_LESS_T; }
                else { state = STATE_ERROR; }
                break;
            case STATE_LESS_T:
                if (peek(l) == 'h') { advance(l); state = STATE_LESS_TH; }
                else { state = STATE_ERROR; }
                break;
            case STATE_LESS_TH:
                if (peek(l) == 'a') { advance(l); state = STATE_LESS_THA; }
                else { state = STATE_ERROR; }
                break;
            case STATE_LESS_THA:
                if (peek(l) == 'n') { advance(l); state = STATE_LESS_THAN; }
                else { state = STATE_ERROR; }
                break;
            case STATE_LESS_THAN:
                if (is_delimiter(peek(l))) { return make_token(TOK_REL_LESS_THAN, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_LET:
                c = peek(l);
                if (is_delimiter(c)) { return make_token(TOK_KW_LET, l); }
                else if (c == 't') { advance(l); state = STATE_LETT; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_LETT:
                if (peek(l) == 'e') { advance(l); state = STATE_LETTE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_LETTE:
                if (peek(l) == 'r') { advance(l); state = STATE_LETTER; }
                else { state = STATE_ERROR; }
                break;
            case STATE_LETTER:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_LETTER, l); }
                else { state = STATE_ERROR; }
                break;
            
            // M
            case STATE_M:
                c = peek(l);
                if (c == 'a') { advance(l); state = STATE_MA; }
                else if (c == 'e') { advance(l); state = STATE_ME; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_MA:
                c = peek(l);
                if (c == 'g') { advance(l); state = STATE_MAG; }
                else if (c == 't') { advance(l); state = STATE_MAT; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_MAG:
                if (peek(l) == 'n') { advance(l); state = STATE_MAGN; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MAGN:
                if (peek(l) == 'i') { advance(l); state = STATE_MAGNI; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MAGNI:
                if (peek(l) == 'f') { advance(l); state = STATE_MAGNIF; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MAGNIF:
                if (peek(l) == 'i') { advance(l); state = STATE_MAGNIFI; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MAGNIFI:
                if (peek(l) == 'e') { advance(l); state = STATE_MAGNIFIE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MAGNIFIE:
                if (peek(l) == 'd') { advance(l); state = STATE_MAGNIFIED; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MAGNIFIED:
                if (is_delimiter(peek(l))) { return make_token(TOK_OP_MAGNIFIED, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_MAT:
                if (peek(l) == 'c') { advance(l); state = STATE_MATC; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MATC:
                if (peek(l) == 'h') { advance(l); state = STATE_MATCH; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MATCH:
                if (peek(l) == 'e') { advance(l); state = STATE_MATCHE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MATCHE:
                if (peek(l) == 's') { advance(l); state = STATE_MATCHES; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MATCHES:
                if (is_delimiter(peek(l))) { return make_token(TOK_REL_MATCHES, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_ME:
                c = peek(l);
                if (c == 'a') { advance(l); state = STATE_MEA; }
                else if (c == 'm') { advance(l); state = STATE_MEM; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_MEA:
                if (peek(l) == 's') { advance(l); state = STATE_MEAS; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MEAS:
                if (peek(l) == 'u') { advance(l); state = STATE_MEASU; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MEASU:
                if (peek(l) == 'r') { advance(l); state = STATE_MEASUR; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MEASUR:
                if (peek(l) == 'e') { advance(l); state = STATE_MEASURE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MEASURE:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_MEASURE, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_MEM:
                if (peek(l) == 'b') { advance(l); state = STATE_MEMB; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MEMB:
                if (peek(l) == 'e') { advance(l); state = STATE_MEMBE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MEMBE:
                if (peek(l) == 'r') { advance(l); state = STATE_MEMBER; }
                else { state = STATE_ERROR; }
                break;
            case STATE_MEMBER:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_MEMBER, l); }
                else { state = STATE_ERROR; }
                break;
            
            // N
            case STATE_N:
                if (peek(l) == 'a') { advance(l); state = STATE_NA; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_NA:
                c = peek(l);
                if (c == 'u') { advance(l); state = STATE_NAU; }
                else if (c == 'y') { advance(l); state = STATE_NAY; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_NAU:
                if (peek(l) == 'g') { advance(l); state = STATE_NAUG; }
                else { state = STATE_ERROR; }
                break;
            case STATE_NAUG:
                if (peek(l) == 'h') { advance(l); state = STATE_NAUGH; }
                else { state = STATE_ERROR; }
                break;
            case STATE_NAUGH:
                if (peek(l) == 't') { advance(l); state = STATE_NAUGHT; }
                else { state = STATE_ERROR; }
                break;
            case STATE_NAUGHT:
                if (is_delimiter(peek(l))) { return make_token(TOK_NAUGHT, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_NAY:
                if (is_delimiter(peek(l))) { return make_token(TOK_LOG_NAY, l); }
                else { state = STATE_ERROR; }
                break;
            
            // O
            case STATE_O:
                if (peek(l) == 'r') { advance(l); state = STATE_OR; }
                else { state = STATE_ERROR; }
                break;
            case STATE_OR:
                if (is_delimiter(peek(l))) { return make_token(TOK_LOG_OR, l); }
                else { state = STATE_ERROR; }
                break;
            
            // P
            case STATE_P:
                c = peek(l);
                if (c == 'a') { advance(l); state = STATE_PAR; }
                else if (c == 'e') { advance(l); state = STATE_PE; }
                else if (c == 'o') { advance(l); state = STATE_PO; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_PAR:
                if (peek(l) == 't') { advance(l); state = STATE_PART; }
                else { state = STATE_ERROR; }
                break;
            case STATE_PART:
                if (peek(l) == 'e') { advance(l); state = STATE_PARTE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_PARTE:
                if (peek(l) == 'd') { advance(l); state = STATE_PARTED; }
                else { state = STATE_ERROR; }
                break;
            case STATE_PARTED:
                if (is_delimiter(peek(l))) { return make_token(TOK_OP_PARTED, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_PE:
                if (peek(l) == 'r') { advance(l); state = STATE_PER; }
                else { state = STATE_ERROR; }
                break;
            case STATE_PER:
                if (peek(l) == 'c') { advance(l); state = STATE_PERC; }
                else { state = STATE_ERROR; }
                break;
            case STATE_PERC:
                if (peek(l) == 'h') { advance(l); state = STATE_PERCH; }
                else { state = STATE_ERROR; }
                break;
            case STATE_PERCH:
                if (peek(l) == 'a') { advance(l); state = STATE_PERCHA; }
                else { state = STATE_ERROR; }
                break;
            case STATE_PERCHA:
                if (peek(l) == 'n') { advance(l); state = STATE_PERCHAN; }
                else { state = STATE_ERROR; }
                break;
            case STATE_PERCHAN:
                if (peek(l) == 'c') { advance(l); state = STATE_PERCHANC; }
                else { state = STATE_ERROR; }
                break;
            case STATE_PERCHANC:
                if (peek(l) == 'e') { advance(l); state = STATE_PERCHANCE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_PERCHANCE:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_PERCHANCE, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_PO:
                if (peek(l) == 'i') { advance(l); state = STATE_POI; }
                else { state = STATE_ERROR; }
                break;
            case STATE_POI:
                if (peek(l) == 'n') { advance(l); state = STATE_POIN; }
                else { state = STATE_ERROR; }
                break;
            case STATE_POIN:
                if (peek(l) == 't') { advance(l); state = STATE_POINT; }
                else { state = STATE_ERROR; }
                break;
            case STATE_POINT:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_POINT, l); }
                else { state = STATE_ERROR; }
                break;
            
            // R
            case STATE_R:
                if (peek(l) == 'e') { advance(l); state = STATE_RE; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_RE:
                c = peek(l);
                if (c == 'm') { advance(l); state = STATE_REM; }
                else if (c == 'p') { advance(l); state = STATE_REP; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_REM:
                if (peek(l) == 'n') { advance(l); state = STATE_REMN; }
                else { state = STATE_ERROR; }
                break;
            case STATE_REMN:
                if (peek(l) == 'a') { advance(l); state = STATE_REMNA; }
                else { state = STATE_ERROR; }
                break;
            case STATE_REMNA:
                if (peek(l) == 'n') { advance(l); state = STATE_REMNAN; }
                else { state = STATE_ERROR; }
                break;
            case STATE_REMNAN:
                if (peek(l) == 't') { advance(l); state = STATE_REMNANT; }
                else { state = STATE_ERROR; }
                break;
            case STATE_REMNANT:
                if (is_delimiter(peek(l))) { return make_token(TOK_OP_REMNANT, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_REP:
                if (peek(l) == 'e') { advance(l); state = STATE_REPE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_REPE:
                if (peek(l) == 'a') { advance(l); state = STATE_REPEA; }
                else { state = STATE_ERROR; }
                break;
            case STATE_REPEA:
                if (peek(l) == 't') { advance(l); state = STATE_REPEAT; }
                else { state = STATE_ERROR; }
                break;
            case STATE_REPEAT:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_REPEAT, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_S:
                if (peek(l) == 'p') { advance(l); state = STATE_SP; }
                else { state = STATE_ERROR; }
                break;
            case STATE_SP:
                if (peek(l) == 'e') { advance(l); state = STATE_SPE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_SPE:
                if (peek(l) == 'a') { advance(l); state = STATE_SPEA; }
                else { state = STATE_ERROR; }
                break;
            case STATE_SPEA:
                if (peek(l) == 'k') { advance(l); state = STATE_SPEAK; }
                else { state = STATE_ERROR; }
                break;
            case STATE_SPEAK:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_SPEAK, l); }
                else { state = STATE_ERROR; }
                break;
            
            // T
            case STATE_T:
                c = peek(l);
                if (c == 'a') { advance(l); state = STATE_TA; }
                else if (c == 'h') { advance(l); state = STATE_TH; }
                else if (c == 'r') { advance(l); state = STATE_TR; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_TA:
                if (peek(l) == 'l') { advance(l); state = STATE_TAL; }
                else { state = STATE_ERROR; }
                break;
            case STATE_TAL:
                if (peek(l) == 'e') { advance(l); state = STATE_TALE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_TALE:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_TALE, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_TH:
                if (peek(l) == 'e') { advance(l); state = STATE_THE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_THE:
                // skip and reset to start
                if (is_delimiter(peek(l))) {
                    state = STATE_START;
                    l->start_pos = l->pos;
                } else {
                    state = STATE_ERROR;
                }
                break;
            
            case STATE_TR:
                if (peek(l) == 'u') { advance(l); state = STATE_TRU; }
                else { state = STATE_ERROR; }
                break;
            case STATE_TRU:
                if (peek(l) == 't') { advance(l); state = STATE_TRUT; }
                else { state = STATE_ERROR; }
                break;
            case STATE_TRUT:
                if (peek(l) == 'h') { advance(l); state = STATE_TRUTH; }
                else { state = STATE_ERROR; }
                break;
            case STATE_TRUTH:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_TRUTH, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_U:
                if (peek(l) == 'n') { advance(l); state = STATE_UN; }
                else { state = STATE_ERROR; }
                break;
            case STATE_UN:
                if (peek(l) == 't') { advance(l); state = STATE_UNT; }
                else { state = STATE_ERROR; }
                break;
            case STATE_UNT:
                if (peek(l) == 'i') { advance(l); state = STATE_UNTI; }
                else { state = STATE_ERROR; }
                break;
            case STATE_UNTI:
                if (peek(l) == 'l') { advance(l); state = STATE_UNTIL; }
                else { state = STATE_ERROR; }
                break;
            case STATE_UNTIL:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_UNTIL, l); }
                else { state = STATE_ERROR; }
                break;
            
            // verily (noise)
            case STATE_V:
                if (peek(l) == 'e') { advance(l); state = STATE_VE; }
                else { state = STATE_ERROR; }
                break;
            case STATE_VE:
                if (peek(l) == 'r') { advance(l); state = STATE_VER; }
                else { state = STATE_ERROR; }
                break;
            case STATE_VER:
                if (peek(l) == 'i') { advance(l); state = STATE_VERI; }
                else { state = STATE_ERROR; }
                break;
            case STATE_VERI:
                if (peek(l) == 'l') { advance(l); state = STATE_VERIL; }
                else { state = STATE_ERROR; }
                break;
            case STATE_VERIL:
                if (peek(l) == 'y') { advance(l); state = STATE_VERILY; }
                else { state = STATE_ERROR; }
                break;
            case STATE_VERILY:
                // skip and reset to start
                if (is_delimiter(peek(l))) {
                    state = STATE_START;
                    l->start_pos = l->pos;
                } else {
                    state = STATE_ERROR;
                }
                break;
            
            // w
            case STATE_W:
                c = peek(l);
                if (c == 'h') { advance(l); state = STATE_WH; }
                else if (c == 'i') { advance(l); state = STATE_WI; }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_WH:
                if (peek(l) == 'i') { advance(l); state = STATE_WHI; }
                else { state = STATE_ERROR; }
                break;
            case STATE_WHI:
                if (peek(l) == 'l') { advance(l); state = STATE_WHIL; }
                else { state = STATE_ERROR; }
                break;
            case STATE_WHIL:
                if (peek(l) == 's') { advance(l); state = STATE_WHILS; }
                else { state = STATE_ERROR; }
                break;
            case STATE_WHILS:
                if (peek(l) == 't') { advance(l); state = STATE_WHILST; }
                else { state = STATE_ERROR; }
                break;
            case STATE_WHILST:
                if (is_delimiter(peek(l))) { return make_token(TOK_KW_WHILST, l); }
                else { state = STATE_ERROR; }
                break;
            
            case STATE_WI:
                if (peek(l) == 't') { advance(l); state = STATE_WIT; }
                else { state = STATE_ERROR; }
                break;
            case STATE_WIT:
                if (peek(l) == 'h') { advance(l); state = STATE_WITH; }
                else { state = STATE_ERROR; }
                break;
            case STATE_WITH:
                c = peek(l);
                if (is_delimiter(c)) {
                    // skip and reset to start
                    state = STATE_START;
                    l->start_pos = l->pos;
                } else if (c == 'o') {
                    advance(l);
                    state = STATE_WITHO;
                } else {
                    state = STATE_ERROR;
                }
                break;
            
            case STATE_WITHO:
                if (peek(l) == 'u') { advance(l); state = STATE_WITHOU; }
                else { state = STATE_ERROR; }
                break;
            case STATE_WITHOU:
                if (peek(l) == 't') { advance(l); state = STATE_WITHOUT; }
                else { state = STATE_ERROR; }
                break;
            case STATE_WITHOUT:
                if (is_delimiter(peek(l))) { return make_token(TOK_OP_WITHOUT, l); }
                else { state = STATE_ERROR; }
                break;
            
            default:
                return make_token(TOK_ERROR, l);
        }
    }
}

void lexer(const char *string){
    char *input = read_file(string);

    if(input == NULL){
        return;
    }

    Lexer lex;
    lexer_init(&lex, input);
    
    printf("%-24s | %s\n", "Token Type", "Lexeme");
    printf("-------------------------|");
    printf("------------------\n");
    
    Token tok;
    do {
        tok = get_next_token(&lex);
        if (tok.type != TOK_EOF) {
            printf("%-24s | %s\n", token_type_to_string(tok.type), tok.lexeme);
        }
    } while (tok.type != TOK_EOF);
    
    free(input);
}

static char *read_file(const char *file){ 
    FILE *fptr = fopen(file, "r");
    if (!fptr){
        perror(file); 
        return NULL; 
    }

    size_t cap = 256, len = 0;
    char *input = malloc(cap);
    if (!input){
        fclose(fptr); 
        return NULL; 
    }

    int c;
    while ((c = fgetc(fptr)) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char *tmp = realloc(input, cap);
            if (!tmp) { free(input); fclose(fptr); return NULL; }
            input = tmp;
        }
        input[len++] = (char)c;
    }
    input[len] = '\0';

    fclose(fptr);
    return input;
}
