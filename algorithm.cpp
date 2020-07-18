#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <limits>
#include <stdexcept>
#include <map>

enum {
    KW_SIGMA = -1,
    KW_GAMMA = -2,
    KW_R = -3,
    KW_A = -4,
    KW_EOF = -5,
    KW_ARROW = -6,
    KW_EPSILON = -7
};

struct rule { // x, y => z
    int x;
    int y;
    int z;
};

struct replacement {
    int position;
    char c;
};

bool is_valid_symbol(int token) {
    std::vector<int> valid_symbols({ '@', '-', '.', '(', ')' });

    return isalnum(token) || std::find(valid_symbols.begin(), valid_symbols.end(), token) != valid_symbols.end();
}

template <typename T> bool is_subset(std::vector<T> A, std::vector<T> B) {
    B.push_back(KW_EPSILON);

    for (T c : A) {
        if (std::find(B.begin(), B.end(), c) == B.end()) return false;
    }

    return true;
}

int token(FILE *fp, bool ignore_linefeed = true) {
    static char lastToken = ' ';

    if (ignore_linefeed) {
        while (isspace(lastToken)) lastToken = fgetc(fp);
    } else {
        while (lastToken != '\n' && isspace(lastToken)) lastToken = fgetc(fp);
        
        if (lastToken == '\n') {
            lastToken = ' ';
            return '\n';
        } 
    }

    if (lastToken == '#') {
        do {
            lastToken = fgetc(fp);
        } while (lastToken != '\n' && lastToken != EOF);

        return token(fp, ignore_linefeed);
    }

    if (lastToken == EOF) return KW_EOF;

    if (lastToken == '-') {
        lastToken = fgetc(fp);

        if (lastToken != '>') {
            std::cerr << "error: unexpected token '-" << ((char) lastToken) << "'.\n\n";
            exit(1);
        }

        lastToken = ' ';
        return KW_ARROW;
    }

    if (lastToken == '!') {
        std::string kw;

        lastToken = fgetc(fp);
        while (isalpha(lastToken)) {
            kw += (char) lastToken;
            lastToken = fgetc(fp);
        }

        if (kw == "eps") return KW_EPSILON;

        if (lastToken != ':') {
            std::cerr << "error: expected ':' after '!" << kw << "'.\n\n";
            exit(1);
        }

        lastToken = fgetc(fp);

        if (kw == "sigma") return KW_SIGMA;
        if (kw == "gamma") return KW_GAMMA;
        if (kw == "rules") return KW_R;
        if (kw == "accept") return KW_A;

        std::cerr << "error: unrecognized identifier '!" << kw << "'\n\n";
        exit(1);
    }

    int res = lastToken;
    lastToken = fgetc(fp);
    return res;
}

std::string readable(int tk) {
    if (tk == KW_SIGMA) return "!sigma";
    if (tk == KW_GAMMA) return "!gamma";
    if (tk == KW_R) return "!rules";
    if (tk == KW_A) return "!accept";
    if (tk == KW_ARROW) return "->";

    std::ostringstream os;
    os << (char) tk;
    return os.str();
}

void expect(FILE *descf, int expected) {
    if (token(descf) != expected) {
        std::cerr << "error: expected '" << readable(expected) << "'.\n\n";
        exit(1);
    }
}

void parse_charset(FILE *descf, std::vector<int> &cset, bool allow_eps = false) {

    int symbol = token(descf, false);
    while (symbol != '\n' && symbol != KW_EOF) {
        
        if (!(is_valid_symbol(symbol) || (allow_eps && symbol == KW_EPSILON))) {
            std::cerr << "error: character '" << (char)symbol << "' cannot be used as symbol.\n\n";
            exit(1);
        }

        cset.push_back(symbol);
        symbol = token(descf, false);
    }

}

void parse_rules(FILE *descf, std::vector<rule> &ruleset, std::vector<int> &gamma) {

    int symbol = token(descf);
    while (symbol != '.') {
        if (symbol != '[') {
            std::cerr << "error: expected '['.\n\n";
            exit(1);
        }

        int x, y, z;

        x = token(descf);
        if (std::find(gamma.begin(), gamma.end(), x) == gamma.end()) {
            std::cerr << "error: only symbols in gamma can be used in rules.\n\n";
            exit(1);
        }

        expect(descf, ',');

        y = token(descf);
        if (std::find(gamma.begin(), gamma.end(), y) == gamma.end()) {
            std::cerr << "error: only symbols in gamma can be used in rules.\n\n";
            exit(1);
        }

        expect(descf, KW_ARROW);

        z = token(descf);
        if (std::find(gamma.begin(), gamma.end(), z) == gamma.end()) {
            std::cerr << "error: only symbols in gamma can be used in rules.\n\n";
            exit(1);
        }

        expect(descf, ']');
        symbol = token(descf);

        ruleset.push_back({ x, y, z });
    }

}

