#include "DepParser.h"
#include "Logger.h"

#include <fstream>
#include <regex>
#include <stdexcept>
#include <cctype>

using Row = std::vector<double>;

/* ===== Утиліта: витяг усіх чисел з рядка (1.23E-04, 3.0, -7, …) ===== */
std::vector<double> DepParser::parseNumbersLine(const std::string& line)
{
    static const std::regex num(R"(([+-]?(?:\d+\.\d*|\d*\.?\d+)(?:[Ee][+-]?\d+)?))");
    std::vector<double> out;
    for (std::sregex_iterator it(line.begin(), line.end(), num), e; it != e; ++it)
        out.push_back(std::stod(it->str()));
    return out;
}

/* ===== Перекладання матриці рядків у колонки по step (col = idx) ===== */
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

/* ============================= Головний парсер ============================== */
void DepParser::parseFile(const std::string& filename)
{
    std::ifstream in(filename);
    if (!in) throw std::runtime_error("Cannot open file: " + filename);

    std::string line;
    std::smatch  m;

    /* --- основні регекспи --- */
    const std::regex reZai  (R"(^\s*ZAI\s*=)");
    const std::regex reNames(R"(^\s*NAMES\s*=)");
    const std::regex reIdx  (R"(^\s*idx\s*=)");
    const std::regex reBU   (R"(^\s*BU\s*=\s*\[)");
    const std::regex reDAYS (R"(^\s*DAYS\s*=\s*\[)");
    // Параметри матеріалів/тоталів (підтримуємо MDENS і MASS)
    const std::regex reMat(R"(^\s*MAT_([A-Za-z0-9_]+)_(ADENS|MDENS|MASS|A|H|SF|GSRC|ING_TOX|INH_TOX|VOLUME|BURNUP)\s*(?:\([^)]*\))?\s*=\s*\[)");
    const std::regex reTot(R"(^\s*TOT_(ADENS|MDENS|MASS|A|H|SF|GSRC|ING_TOX|INH_TOX|VOLUME|BURNUP)\s*(?:\([^)]*\))?\s*=\s*\[)");

    bool zaiSeen   = false;
    bool namesSeen = false;

    /* ------------------ основний цикл по рядках ---------------------- */
    while (std::getline(in, line))
    {
        if (line.empty() || line[0]=='%') continue;      // пропустити коментар/порожній

        /* ---- BU / DAYS (одновимірні вектори по step) ---------------- */
        if (std::regex_search(line, reBU) || std::regex_search(line, reDAYS)) {
            std::string firstLine = line;
            std::vector<double> vals;

            // знайти початковий '['
            std::string arrLine = firstLine;
            if (arrLine.find('[') == std::string::npos)
                while (std::getline(in, arrLine) && arrLine.find('[') == std::string::npos) {}

            auto pos = arrLine.find('[');
            std::string tail = arrLine.substr(pos + 1);
            bool closedSameLine = false;
            if (auto rb = tail.find(']'); rb != std::string::npos) {
                closedSameLine = true;
                tail.erase(rb);
            }
            if (auto r = parseNumbersLine(tail); !r.empty())
                vals.insert(vals.end(), r.begin(), r.end());

            while (!closedSameLine && std::getline(in, line)) {
                auto rb = line.find(']');
                if (rb != std::string::npos) {
                    std::string before = line.substr(0, rb);
                    if (auto r = parseNumbersLine(before); !r.empty())
                        vals.insert(vals.end(), r.begin(), r.end());
                    break;
                }
                if (auto cpos = line.find('%'); cpos != std::string::npos) line.erase(cpos);
                if (auto r = parseNumbersLine(line); !r.empty())
                    vals.insert(vals.end(), r.begin(), r.end());
            }

            // зберегти у BU / DAYS
            if (std::regex_search(firstLine, reBU) || std::regex_search(arrLine, reBU))
                data_.BU = std::move(vals);
            else
                data_.DAYS = std::move(vals);

            // узгодити довжину з кількістю колонок матриць
            const size_t k = !data_.BU.empty() ? data_.BU.size() : data_.DAYS.size();
            if (!nSteps_) nSteps_ = k;
            else if (k && nSteps_ && k != nSteps_)
                slog::Logger::warn("BU/DAYS length {} differs from matrix columns {}", k, nSteps_);
            continue;
        }

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

        /* ---- знайти '[' (може бути не на цьому рядку) --------------- */
        std::string arrLine = line;
        if (arrLine.find('[')==std::string::npos)
            while (std::getline(in,arrLine) && arrLine.find('[')==std::string::npos){}

        /* ---- прочитати до відповідного ']' -------------------------- */
        std::vector<Row> rows;
        auto apos = arrLine.find('[');
        std::string tail = arrLine.substr(apos + 1);
        bool closedSameLine = false;
        if (auto rb = tail.find(']'); rb != std::string::npos) {
            closedSameLine = true;
            tail.erase(rb);
        }
        if (auto r = parseNumbersLine(tail); !r.empty())
            rows.push_back(std::move(r));

        while (!closedSameLine && std::getline(in, line)) {
            auto rb = line.find(']');
            if (rb != std::string::npos) {
                std::string before = line.substr(0, rb);
                if (auto r = parseNumbersLine(before); !r.empty())
                    rows.push_back(std::move(r));
                break;
            }
            if (auto cpos = line.find('%'); cpos != std::string::npos) line.erase(cpos);
            if (auto r = parseNumbersLine(line); !r.empty())
                rows.push_back(std::move(r));
        }

        if (rows.empty()) { slog::Logger::warn("Empty {}_{}", material, param); continue; }

        /* ---- перевірка консистентності ------------------------------ */
        const size_t cols = rows.front().size();
        for (const auto& r: rows)
            if (r.size()!=cols)
                throw std::runtime_error(material+'_'+param+": inconsistent column count");

        if (!nSteps_) nSteps_ = cols;
        else if (cols!=nSteps_)
            throw std::runtime_error(material+'_'+param+": column count ≠ previous matrices");

        /* ---- ізотопний vs скалярний (лише попередження) ------------- */
        const size_t expectRows = data_.ZAI.size() + 2;   // +Lost, +Total
        if (rows.size()!=expectRows && rows.size()!=1)
            slog::Logger::warn("{}_{}: rows {} vs expected {}", material, param, rows.size(), expectRows);

        /* ---- зберегти ----------------------------------------------- */
        storeMatrix(material, param, rows);
    }

    if (!zaiSeen || !namesSeen)
        throw std::runtime_error("Missing ZAI or NAMES in " + filename);
}
