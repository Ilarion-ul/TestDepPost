#include "DepParser.h"
#include "Logger.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <cctype>

using Row = std::vector<double>;

/* ===== Утиліта: числа у форматах 1.23E-04, 3.0, -7 ===== */
std::vector<double> DepParser::parseNumbersLine(const std::string& line)
{
    static const std::regex num(R"(([+-]?(?:\d+\.\d*|\d*\.?\d+)(?:[Ee][+-]?\d+)?))");
    std::vector<double> out;
    for (std::sregex_iterator it(line.begin(), line.end(), num), e; it != e; ++it)
        out.push_back(std::stod(it->str()));
    return out;
}

/* ===== Запис матриці покроково (col = idx) ============= */
void DepParser::storeMatrix(const std::string& material,
                            const std::string& param,
                            const std::vector<std::vector<double>>& rows)
{
    if (rows.empty()) return;

    const size_t nRows = rows.size();
    const size_t nCols = rows.front().size();

    for (size_t c = 0; c < nCols; ++c) {
        std::vector<double> col(nRows);
        for (size_t r = 0; r < nRows; ++r) col[r] = rows[r][c];
        data_.steps[int(c)][material][param] = std::move(col);
    }
}

/* ======== Головний парсер =========================================== */
void DepParser::parseFile(const std::string& filename)
{
    std::ifstream in(filename);
    if (!in) throw std::runtime_error("Cannot open file: " + filename);

    std::string line;
    std::smatch  m;

    /* --- основні регекспи --- */
    const std::regex reZai  (R"(^\s*ZAI\s*=)");
    const std::regex reNames(R"(^\s*NAMES\s*=)");
    const std::regex reMat  (R"(^\s*MAT_([A-Za-z0-9_]+)_([A-Z0-9_]+)\s*\(.*=\s*\[)");
    const std::regex reTot  (R"(^\s*TOT_([A-Z0-9_]+)\s*\(.*=\s*\[)");
    const std::regex reIdx  (R"(^\s*idx\s*=)");          /* просто пропускаємо */

    bool zaiSeen   = false;
    bool namesSeen = false;

    /* ------------------ основний цикл по рядках ---------------------- */
    while (std::getline(in, line))
    {
        if (line.empty() || line[0]=='%') continue;      // коментар
        /* ---- ZAI ---------------------------------------------------- */
        if (std::regex_search(line, reZai))
        {
            zaiSeen = true;
            while (line.find('[')==std::string::npos && std::getline(in,line)) {}
            while (std::getline(in,line)) {
                if (line.find(']')!=std::string::npos) break;
                Row v = parseNumbersLine(line);
                if (v.size()!=1) throw std::runtime_error("ZAI: 1 number expected");
                data_.ZAI.push_back(int(v.front()+0.5));
            }
            continue;
        }

        /* ---- NAMES -------------------------------------------------- */
        if (std::regex_search(line, reNames))
        {
            namesSeen = true;
            while (line.find('[')==std::string::npos && std::getline(in,line)) {}
            while (std::getline(in,line)) {
                if (line.find(']')!=std::string::npos) break;
                auto first = line.find('\'');
                auto last  = line.rfind('\'');
                if (first==std::string::npos || first==last) continue;
                std::string name = line.substr(first+1, last-first-1);
                while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back())))
                    name.pop_back();
                data_.NAMES.push_back(std::move(name));
            }
            continue;
        }

        /* ---- idx (ігноруємо) --------------------------------------- */
        if (std::regex_search(line, reIdx)) continue;

        /* ---- MAT_xxx_PARAM або TOT_PARAM --------------------------- */
        std::string material, param;
        if (std::regex_search(line, m, reMat)) { material = m[1]; param = m[2]; }
        else if (std::regex_search(line, m, reTot)) { material = "TOT"; param = m[1]; }
        else continue;

        /* ---- шукаємо символ '[' (може бути на цьому рядку) --------- */
        std::string arrLine = line;
        if (arrLine.find('[')==std::string::npos)
            while (std::getline(in,arrLine) && arrLine.find('[')==std::string::npos){}

        /* ---- читаємо до відповідного ']' --------------------------- */
        std::vector<Row> rows;
        auto pos = arrLine.find('[');
        std::string tail = arrLine.substr(pos+1);
        if (tail.find(']')!=std::string::npos)
            tail.erase(tail.find(']'));          // усе після ']' не потрібне
        if (auto r = parseNumbersLine(tail); !r.empty()) rows.push_back(std::move(r));

        while (std::getline(in,line))
        {
            if (line.find(']')!=std::string::npos) {
                std::string before = line.substr(0, line.find(']'));
                if (auto r = parseNumbersLine(before); !r.empty()) rows.push_back(std::move(r));
                break;
            }
            auto comment = line.find('%');
            if (comment!=std::string::npos) line.erase(comment);
            if (auto r = parseNumbersLine(line); !r.empty()) rows.push_back(std::move(r));
        }

        if (rows.empty()) { log::Logger::warn("Empty {}_{}", material, param); continue; }

        /* ---- перевірка консистентності ----------------------------- */
        const size_t cols = rows.front().size();
        for (const auto& r: rows)
            if (r.size()!=cols)
                throw std::runtime_error(material+'_'+param+": inconsistent column count");

        if (!nSteps_) nSteps_ = cols;
        else if (cols!=nSteps_)
            throw std::runtime_error(material+'_'+param+": column count ≠ previous matrices");

        /* ---- isotopic vs scalar validation (не фатальна) ----------- */
        const size_t expectRows = data_.ZAI.size() + 2;   // +Lost, +Total
        if (rows.size()!=expectRows && rows.size()!=1)
            log::Logger::warn("{}_{}: rows {} vs expected {}", material, param,
                              rows.size(), expectRows);

        storeMatrix(material, param, rows);
    }

    if (!zaiSeen || !namesSeen)
        throw std::runtime_error("Missing ZAI or NAMES in "+filename);

    log::Logger::info("Parsed '{}': steps={}, materials={}",
                      filename, data_.steps.size(),
                      data_.steps.begin()->second.size());
}
