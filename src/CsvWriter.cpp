#include "CsvWriter.h"
#include <fstream>
#include <unordered_map>
#include <set>
#include <map>
#include <string_view>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

static bool isScalarParam(std::string_view p)
{
    return p == "VOLUME" || p == "BURNUP" || p == "BU" || p == "DAYS";
}

static inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return char(std::tolower(c)); });
    return s;
}

static bool looks_like_lost(std::string_view s) {
    auto ls = to_lower(std::string(s));
    // допускаємо 'lost', 'lost nuclides', 'lost_nuclides', пробіли/паддінг
    return ls == "lost" || ls.find("lost") != std::string::npos;
}
static bool looks_like_total(std::string_view s) {
    auto ls = to_lower(std::string(s));
    // допускаємо 'total', 'tot'
    return ls == "total" || ls == "tot" || ls.find("total") != std::string::npos;
}

void CsvWriter::writePerMaterial(const DepData& d, const fs::path& outDir)
{
    fs::create_directories(outDir);

    const size_t Nall = d.NAMES.size();

    // 1) Надійно знаходимо індекси Lost/Total
    int idxLost  = -1;
    int idxTotal = -1;

    // пріоритет — ZAI
    if (d.ZAI.size() == Nall) {
        for (size_t i = 0; i < Nall; ++i) {
            if (d.ZAI[i] == 666) idxLost  = int(i);
            if (d.ZAI[i] ==    0) idxTotal = int(i);
        }
    }
    // fallback — за назвами
    if (idxLost < 0 || idxTotal < 0) {
        for (size_t i = 0; i < Nall; ++i) {
            if (idxLost  < 0 && looks_like_lost(d.NAMES[i]))  idxLost  = int(i);
            if (idxTotal < 0 && looks_like_total(d.NAMES[i])) idxTotal = int(i);
        }
    }

    // 2) Індекси лише ізотопів (виключаємо знайдені Lost/Total)
    std::vector<size_t> isoIdx;
    isoIdx.reserve(Nall);
    for (size_t i = 0; i < Nall; ++i) {
        if (int(i) == idxLost || int(i) == idxTotal) continue;
        isoIdx.push_back(i);
    }
    const size_t Niso = isoIdx.size();

    // Імена ізотопів для заголовка
    std::vector<std::string> isoNames;
    isoNames.reserve(Niso);
    for (size_t k = 0; k < Niso; ++k) isoNames.push_back(d.NAMES[isoIdx[k]]);

    // 3) Збираємо дані: material → param → step → vec
    struct RowRef { int step; std::vector<double> vec; };
    std::unordered_map<std::string,
        std::unordered_map<std::string, std::map<int, RowRef>>> bucket;

    // з матриць
    for (const auto& [step, matMap] : d.steps)
        for (const auto& [mat, paramMap] : matMap)
            for (const auto& [param, vec] : paramMap)
                bucket[mat][param][step] = { step, vec };

    // додати BU/DAYS як скаляри у кожен material
    for (auto& [mat, params] : bucket) {
        for (const auto& [step, _] : d.steps) {
            if (!d.BU.empty()   && step < (int)d.BU.size())
                params["BU"][step]   = { step, std::vector<double>{ d.BU[step] } };
            if (!d.DAYS.empty() && step < (int)d.DAYS.size())
                params["DAYS"][step] = { step, std::vector<double>{ d.DAYS[step] } };
        }
    }

    // 4) Вивід по кожному material
    for (auto& [mat, params] : bucket)
    {
        std::ofstream out(outDir / (mat + ".csv"));
        if (!out) throw std::runtime_error("CSV open: " + mat);

        // Header: step,param, <iso...>, Lost, Total  (саме у такому порядку)
        out << "step,param";
        for (const auto& n : isoNames) out << ',' << n;
        out << ",Lost,Total\n";

        // param-и алфавітно
        std::set<std::string> orderedParams;
        for (const auto& kv : params) orderedParams.insert(kv.first);

        // всередині кожного param — step ↑
        for (const auto& param : orderedParams)
        {
            const auto& stepMap = params[param];

            for (const auto& [step, rr] : stepMap)
            {
                const auto& vec = rr.vec;
                const size_t sz = vec.size();

                out << step << ',' << param;

                if (isScalarParam(param)) {
                    // порожні ізотопи
                    for (size_t i = 0; i < Niso; ++i) out << ',';
                    // порожній Lost, потім значення в Total
                    out << ','   // Lost порожній
                        << ','   // перехід до Total
                        << (vec.empty() ? 0.0 : vec.back());
                    out << '\n';
                    continue;
                }

                if (sz == Nall && (idxLost >= 0 || idxTotal >= 0)) {
                    // Вектор узгоджений з NAMES: беремо по індексах
                    // 1) ізотопи в порядку isoIdx
                    for (size_t k = 0; k < Niso; ++k)
                        out << ',' << vec[ isoIdx[k] ];
                    // 2) Lost, 3) Total — тільки якщо індекси відомі
                    if (idxLost  >= 0) out << ',' << vec[ size_t(idxLost)  ];
                    else               out << ',';
                    if (idxTotal >= 0) out << ',' << vec[ size_t(idxTotal) ];
                    else               out << ',';
                }
                else if (sz == Niso + 2) {
                    // Приймаємо порядок [iso..., Lost, Total]
                    for (size_t i = 0; i < Niso; ++i) out << ',' << vec[i];
                    out << ',' << vec[Niso]          // Lost
                        << ',' << vec[Niso + 1];     // Total
                }
                else if (sz == Niso + 1) {
                    // Трактуємо як [iso..., Total] — Lost відсутній
                    for (size_t i = 0; i < Niso; ++i) out << ',' << vec[i];
                    out << ','                        // Lost порожній
                        << ','
                        << vec[Niso];          // Total
                }
                else if (sz == Niso) {
                    // Лише ізотопи
                    for (double v : vec) out << ',' << v;
                    out << ",,";                      // Lost,Total порожні
                }
                else if (sz == 1) {
                    // Скаляр TOT_xxx — лише Total (Lost порожній)
                    for (size_t i = 0; i < Niso; ++i) out << ',';
                    out << ','
                        << ','
                        << vec[0];             // Lost порожній, Total = значення
                }
                else {
                    throw std::runtime_error(param + ": unexpected vector size " + std::to_string(sz));
                }

                out << '\n';
            }
        }
    }
}