bool is_alphabet(std::string &s, std::vector<int> &sigma) {
    for (char c : s) {
        if (std::find(sigma.begin(), sigma.end(), c) == sigma.end()) return false;
    }
    return true;
}


replacement select_rule(std::string input, std::vector<rule> &ruleset) {
    if (input.size() < 2) return { -1, '\0' };

    for (rule &r : ruleset) {
        for (int k = 0; k < input.size() - 1; k++) {
            if (input[k] == r.x && input[k+1] == r.y) return { k, (char) r.z };
        }
    }

    return { -1, '\0' };
}

void print_symbol(int sym) {
    if (sym == KW_EPSILON) std::cout << "!eps";
    else std::cout << (char)sym;
}

void print_charset(const std::vector<int> &cset) {
    std::cout << "{ ";

    if (!cset.empty()) {
        print_symbol(cset[0]);

        for (int i = 1; i < cset.size(); i++) {
            std::cout << ", ";
            print_symbol(cset[i]);
        }
    }

    std::cout << " }";
}

void print_ruleset(const std::vector<rule> &rset) {
    std::cout << "    R = (\n";

    for (const rule &r : rset) {
        std::cout << "         [";
        print_symbol(r.x);
        std::cout << ", ";
        print_symbol(r.y);
        std::cout << " -> ";
        print_symbol(r.z);
        std::cout << "],\n";
    }

    std::cout << "        )\n";
}


int main(int argc, char const *argv[]) {

    if (argc < 2) {
        std::cerr << "error: expected description file name.\n\n";
        return 1;
    }

    FILE *descf = fopen(argv[1], "r");

    if (!descf) {
        std::cerr << "error: file not found.\n\n";
        return 1;
    }

    std::vector<int> sigma;
    std::vector<int> gamma;

    std::vector<rule> rules;
    std::vector<int> accepts;


    expect(descf, KW_SIGMA);
    parse_charset(descf, sigma);
    if (sigma.empty()) {
        std::cerr << "error: sigma must be non-empty.\n\n";
        return 1;
    }

    expect(descf, KW_GAMMA);
    parse_charset(descf, gamma);
    if (!is_subset(sigma, gamma)) {
        std::cerr << "error: gamma must be a superset of sigma.\n\n";
        return 1;
    }

    expect(descf, KW_R);
    parse_rules(descf, rules, gamma);

    expect(descf, KW_A);
    parse_charset(descf, accepts, true);
    if (!is_subset(accepts, gamma)) {
        std::cerr << "error: A must be a subset of gamma.\n\n";
        return 1;
    }

    fclose(descf);
    std::cout << "Pairing system was successfully parsed:\n\n";

    std::cout << "Sigma = ";
    print_charset(sigma);
    std::cout << "\nGamma = ";
    print_charset(gamma);
    std::cout << "\n";
    print_ruleset(rules);
    std::cout << "    A = ";
    print_charset(accepts);
    std::cout << "\n\n";

    std::istream &fp = std::cin;
    while (!fp.eof()) {
        std::cout << "Insert an input string (! for the empty string):\n";

        std::string input;
        fp >> input;

        if (input == "!") input = "";

        if (!is_alphabet(input, sigma)) std::cerr << "error: input string not valid: must have sigma as alphabet.\n\n";

        std::cout << "evaluating input...\n\n";

        replacement rep = select_rule(input, rules);

        bool first_step = true;
        while (rep.position >= 0) {
            if (first_step) {
                std::cout << "   ";
                first_step = false;
            } else std::cout << "=> ";

            std::cout << input.substr(0, rep.position) << "\033[1;31m" 
                      << input.substr(rep.position, 2) << "\033[0m"
                      << input.substr(rep.position+2) << "\n";

            input = input.substr(0, rep.position) + (char) rep.c + input.substr(rep.position+2);
            rep = select_rule(input, rules);
        }

        std::cout << (first_step ? "   " : "=> ");
        std::cout << input << "\n\n";

        int result = input == "" ? KW_EPSILON : input[0];

        std::cout << "no more rules applicable.\n";
        if (input.size() <= 1 && std::find(accepts.begin(), accepts.end(), result) != accepts.end()) {
            std::cout << "the input is \033[1;37maccepted\033[0m as '" << input << "' is in A.\n\n";
        } else {
            std::cout << "the input is \033[1;37mrejected\033[0m as '" << input << "' is not in A.\n\n";
        }
    }

    return 0;
}
